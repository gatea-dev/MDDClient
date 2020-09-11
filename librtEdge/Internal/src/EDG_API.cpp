/******************************************************************************
*
*  EDG_API.cpp
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*      2 SEP 2010 jcs  Build  6: LVC
*     18 SEP 2010 jcs  Build  7: LVC admin channel
*     22 SEP 2010 jcs  Build  8: Publication Channel
*      1 OCT 2010 jcs  Build  8a:time_t, not long
*     30 DEC 2010 jcs  Build  9: LVCData._dSnap; LVC_SetFilter()
*     13 JAN 2011 jcs  Build 10: LVC_View() / LVC_ViewAll()
*     12 JUL 2011 jcs  Build 14: rtEdge_XxxField()
*     29 JUL 2011 jcs  Build 15: rtEdge_GetSchema()
*     20 JAN 2012 jcs  Build 17: Log : strlen( pFile ); FieldByID()
*     20 MAR 2012 jcs  Build 18: rtEdge_Conflate() / rtEdge_Dispatch()
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*     20 OCT 2012 jcs  Build 20: ChartDB
*     15 NOV 2012 jcs  Build 21: rtEdge_CPU()
*     14 FEB 2013 jcs  Build 24: rtEdge_ioctl()
*      7 MAY 2013 jcs  Build 25: rtBUF
*     10 JUL 2013 jcs  Build 26: rtEdge_PubInitSchema(); int rtEdge_Publish()
*     11 JUL 2013 jcs  Build 26a:rtEdge_SetMonStats()
*     12 NOV 2014 jcs  Build 28: GLlvcLock; rtEdge_MapFile; RTEDGE_PRIVATE
*      8 JAN 2015 jcs  Build 29: int rtEdge_Unsubscribe(); srand()
*     20 JUN 2015 jcs  Build 31: rtEdge_PubGetData()
*      6 FEB 2016 jcs  Build 32: Linux compatibility in libmddWire; Binary LVC
*      3 JUL 2016 jcs  Build 33: AsciiFileMap
*     13 JUL 2017 jcs  Build 34: Socket "has-a" Thread; OS_GetXxxStats()
*     24 SEP 2017 jcs  Build 35: Cockpit; No mo LVC_XxxTicker()
*     12 OCT 2017 jcs  Build 36: rtBuf64; tape
*      7 NOV 2017 jcs  Build 38: OS_GetFileSize() / rtEdge_RemapFile()
*     20 JAN 2018 jcs  Build 39: Lock LVC during Snapshot / SnapAll()
*      6 MAR 2018 jcs  Build 40: OS_XxxThread()
*      6 APR 2020 jcs  Build 43: rtEdge_Destroy : thr().Stop()
*      7 SEP 2020 jcs  Build 44: MDD_Query()
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>
#include <OS_cpu.h>
#include <OS_disk.h>

static DWORD _LVCwaitMillis = INFINITE;
static bool  _LVClock       = false;

using namespace RTEDGE_PRIVATE;

//////////////////////////////
// Internal Helpers
//////////////////////////////

/*
 * The rtEdge_Context is simply an integer used to find the EdgChannel 
 * object in the collection below.  An integer-based index is safer 
 * that passing the EdgChannel object reference back to the user since
 * the user may call an API after calling rtEdge_Destroy() and not crash 
 * the library if we are index-based; If object-based, we crash.
 */

#define _MAX_ENG 64*K

static EdgChannel    *_subs[_MAX_ENG];
static PubChannel    *_pubs[_MAX_ENG];
static LVCDef        *_lvc[_MAX_ENG];
static GLchtDb       *_cdb[_MAX_ENG];
static Cockpit       *_cock[_MAX_ENG];
static Thread        *_work[_MAX_ENG];
static long           _nCxt   = 1;
static struct timeval _tStart = { 0,0 };

static void _Touch()
{
   // Once

   if ( _tStart.tv_sec )
      return;

   // Initialize _subs, _pubs, etc.

   _tStart = _tvNow();
   ::srand( _tStart.tv_sec );
   ::memset( &_subs, 0, sizeof( _subs ) );
   ::memset( &_pubs, 0, sizeof( _pubs ) );
   ::memset( &_lvc, 0, sizeof( _lvc ) );
   ::memset( &_cdb, 0, sizeof( _cdb ) );
   ::memset( &_cock, 0, sizeof( _cock ) );
   ::memset( &_work, 0, sizeof( _work ) );
}

static EdgChannel *_GetSub( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _subs[cxt];
   return (EdgChannel *)0;
}

static PubChannel *_GetPub( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _pubs[cxt];
   return (PubChannel *)0;
}

static LVCDef *_GetLVC( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _lvc[cxt];
   return (LVCDef *)0;
}

static GLchtDb *_GetCDB( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _cdb[cxt];
   return (GLchtDb *)0;
}

static Cockpit *_GetCockpit( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _cock[cxt];
   return (Cockpit *)0;
}

static Thread *_GetThread( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _work[cxt];
   return (Thread *)0;
}

static int _GetPageSize()
{
   int rtn;
#ifdef WIN32
   SYSTEM_INFO si;

   ::GetSystemInfo( &si );
   rtn = si.dwAllocationGranularity;
#else
   rtn = ::getpagesize();
#endif // WIN32
   return rtn;
}


/////////////////////////////////////////////////////////////////////////////
//
//                      E x p o r t e d   A P I
//
/////////////////////////////////////////////////////////////////////////////

static int _nWinsock = 0;
static GLmmapView *_mmMon = (GLmmapView *)0;
static GLlibStats *_stats = (GLlibStats *)0;

