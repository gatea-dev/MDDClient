/******************************************************************************
*
*  API.cpp
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <Internal.h>

using namespace YAMR_PRIVATE;

//////////////////////////////
// Internal Helpers
//////////////////////////////

/*
 * The yamr_Context is simply an integer used to find the Channel object.
 * object in the collection below.  An integer-based index is safer 
 * that passing the Channel object reference back to the user since
 * the user may call an API after calling yamr_Destroy() and not crash 
 * the library if we are index-based; If object-based, we crash.
 */

#define _MAX_ENG 64*K

static Channel  *_clis[_MAX_ENG];
static Reader   *_read[_MAX_ENG];
static Thread   *_work[_MAX_ENG];
static long      _nCli   = 1;
static long      _nRdr   = 1;
static u_int64_t _tStart = 0;

static void _Touch()
{
   // Once

   if ( _tStart )
      return;

   // Initialize _clis, _pubs, etc.

   _tStart = ::yamr_TimeNs();
   ::srand( _tStart );
   ::memset( &_clis, 0, sizeof( _clis ) );
   ::memset( &_read, 0, sizeof( _read ) );
   ::memset( &_work, 0, sizeof( _work ) );
}

static Channel *_GetSub( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _clis[cxt];
   return (Channel *)0;
}

static Reader *_GetRead( int cxt )
{
   if ( InRange( 0, cxt, _MAX_ENG-1 ) )
      return _read[cxt];
   return (Reader *)0;
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

static void _sigHandler( int sigNum )
{
   switch( sigNum ) {
      case SIGPIPE:
         break;
   }
}

static void StartWinsock()
{
   char *pp, *pl, *pd, *rp;

   // Log from Environment

   if ( !_nWinsock && (pp=::getenv( "YAMRLOG" )) ) {
      pl = ::strtok_r( pp,   ":", &rp );
      pd = ::strtok_r( NULL, ":", &rp );
      if ( pl && pd ) 
         ::yamr_Log( pl, atoi( pd ) );
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
#else
   if ( !_nWinsock )
      ::signal( SIGPIPE, _sigHandler );
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
// Client Channel
//////////////////////////////
extern "C" {
yamr_Context yamr_Initialize( yamrAttr attr )
{
   Channel *qod;
   Logger  *lf;
   long     rtn;

   // 1) Logging; WIN32

   _Touch();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "yamr_Initialize( %s )\n", attr._pSvrHosts );
   StartWinsock();

   // 2) Channel object

   rtn        = ATOMIC_INC( &_nCli );
   qod        = new Channel( attr, rtn );
   _clis[rtn] = qod;
   return rtn;
}

const char *yamr_Start( yamr_Context cxt )
{
   Channel *qod;
   Logger  *lf;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "yamr_Start()\n" );

   // Operation

   if ( (qod=_GetSub( (int)cxt )) )
      return qod->Connect();
   return (const char *)0;
}

void yamr_Destroy( yamr_Context cxt )
{
   Channel *qod;
   Logger  *lf;

   // 1) Channel object / Stats

   if ( (qod=_GetSub( cxt )) ) {
      delete qod;
      _clis[cxt] = (Channel *)0;
   }

   // 2) Kill winsock; Logging

   StopWinsock();
   if ( (lf=Socket::_log) )
      lf->logT( 3, "yamr_Destroy() Done!!\n" );
}

void yamr_ioctl( yamr_Context cxt, yamrIoctl ty, void *arg )
{
   Channel *qod;
   Logger  *lf;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "yamr_ioctl( %d )\n", ty );

   // Operation

   if ( (qod=_GetSub( (int)cxt )) )
      qod->Ioctl( ty, arg );
}

char yamr_SetStats( yamr_Context cxt, yamrChanStats *st )
{
   Logger  *lf;
   Channel *qod;

   // Logging

   if ( (lf=Socket::_log) )
      lf->logT( 3, "yamr_SetStats( %d )\n", cxt );

   // Operations

   qod = (Channel *)0;
   if ( (qod=_GetSub( (int)cxt )) )
      qod->SetStats( st );
   return qod ? 1 : 0;
}

const char *yamr_Version()
{
   static string _ver;

   // Once

   if ( !_ver.size() )
      _ver  = libyamrID();
   return _ver.data();
}

char yamr_Send( yamr_Context cxt, 
                yamrBuf      yb, 
                u_int16_t    WireProto,
                u_int16_t    MsgProto )
{
   Channel *qod;
   Logger  *lf;
   int      nWr;

   // Logging; Find Channel

   if ( (lf=Socket::_log) )
      lf->logT( 3, "yamr_Send() : %ld bytes\n", yb._dLen );

   // Operation

   qod = _GetSub( (int)cxt );
   nWr = qod ? qod->Send( yb, WireProto, MsgProto ) : 0;
   return nWr ? 1 : 0;
}


//////////////////////////////
// Tape Reader
//////////////////////////////
yamrTape_Context yamrTape_Open( const char *tape )
{
   Reader *qod;
   long    rtn;

   rtn        = ATOMIC_INC( &_nRdr );
   qod        = new Reader( tape, rtn );
   _read[rtn] = qod;
   return rtn;
}

void yamrTape_Close( yamrTape_Context cxt )
{
   Reader *qod;

   if ( (qod=_GetRead( cxt )) ) {
      delete qod;
      _read[cxt] = (Reader *)0;
   }
}

u_int64_t yamrTape_Rewind( yamrTape_Context cxt )
{
   Reader   *qod;
   u_int64_t pos;

   pos = (qod=_GetRead( cxt )) ? qod->Rewind() : 0;
   return pos;
}

u_int64_t yamrTape_RewindTo( yamrTape_Context cxt, u_int64_t tNano )
{
   Reader   *qod;
   u_int64_t pos;

   pos = (qod=_GetRead( cxt )) ? qod->RewindTo( tNano ) : 0;
   return pos;
}

char yamrTape_Read( yamrTape_Context cxt, yamrMsg *ym )
{
   Reader *qod;

   if ( (qod=_GetRead( cxt )) )
      return qod->Read( *ym ) ? 1 : 0;
   return 0;
}


//////////////////////////////
// Library Utilities
//////////////////////////////
void yamr_Log( const char *pFile, int debugLevel )
{
   // Pre-condition

   if ( !pFile || !strlen( pFile ) )
      return;

   // Blow away existing ...

   if ( Socket::_log )
      delete Socket::_log;
   Socket::_log = new Logger( pFile, debugLevel );
}
 
u_int64_t yamr_TimeNs()
{
   return Logger::NanoNow();
}

time_t yamr_TimeSec()
{
   return Logger::tmNow();
}

u_int64_t yamr_TimeFromString( const char *pTime )
{
   string     s( pTime );
   string     ymd, hms, sub;
   char      *p1, *p2, *pd, *pt, *pm, *rp, buf[K];
   u_int64_t  tn, now, i64;
   time_t     tNow;
   struct tm *tm, lt;
   size_t     sz;
   int        nS;

   // 1) Current Time

   now  = ::yamr_TimeNs();
   i64  = now / _NANO;
   tNow = (time_t)i64;
   i64  = now % _NANO;
   tm   = ::localtime_r( &tNow, &lt );
   lt   = *tm;
   nS   = 0;

   /*
    * 2) Any combination of the following are supported:
    *    Type | Supported | Value
    *    --- | --- | ---
    *    Date | YYYY-MM-DD | As is
    *    Date | YYYYMMDD | As is
    *    Date | <empty> | Current Date
    *    Time | HH:MM:SS | As is
    *    Time | HHMMSS | As is
    *    Sub-Second | uuuuuu | Micros (6 digits)
    *    Sub-Second | mmm | Millis (3 digits)
    *    Sub-Second | <empty> | 0
    */
   tn = 0;
   pd = (char *)s.data();
   p1 = ::strtok_r( pd, " ", &rp );
   p2 = ::strtok_r( NULL, " ", &rp );
   /*
    * Date Included
    */
   pd = p2 ? p1 : (char *)0;
   pt = p2 ? p2 : p1;
   if ( pd ) {
      sz = strlen( pt );
      switch( sz ) {
         case  8:  // YYYYMMDD
            ::memcpy( buf, pd, 4 );
            buf[4]  = '-';
            buf[5]  = pd[4];
            buf[6]  = pd[5];
            buf[7]  = '-';
            buf[8]  = pd[6];
            buf[9]  = pd[7];
            buf[10] = '\0';
            pd      = buf;
            // Fall-through
         case 10:  // YYYY-MM-DD
            lt.tm_year  = atoi( pd );
            lt.tm_mon   = atoi( pd+5 );
            lt.tm_mday  = atoi( pd+8 );
            lt.tm_year -= 1900; // Num years since 1900
            lt.tm_mon  -= 1;    // Num months since Jan in range { 0 ... 11 }
            break;
      }
   }
   /*
    * Now Time
    */
   if ( (pm=::strchr( pt, '.' )) ) {
      *pm = '\0';
      pm++;
   }
   sz = strlen( pt );
   switch( sz ) {
      case 4: // HHMM
         buf[0] = pt[0]; 
         buf[1] = pt[1];
         buf[2] = ':';
         buf[3] = pt[2];
         buf[4] = pt[3];
         buf[5] = '\0'; 
         pt     = buf;
         // Fall-thru;
      case 5: // HH:MM
         lt.tm_hour = atoi( pt );
         lt.tm_min  = atoi( pt+3 );
         lt.tm_sec  = 0;
         break;
      case 6: // HHMMSS
         buf[0] = pt[0];
         buf[1] = pt[1];
         buf[2] = ':';
         buf[3] = pt[2];
         buf[4] = pt[3];
         buf[5] = ':';
         buf[6] = pt[4];
         buf[7] = pt[5];
         buf[8] = '\0';
         pt     = buf;
         // Fall-thru;
      case 8: // HH:MM:SS
         lt.tm_hour = atoi( pt );
         lt.tm_min  = atoi( pt+3 );
         lt.tm_sec  = atoi( pt+6 );
         break;
   }
   tn  = ::mktime( &lt );
   tn *= _NANO;
   tn += nS;
   return tn;
}

char *yamr_pDateTimeMs( char *buf, u_int64_t tNano )
{
   u_int64_t  i64;
   time_t     tNow;
   struct tm *tm, l;
   int        tMs;

   if ( !tNano )
      tNano = ::yamr_TimeNs();
   i64  = tNano / _NANO;
   tNow = (time_t)i64;
   i64  = tNano % _NANO;
   tMs  = (int)( i64 / 1000000 );
   tm   = ::localtime_r( &tNow, &l );
   l    = *tm;
   sprintf( buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
                    l.tm_year+1900, l.tm_mon+1, l.tm_mday,
                    l.tm_hour, l.tm_min, l.tm_sec, tMs );
   return buf;
}

char *yamr_pTimeMs( char *buf, u_int64_t tNano )
{
   char *cp, tmp[K];

   cp = ::yamr_pDateTimeMs( tmp, tNano );
   strcpy( buf, cp+13 );   
   return buf;
}

void yamr_Sleep( double dSlp )
{
   SLEEP( dSlp );
}

int yamr_hexMsg( char *msg, int len, char *obuf )
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
      else if ( InRange( 0x1c, *cp, 0x1f ) ) {
         off  = ( *cp - 0x1c );
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

double yamr_CPU()
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

int yamr_MemSize()
{
   int memSz, pgSz;

   memSz = 0;
   pgSz  = _GetPageSize();
   if ( !pgSz )
      ::yamr_breakpoint();
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

yamrBuf yamr_MapFile( char *pFile, char bCopy )
{
   GLmmap *m;
   yamrBuf r;
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

yamrBuf yamr_RemapFile( yamrBuf b )
{
   GLmmap    *m;
   yamrBuf    r;
   struct STAT st;

   r = b;
   if ( (m=(GLmmap *)b._opaque) && !::STAT( m->filename(), &st ) ) {
      if ( m->siz() < (OFF_T)st.st_size ) {
         r._data = m->map( 0, st.st_size );
         r._dLen = m->siz();
      }
   }
   return r;
}

void yamr_UnmapFile( yamrBuf b )
{
   GLmmap *m;

   if ( b._opaque ) {
      m = (GLmmap *)b._opaque;
      delete m;
   }
   else if ( b._data )
      delete[] b._data;
}

void yamr_breakpoint() { ; }


//////////////////////////////
// Worker Threads
//////////////////////////////
Thread_Context OS_StartThread( yamrThreadFcn fcn, void *arg )
{
   Thread_Context cxt;

   cxt        = ATOMIC_INC( &_nCli );
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
