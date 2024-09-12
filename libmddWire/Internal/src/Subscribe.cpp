/******************************************************************************
*
*  Subscribe.cpp
*     MD-Direct subscription channel data
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     12 OCT 2013 jcs  Build  3: All mddDataType's
*     10 MAR 2014 jcs  Build  6: _MF_ParseHdr() : No Img/Upd w/o RS
*     12 NOV 2014 jcs  Build  8: -Wall
*     17 JAN 2015 jcs  Build  9: _MF_str2dbl(); _MF_ParseHdr() : slop
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*     29 MAR 2022 jcs  Build 13: Binary._bPackFlds
*     23 MAY 2022 jcs  Build 14: mddFld_unixTime
*     28 OCT 2022 jcs  Build 16: mddFld_vector
*     12 SEP 2024 jcs  Build 21: _Binary_ParseHdr() : Gracefully handle bad msg
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>
#include <GLedgDTD.h>

using namespace MDDWIRE_PRIVATE;

static char _sRS[] = { RS, '\0' };

/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      S u b s c r i b e
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Subscribe::Subscribe() :
   Data( false )
{
}

Subscribe::~Subscribe()
{
}


////////////////////////////////////////////
// API - Data Interface
////////////////////////////////////////////
int Subscribe::Parse( mddMsgBuf b, mddWireMsg &r )
{
   int sz;

   switch( _proto ) {
      case mddProto_Undef:  sz = 0;                     break;
      case mddProto_XML:    sz = _XML_Parse( b, r );    break;
      case mddProto_MF:     sz = _MF_Parse( b, r );     break;
      case mddProto_Binary: sz = _Binary_Parse( b, r ); break;
   }
   r._proto = _proto;
   return sz;
}

int Subscribe::ParseHdr( mddMsgBuf b, mddMsgHdr &r )
{
   b._hdr   = (mddMsgHdr *)0;
   r._proto = _proto;
   switch( r._proto ) {
      case mddProto_Undef:  return 0;
      case mddProto_XML:    return _XML_ParseHdr( b, r );
      case mddProto_MF:     return _MF_ParseHdr( b, r );
      case mddProto_Binary: return _Binary_ParseHdr( b, r );
   }
   return 0;
}


////////////////////////////////////////////
// MarketFeed
////////////////////////////////////////////
int Subscribe::_MF_Parse( mddMsgBuf b, mddWireMsg &r )
{
   bool          bUp;
   char         *us, *rs, *fs, *ms, *rp, *p1, *p2, *pSvc, *pTkr, *pUp;
   int           sz, nf, nb, estFlds;
   mddMsgHdr     h;
   mddFldDef    *def;
   mddFldType    fTy;
   mddFieldList &fl  = r._flds;
   mddBuf       &nm  = h._svc;

   // ENSURE : 1st byte == FS

   if ( b._data[0] != FS ) 
      return 0;
   nb = _MF_ParseHdr( b, h );
   if ( !nb )
      return 0;

   // 1) Estimated Fields = nb / 4 = <RS>1<US>2

   estFlds = (nb/4) + 1;
   _InitFieldList( fl, estFlds );

   // 2) Log, if required

   if ( _log ) {
      Locker lck( _log->mtx() );
      string s( b._data, nb );

      _log->logT( 1, "[MF-RX %06d bytes]\n    ", nb );
      _log->HexLog( 4, s.c_str(), nb );
   }

   /*
    * 3) Parse:
    *       <FS>340<US>Svc<GS>Ticker<US>RTL<US>tag<RS>fid<US>val<RS> ... <FS>
    *       <FS>316<US>Svc<GS>Ticker<US>tag<RS>fid<US>val<RS> ... <FS>
    */

   ms  = (char *)b._data;
   fs  = ms;
   fs += nb;
   rs  = (char *)::memchr( b._data, RS, nb );
   for ( nf=0; _bParseInC && rs && ( rs != fs ); nf++ ) {
      sz = nb - ( rs-b._data );
      us = (char *)::memchr( rs, US, sz );
      if ( !us )
         break;
      ms = rs;
      sz = nb - ( us-b._data );
      rs = (char *)::memchr( us, RS, sz );
      rs = !rs ? fs : rs;

      mddField &f  = fl._flds[nf];
      mddValue &v  = f._val;
      mddBuf   &b1 = v._buf;

      f._fid   = atoi( ms+1 );
      def      = _GetDef( f._fid );
      f._name  = def ? def->pName() : "Undefined";
      f._type  = mddFld_string;
      b1._data = us+1;
      b1._dLen = rs-us-1;
      if ( _bNativeFld && def ) {
         fTy = def->fType();
         switch( fTy ) {
            case mddFld_string:
               f._type = fTy;
               break;
            case mddFld_int32:
               v._i32  = atoi( us+1 );
               f._type = fTy;
               break;
            case mddFld_double:
               v._r64  = atof( us+1 );
               f._type = fTy;
               break;
            case mddFld_date:
               _MF_Time2Native( f, true );
               break;
            case mddFld_time:
            case mddFld_timeSec:
               _MF_Time2Native( f );
               break;
            case mddFld_undef:
            case mddFld_float:
            case mddFld_int8:
            case mddFld_int16:
            case mddFld_int64:
            case mddFld_real:
            case mddFld_bytestream:
            case mddFld_vector:
            case mddFld_unixTime:
               break;
         }
      }
   }

   // Trailing FS

   if ( nf ) {
      mddField &f   = fl._flds[nf-1];
      mddValue &v   = f._val;
      mddBuf   &b1  = v._buf;
      int       fSz = b1._dLen;

      if ( f._type == mddFld_string ) 
         b1._dLen -= fSz ? 1 : 0;
   }

   // 4) On-pass by msg type ...

   mddBuf &svc = r._svc;
   mddBuf &tkr = r._tkr;
   mddBuf &err = r._err;
   mddBuf &raw = r._rawData;

   _InitMsg( r, h );
   fl._nFld  = nf;
   raw._data = b._data;
   raw._dLen = nb;
   switch( r._mt ) {
      case mddMt_update:
      case mddMt_image:
         break;
      case mddMt_gblStatus:
      {
         string tmp( b._data+1, nb-2 );

         _gblSts = tmp;
         p1      = ::strtok_r( (char *)_gblSts.c_str(), _sRS, &rp );
         p2      = ::strtok_r( NULL, _sRS, &rp );
         if ( !p2 )
            return nb;
         pSvc     = ::strtok_r( p2, ":", &rp );
         pUp      = ::strtok_r( NULL, ":", &rp );
         bUp      = ( ::strcmp( pUp, "UP" ) == 0 );
         svc      = _SetBuf( pSvc );
         r._state = bUp ? mdd_up : mdd_down;
         break;
      }
      case mddMt_dead:
      {
         string tmp( b._data+1, nb-2 );
         string ric( nm._data, nm._dLen );

         _gblSts  = tmp;
         _tkrSts  = ric;
         pTkr     = (char *)_tkrSts.c_str();
         p1       = ::strtok_r( (char *)_gblSts.c_str(), _sRS, &rp );
         p2       = ::strtok_r( NULL, _sRS, &rp );
         p2       = p2 ? p2 : (char *)"Undefined MT_STAT_RES";
         svc      = _SetBuf( h._tag );
/*
 * 14-01-31 jcs  Build 5
 *
         tkr      = _SetBuf( pTkr );
 */
         tkr      = h._tkr;
         p1       = tkr._data;
         p1      += tkr._dLen;
         *p1      = '\0';
         err      = _SetBuf( p2 );
         r._state = mdd_down;
         break;
      }
      case mddMt_stale:
      case mddMt_recovering:
      case mddMt_undef:
      case mddMt_mount:
      case mddMt_ping:
      case mddMt_ctl:
      case mddMt_open:
      case mddMt_close:
      case mddMt_query:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }
   return nb;
}