static void StartWinsock()
{
   char *pp, *pl, *pd, *rp;

   // Log from Environment

   if ( !_nWinsock && (pp=::getenv( "RTEDGE_LOG" )) ) {
      pl = ::strtok_r( pp,   ":", &rp );
      pd = ::strtok_r( NULL, ":", &rp );
      if ( pl && pd ) 
         ::rtEdge_Log( pl, atoi( pd ) );
   }
   // Windows Only
#ifdef WIN32
   char    buf[1024];
   int     rtn;
   WSADATA wsa;

   // 1) Start WINSOCK

   if ( !_nWinsock && (rtn=::WSAStartup( MAKEWORD( 1,1 ), &wsa )) != 0 ) {
      sprintf( buf, "WSAStartup() error - %d", rtn );
      ::MessageBox( NULL, buf, "WSAStartup()", MB_OK );
      return;
   }
#endif // WIN32
   _nWinsock++;
}

static void StopWinsock()
{
   _nWinsock--;
#ifdef WIN32
   if ( !_nWinsock )
      ::WSACleanup();
#endif // WIN32
}


//////////////////////////////
// Initialization, Connection
//////////////////////////////
extern "C" {
rtEdge_Context rtEdge_Initialize( rtEdgeAttr attr )
{
   EdgChannel *edg;
   Logger     *lf;
   long        rtn;

   // 1) Logging; WIN32

   _Touch();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Initialize( %s )\n", attr._pSvrHosts );
   StartWinsock();

   // 2) EdgChannel object

   rtn = ATOMIC_INC( &_nCxt );
   edg = new EdgChannel( attr, rtn );
   _subs[rtn] = edg;

   // 3) Stats / Return

   if ( _stats && !_stats->_nSub ) {
      _stats->_nSub += 1;
      edg->SetStats( &_stats->_sub );
      safe_strcpy( edg->stats()._chanName, "SUBSCRIBE" );
   }
   return rtn;
}

const char *rtEdge_Start( rtEdge_Context cxt )
{
   EdgChannel  *edg;
   TapeChannel *tape;
   Logger      *lf;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Start()\n" );

   // Operation

   if ( (edg=_GetSub( (int)cxt )) ) {
      if ( (tape=edg->tape()) )
         return tape->Load() ? "OK" : tape->err();
      return edg->Connect();
   }
   return (const char *)0;
}

void rtEdge_Destroy( rtEdge_Context cxt )
{
   EdgChannel      *edg;
   Logger          *lf;
   rtEdgeChanStats *st;

   // 1) EdgChannel object / Stats

   if ( (edg=_GetSub( cxt )) ) {
      st  = &edg->stats();
      edg->thr().Stop();
      delete edg;
      _subs[cxt] = (EdgChannel *)0;

      // Stats

      if ( _stats && ( st == &_stats->_sub ) )
         _stats->_nSub -= 1;
   }

   // 2) Kill winsock; Logging

   StopWinsock();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Destroy() Done!!\n" );
}

void rtEdge_ioctl( rtEdge_Context cxt, rtEdgeIoctl ty, void *arg )
{
   EdgChannel *edg;
   PubChannel *pub;
   Cockpit    *pit;
   Logger     *lf;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_ioctl( %d )\n", ty );

   // Operation

   if ( (edg=_GetSub( (int)cxt )) )
      edg->Ioctl( ty, arg );
   else if ( (pub=_GetPub( (int)cxt )) )
      pub->Ioctl( ty, arg );
   else if ( (pit=_GetCockpit( (int)cxt )) )
      pit->Ioctl( ty, arg );
}

char rtEdge_SetStats( rtEdge_Context cxt, rtEdgeChanStats *st )
{
   Logger     *lf;
   EdgChannel *edg;
   PubChannel *pub;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_SetStats( %d )\n", cxt );

   // Operations

   edg = (EdgChannel *)0;
   pub = (PubChannel *)0;
   if ( (edg=_GetSub( (int)cxt )) )
      edg->SetStats( st );
   else if ( (pub=_GetPub( (int)cxt )) )
      pub->SetStats( st );
   return ( edg  || pub ) ? 1 : 0;
}

const char *rtEdge_Version()
{
   static string _ver;

   // Once

   if ( !_ver.size() ) {
      _ver  = librtEdgeID();
      _ver += "\n";
      _ver += ::mddWire_Version();
   }
   return _ver.data();
}


//////////////////////////////
// Subscription - rtEdgeCache
//////////////////////////////
int rtEdge_Subscribe( rtEdge_Context cxt,
                      const char    *pSvc,
                      const char    *pTkr,
                      void          *arg )
{
   EdgChannel *edg;
   Logger     *lf;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Subscribe( %s,%s )\n", pSvc, pTkr );

   // Operation : Create, if not found

   if ( (edg=_GetSub( (int)cxt )) )
      return edg->Subscribe( pSvc, pTkr, arg );
   return 0;
}

char rtEdge_GetSchema( rtEdge_Context cxt, rtEdgeData *rtn )
{
   EdgChannel *edg;
   Logger     *lf;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_GetSchema()\n" );

   // Operation : Create, if not found

   ::memset( rtn, 0, sizeof( *rtn ) );
   if ( (edg=_GetSub( (int)cxt )) )
      *rtn = edg->GetSchema();
   return rtn->_nFld ? 1 : 0;
}

rtFIELD rtEdge_GetField( rtEdge_Context cxt, const char *pFld )
{
   EdgChannel *edg;
   Logger     *lf;
   rtFIELD    *fld, fz;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_GetField( %s )\n", pFld );

   // Operation

   edg = _GetSub( (int)cxt );
   if ( !edg || !(fld=edg->GetField( pFld )) ) {
      fld = &fz;
      ::memset( fld, 0, sizeof( *fld ) );
   }
   return *fld;
}

rtFIELD rtEdge_GetFieldByID( rtEdge_Context cxt, int fid )
{
   EdgChannel *edg;
   Logger     *lf;
   rtFIELD    *fld, fz;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_GetFieldByID( %d )\n", fid );

   // Operation

   edg = _GetSub( (int)cxt );
   if ( !(fld=edg->GetField( fid )) ) {
      fld = &fz;
      ::memset( fld, 0, sizeof( *fld ) );
   }
   return *fld;
}

char rtEdge_HasField( rtEdge_Context cxt, const char *pFld )
{
   EdgChannel *edg;
   Logger     *lf;
   bool        rtn;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_HasField( %s )\n", pFld );

   // Operation : Create, if not found

   edg = _GetSub( (int)cxt );
   rtn = edg ? edg->HasField( pFld ) : false;
   return rtn ? 1 : 0;
}

char rtEdge_HasFieldByID( rtEdge_Context cxt, int fid )
{   
   EdgChannel *edg;
   Logger     *lf;
   bool        rtn;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_HasFieldByID( %d )\n", fid );

   // Operation : Create, if not found

   edg = _GetSub( (int)cxt );
   rtn = edg ? edg->HasField( fid ) : false;
   return rtn ? 1 : 0;
}

int rtEdge_Unsubscribe( rtEdge_Context cxt, 
                        const char    *pSvc,
                        const char    *pTkr )
{
   EdgChannel *edg;
   Logger     *lf;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Unsubscribe( %s,%s )\n", pSvc, pTkr );

   // Operation : Create, if not found

   if ( (edg=_GetSub( (int)cxt )) )
      return edg->Unsubscribe( pSvc, pTkr );
   return 0;
}


//////////////////////////////
// Subscription Channel Cache 
//////////////////////////////
rtEdgeData rtEdge_GetCache( rtEdge_Context cxt, 
                            const char    *pSvc, 
                            const char    *pTkr )
{
   rtEdgeData  d;
   EdgChannel *edg;

   ::memset( &d, 0, sizeof( d ) );
   if ( (edg=_GetSub( (int)cxt )) )
      d = edg->GetCache( pSvc, pTkr );
   return d;
}

rtEdgeData rtEdge_GetCacheByStreamID( rtEdge_Context cxt, int StreamID )
{
   rtEdgeData  d;
   EdgChannel *edg;

   ::memset( &d, 0, sizeof( d ) );
   if ( (edg=_GetSub( (int)cxt )) )
      d = edg->GetCache( StreamID );
   return d;
}


//////////////////////////////
// Conflation - Subscribe
//////////////////////////////
void rtEdge_Conflate( rtEdge_Context cxt, char conflate )
{
   EdgChannel *edg;
   Logger     *lf;
   bool        bConflate;

   // Logging; Find EdgChannel

   bConflate = ( conflate != 0 ) ? true : false;
   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Conflate( %s )\n", bConflate ? "ON" : "OFF" );

   // Operation : Create, if not found

   if ( (edg=_GetSub( (int)cxt )) )
      edg->Conflate( bConflate );
}

int rtEdge_Dispatch( rtEdge_Context cxt, int maxUpd, double dWait )
{
   EdgChannel *edg;
   Logger     *lf;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Dispatch( %d,%.3f )\n", maxUpd, dWait );

   // Operation : Create, if not found

   if ( (edg=_GetSub( (int)cxt )) )
      return edg->Dispatch( maxUpd, dWait );
   return 0;
}

int rtEdge_Read( rtEdge_Context cxt, double dWait, rtEdgeRead *rd )
{
   EdgChannel *edg;
   Logger     *lf;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Read( %.3f )\n", dWait );

   // Operation : Create, if not found

   if ( (edg=_GetSub( (int)cxt )) )
      return edg->Read( dWait, *rd );
   return 0;
}


//////////////////////////////
// Publication
//////////////////////////////
rtEdge_Context rtEdge_PubInit( rtEdgePubAttr attr )
{
   PubChannel *pub;
   Logger     *lf;
   const char *ph, *pn;
   long        rtn;

   // 1) Logging; Winsock

   _Touch();
   ph = attr._pSvrHosts;
   pn = attr._pPubName;
   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_PubInit( %s,%s )\n", ph, pn );
   StartWinsock();

   // 2) PubChannel object

   rtn = ATOMIC_INC( &_nCxt );
   pub = new PubChannel( attr, rtn );
   _pubs[rtn] = pub;

   // 3) Stats / Return

   if ( _stats && !_stats->_nPub ) { 
      _stats->_nPub += 1;
      pub->SetStats( &_stats->_pub );
      safe_strcpy( pub->stats()._chanName, "PUBLISH" );
   }
   return rtn;
}

void rtEdge_PubInitSchema( rtEdge_Context cxt, rtEdgeDataFcn schemaCbk )
{
   PubChannel *pub;
   Logger     *lf;

   // 1) Logging; Winsock

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_PubInitSchema()\n" );

   // 2) PubChannel object

   // Operation

   if ( (pub=_GetPub( (int)cxt )) )
      pub->InitSchema( schemaCbk );
}

const char *rtEdge_PubStart( rtEdge_Context cxt )
{
   PubChannel *pub;
   Logger     *lf;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_PubStart()\n" );

   // Operation

   if ( (pub=_GetPub( (int)cxt )) )
      return pub->Connect();
   return (const char *)0;
}

void rtEdge_PubDestroy( rtEdge_Context cxt )
{
   PubChannel      *pub;
   Logger          *lf;
   rtEdgeChanStats *st;

   // 1) EdgChannel object

   if ( (pub=_GetPub( cxt )) ) {
      st  = &pub->stats();
      pub->thr().Stop();
      delete pub; 
      _pubs[cxt] = (PubChannel *)0; 

      // Stats

      if ( _stats && ( st == &_stats->_pub ) )
         _stats->_nPub -= 1; 
   }

   // 2) Kill winsock; Loggin

   StopWinsock();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_PubDestroy() Done!!\n" );
}

int rtEdge_Publish( rtEdge_Context cxt,
                    rtEdgeData     d )
{
   PubChannel *pub;
   Logger     *lf;
   int         rtn;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_Publish( %s,%s )\n", d._pSvc, d._pTkr );

   // Publish, if found

   pub = _GetPub( (int)cxt );
   rtn = pub ?  pub->Publish( d ) : 0;
   return rtn;
}

int rtEdge_PubError( rtEdge_Context cxt, rtEdgeData d )
{
   PubChannel *pub;
   Logger     *lf;
   const char *svc, *tkr, *err;
   int         rtn;

   // Logging; Find EdgChannel

   svc = d._pSvc;
   tkr = d._pTkr;
   err = !d._pErr ? "DEAD" : d._pErr;
   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_PubError( %s,%s ) : %s\n", svc, tkr, err );

   // Publish, if found

   pub = _GetPub( (int)cxt );
   rtn = pub ?  pub->PubError( d, err ) : 0;
   return rtn;
}

rtBUF rtEdge_PubGetData( rtEdge_Context cxt )
{
   PubChannel *pub;
   Logger     *lf;
   rtBUF       b;

   // Logging; Find EdgChannel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "rtEdge_PubGetData()\n" );

   // Publish, if found

   b._data = (char *)0;
   b._dLen = 0;
   if ( (pub=_GetPub( (int)cxt )) )
      b = pub->PubGetData();
   return b;
}



//////////////////////////////
// Snapshot - LVC
//////////////////////////////
void LVC_SetLock( char bLock, long dwWaitMillis )
{
   _LVClock       = bLock ? true : false;
   _LVCwaitMillis = dwWaitMillis;
}

LVC_Context LVC_Initialize( const char *pFile )
{
   LVCDef *lvc;
   Logger *lf;
   long    rtn;

   // 0) Logging

   _Touch();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "LVC_Initialize( %s )\n", pFile );

   // 1) GLlvcDb object

   rtn       = ATOMIC_INC( &_nCxt );
   lvc       = new LVCDef( pFile, _LVClock, _LVCwaitMillis );
   _lvc[rtn] = lvc;
   return rtn;
}

