/******************************************************************************
*
*  Binary.cpp
*     MD-Direct binary data 
*
*  TODO : 
*      - Orc packing sucks for large value i32's : 5 bytes;
*      - ILX packing?  0x80 = 16-bit; 0xc0 = 32-bit; 0xe0 : 64-bit
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created
*      5 DEC 2013 jcs  Build  4: UNPACKED_BINFLD
*      4 JUN 2014 jcs  Build  7: mddFld_real / mddFld_bytestream
*     12 NOV 2014 jcs  Build  8: -Wall
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_dNow(); MDW_Internal.h
*     29 MAR 2022 jcs  Build 13: Binary._bPackFlds
*     23 MAY 2022 jcs  Build 14: mddFld_unixTime
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>

using namespace MDDWIRE_PRIVATE;

/*
 * date    : YYYYMMDDHHMMSS
 * time    : 100s of micros
 * floats  : 4 implied decimal places
 * doubles : 10 implied decimal places
 */

static double _dt_mul = 1000000.0;
static double _dt_div = 1.0 / _dt_mul;
static double _t_mul  = 10000.0;
static double _t_div  = 1.0 / _t_mul;  
static double _f_mul  = 10000.0;
static double _f_div  = 1.0 / _f_mul;
static double _d_mul  = 10000000000.0;
static double _d_div  = 1.0 / _d_mul;
static double _d_MAX  = 1750.0;   // 6-byte packed double / _d_mul

// Packed

static u_char    _NEG8  = PACKED_BINARY;
static u_int     _NEG32 = 0x80000000;
static u_int     _MSK32 = ~_NEG32;
static u_int64_t _l64   = _NEG32;
static u_int64_t _NEG64 = ( (u_int64_t)_NEG32 << 32 );
static u_int64_t _MSK64 = ~_NEG64;
static u_char    _PACK  = (u_char)PACKED_BINARY;
static u_char    _PROTO = ~_PACK;


/////////////////////////////////////////////////////////////////////////////
//
//                c l a s s       E n d i a n
//
/////////////////////////////////////////////////////////////////////////////

#define GL_ENDIAN  0x01020304
#define _swapl(c)  ( ( (c) & 0xff000000L ) >> 24 ) | \
                   ( ( (c) & 0x00ff0000L ) >>  8 ) | \
                   ( ( (c) & 0x0000ff00L ) <<  8 ) | \
                   ( ( (c) & 0x000000ffL ) << 24 )
#define _swaps(c)  ( ( (c) & 0x0000ff00L ) >>  8 ) | \
                   ( ( (c) & 0x000000ffL ) <<  8 )

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Endian::Endian() : 
   _bBig( _IsBig() )
{
}