int Subscribe::_MF_ParseHdr( mddMsgBuf d, mddMsgHdr &h )
{
   mddBuf &svc = h._svc;
   mddBuf &tkr = h._tkr;
   char   *data, *wp, *cp, *us, *u1, *u2, *rs, *gs, *fs, *beg;
   int     i, dLen, len, iVal, mt, nSz, hSz, mSz, slop;
   char    buf[K];

   /*
    * Pre-conditions:
    *    1) Not already parsed
    *    2) Full Message
    */
   if ( d._hdr ) {
      ::memcpy( &h, d._hdr, sizeof( h ) );
      return h._len;
   }
   fs  = (char *)0;
   beg = (char *)::memchr( d._data, FS, d._dLen );
   if ( beg ) {
      hSz  = beg - d._data;
      hSz += 1;
      fs   = (char *)::memchr( beg+1, FS, d._dLen-hSz );
   }
   if ( !fs )
      return 0;

   // Handle slop in front of message (e.g., unsolicited XML update)

   slop = beg - d._data;
   mSz  = slop;
   mSz += fs - beg;
   mSz += 1; // Trailing FS
if ( slop )
breakpoint();

   /////////////////////////////////////////////////////////
   // <FS>340<US>Service<GS>name<US>fieldList<US>rtl<RS> ...
   // <FS>318<US>Service<GS>name<US>fieldList<US>rtl<RS> ...
   // <FS>316<US>Service<GS>name<US>rtl<RS> ...
   /////////////////////////////////////////////////////////

   // Initial values

   h._len    = fs ? fs - beg : -1;
   h._len   += fs ? 1 : 0; // Trailing FS
   h._mt     = mddMt_undef;
   h._dt     = mddDt_FieldList;
   h._tag[0] = '\0';
   h._iTag   = -1;
   h._RTL    = -1;
   h._time   = 0.0;
   h._attrs  = _fz;
   svc._data = (char *)0;
   svc._dLen = 0;
   tkr._data = (char *)0;
   tkr._dLen = 0;

   // 1) Message type

   data      = beg;
   dLen      = fs - beg;
   cp        = data+1;
   wp        = cp;
   gs        = (char *)0;
   us        = (char *)0;
   u2        = (char *)0;
   rs        = (char *)0;
   mt        = 0;
   rs        = (char *)::memchr( cp, RS, dLen-1 );
   h._hdrLen = rs ? ( rs-cp ) : dLen-1;
   for ( i=1; i<h._hdrLen; i++,cp++ ) {
      switch( *cp ) { 
         case GS: 
            gs = cp;
         case US: 
            if ( *cp == US ) {
               u2 = ( us && !u2 ) ? cp : u2;
               us = !us ? cp : us;
            }
         case FS: 
            if ( !mt ) {
               len      = cp - wp; 
               ::memcpy( buf, wp, len );
               buf[len] = '\0';
               mt       = (mddMsgType)atoi( buf );
            }
            break;
      }   
   }
   if ( !mt ) {
      h._hdrLen = 0;
      return mSz; // h._len;
   }

   // 2) Tag

   if ( us ) {
      nSz = gs - (us+1);
      if ( nSz > 0 ) {
         svc = _SetBuf( us+1, nSz );
         nSz = gmin( nSz, K-1 );
         ::memcpy( h._tag, us+1, nSz );
         h._tag[nSz] = '\0';

      }
   }
   if ( gs ) {
      u2  = !u2 ? rs : u2;
      if ( u2 ) {
         u1  = u2-1;
         u2 -= ( *u1 == US ) ? 1 : 0;
      }
      nSz = u2 ? ( u2 - (gs+1) ) : 0;
      if ( nSz > 0 ) 
         tkr = _SetBuf( gs+1, nSz );
   }

   // 3) RTL / Template

   if ( (us=_memrchr( data, US, h._hdrLen )) ) {
      nSz = h._hdrLen - ( us-data );
      ::memcpy( buf, us+1, nSz );
      buf[nSz] = '\0';
      iVal     = atoi( buf );
      h._RTL  = len ? iVal : h._RTL;
      h._iTag = h._RTL;
   }
   switch( mt ) {
      case MT_UPDATE:      h._mt = mddMt_update;    break;
      case MT_REC_RES:     h._mt = mddMt_image;     break;
      case MT_STAT_RES:    h._mt = mddMt_dead;      break;
      case MT_STATUS_MESS: h._mt = mddMt_gblStatus; break;
      case MT_COM_RESP:    h._mt = mddMt_ctl;       break;
      case MT_PING:        h._mt = mddMt_ping;      break;
   }
   h._hdrLen += rs ? 1 : 0;  // Include RS
   if ( !rs ) {
      switch( h._mt ) {
         case mddMt_update:
         case mddMt_image:
            h._mt = mddMt_insAck;  // So caller can hook ...
            break;
         case mddMt_dead:
         case mddMt_stale:
         case mddMt_recovering:
         case mddMt_undef:
         case mddMt_mount:
         case mddMt_ping:
         case mddMt_ctl:
         case mddMt_open:
         case mddMt_close:
         case mddMt_query:
         case mddMt_insert:
         case mddMt_insAck:
         case mddMt_gblStatus:
         case mddMt_history:
         case mddMt_dbQry:
         case mddMt_dbTable:
            break;
      }
   }
   h._hdrLen += slop;
   return mSz; // h._len;
}