int LVC_GetSchema( LVC_Context cxt, LVCData *rtn )
{
   LVCData     &d = *rtn;
   LVCDef      *ld;
   GLlvcFldDef *fdb;
   rtFIELD      f;
   rtVALUE     &v = f._val;
   rtBUF       &b = v._buf;
   char        *pn;
   int          i, sz;

   // Operation : Create, if not found

   ::memset( rtn, 0, sizeof( *rtn ) );
   if ( (ld=_GetLVC( (int)cxt )) ) {
      GLlvcDb &lvc = ld->lvc();

      // Pre-conditions

      if ( !lvc.isValid() )
         return 0;
      if ( !lvc.IsLocked() )
         return 0;
      if ( !(d._nFld=lvc.db()._nFlds) )
         return 0;

      // OK to copy

      d._flds     = new rtFIELD[d._nFld];
      d._bShallow = 0;
      d._copy     = (char *)0;
      fdb         = lvc.fdb();
      for ( i=0; i<d._nFld; i++ ) {
         pn      = fdb[i]._name;
         sz      = strlen( pn );
         f._fid  = fdb[i]._fid;
         b._data = new char[sz+1];
         b._dLen = sz;
         ::memcpy( b._data, pn, sz );
         b._data[sz] = '\0';
         f._name     = b._data;
         f._type     = (rtFldType)fdb[i]._type;
         d._flds[i]  = f;
      }
      return d._nFld;
   }
   return 0;
}

