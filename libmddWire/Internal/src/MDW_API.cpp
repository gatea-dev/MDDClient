/***************************************************************************** 
*
*  MDW_API.cpp
*
*  REVISION HISTORY:
*     16 SEP 2013 jcs  Created (from librtEdge).
*     19 JUN 2014 jcs  Build  9: mddWire_Alloc() / Free()
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 11: MDW_SLEEP()
*      1 NOV 2022 jcs  Build 16: mddFld_vector; mddWire_vectorSize
*     16 MAR 2024 jcs  Build 20: mddWire_RealToDouble() / mddWire_DoubleToReal()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>
#include <GLedgDTD.h>

using namespace MDDWIRE_PRIVATE;

//////////////////////////////
// Internal Helpers
//////////////////////////////
#define WireMap  GLvector<mddWire *>

static WireMap        _wdb;
static Mutex          _mtx;
static int            _nCxt   = 1;
static struct timeval _tStart = { 0,0 };
static mddBuf         _bz;
static mddFieldList   _fz;

static void _Touch()
{
   if ( _tStart.tv_sec )
      return;

   // Once

   _tStart = _tvNow();
   ::memset( &_bz, 0, sizeof( _bz ) );
   ::memset( &_fz, 0, sizeof( _fz ) );
}

static mddWire *_GetWire( int cxt )
{
   mddWire *mdd;
   bool     bOK;

   bOK = InRange( 0, cxt, _wdb.size()-1 );
   mdd = bOK ? _wdb[cxt] : (mddWire *)0;
   return mdd;
}

static Subscribe *_GetSub( int cxt )
{
   mddWire *mdd;

   return (mdd=_GetWire( cxt )) ? mdd->_sub : (Subscribe *)0;
}

static Publish *_GetPub( int cxt )
{
   mddWire *mdd;

   return (mdd=_GetWire( cxt )) ? mdd->_pub : (Publish *)0;
}




/////////////////////////////////////////////////////////////////////////////
//
//                      E x p o r t e d   A P I
//
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////
// Subscription
//////////////////////////////
extern "C" {
mddWire_Context mddSub_Initialize()
{
   Locker   lck( _mtx );
   mddWire *sub;
   Logger  *lf;
   int      rtn;

   // 1) Logging

   _Touch();
   if ( (lf=Data::_log) )
      lf->logT( 3, "mddSub_Initialize()\n" );

   // 2) MDDContext object

   rtn = _nCxt++;
   sub = new mddWire( rtn, true );
   _wdb.InsertAt( sub, rtn );
   return rtn;
}

int mddSub_ParseMsg( mddWire_Context cxt, mddMsgBuf b, mddWireMsg *rtn )
{
   Subscribe *sub;
   int        mSz;

   sub = _GetSub( cxt );
   mSz = sub ? sub->Parse( b, *rtn ) : 0;
   return mSz;
}

int mddSub_ParseHdr( mddWire_Context cxt, mddMsgBuf b, mddMsgHdr *h )
{
   mddWire *mdd;
   int      mSz;

   mdd = _GetWire( cxt );
   mSz = mdd ? mdd->ParseHdr( b, *h ) : 0;
   return mSz;
}

void mddSub_Destroy( mddWire_Context cxt )
{
   mddWire *sub;
   Logger    *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddSub_Destroy( %d )\n", cxt );
   if ( (sub=_GetWire( cxt )) ) {
      _wdb.InsertAt( (mddWire *)0, cxt );
      delete sub;
   }
}


//////////////////////////////
// Publication
//////////////////////////////
mddWire_Context mddPub_Initialize()
{
   Locker   lck( _mtx );
   mddWire *pub;
   Logger  *lf;
   int      rtn;

   // 1) Logging

   _Touch();
   if ( (lf=Data::_log) )
      lf->logT( 3, "mddPub_Initialize()\n" );

   // 2) MDDContext object

   rtn = _nCxt++;
   pub = new mddWire( rtn, false );
   _wdb.InsertAt( pub, rtn );
   return rtn;
}

int mddPub_AddFieldList( mddWire_Context cxt, mddFieldList fl )
{
   Publish *pub;

   if ( (pub=_GetPub( cxt )) )
      return pub->AddFieldList( fl );
   return 0;
}

mddBuf mddPub_BuildMsg( mddWire_Context cxt, 
                        mddMsgHdr       h, 
                        mddBldBuf      *outBuf )
{
   mddBldBuf  &bb = *outBuf;
   mddWire    *mdd;
   mddBuf      rb;
   Publish    *pub;
   mddProtocol pro;

   // Auto-create in Subscribe-only channel, if needed

   if ( !(pub=_GetPub( cxt )) ) {
      if ( (mdd=_GetWire( cxt )) )
         mdd->_pub = new Publish() ;
      pub = _GetPub( cxt );
   }
   pro         = mddProto_Undef;  // Use Publish._proto
   bb._payload = bb._data;
   rb          = pub ? pub->BuildMsg( h, pro, bb ) : _bz;
   bb._dLen    = rb._dLen;
   return rb;
}

mddBuf mddPub_SetHdrTag( mddWire_Context cxt,
                         u_int           iTag,
                         mddBuf         *outBuf )
{
   mddWire *mdd;
   Publish *pub;

   // Auto-create in Subscribe-only channel, if needed

   if ( !(pub=_GetPub( cxt )) ) {
      if ( (mdd=_GetWire( cxt )) )
         mdd->_pub = new Publish() ;
      pub = _GetPub( cxt );
   }
   return pub ? pub->SetHdrTag( iTag, *outBuf ) : _bz;
}

mddBuf mddPub_BuildRawMsg( mddWire_Context cxt, 
                           mddMsgHdr       h, 
                           mddBuf          payload,
                           mddBldBuf      *outBuf )
{
   mddBldBuf &bb = *outBuf;
   mddWire   *mdd;
   mddBuf     rb;
   Publish   *pub;

   // Auto-create in Subscribe-only channel, if needed

   if ( !(pub=_GetPub( cxt )) ) {
      if ( (mdd=_GetWire( cxt )) )
         mdd->_pub = new Publish() ;
      pub = _GetPub( cxt );
   }
   bb._payload = bb._data;
   rb          = pub ? pub->BuildRawMsg( h, payload, bb ) : _bz;
   bb._dLen    = rb._dLen;
   return rb;
}

void mddPub_Destroy( mddWire_Context cxt )
{
   Publish *pub;
   Logger  *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddPub_Destroy( %d )\n", cxt );
   if ( (pub=_GetPub( cxt )) ) {
      _wdb.InsertAt( (mddWire *)0, cxt );
      delete pub;
   }
}


//////////////////////////////
// Control / Schema
//////////////////////////////
mddBuf mddWire_Ping( mddWire_Context cxt )
{
   mddWire *mdd;

   if ( (mdd=_GetWire( cxt )) )
      return mdd->Ping();
   return _bz;
}

void mddWire_ioctl( mddWire_Context cxt, mddIoctl ctl, void *arg )
{
   mddWire *mdd;
   Logger  *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddWire_Ioctl( %d ) : %d\n", cxt, ctl );
   if ( (mdd=_GetWire( cxt )) )
      mdd->Ioctl( ctl, arg );
}

void mddWire_SetProtocol( mddWire_Context cxt, mddProtocol pro )
{
   mddWire *mdd;
   Logger  *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddWire_SetProtocol( %d ) : %c\n", cxt, pro );
   if ( (mdd=_GetWire( cxt )) )
      mdd->SetProtocol( pro );
}

mddProtocol mddWire_GetProtocol( mddWire_Context cxt )
{
   mddWire *mdd;

   if ( (mdd=_GetWire( cxt )) )
      return mdd->GetProtocol();
   return mddProto_Undef;
}

int mddWire_SetSchema( mddWire_Context cxt, const char *ps )
{
   mddWire *mdd;
   Logger  *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddWire_SetSchema( %d )\n", cxt );
   if ( (mdd=_GetWire( cxt )) )
      return mdd->SetSchema( ps );
   return 0;
}

mddFieldList mddWire_GetSchema( mddWire_Context cxt )
{
   mddWire *mdd;
   Logger  *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddWire_GetSchema( %d )\n", cxt );
   if ( (mdd=_GetWire( cxt )) )
      return mdd->GetSchema();
   return _fz;
}

int mddWire_CopySchema( mddWire_Context cxtDst, mddWire_Context cxtSrc )
{
   mddWire *mddSrc, *mddDst;
   Logger  *lf;

   if ( (lf=Data::_log) )
      lf->logT( 3, "mddWire_CopySchema( %d, %d )\n", cxtDst, cxtSrc );
   if ( (mddSrc=_GetWire( cxtSrc )) && (mddDst=_GetWire( cxtDst )) )
      return mddDst->CopySchema( mddSrc->schema() );
   return 0;
}

mddField *mddWire_GetFldDefByFid( mddWire_Context cxt, int fid )
{
   mddWire  *mdd;
   mddField *rtn;

   rtn = (mddField *)0;
   if ( (mdd=_GetWire( cxt )) )
      rtn = mdd->GetFldDef( fid );
   return rtn;
}

mddField *mddWire_GetFldDefByName( mddWire_Context cxt, const char *pFld )
{
   mddWire  *mdd;
   mddField *rtn;

   rtn = (mddField *)0;
   if ( (mdd=_GetWire( cxt )) )
      rtn = mdd->GetFldDef( pFld );
   return rtn;
}


//////////////////////////////
// Data Conversion - FieldList
//////////////////////////////
mddBuf mddWire_ConvertFieldList( mddWire_Context cxt, mddConvertBuf c )
{
   mddWireMsg &m   = *c._msgIn;
   mddProtocol pro = c._proto;
   mddBldBuf  &bb = *c._bufOut;
   Publish    *pub;
   mddWire    *mdd;
   mddBuf      rb;
   bool        bFldNm;

  // Auto-create in Subscribe-only channel, if needed

   if ( !(pub=_GetPub( cxt )) ) {
      if ( (mdd=_GetWire( cxt )) )
         mdd->_pub = new Publish() ;
      pub = _GetPub( cxt );
   }
   bb._payload = bb._data;
   bFldNm      = ( c._bFldNm != 0 );
   rb          = pub ? pub->ConvertFieldList( m, pro, bb, bFldNm ) : _bz;
   bb._dLen    = rb._dLen;
   return rb;
}


//////////////////////////////
// Real Conversion
//////////////////////////////
static double _r_mul[] = { 1.0,
                           10.0,
                           100.0,
                           1000.0,
                           10000.0,
                           100000.0,
                           1000000.0,
                           10000000.0,
                           100000000.0,
                           1000000000.0,
                           10000000000.0,
                           100000000000.0,
                           1000000000000.0,
                           10000000000000.0,
                           100000000000000.0,
                         };

static double _d_mul[] = { 1.0,
                           0.1,
                           0.01,
                           0.001,
                           0.0001,
                           0.00001,
                           0.000001,
                           0.0000001,
                           0.00000001,
                           0.000000001,
                           0.0000000001,
                           0.00000000001,
                           0.000000000001,
                           0.0000000000001,
                           0.00000000000001,
                         };

double mddWire_RealToDouble( mddReal r )
{
   double  d;
   int64_t i64;
   int     hint;

   // Pre-condition

   if ( r.isBlank )
      return 0.0;

   // Support negative numbers

   hint = WithinRange( 0, r.hint, _MAX_REAL_HINT );
   i64  = (int64_t)r.value;
   d    = _d_mul[hint] * i64;
   return d;
}

mddReal mddWire_DoubleToReal( double d, int hint )
{
   mddReal r;
   double  r64;
   int64_t i64;

   // 1) Support negative values

   hint = WithinRange( 0, hint, _MAX_REAL_HINT );
   r64  = d * _r_mul[hint];
   i64  = (int64_t)r64;

   // 2) Stuff it in

   r.value   = i64;
   r.hint    = hint;
   r.isBlank = 0;
   return r;
}


//////////////////////////////
// Library Management
//////////////////////////////
const char *mddWire_Version()
{
   return libmddWireID();
}

void mddWire_Log( const char *pFile, int debugLevel )
{
   // Pre-condition

   if ( !pFile || !strlen( pFile ) )
      return;

   // Blow away existing ...

   if ( Data::_log )
      delete Data::_log;
   Data::_log = new Logger( pFile, debugLevel );
}
 

//////////////////////////////
// Utilities
//////////////////////////////
double mddWire_TimeNs()
{
   return Logger::dblNow();
}

time_t mddWire_TimeSec()
{
   return Logger::tmNow();
}

char *mddWire_pDateTimeMs( char *buf )
{
   double     dNoW = mddWire_TimeNs();
   time_t     tNoW = (time_t)dNoW;
   struct tm *tm, l;
   int        tMs;

   tMs = (int)( 1000.0 * ( dNoW - tNoW ) );
   tm  = ::localtime_r( &tNoW, &l );
   l   = *tm;
   sprintf( buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
                    l.tm_year+1900, l.tm_mon+1, l.tm_mday,
                    l.tm_hour, l.tm_min, l.tm_sec, tMs );
   return buf;
}

char *mddWire_pTimeMs( char *buf )
{
   double     dNoW = mddWire_TimeNs();
   time_t     tNoW = (time_t)dNoW;
   struct tm *tm, l;
   int        tMs;

   tMs = (int)( 1000.0 * ( dNoW - tNoW ) );
   tm  = ::localtime_r( &tNoW, &l );
   l   = *tm;
   sprintf( buf, "%02d:%02d:%02d.%03d", l.tm_hour, l.tm_min, l.tm_sec, tMs );
   return buf;
}

int mddWire_Time2Mark( int h, int m, int s )
{
   time_t     tNow, tMrk;
   struct tm *tm, l;

   tNow      = mddWire_TimeSec();
   tm        = ::localtime_r( &tNow, &l );
   l         = *tm;
   l.tm_hour = h;
   l.tm_min  = m;
   l.tm_sec  = s;
   tMrk      = ::mktime( &l );
   tMrk     += ( tMrk < tNow ) ? 86400 : 0;
   return( tMrk - tNow );
}

void mddWire_Sleep( double dSlp )
{
   MDW_SLEEP( dSlp );
}

int mddWire_hexMsg( char *msg, int len, char *obuf )
{
   char       *op, *cp;
   int         i, off, n, oLen;
   const char *rs[] = { "<FS>", "<GS>", "<RS>", "<US>" };

   op = obuf;
   cp = msg;
   for ( i=n=0; i<len; cp++, i++ ) {
      if ( IsAscii( *cp ) ) {
         *op++ = *cp;
         n++;
      }
      else if ( InRange( FS, *cp, US ) ) {
         off  = ( *cp - FS );
         oLen = sprintf( op, rs[off] );
         op  += oLen;
         n   += oLen;
      }
      else {
         oLen = sprintf( op, "<%02x>", ( *cp & 0x00ff ) );
         op  += oLen;
         n   += oLen;
      }
      if ( n > 72 ) {
         sprintf( op, "\n    " );
         op += strlen( op );
         n=0;
      }
   }
   op += sprintf( op, "\n" );
   *op = '\0';
   return op-obuf;
}

int mddWire_vectorSize( mddBuf b )
{
   u_int i32;

   i32 = b._dLen - 1; // hint
   return b._dLen / sizeof( u_int64_t );
}


/////////////////////////
// Fucking WIN32 dll
/////////////////////////
mddFieldList mddFieldList_Alloc( int nAlloc )
{
   mddFieldList fl;

   ::memset( &fl, 0, sizeof( fl ) );
   fl._flds   = new mddField[nAlloc];
   fl._nAlloc = nAlloc;
   return fl;
}

void mddFieldList_Free( mddFieldList fl )
{
   if ( fl._flds )
      delete[] fl._flds;
}

mddBldBuf mddBldBuf_Alloc( int nAlloc )
{
   mddBldBuf bb;

   ::memset( &bb, 0, sizeof( bb ) );
   bb._data   = new char[nAlloc];
   bb._nAlloc = nAlloc;
   return bb;
}

void mddBldBuf_Free( mddBldBuf bb )
{
   if ( bb._data )
      delete[] bb._data;
}


/////////////////////////
// Linux Compatibility
/////////////////////////
#ifdef WIN32
struct tm *localtime_r( const time_t *tm, struct tm *out )
{
   struct tm *rtn;

   rtn  = ::localtime( tm );
   *out = *rtn;
   return out;
}

char *strtok_r( char *str, const char *delim, char **notUsed )
{
   return ::strtok( str, delim );
}

#endif // WIN32

} // extern "C"