void Subscribe::_MF_Time2Native( mddField &f, bool bDate )
{
   mddValue             &v = f._val;
   mddBuf               &b = v._buf;
   char                *Y, *M, *D, *h, *m, *s, *ms;
   char                 mon[K];
   double               ymd;
   struct timeval       tv;
   struct tm           *tm, t;
   time_t               t64;
   int                  hms, sz, y;
   Str2IntMap::iterator it;
   static char         *_z = (char *)"00";

   /*
    * 16 SEP 2013  -> 20130916000000.000
    * 16 SEP 2013 10:34:56.123 -> 20130916103456.123
    * 20130916-10:34:56.123 -> 20130916103456.123
    * 10:34:56.123 -> 103456.123
    * 10:34        -> 103400.000
    */
   sz = b._dLen;
   if ( (M=(char *)::memchr( b._data, ' ', sz )) ) {
      string s( M+1, 3 );

      D  = b._data;
      M += 1;
      sz = b._dLen - ( M - b._data );
      if ( (Y=(char *)::memchr( M, ' ', sz )) ) {
         Y += 1;
         sz = b._dLen - ( Y - b._data );
         h  = (char *)::memchr( Y, ' ', sz );
         h  = !h ? _z : h+1;
      }
      else {
         h = _z;
         Y = _z;
      }
      if ( (it=_mons.find( s )) != _mons.end() )
         sprintf( (M=mon), "%d", (*it).second );
      else
         M = _z;
      ymd = ( 10000 * atoi( Y ) ) + ( 100 * atoi( M ) ) + atoi( D );
   }
   else if ( (M=(char *)::memchr( b._data, '-', sz )) ) {
      string s( b._data, 8 );

      ymd = atoi( s.c_str() );
      h   = M+1;
      sz -= 9;
   }
   else {
      h   = b._data;
      tv  = Logger::tvNow();
      t64 = (time_t)tv.tv_sec;
      ymd = 0;
      if ( (tm=::localtime_r( &t64, &t )) ) {
         y   = 1900 + tm->tm_year; 
         ymd = ( 10000 * y ) + ( 100 * ( 1+tm->tm_mon ) ) + tm->tm_mday;
      }
   }
   s  = (char *)0;
   ms = (char *)0;
   if ( (m=::strchr( h, ':' )) ) {
      if ( (s=::strchr( m+1, ':' )) )
         ms = ::strchr( s+1, '.' );
   }
   m       = !m  ? _z : m+1;
   s       = !s  ? _z : s+1;
   ms      = !ms ? _z : ms+1;
   hms     = ( 10000 * atoi( h ) ) + ( 100 * atoi( m ) ) + atoi( s );
   v._r64  = ( 1.0 * hms ) + ( 0.001 * atoi( ms ) );
   v._r64 += ( ymd * Binary::_ymd_mul );
   f._type = bDate ? mddFld_date : mddFld_timeSec;
}