int LVC_SetFilter( LVC_Context cxt, const char *pFids )
{
   LVCDef *lvc;
   Logger *lf;

   // Pre-condition

   if ( !pFids )
      return 0;

   // Logging; Find GLlvcDb

   if ( (lf=Socket::_log) )
      lf->logT( 3, "LVC_SetFilter( %s )\n", pFids );

   // LVCDef object

   lvc = _GetLVC( cxt );
   return lvc ? lvc->SetFilter( pFids ) : 0;
}

void LVC_SetCopyType( LVC_Context cxt, char bFull )
{
   LVCDef          *lvc;
   Logger          *lf;

   // Logging; Find GLlvcDb

   if ( (lf=Socket::_log) )
      lf->logT( 3, "LVC_SetCopyType( %s )\n", bFull ? "FULL" : "By Field" );

   // GLlvcDb object

   if ( (lvc=_GetLVC( cxt )) )
      lvc->SetCopyType( bFull ? true : false );
}

LVCData LVC_Snapshot( LVC_Context cxt,
                      const char *pSvc,
                      const char *pTkr )
{
   LVCDef *ld;
   Logger *lf;
   LVCData rtn;
   double  d0, d1;

   // Logging; Find GLlvcDb

   d0 = dNow();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "LVC_Snapshot( %s,%s )\n", pSvc, pTkr );

   // Operation : Create, if not found

   ::memset( &rtn, 0, sizeof( rtn ) );
   rtn._pSvc = pSvc;
   rtn._pTkr = pTkr;
   if ( (ld=_GetLVC( (int)cxt )) ) {
      GLlvcDb &lvc = ld->lvc();
      Locker   lck( lvc.mtx() );

      if ( lvc.isValid() && lvc.IsLocked() ) 
         rtn = lvc.GetItem( pSvc, pTkr, False );
   }
   d1         = dNow();
   rtn._dSnap = ( d1-d0 );
   return rtn;
}