Endian::~Endian()
{
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
float Endian::_swapf( float c )
{
   float f;
   u_int l, l1;

   ::memcpy( &l, &c, sizeof( c ) );
   l1 = _swapl( l );
   ::memcpy( &f, &l1, sizeof( l1 ) );
   return f;
}

double Endian::_swapd( double c )
{
   float d[2], r[2];

   ::memcpy( d, &c, sizeof( c ) );
   r[0] = _swapf( d[1] );
   r[1] = _swapf( d[0] );
   ::memcpy( &c, r, sizeof( c ) );
   return c;
}


////////////////////////////////////////////
// Class-wide
////////////////////////////////////////////
bool Endian::_IsBig()
{
   long  l = GL_ENDIAN;  // 0x01020304
   char *c = (char *)&l;

   return( ( c[0] == 1 ) && ( c[1] == 2 ) && ( c[2] == 3 ) && ( c[3] == 4 ) );
}



/////////////////////////////////////////////////////////////////////////////
//
//                c l a s s       B i n a r y
//
/////////////////////////////////////////////////////////////////////////////

double Binary::_dMidNt  = 0.0;
double Binary::_ymd_mul = 1000000.0;

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Binary::Binary( bool bPackFlds ) :
   _bPackFlds( bPackFlds )
{
}

Binary::~Binary()
{
}


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
double Binary::MsgTime( mddBinHdr &h )
{
   double mTm;

   // 100s of micros since midnight

   mTm  = _t_div * h._time;
   mTm += _dMidNt;
   return mTm;
}


////////////////////////////////////////////
// Wire Protocol - Get
////////////////////////////////////////////
int Binary::Get( u_char *bp, mddBinHdr &h )
{
   u_char     *cp, mt, dt, pro;
   static int _proOff = 6; // wmddBinHdr._len + _dt + _mt

   // Pack = wmddBinHdr._protocol & _PACK

   cp  = bp;
   cp += _proOff;
   Get( cp, pro );            // wmddBinHdr._protocol;
   h._protocol = ( pro & _PROTO );
   h._bPack    = ( ( pro & _PACK ) == _PACK );

   // Individually from the structure

   cp  = bp;
   cp += Get( cp, h._len, true );  // wmddBinHdr._len[4];
   cp += Get( cp, h._tag, true );  // wmddBinHdr._tag[4];
   cp += Get( cp, dt );            // wmddBinHdr._dt;
   cp += Get( cp, mt );            // wmddBinHdr._mt;
   cp += sizeof( h._protocol );    // wmddBinHdr._protocol;
   cp += Get( cp, h._reserved );   // wmddBinHdr._reserved;
   cp += _u_unpack( cp, h._time ); // wmddBinHdr._time[4];
   cp += _u_unpack( cp, h._RTL );  // wmddBinHdr._RTL[4];
   h._dt = (mddDataType)dt;
   h._mt = (mddMsgType)mt;
   return( cp-bp );
}

int Binary::Get( u_char *bp, mddField &f )
{
   mddValue &v = f._val;
   u_char   *cp, ty;
   bool      bNeg, bUnpack;

   // Common header : Packed or Unpacked

   cp      = bp;
   cp     += Get( cp, ty );
   cp     += _u_unpack( cp, f._fid );
   f._type = (mddFldType)( ty & 0x7f );
   bNeg    = ( ty & _NEG8 ) == _NEG8;
   bUnpack = ( ty & UNPACKED_BINFLD ) == UNPACKED_BINFLD;

   // Contents differ : Packed vs Unpacked

   if ( bUnpack ) {
      cp += _Get_unpacked( cp, f, bNeg );
      return( cp-bp );
   }
   switch( f._type ) {
      case mddFld_undef:
         break;
      case mddFld_string:
         cp += Get( cp, v._buf );
         break;
      case mddFld_int32:
         cp += _u_unpack( cp, v._i32 );
         if ( bNeg )
//            v._i32 = -v._i32;
            v._i32 |= 0x80000000L;
         break;
      case mddFld_double:
         cp += Get( cp, v._r64, _d_div );
         if ( bNeg )
            v._r64 = -v._r64;
         break;
      case mddFld_date:
         cp     += Get( cp, v._r64, 1.0 );
         v._r64 *= _dt_mul;
         break;
      case mddFld_time:
      case mddFld_timeSec:
         cp    += Get( cp, v._r32 );
         v._r64 = v._r32;
         break;
      case mddFld_float:
         cp += Get( cp, v._r32 );
         if ( bNeg )
            v._r32 = -v._r32;
         break;
      case mddFld_int8:
         cp += Get( cp, v._i8 );
         break;
      case mddFld_int16:
         cp += Get( cp, v._i16 );
         break;
      case mddFld_int64:
         cp += Get( cp, v._i64, bUnpack );
         break;
      case mddFld_real:
         cp += Get( cp, v._real );
         break;
      case mddFld_bytestream:
         cp += Get( cp, v._buf );
         break;
      case mddFld_unixTime:
         cp += Get( cp, v._i64, bUnpack );
         break;
   }
   return( cp-bp );
}

int Binary::Get( u_char *bp, mddBuf &b ) 
{
   u_char *cp;

   // <u_int>String

   cp      = bp;
   cp     += _u_unpack( cp, b._dLen );
   b._data = (char *)cp;
   cp     += b._dLen;
   return( cp-bp );
}

int Binary::Get( u_char *bp, u_char &i8 )
{
   i8 = *bp;
   return 1;
}

int Binary::Get( u_char *bp, u_short &i16 )
{
   u_int i32;
   int   sz;

   sz  = _u_unpack( bp, i32 );
   i16 = i32;
   return sz;
}

int Binary::Get( u_char *bp, u_int &i32, bool bUnpacked )
{
   int sz;

   // Unpacked

   if ( bUnpacked ) {
      ::memcpy( &i32, bp, sizeof( i32 ) );
      i32 = ntohl( i32 );
      sz = sizeof( i32 );
   }
   else
      sz = _u_unpack( bp, i32 );
   return sz;
}

int Binary::Get( u_char *bp, u_int64_t &i64, bool bUnpacked )
{
   int sz;
   
   // Unpacked

   if ( bUnpacked ) {
      ::memcpy( &i64, bp, sizeof( i64 ) );
      sz = sizeof( i64 );
   }
   else
      sz = _u_unpack( bp, i64 );
   return sz;
}

int Binary::Get( u_char *bp, float &f )
{
   u_int i32;
   int   sz; 

   // 4 implied decimal places

   sz = Get( bp, i32 );
   f  = _f_div * i32; 
   return sz; 
}

int Binary::Get( u_char *bp, double &d, double mul )
{
   u_int64_t i64;
   int       sz;

   // 10 implied decimal places

   sz  = Get( bp, i64 );
   d   = mul * i64;
   return sz;
}

int Binary::Get( u_char *bp, mddReal &r )
{
   u_char *cp;

   cp        = bp;
   cp       += Get( cp, r.value );
   r.hint    = *cp++;
   r.isBlank = *cp++;
   return( cp-bp );
}


////////////////////////////////////////////
// Wire Protocol - Set
////////////////////////////////////////////
int Binary::Set( u_char *bp, mddBinHdr &h, bool bLenOnly )
{
   u_char *cp, mt, dt;

   // Always (OBSOLETE)

   h._protocol |= _PACK;

   // h._len = size of full payload

   dt  = (u_char)h._dt;
   mt  = (u_char)h._mt;
   cp  = bp;
   cp += Set( cp, h._len, true );  // wmddBinHdr._len[4]
   if ( bLenOnly )
      return( cp-bp );
   cp += Set( cp, h._tag, true );  // wmddBinHdr._tag[4]
   cp += Set( cp, dt );            // wmddBinHdr._dt;
   cp += Set( cp, mt );            // wmddBinHdr._mt
   cp += Set( cp, h._protocol );   // wmddBinHdr._protocol
   cp += Set( cp, h._reserved );   // wmddBinHdr._reserved
   if ( !h._time )
      h._time = _TimeNow();
   cp += _u_pack( cp, h._time );   // wmddBinHdr._time[4]
   cp += _u_pack( cp, h._RTL );    // wmddBinHdr._RTL[4]
   return( cp-bp );
}

int Binary::Set( u_char *bp, mddField f )
{
   mddValue &v = f._val;
   u_char   *cp, ty;
   u_int     i32;
   u_int64_t i64;
   float     r32;
   double    r64;
   bool      bNeg, bPack;

   // Common header : Packed or Unpacked

   cp    = bp;
   ty    = (u_char)f._type;
   ty   |= _bPackFlds ? 0 : UNPACKED_BINFLD;
   *cp++ = ty;
   cp   += _u_pack( cp, f._fid );

   // Contents differ : Packed vs Unpacked

   if ( !_bPackFlds ) { 
      cp += _Set_unpacked( cp, f ); 
      return( cp-bp );
   }
   switch( f._type ) {
      case mddFld_undef:
         break;
      case mddFld_string:
         cp += Set( cp, v._buf );
         break;
      case mddFld_int32:
         bNeg = ( v._i32 & _NEG32 ) == _NEG32;
         i32  = ( v._i32 & _MSK32 );
         cp  += _u_pack( cp, i32 );
         *bp |= bNeg ? _NEG8 : 0;
         break;
      case mddFld_double:
         bNeg = ( v._r64 < 0.0 );
         r64  = bNeg ? -v._r64 : v._r64;

         // Re-cast to int64, if too big 

         if ( r64 > _d_MAX ) {
            v._i64  = (u_int64_t)r64;
            v._i64 |= bNeg ? _NEG64 : 0;
            f._type = mddFld_int64;
            return Set( bp, f ); 
         }
         cp  += Set( cp, r64, _d_mul );
         *bp |= bNeg ? _NEG8 : 0;
         break;
      case mddFld_date:
         v._r64 *= _dt_div;
         cp     += Set( cp, v._r64, 1.0 );
         break;
      case mddFld_time:
      case mddFld_timeSec:
         r64 = ::fmod( v._r64, _ymd_mul );
         r32 = r64;
         cp += Set( cp, r32 );
         break;
      case mddFld_float:
         bNeg = ( v._r32 < 0.0 );
         r32  = bNeg ? -v._r32 : v._r32;
         cp  += Set( cp, r32 );
         *bp |= bNeg ? _NEG8 : 0;
         break;
      case mddFld_int8:
         cp += Set( cp, v._i8 );
         break;
      case mddFld_int16:
         cp += Set( cp, v._i16 );
         break;
      case mddFld_int64:
         bNeg = ( v._i64 & _NEG64 ) == _NEG64;
         i64  = ( v._i64 & _MSK64 );
         cp  += _u_pack( cp, i64, bPack );
         *bp |= bNeg  ? _NEG8 : 0;
         *bp |= bPack ? 0 : UNPACKED_BINFLD;
         break;
      case mddFld_real:
         cp += Set( cp, v._real );
         break;
      case mddFld_bytestream:
         cp += Set( cp, v._buf );
         break;
      case mddFld_unixTime:
         cp  += _u_pack( cp, v._i64, bPack );
         break;
   }
   return( cp-bp );
}

int Binary::Set( u_char *bp, mddBuf b )
{
   u_char *cp;

   // <u_int>String

   cp  = bp;
   cp += _u_pack( cp, b._dLen );
   ::memcpy( cp, b._data, b._dLen );
   cp += b._dLen;
   return( cp-bp ); 
}

int Binary::Set( u_char *bp, u_char i8 )
{
   *bp = i8;
   return 1;
}

int Binary::Set( u_char *bp, u_short i16 )
{
   return _u_pack( bp, (u_int)i16 );
}

int Binary::Set( u_char *bp, u_int i32, bool bUnpacked )
{
   int sz;

   if ( bUnpacked ) {
      i32 = htonl( i32 );
      ::memcpy( bp, &i32, sizeof( i32 ) );
      sz = sizeof( i32 );
   }
   else
      sz = _u_pack( bp, i32 );
   return sz;
}

int Binary::Set( u_char *bp, u_int64_t i64 )
{
   return _u_pack( bp, i64 );
}

int Binary::Set( u_char *bp, float f )
{
   float r32;
   u_int i32;

   // 4 implied decimal places

   r32 = f * _f_mul;
   i32 = (u_int)r32;
   return _u_pack( bp, i32 );
}

int Binary::Set( u_char *bp, double d, double mul )
{
   double    r64;
   u_int64_t i64;
   bool      bPack;

   // 10 implied decimal places

   r64 = d * mul;
   i64 = (u_int64_t)r64;
   return _u_pack( bp, i64, bPack );
}

int Binary::Set( u_char *bp, mddReal r )
{
   u_char *cp;

   cp    = bp;
   cp   += Set( cp, r.value );
   *cp++ = r.hint;
   *cp++ = r.isBlank;
   return( cp-bp );
}


/////////////////////////////////////
// Wire Protocol - Unpacked
/////////////////////////////////////
#define _COPY_EITHER( v, cp, bSet )  \
   do {                              \
      void *pv = (void *)&v;         \
      void *d  = bSet ? cp : pv;     \
      void *s  = bSet ? pv : cp;     \
                                     \
      ::memcpy( d, s, sizeof( v ) ); \
      cp += sizeof( v );             \
   } while( 0 )

#define _COPY_SET( v, cp ) _COPY_EITHER( v, cp, true )
#define _COPY_GET( v, cp ) _COPY_EITHER( v, cp, false )

int Binary::_Get_unpacked( u_char *bp, mddField &f, bool bNeg )
{
   mddValue &v = f._val;
   mddReal  &r = v._real;
   u_char   *cp, ty;
   u_int     i32;
   u_int64_t i64;

   cp      = bp;
   ty      = (u_char)f._type;
   ty     &= ~UNPACKED_BINFLD;
   f._type = (mddFldType)ty;
   switch( f._type ) {
      case mddFld_undef:
         break;
      case mddFld_string:
         cp += Get( cp, v._buf );
         break;
      case mddFld_int32:
         _COPY_GET( v._i32, cp );
         if ( bNeg )
            v._i32 |= 0x80000000L;
         break;
      case mddFld_double:
         _COPY_GET( i64, cp );
         v._r64 = _d_div * i64;
         if ( bNeg )
            v._r64 = -v._r64;
         break;
      case mddFld_date:
         _COPY_GET( i64, cp );
         v._r64 = _dt_mul * i64;
         break;
      case mddFld_time:
      case mddFld_timeSec:
         _COPY_GET( i32, cp );
         v._r64 = i32;
         break;
      case mddFld_float:
         _COPY_GET( i32, cp );
         v._r32  = _f_div * i32;
         if ( bNeg )
            v._r32 = -v._r32;
         break;
      case mddFld_int8:
         _COPY_GET( v._i8, cp );
         break;
      case mddFld_int16:
         _COPY_GET( v._i16, cp );
         break;
      case mddFld_int64:
         _COPY_GET( v._i64, cp );
/*
 * TODO : ::int64_t instead of u_int64_t??
 *
         if ( bNeg )
            v._i64 = -v._i64;
 */
         break;
      case mddFld_real:
         _COPY_GET( r.value, cp );
         r.hint    = *cp++;
         r.isBlank = *cp++;
         break;
      case mddFld_bytestream:
         cp += Get( cp, v._buf );
         break;
      case mddFld_unixTime:
         _COPY_GET( v._i64, cp );
         break;
   }
   return( cp-bp );
}

int Binary::_Set_unpacked( u_char *bp, mddField f )
{
   mddValue &v = f._val;
   mddReal  &r = v._real;
   u_char   *cp;
   u_int     i32;
   u_int64_t i64;
   float     r32;
   double    r64;
   bool      bNeg;

   cp   = bp;
   bNeg = false;
   switch( f._type ) {
      case mddFld_undef:
         break;
      case mddFld_string:
         cp += Set( cp, v._buf );
         break;
      case mddFld_int32:
         bNeg = ( v._i32 & _NEG32 ) == _NEG32;
         i32  = ( v._i32 & _MSK32 );
         _COPY_SET( i32, cp );
         break;
      case mddFld_double:
         bNeg = ( v._r64 < 0.0 );
         r64  = bNeg ? -v._r64 : v._r64;
         r64 *= _d_mul;
         i64  = (u_int64_t)r64;
         _COPY_SET( i64, cp );
         break;
      case mddFld_date:
         v._r64 *= _dt_div;
         i64     = (u_int64_t)v._r64;
         _COPY_SET( i64, cp );
         break;
      case mddFld_time:
      case mddFld_timeSec:
         r64 = ::fmod( v._r64, _ymd_mul );
         r32 = r64;
         i32 = r32;
         _COPY_SET( i32, cp );
         break;
      case mddFld_float:
         bNeg = ( v._r32 < 0.0 );
         r32  = bNeg ? -v._r32 : v._r32;
         r32 *= _f_mul;
         i32  = (u_int)r32;
         _COPY_SET( i32, cp );
         break;
      case mddFld_int8:
         _COPY_SET( v._i8, cp );
         break;
      case mddFld_int16:
         _COPY_SET( v._i16, cp );
         break;
      case mddFld_int64:
         bNeg = ( v._i64 & _NEG64 ) == _NEG64;
         i64  = ( v._i64 & _MSK64 );
         _COPY_SET( i64, cp );
         break;
      case mddFld_real:
         cp += Set( cp, v._real );
         break;
      case mddFld_bytestream:
         _COPY_SET( r.value, cp );
         *cp++ = r.hint;
         *cp++ = r.isBlank;
         break;
      case mddFld_unixTime:
         _COPY_SET( v._i64, cp );
         break;
   }
   *bp |= bNeg ? _NEG8 : 0;
   return( cp-bp );
}


/////////////////////////////////////
// Packing Helpers
/////////////////////////////////////
u_int Binary::_TimeNow()
{
   double now, dMid, dd;
   u_int  rtn;

   // 100 micros since midnight

   now  = MDW_dNow();
   dMid = _tMidNt( now );
   dd   = ( now - dMid ) * _t_mul;
   rtn  = (u_int)dd;
   return rtn;
}

double Binary::_tMidNt( double tNow )
{
   time_t        t64;
   u_int         tOff;
   double        uS;
   struct tm    *tm, t;
   static double _SECPERDAY = 86400.0;

   // Reset, if needed

   if ( ( tNow - _dMidNt ) > _SECPERDAY ) {
      t64      = (time_t)tNow;
      tm       = ::localtime_r( &t64, &t );
      tOff     = ( tm->tm_hour * 3600 ) + ( tm->tm_min * 60 ) + tm->tm_sec;
      _dMidNt  = t64 - tOff;
      uS       = ( tNow - t64 ); 
      _dMidNt += uS;
   }
   return _dMidNt;
}

int Binary::_u_unpack( u_char *bp, u_int &i32 )
{
   u_int64_t i64;
   int       sz;

   i64 = i32;
   sz  = _u_unpack( bp, i64 );
   i32 = i64;
   return sz;
}  

int Binary::_u_unpack( u_char *bp, u_int64_t &i64 )
{
   int sz;

   /*
    * ILX-type packing InRange( 0, i64, 1073741823 )
    *   1) bp[0] & 0xe0 : 6-bytes : 536,870,911 <  i64 <= 17,592,186,044,416
    *   2) bp[0] & 0xc0 : 4-bytes :      16,383 <  i64 <= 536,870,911
    *   3) bp[0] & 0x80 : 2-bytes :         127 <  i64 <= 16,383
    *   3) Else         : 1-byte  :           0 <= i64 <= 127
    */
   if ( ( bp[0] & 0xe0 ) == 0xe0 ) {
      sz  = 6; 
      i64  = ( bp[0] & 0x1f ); i64 = ( i64 << 8 );
      i64 += bp[1];            i64 = ( i64 << 8 );
      i64 += bp[2];            i64 = ( i64 << 8 );
      i64 += bp[3];            i64 = ( i64 << 8 );
      i64 += bp[4];            i64 = ( i64 << 8 );
      i64 += bp[5];
   }      
   else if ( ( bp[0] & 0xc0 ) == 0xc0 ) {
      sz  = 4; 
      i64 = ( ( bp[0] & 0x3f ) << 24 ) +
              ( bp[1] << 16 ) +
              ( bp[2] <<  8 ) + 
                bp[3];
   }
   else if ( ( bp[0] & 0x80 ) == 0x80 ) {
      sz  = 2; 
      i64 = ( ( bp[0] & 0x3f ) << 8 ) + bp[1];
   }      
   else if ( ( bp[0] & 0x80 ) == 0 ) {
      i64 = *bp;
      sz  = 1;
   }
   else
assert( 0 );
   return sz;
}

int Binary::_u_pack( u_char *bp, u_int i32 )
{
   u_int64_t i64;
   bool      bPack;

   i64 = i32;
   return _u_pack( bp, i64, bPack );
}

static u_int64_t _l48 = 0x0fff;
static u_int64_t _u48 = ( _l48 << 32 ) + 0x00000000ffffffffL;

#define _MAX_U48  _u48
#define _MAX_U32  0x00001fffffff
#define _MAX_U16  0x000000003fff
#define _MAX_U8   0x00000000007f

int Binary::_u_pack( u_char *bp, u_int64_t i64, bool &bPack )
{
   u_int64_t u64;
   int       sz;

   /*
    * ILX-type packing InRange( 0, i64, 1073741823 )
    *   1) bp[0] & 0xe0 : 6-bytes : 536,870,911 <  i64 <= 17,592,186,044,416
    *   2) bp[0] & 0xc0 : 4-bytes :      16,383 <  i64 <= 536,870,911
    *   3) bp[0] & 0x80 : 2-bytes :         127 <  i64 <= 16,383
    *   3) Else         : 1-byte  :           0 <= i64 <= 127
    */
   bPack = true;
   if ( InRange( 0, i64, _MAX_U8 ) ) {
      *bp = (u_char)i64;
      sz  = 1;
   }
   else if ( InRange( _MAX_U8+1, i64, _MAX_U16 ) ) {
      bp[0]  = ( i64 & 0x0000bf00 ) >> 8;
      bp[1]  = ( i64 & 0x000000ff );
      sz     = 2;
      bp[0] |= 0x80;
   }
   else if ( InRange( _MAX_U16+1, i64, _MAX_U32 ) ) {
      bp[0]  = ( i64 & 0x3f000000 ) >> 24;
      bp[1]  = ( i64 & 0x00ff0000 ) >> 16;
      bp[2]  = ( i64 & 0x0000ff00 ) >>  8;
      bp[3]  = ( i64 & 0x000000ff );
      sz     = 4;
      bp[0] |= 0xc0;
   }
   else if ( InRange( _MAX_U32+1, i64, _MAX_U48 ) ) {
      u64    = ( i64 >> 32 );
      bp[0]  = ( u64 & 0x0700 ) >> 8;
      bp[1]  = ( u64 & 0x00ff );
      bp[2]  = ( i64 & 0x0000ff000000 ) >> 24;
      bp[3]  = ( i64 & 0x000000ff0000 ) >> 16;
      bp[4]  = ( i64 & 0x00000000ff00 ) >>  8;
      bp[5]  = ( i64 & 0x0000000000ff );
      sz     = 6;
      bp[0] |= 0xe0;
   }
   else {
      u64   = ( i64 >> 32 );
      bp[0] = ( u64 & 0x0000ff000000 ) >> 24;
      bp[1] = ( u64 & 0x000000ff0000 ) >> 16;
      bp[2] = ( u64 & 0x00000000ff00 ) >>  8;
      bp[3] = ( u64 & 0x0000000000ff );
      bp[4] = ( i64 & 0x0000ff000000 ) >> 24;
      bp[5] = ( i64 & 0x000000ff0000 ) >> 16;
      bp[6] = ( i64 & 0x00000000ff00 ) >>  8;
      bp[7] = ( i64 & 0x0000000000ff );
      sz    = 8;
      bPack = false;
   }
   return sz;
}

#ifdef TODO

/////////////////////////////////////////////////////////////////////////////
//
//               c l a s s        G L m d B i n T a b l e
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// Constructor / Destructor
/////////////////////////////////////
BinaryTable::BinaryTable( char *bp, int dLen ) :
   _b( (GLdynBuf *)0 ),
   _rOff( (u_long *)0 ),
   _bp( bp ),
   _len( dLen )
{
   char *cp;
   int   i, sz;

   // Copy in; Endian

   cp  = _bp;
   ::memcpy( &_rh, cp, sizeof( _rh ) );
   cp += sizeof( _rh );
   _rh._nRow = ntohl( _rh._nRow );
   _rh._nCol = ntohl( _rh._nCol );
   _rh._hdrs = ntohl( _rh._hdrs );
   if ( _rh._nRow ) {
      _rOff = new u_long[_rh._nRow];
      sz    = sizeof( u_long ) * nRow();
      ::memcpy( _rOff, cp, sz );
      for ( i=0; i<nRow(); i++ )
         _rOff[i] = ntohl( _rOff[i] );
   }
}

BinaryTable::BinaryTable( GLdynBuf &b, 
                          int       nRow, 
                          int       nCol, 
                          bool      bHdr ) :
   _b( &b ),
   _rOff( (u_long *)0 ),
   _bp( b.bp() ),
   _len( b.len() )
{
   BinaryTblHdr *rh;
   char          *cp;
   int            i, sz;

   // Ensure space for big rows

   sz = 0;
   if ( nRow ) {
      sz  = sizeof( u_long ) * nRow;
      sz += sizeof( BinaryTblHdr );
   }
   for ( ; sz>b.nLeft(); b.Grow() );
   _bp = b.bp();

   // Init

   b         = 0;
   _len      = b.len();
   rh        = (BinaryTblHdr *)b.bp();
   b        += sizeof( BinaryTblHdr );
   _rh._nRow = nRow;
   _rh._nCol = nCol;
   rh->_nRow = htonl( _rh._nRow );
   rh->_nCol = htonl( _rh._nCol );
   if ( _rh._nRow ) {
      _rOff = (u_long *)b.cp();
      sz    = sizeof( u_long ) * nRow;
      b    += sz;
      ::memset( _rOff, 0, sz );
   }
   _rh._hdrs = bHdr ? b.len() : 0;
   rh->_hdrs = htonl( _rh._hdrs );
}

BinaryTable::~BinaryTable()
{
   if ( !_b && _rOff )
      delete[] _rOff;
}


/////////////////////////////////////
// Access - Retrieve
/////////////////////////////////////
int BinaryTable::nRow()
{
   return _rh._nRow;
}

int BinaryTable::nCol()
{
   return _rh._nCol;
}

bool BinaryTable::HasHdr()
{
   return( _rh._hdrs != 0 );
}

char *BinaryTable::pHdr( int c, GLOString &rtn )
{
   char  *rp, *cp;
   u_long l;
   int    n;

   // Pre-condition

   rtn.jreplace( "" );
   if ( !nRow() )
      return rtn.pName();

   // Bounds-checking

   c  = WithinRange( 0, c, nCol() );
   rp = _bp + _rh._hdrs;
   cp = rp;
l = 0;
#ifdef FOO
   for ( n=0; n<c; n++ ) {
      cp += Binary::Get( cp, l );
      cp += l;
   }
   cp += Binary::Get( cp, l );
#endif // FOO

   GLOString t( cp, l );

   rtn.jreplace( t.pName() );
   return rtn.pName();
}

char *BinaryTable::pVal( int r, int c, GLOString &rtn )
{
   char  *rp, *cp;
   u_long l;
   int    n;

   // Pre-condition

   rtn.jreplace( "" );
   if ( !nRow() )
      return rtn.pName();

   // Bounds-checking

   r  = WithinRange( 0, r, nRow() );
   c  = WithinRange( 0, c, nCol() );
   rp = _bp + _rOff[r];
   cp = rp;
l = 0;
#ifdef FOO
   for ( n=0; n<c; n++ ) {
      cp += Binary::Get( cp, l );
      cp += l;
   }
   cp += Binary::Get( cp, l );
#endif // FOO

   GLOString t( cp, l );

   rtn.jreplace( t.pName() );
   return rtn.pName();
}


/////////////////////////////////////
// Operations - Store
/////////////////////////////////////
bool BinaryTable::AddHdr( int c, char *pv )
{
   return AddVal( c, pv );
}

bool BinaryTable::AddHdr( char *pv, char *sep )
{
   return AddVal( pv, sep );
}

bool BinaryTable::AddRow( int r )
{
   if ( _b && ( r<nRow() ) ) {
      _rOff[r] = htonl( _b->len() );
      return true;
   }
   return false;
}

bool BinaryTable::AddVal( int c, char *pv )
{
   static bool _bAddLen = true;
   char *pr;
   int   ro;

   // Update _bp and _rOff, as Append() can Grow() '_b'

   if ( _b && ( c<nCol() ) ) {
      _b->Append( pv, strlen( pv ), _bAddLen );
      pr    = (char *)_rOff;
      ro    = pr - _bp;
      _bp   = _b->bp();
      pr    = _bp + ro;
      _rOff = (u_long *)pr;
      return true;
   }
   return false;
}

bool BinaryTable::AddVal( char *pv, char *sep )
{
   RW94CString    rwb( pv );
   RW94CString    t1;
   RW94CTokenizer nxt( rwb );
   char          *p1;
   int            c;

   for ( c=0; !(t1=nxt(sep)).isNull(); c++ ) {
      p1 = (char *)t1.data();
      if ( !AddVal( c, p1 ) )
         return false;
   }
   return true;
}

#endif // TODO