double Subscribe::_MF_str2dbl( mddBuf b )
{
   const char *str, *num, *den;
   int         iNum, iDen, dSz;
   double      dv, dd;
   mddBuf      b1, b2;
   
   // 1) double : 123.5

   str = b._data;
   dv  = _MF_atofn( b );
   num = (const char *)::memchr( str, 0x20, b._dLen ); // ' '
   if ( !num )
      return dv;
   dSz = b._dLen - ( num-str );
   den = (const char *)::memchr( num, 0x2f, dSz ); // '/'
   if ( !den )
      return dv;
   
   // 2) fraction : 123 128/256
   
   b1._data = (char *)num;
   b1._dLen = den - num;
   b2._data = (char *)den + 1;
   b2._dLen = b._dLen - ( den-num );
   iNum     = _MF_atoin( b1 );
   iDen     = _MF_atoin( b2 );
   dd   = iDen ?  ( 1.0 * iNum ) / iDen : 0.0;
   dv  += dd;
   return dv;
}

double Subscribe::_MF_atofn( mddBuf b )
{
   char buf[K];
   int  sz;

   sz = gmin( K-1, b._dLen );
   ::memcpy( buf, b._data, sz );
   buf[sz] = '\0';
   return atof( buf );
}

int Subscribe::_MF_atoin( mddBuf b )
{
   char buf[K];
   int  sz;

   sz = gmin( K-1, b._dLen );
   ::memcpy( buf, b._data, sz );
   buf[sz] = '\0';
   return atoi( buf );
}