LVCData LVC_View( LVC_Context cxt,
                  const char *pSvc,
                  const char *pTkr )
{
   return LVC_Snapshot( cxt, pSvc, pTkr );
}

void LVC_Free( LVCData *d )
{
   rtFIELD  f;
   rtVALUE &v = f._val;
   rtBUF   &b = v._buf;
   bool     bFldCopy;
   int      i, nf;

   if ( d ) {
      nf = d->_nFld;
      if ( d->_flds ) {
         bFldCopy = ( !d->_bShallow && !d->_copy );
         for ( i=0; bFldCopy && i<nf; i++ ) {
            f = d->_flds[i];
            if ( f._type == rtFld_string && b._data )
               delete[] b._data;
         }
         delete[] d->_flds;
      }
      if ( d->_copy )
         delete[] d->_copy;
      ::memset( d, 0, sizeof( *d ) );
   }
}

LVCDataAll LVC_SnapAll( LVC_Context cxt )
{
   LVCDef    *ld;
   Logger    *lf;
   LVCDataAll rtn;
   LVCData    d;
   double     d0, d1, d2, d3;

   // Logging; Find LVCDef

   d0 = dNow();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "LVC_SnapAll()\n" );
   ::memset( &rtn, 0, sizeof( rtn ) );
   if ( !(ld=_GetLVC( (int)cxt )) )
      return rtn;

   // Walk all records, if locked

   GLlvcDb &lvc = ld->lvc();
   Locker   lck( lvc.mtx() );

   if ( !lvc.isValid() || !lvc.IsLocked() )
      return rtn;

   RecMap          &rdb = lvc.recs();
   int              sz  = rdb.size();
   RecMap::iterator it;
   int              i;
   string           s;
   LVCData         *tkrs;
   char            *bp, *pSvc, *pTkr, *rp;

   tkrs = sz ? new LVCData[sz] : rtn._tkrs;
   for ( i=0,it=rdb.begin(); it!=rdb.end(); it++,i++ ) {
      s        = (*it).first.data();
      bp       = (char *)s.data();
      pSvc     = ::strtok_r( bp, LVC_SVCSEP, &rp );
      pTkr     = ::strtok_r( NULL, LVC_SVCSEP, &rp );
      d2       = dNow();
      d        = lvc.GetItem( pSvc, pTkr, False );
      d3       = dNow();
      d._dSnap = ( d3-d2 );
      tkrs[i]  = d;
   }
   d1           = dNow();
   rtn._tkrs    = tkrs;
   rtn._nTkr    = i;
   rtn._dSnap   = ( d1-d0 );
   rtn._bBinary = lvc.IsBinary() ? 1 : 0;
   return rtn;
}

LVCDataAll LVC_ViewAll( LVC_Context cxt )
{
   return LVC_SnapAll( cxt );
}

void LVC_FreeAll( LVCDataAll *d )
{
   int i, n;

   if ( d ) {
      n = d->_nTkr;
      for ( i=0; i<n; LVC_Free( &d->_tkrs[i++] ) );
      if ( n && d->_tkrs )
         delete[] d->_tkrs;
      d->_nTkr = 0;
      d->_tkrs = (LVCData *)0;
   }
}

void LVC_Destroy( LVC_Context cxt )
{
   LVCDef *lvc;
   Logger *lf;

   // GLlvcDb object

   if ( (lvc=_GetLVC( cxt )) )
      delete lvc; 
   _lvc[cxt] = (LVCDef *)0; 

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "LVC_Destroy() Done!!\n" );
}


//////////////////////////////
// Snapshot Series - ChartDB
//////////////////////////////
CDB_Context CDB_Initialize( const char *pFile, 
                            const char *pAdm )
{
   GLchtDb *lvc;
   Logger  *lf;
   long     rtn;

   // 0) Logging

   _Touch();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "CDB_Initialize( %s, %s )\n", pFile, pAdm );

   // 1) GLchtDb object

   rtn = 0;
   lvc = new GLchtDb( (char *)pFile, pAdm );
   if ( 1 || lvc->isValid() ) {
      rtn       = ATOMIC_INC( &_nCxt );
      _cdb[rtn] = lvc;
   }
   else
      delete lvc;
   return rtn;
}

MDDResult MDD_Query( CDB_Context cxt )
{
   GLchtDb     *cdb;
   EdgChannel  *edg;
   TapeChannel *tape;
   Logger      *lf;
   MDDResult    rtn;
   double       d0, d1;
   int          iCxt;

   // Logging; Find GLchtDb

   d0 = dNow();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "CDB_Query()\n" );

   // Operation : Create, if not found

   iCxt = (int)cxt;
   ::memset( &rtn, 0, sizeof( rtn ) );
   if ( (cdb=_GetCDB( iCxt )) )
      rtn = cdb->Query();
   else if ( (edg=_GetSub( iCxt )) && (tape=edg->tape()) )
      rtn = tape->Query();
   d1         = dNow();
   rtn._dSnap = ( d1-d0 );
   return rtn;
}

void MDD_FreeResult( MDDResult *q )
{
   MDDRecDef *rr;

   if ( q ) {
      if ( (rr=q->_recs) )
         delete[] rr;
      ::memset( q, 0, sizeof( *q ) );
   }
}

CDBData CDB_View( CDB_Context cxt,
                  const char *pSvc,
                  const char *pTkr,
                  int         fid )
{
   GLchtDb   *lvc;
   Logger    *lf;
   CDBData    rtn;
   double     d0, d1;

   // Logging; Find GLchtDb

   d0 = dNow();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "CDB_View( %s,%s,%d )\n", pSvc, pTkr, fid );

   // Operation : Create, if not found

   ::memset( &rtn, 0, sizeof( rtn ) );
   rtn._pSvc = pSvc;
   rtn._pTkr = pTkr;
   if ( (lvc=_GetCDB( (int)cxt )) )
      rtn = lvc->GetItem( pSvc, pTkr, fid );
   d1         = dNow();
   rtn._dSnap = ( d1-d0 );
   return rtn;
}

void CDB_Free( CDBData *d )
{
   float *fp;

   if ( d ) {
      if ( (fp=d->_flds) )
         delete[] fp;
      ::memset( d, 0, sizeof( *d ) );
   }
}

void CDB_AddTicker( CDB_Context cxt,
                    const char *pSvc,
                    const char *pTkr,
                    int         fid )
{
   GLchtDb *lvc;
   Logger  *lf;

   // Logging; Find GLchtDb

   if ( (lf=Socket::_log) )
      lf->logT( 3, "CDB_AddTicker( %s,%s,%d )\n", pSvc, pTkr, fid );

   // Operation : Create, if not found

   StartWinsock();
   if ( (lvc=_GetCDB( (int)cxt )) )
      lvc->AddTicker( pSvc, pTkr, fid );
   StopWinsock();
}

void CDB_DelTicker( CDB_Context cxt,
                    const char    *pSvc,
                    const char    *pTkr,
                    int            fid )
{
   GLchtDb *lvc;
   Logger  *lf;

   // Logging; Find GLchtDb

   if ( (lf=Socket::_log) )
      lf->logT( 3, "CDB_DelTicker( %s,%s,%d )\n", pSvc, pTkr, fid );

   // Operation : Create, if not found

   StartWinsock();
   if ( (lvc=_GetCDB( (int)cxt )) )
      lvc->DelTicker( pSvc, pTkr, fid );
   StopWinsock();
}

void CDB_Destroy( CDB_Context cxt )
{
   GLchtDb *lvc;
   Logger  *lf;

   // GLchtDb object

   if ( (lvc=_GetCDB( cxt )) )
      delete lvc; 
   _cdb[cxt] = (GLchtDb *)0; 

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "CDB_Destroy() Done!!\n" );
}



//////////////////////////////
// Cockpit
//////////////////////////////
Cockpit_Context Cockpit_Initialize( CockpitAttr attr )
{
   Cockpit *edg;
   LVCDef  *ld;
   GLlvcDb *lvc;
   Logger  *lf;
   long     rtn;

   // 0) Logging

   _Touch();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "Cockpit_Initialize( %s )\n", attr._pSvrHosts );
   StartWinsock();

   // 1) Affiliated LVC?

   ld  = _GetLVC( (int)attr._cxtLVC );
   lvc = ld ? &ld->lvc() : (GLlvcDb *)0;

   // 2) Cockpit object

   rtn = ++_nCxt;
   edg = new Cockpit( attr, rtn, lvc );
   _cock[rtn] = edg;

   // 3) Stats / Return

   if ( _stats && !_stats->_nSub ) {
      _stats->_nSub += 1;
      edg->SetStats( &_stats->_sub );
      safe_strcpy( edg->stats()._chanName, "SUBSCRIBE" );
   }
   return rtn;
}

const char *Cockpit_Start( Cockpit_Context cxt )
{
   Cockpit *edg;
   Logger  *lf;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "Cockpit_Start()\n" );

   // Operation

   if ( (edg=_GetCockpit( (int)cxt )) )
      return edg->Connect();
   return (const char *)0;
}

void Cockpit_Send( Cockpit_Context cxt, const char *msg )
{
   Cockpit *edg;

   if ( (edg=_GetCockpit( cxt )) )
      edg->Write( msg, strlen( msg ) );
}

void Cockpit_Destroy( Cockpit_Context cxt )
{
   Cockpit *edg;
   Logger  *lf;

   // Cockpit object

   if ( (edg=_GetCockpit( cxt )) )
      delete edg; 
   _cock[cxt] = (Cockpit *)0; 

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "Cockpit_Destroy() Done!!\n" );
}




//////////////////////////////
// Library Management
//////////////////////////////
void rtEdge_Log( const char *pFile, int debugLevel )
{
   // Pre-condition

   if ( !pFile || !strlen( pFile ) )
      return;

   // Blow away existing ...

   if ( Socket::_log )
      delete Socket::_log;
   Socket::_log = new Logger( pFile, debugLevel );
}
 
char rtEdge_SetMDDirectMon( const char *pFile,
                            const char *pExe,
                            const char *pBld )
{
   char       *bp;
   int         fSz;
   long        i;
   FPHANDLE    fp;
   EdgChannel *sub;
   PubChannel *pub;

   // Pre-condition(s)

   if ( !pFile || !strlen( pFile ) )
      return 0;
   if ( !pExe || !strlen( pExe ) || !pBld || !strlen( pBld ) )
      return 0;
   if ( _mmMon )
      return 1;

   /*
    * OK to create:
    *    ASSUMPTION 1: <= 1 Pub / <= 1 Sub
    *    ASSUMPTION 2: Set 1st Sub / 1st Pub from map, if created
    */
   fSz = sizeof( GLlibStats );
   if ( (fp=GLmmap::Open( pFile, "w+" )) ) {
      bp = new char[fSz];
      ::memset( bp, 0, fSz );
      GLmmap::Grow( bp, fSz, fp );
      delete[] bp;
      _mmMon = new GLmmapView( fp, fSz, 0, True );
      if ( !_mmMon->isValid() ) {
         delete _mmMon;
         _mmMon = (GLmmapView *)0;
      }
   }
   if ( !_mmMon )
      return 0;

   // Initialize

   _stats = (GLlibStats *)_mmMon->data();
   ::memset( _stats, 0, fSz );
   _stats->_version  = 1;
   _stats->_fileSiz  = fSz;
   _stats->_tStart   = _tStart.tv_sec;
   _stats->_tStartUs = _tStart.tv_usec;
   _stats->_nSub     = 0;  // ASSUMPTION 1
   _stats->_nPub     = 0;  // ASSUMPTION 1
   safe_strcpy( _stats->_exe, pExe );
   safe_strcpy( _stats->_build, pBld );

   // ASSUMPTION 2

   for ( i=0; i<_nCxt && !_stats->_nSub && !_stats->_nPub; i++ ) {
      if ( (sub=_GetSub( i )) ) {
         _stats->_nSub += 1;
         sub->SetStats( &_stats->_sub );
         safe_strcpy( pub->stats()._chanName, "SUBSCRIBE" );
      }
      else if ( (pub=_GetPub( i )) ) {
         _stats->_nPub += 1;
         pub->SetStats( &_stats->_pub );
         safe_strcpy( pub->stats()._chanName, "PUBLISH" );
      }
   }
   return 1;
}