////////////////////////////////////////////
// Binary
////////////////////////////////////////////
int Subscribe::_Binary_Parse( mddMsgBuf b, mddWireMsg &r )
{
   mddFieldList &fl  = r._flds;
   mddBuf       &svc = r._svc;
   mddBuf        fb;
   mddBinHdr     h;
   mddField      f;
   mddBuf       &bb = f._val._buf;
   u_char       *bp, *cp;
   char         *rp, *pSvc, *pUp;
   bool          bUp;
   int           nb, nf, estFlds;

   // ENSURE : 1st byte == FS

   bp = (u_char *)b._data;
   cp = bp;
   nb = _Binary_ParseHdr( b, h );
   if ( !nb )
      return 0;

   // 1) Estimated FieldList size = # bytes (be safe)

   estFlds = h._len;
   _InitFieldList( fl, estFlds );

   // 2) Full Header

   Binary bin( false );  // Let the message tell us packing

   bp  = (u_char *)b._data;
   cp  = bp;
   cp += h._hdrLen;
   nf  = 0;
   _InitMsg( r, h );
   switch( r._dt ) {
      case mddDt_undef:
      case mddDt_Control:
         // All info copied in _InitMsg()
         break;
      case mddDt_FieldList:
      {
         r._state = mdd_up;
         r._tag   = h._tag;
         for ( nf=0; (cp-bp) < (int)h._len; nf++ ) {
            cp          += bin.Get( cp, f );
            fl._flds[nf] = f;
         }
         fl._nFld = nf;
assert( fl._nFld < fl._nAlloc );
         break;
      }
      case mddDt_BookOrder:
      case mddDt_BookPriceLvevel:
      case mddDt_BlobList:
      case mddDt_BlobTable:
         // TODO - Future Binary Data Formats to Support
         break;
      case mddDt_FixedMsg:
      {
         /*
          * Call out to loadable driver; 
          * Else pass thru unmodified
          */
         if ( _fcn ) {
            fb._data  = b._data;
            fb._dLen  = b._dLen;
            fb._data += h._hdrLen;
            fb._dLen -= h._hdrLen;
            if ( (*_fcn)( &fb, &fl ) )
               r._dt = mddDt_FieldList;
         }
         break;
      }
   }

   // FieldList used for specific mddMsgType's?

   if ( ( r._mt == mddMt_gblStatus ) && nf ) {
      f = fl._flds[0];

      string tmp( bb._data, bb._dLen );

      // TUNAHEAD:UP:0:127.0.0.1:34766

      _gblSts  = tmp;
      pSvc     = ::strtok_r( (char *)_gblSts.c_str(), ":", &rp );
      pUp      = ::strtok_r( NULL, ":", &rp );
      bUp      = ( ::strcmp( pUp, "UP" ) == 0 );
      svc      = _SetBuf( pSvc );
      r._state = bUp ? mdd_up : mdd_down;
   }
   return h._len;
}   

int Subscribe::_Binary_ParseHdr( mddMsgBuf b, mddMsgHdr &r )
{
   mddBinHdr h;
   int       sz;

   /*
    * Pre-conditions:
    *    1) Not already parsed
    *    2) Full Message
    */
   if ( b._hdr ) {
      ::memcpy( &r, b._hdr, sizeof( r ) );
      return r._len;
   }
   if ( !(sz=_Binary_ParseHdr( b, h )) )
      return 0;

   // Pack into mddMsgHdr

   Binary bin( false );  // Let the message tell us packing

   _InitMsgHdr( r, h._len, h._mt, h._dt );
   r._bPack  = h._bPack;
   r._iTag   = h._tag;
   r._RTL    = h._RTL;
   r._time   = bin.MsgTime( h );
   r._hdrLen = h._hdrLen;
   return r._len;
}

int Subscribe::_Binary_ParseHdr( mddMsgBuf b, mddBinHdr &h )
{
   u_char *bp, *cp;
   Binary  bin( false );  // Let the message tell us packing
   u_int   mSz;

   bp = (u_char *)b._data;
   cp = bp;
   if ( b._dLen < sizeof( h._len ) )
      return 0;
   bin.Get( cp, h._len, true );
mSz = h._len;
   if ( b._dLen < h._len )
      return 0;
/*
 * 24-09-12 jcs  Build 21: Pass her up : GLmdXmlSink handles this ...
 *
assert( mSz > 0 );
 */
   if ( mSz <= 0 )
      return -1;
   h._hdrLen = bin.Get( cp, h );
   cp       += h._hdrLen; 
   return( cp-bp );
}