//////////////////////////////
// Utilities
//////////////////////////////
double rtEdge_TimeNs()
{
   return Logger::Time2dbl( Logger::tvNow() );
}

time_t rtEdge_TimeSec()
{
   return Logger::tmNow();
}

char *rtEdge_pDateTimeMs( char *buf, double dTime )
{
   double     dNoW = dTime ? dTime : rtEdge_TimeNs();
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

char *rtEdge_pTimeMs( char *buf, double dTime )
{
   double     dNoW = dTime ? dTime : rtEdge_TimeNs();
   time_t     tNoW = (time_t)dNoW;
   struct tm *tm, l;
   int        tMs;

   tMs = (int)( 1000.0 * ( dNoW - tNoW ) );
   tm  = ::localtime_r( &tNoW, &l );
   l   = *tm;
   sprintf( buf, "%02d:%02d:%02d.%03d", l.tm_hour, l.tm_min, l.tm_sec, tMs );
   return buf;
}

int rtEdge_Time2Mark( int h, int m, int s )
{
   time_t     tNow, tMrk;
   struct tm *tm, l;

   tNow      = rtEdge_TimeSec();
   tm        = ::localtime_r( &tNow, &l );
   l         = *tm;
   l.tm_hour = h;
   l.tm_min  = m;
   l.tm_sec  = s;
   tMrk      = ::mktime( &l );
   tMrk     += ( tMrk < tNow ) ? 86400 : 0;
   return( tMrk - tNow );
}

void rtEdge_Sleep( double dSlp )
{
   SLEEP( dSlp );
}

int rtEdge_hexMsg( char *msg, int len, char *obuf )
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

double rtEdge_atof( rtFIELD f )
{
   rtVALUE &v = f._val;
   rtBUF   &b = v._buf;

   switch( f._type ) {
      case rtFld_string:
      {
         string s( b._data, b._dLen );

         return atof( s.data() );
      }
      case rtFld_int:
         return v._i32;
      case rtFld_double:
      case rtFld_date:
      case rtFld_time:
      case rtFld_timeSec:
         return v._r64;
      case rtFld_float:
         return v._r32;
      case rtFld_int16:
         return v._i16;
      case rtFld_int64:
         return v._i64;
      case rtFld_undef:
      case rtFld_int8:
      case rtFld_real:
      case rtFld_bytestream:
         break;
   }
   return 0.0;
}

int rtEdge_atoi( rtFIELD f )
{
   return (int)rtEdge_atof( f );
}


//////////////////////////////
// Utilities - CPU Time
//////////////////////////////
#ifndef CLK_TCK
#define CLK_TCK ((clock_t) sysconf (2))  /* 2 is _SC_CLK_TCK */
#endif //  CLK_TCK
static double _dClk  = 1 / (double)CLK_TCK;
#ifdef WIN32
static int    _iNano = 10000; // 1E+4
static double _dmS   = 1 / 1000.0;
#endif // WIN32

double rtEdge_CPU()
{
   double   rtn;
#ifdef WIN32
   FILETIME      t0, t1, tSys, tUsr;
   LARGE_INTEGER l, n;
   HANDLE        hProc;
   int           i;

   hProc = ::GetCurrentProcess();
   ::GetProcessTimes( hProc, &t0, &t1, &tSys, &tUsr );
   n.LowPart  = _iNano;
   n.HighPart = 0;
   rtn        = 0.0;
   for ( i=0; i<2; i++ ) {
      t0         = i ? tSys : tUsr;
      l.LowPart  = t0.dwLowDateTime;
      l.HighPart = t0.dwHighDateTime;
      rtn       += (double)( l.QuadPart / n.QuadPart );
   }
   rtn *= _dmS;
#else
   struct tms cpu;

   ::times( &cpu );
   rtn  = (double)cpu.tms_utime * _dClk;
   rtn += (double)cpu.tms_stime * _dClk;
#endif // WIN32
   return rtn;
}

#ifdef WIN32
#include <tlhelp32.h>
#endif // WIN32

int rtEdge_MemSize()
{
   int memSz, pgSz;

   memSz = 0;
   pgSz  = _GetPageSize();
#if !defined(WIN32)
   FILE *fp;
   char *p1, *rp;
   char  sFile[K], buf[K];
   bool  bCur;
   pid_t pid;
   int   i;

   pid = ::getpid();
   sprintf( sFile, "/proc/%d/status", pid );
   fp = ::fopen( sFile, "r" );
   for ( i=0; ::fgets( buf, K, fp ); i++ ) {
      bCur = ( ::strstr( buf, "VmSize" ) == buf );
      if ( bCur ) {

         // VmSize:   938176 kB

         p1 = ::strtok_r( buf, " ", &rp );
         p1 = ::strtok_r( NULL, " ", &rp );
         memSz = atoi( p1 );
         break;
      }
   }
   if ( fp )
      ::fclose( fp );
#else
   HANDLE      hProc;
   DWORD       wMin, wMax;
   BOOL        bRtn;

   hProc = ::GetCurrentProcess();
#if defined(_WIN64)
   SIZE_T wMin64, wMax64;

   bRtn = ::GetProcessWorkingSetSize( hProc, &wMin64, &wMax64 );
   wMin = wMin64;
   wMax = wMax64;
#else
   bRtn  = ::GetProcessWorkingSetSize( hProc, &wMin, &wMax );
#endif // defined(_WIN64)
   memSz = (int)( ( wMin*pgSz ) / K );
/*
   pid       = ::GetCurrentProcessId();
   hSnap     = ::CreateToolhelp32Snapshot( TH32CS_SNAPHEAPLIST, pid );
   hl.dwSize = sizeof( hl );
   if ( ::Heap32ListFirst( hSnap, &hl ) ) {
      do {
         ::memset( &he, 0, sizeof( he ) );
         he.dwSize = sizeof( he );
         if ( ::Heap32First( &he, pid, hl.th32HeapID ) ) {
            do {
               memSz += he.dwBlockSize;
            } while( ::Heap32Next( &he ) );
         }
      } while( ::Heap32ListNext( hSnap, &hl ) );
   }
   ::CloseHandle( hSnap );
 */
#endif // !defined(WIN32)
   return memSz;
}

rtBuf64 rtEdge_MapFile( char *pFile, char bCopy )
{
   GLmmap *m;
   rtBuf64 r;
   OFF_T   fSz;

   // Pre-condition

   r._data   = (char *)0;
   r._dLen   = 0;
   r._opaque = (void *)0;
   if ( !pFile )
      return r;
   fSz  = 0x7fffffffL;
   fSz  = ( fSz << 32 );
   fSz += 0xffffffffL;
   m = new GLmmap( pFile, (char *)0, 0, fSz /* 0x7fffffffffffffff */ );
   if ( m->isValid() ) {
      fSz     = m->siz();
      r._dLen = fSz;
      if ( bCopy ) {
         r._data = new char[fSz+4];
         ::memset( r._data, 0, fSz+4 );
         ::memcpy( r._data, m->data(), fSz );
      }
      else {
         r._data   = m->data();
         r._opaque = m;
      }
   }
   if ( bCopy )
      delete m;
   return r;
}

rtBuf64 rtEdge_RemapFile( rtBuf64 b )
{
   GLmmap    *m;
   rtBuf64    r;
   OSFileStat fs;

   r = b;
   if ( (m=(GLmmap *)b._opaque) ) {
      fs = ::OS_GetFileStats( m->filename() );
      if ( m->siz() < fs._Size ) {
         r._data = m->map( 0, fs._Size );
         r._dLen = m->siz();
      }
   }
   return r;
}

void rtEdge_UnmapFile( rtBuf64 b )
{
   GLmmap *m;

   if ( b._opaque ) {
      m = (GLmmap *)b._opaque;
      delete m;
   }
   else if ( b._data )
      delete[] b._data;
}

char rtEdge_Is64bit()
{
#if defined(__LP64__) || defined(_WIN64)
   return 1;
#else
   return 0;
#endif // __LP64__
}


//////////////////////////////
// OS - Disk / CPU
//////////////////////////////
int OS_GetCpuStats( OSCpuStat *cdb, int maxCpu )
{
   static Mutex      _dsMtx;
   static GLprocStat _ps;
   Locker            lck( _dsMtx );
   GLvecCPUstat     &tdb = _ps.tops();
   OSCpuStat        *c, cz;
   int               i, n;

   ::memset( &cz, 0, sizeof( cz ) );
   _ps.snap();
   n = gmin( maxCpu, _ps.nCpu() );
   for ( i=0; i<n; i++ ) {
      c      = tdb[i];
      cdb[i] = c ? *c : cz;
   }
   return n;
}

int OS_GetDiskStats( OSDiskStat *ddb, int maxDisk )
{
   static Mutex      _dsMtx;
   static GLdiskStat _ds;
   Locker            lck( _dsMtx );
   GLvecDiskStat    &tdb = _ds.tops();
   OSDiskStat       *d;
   int               i, n;

   _ds.Snap();
   n = gmin( maxDisk, _ds.nDsk() );
   for ( i=0; i<n; i++ ) {
      d      = tdb[i];
      ddb[i] = *d;
   }
   return n;
}

OSFileStat OS_GetFileStats( const char *pFile )
{
   OSFileStat  r;
   struct STAT st;

   ::memset( &r, 0, sizeof( r ) );
   if ( !::STAT( pFile, &st ) ) {
      r._Size    = st.st_size; 
      r._tAccess = st.st_atime;
      r._tMod    = st.st_mtime;
   }
   return r;
}


//////////////////////////////
// Worker Threads
//////////////////////////////
Thread_Context OS_StartThread( rtEdgeThreadFcn fcn, void *arg )
{
   Thread_Context cxt;

   cxt        = ATOMIC_INC( &_nCxt );
   _work[cxt] = new Thread( fcn, arg );
   return cxt;
}

char OS_ThreadIsRunning( Thread_Context cxt )
{
   Thread *thr;
   bool    bRun;

   thr  = _GetThread( cxt );
   bRun = thr ? thr->IsRunning() : false;
   return bRun ? 1 : 0;
}

void OS_StopThread( Thread_Context cxt )
{
   Thread *thr;

   if ( (thr=_GetThread( cxt )) ) {
      thr->Stop();
      delete thr;
   }
   _work[cxt] = (Thread *)0; 
}

} // extern "C"
