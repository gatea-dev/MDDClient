/******************************************************************************
*
*  OS_cpu.cpp
*    TODO : ::getrusage( RUSAGE_THREAD )
*
*  REVISION HISTORY:
*     23 OCT 2002 jcs  Created.
*     13 JUL 2017 jcs  librtEdge
*
*  (c) 1994-2017 Gatea Ltd.
*******************************************************************************/
#include <OS_cpu.h>

using namespace RTEDGE_PRIVATE;

#ifdef WIN32
//static int _iNano = 10000000; // 1E+7
static int    _iNano = 10000; // 1E+4
static double _dmS   = 1 / 1000.0;
#endif // WIN32

#ifndef CLK_TCK
#define CLK_TCK ((clock_t) sysconf (2))  /* 2 is _SC_CLK_TCK */
#endif //  CLK_TCK
static double _dClk  = 1 / (double)CLK_TCK;

static char *_SEP = " ";

////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L c p u T i m e
//
////////////////////////////////////////////////////////////////////////////

bool GLcpuTime::_bEnable = false;

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLcpuTime::GLcpuTime( bool bOverride ) :
   _bOverride( bOverride ),
#ifdef WIN32
   _hProc( ::GetCurrentProcess() ),
#endif // WIN32
   _nCpu( 0 ),
   _pCpu( (string *)0 ),
   _dSys( 0.0 ),
   _dUsr( 0.0 )
{
   (*this)();
}

GLcpuTime::~GLcpuTime()
{
   if ( _pCpu )
      delete _pCpu;
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
double GLcpuTime::dUsr()
{
   return _dUsr;
}
 
double GLcpuTime::dSys()
{
   return _dSys;
}

double GLcpuTime::dCpu()
{
   return _dUsr + _dSys;
}

int GLcpuTime::nCpu()
{
   if ( !_pCpu )
      InitCPU();
   return _nCpu;
}

char *GLcpuTime::pCpu()
{
   if ( !_pCpu )
      InitCPU();
   return (char *)_pCpu->data();
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
double GLcpuTime::operator()()
{
   // Pre-condition

   if ( !_bEnable && !_bOverride )
      return dCpu();

   // Safe to to it
#ifdef WIN32
   FILETIME tCreate, tExit;

   ::GetProcessTimes( _hProc, &tCreate, &tExit, &_tSys, &_tUsr );
#else
   ::times( &_cpu );
#endif // WIN32
   CalcUsr();
   CalcSys();
   return dCpu();
}

double GLcpuTime::dElapsed( GLcpuTime *c0 )
{
   double d0, d1;

   // Moves it forward

   d0 = dCpu();
   d1 = c0 ? c0->dCpu() : (*this)();
   return d1 - d0;
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
void GLcpuTime::CalcUsr()
{
   double dRtn;

#ifdef WIN32
  LARGE_INTEGER l, n;

   l.LowPart  = _tUsr.dwLowDateTime;
   l.HighPart = _tUsr.dwHighDateTime;
   n.LowPart  = _iNano;
   n.HighPart = 0;
   dRtn       = (double)( l.QuadPart / n.QuadPart );
   dRtn      *= _dmS;
#else
   dRtn = (double)_cpu.tms_utime * _dClk;
#endif // WIN32
   _dUsr = dRtn;
}

void GLcpuTime::CalcSys()
{
   double dRtn;

#ifdef WIN32
   LARGE_INTEGER l, n;

   l.LowPart  = _tSys.dwLowDateTime;
   l.HighPart = _tSys.dwHighDateTime;
   n.LowPart  = _iNano;
   n.HighPart = 0;
   dRtn       = (double)( l.QuadPart / n.QuadPart );
   dRtn      *= _dmS;
#else
   dRtn = (double)_cpu.tms_stime * _dClk;
#endif // WIN32
   _dSys = dRtn;
}

void GLcpuTime::InitCPU()
{
   char *mp;

   // Pre-condition

   if ( _pCpu )
      return;
   _pCpu = new string( "Undefined" );
#if !defined(WIN32)
   FILE *fp;
   char  buf[K];
   int   i; 

   fp = ::fopen( "/proc/cpuinfo", "r" ); 
   for ( i=0; ::fgets( buf, K, fp ); i++ ) {
      chop( buf ); 
      if ( ::strstr( buf, "model name" ) == buf ) {
         mp = ::strchr( buf, ':' ); 
         mp = ::strchr( buf, ':' ); 
         _pCpu->assign( mp ? mp+2 : "Undefined" ); 
         _nCpu += 1; 
      }  
   }  
   if ( fp )
      ::fclose( fp ); 
#else
   const char *pBase = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
   const char *pKey  = "ProcessorNameString";
   HKEY        key;
   char        sKey[K];
   BYTE        buf[K];
   int         i; 
   LONG        rtn;
   DWORD       ty, dLen;

   for ( i=0; i<128; i++ ) {
      sprintf( sKey, "%s\\%d", pBase, i ); 
      rtn = ::RegOpenKey( HKEY_LOCAL_MACHINE, sKey, &key ); 
      if ( rtn != ERROR_SUCCESS )
         break; // for-loop
      _nCpu += 1; 
      if ( !i ) {
         ::memset( buf, 0, K ); 
         dLen = K; 
         rtn = ::RegQueryValueEx( key, pKey, NULL, &ty, buf, &dLen ); 
         if ( rtn == ERROR_SUCCESS )
            _pCpu->assign( (char *)buf ); 
      }  
      ::RegCloseKey( key ); 
   }
   if ( (mp=::getenv( "NUMBER_OF_PROCESSORS" )) )
      _nCpu = atoi( mp );
#endif // !defined(WIN32) 
}


////////////////////////////////////////////
// Class-wide
////////////////////////////////////////////
#ifdef WIN32
#include <tlhelp32.h>

// Hacked from psapi.h

#if defined(_WIN64)
#include <psapi.h>
#else
typedef struct {
   DWORD  cb;
   DWORD  PageFaultCount;
   SIZE_T PeakWorkingSetSize;
   SIZE_T WorkingSetSize;
   SIZE_T QuotaPeakPagedPoolUsage;
   SIZE_T QuotaPagedPoolUsage;
   SIZE_T QuotaPeakNonPagedPoolUsage;
   SIZE_T QuotaNonPagedPoolUsage;
   SIZE_T PagefileUsage;
   SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
#endif // defined(_WIN64)

#define Win32MemArgs HANDLE, PROCESS_MEMORY_COUNTERS *, DWORD

extern BOOL WINAPI GetProcessMemoryInfo( Win32MemArgs );

typedef BOOL (WINAPI *GetMemInfo)( Win32MemArgs );

#endif // WIN32

int GLcpuTime::MemSize( GLvecInt &v, int pid )
{
   v.clear();
   v.Insert( -1 );
   v.Insert( -1 );
#if !defined(WIN32)
   FILE *fp;
   char *p1, *p2, *sp, *rp;
   char  sFile[K], buf[K];
   bool  bCur, bMax;
   int   i, n;

   pid = !pid ? ::getpid() : pid;
   sprintf( sFile, "/proc/%d/status", pid );
   fp = ::fopen( sFile, "r" );
   for ( i=0; ::fgets( buf, K, fp ); i++ ) {
      chop( buf );
      bCur = ( ::strstr( buf, "VmSize" ) == buf );
      bMax = ( ::strstr( buf, "VmPeak" ) == buf );
      if ( bCur || bMax ) {
         string s( buf );

         // VmSize:   938176 kB

         sp = (char *)s.data();
         p1 = ::strtok_r( sp,   _SEP, &rp );
         p2 = ::strtok_r( NULL, _SEP, &rp );
         n  = bCur ? 0 : 1;
         v.InsertAt( p2 ? atoi( p2 ) : 0, n );
      }
   }
   if ( fp )
      ::fclose( fp );
#else
   PROCESS_MEMORY_COUNTERS pmc;
   HANDLE                  hProc;
   SIZE_T                  wSz, pSz;
   char                   *pf;
   static Mutex            _libMtx;
   static HINSTANCE        _hLib = (HINSTANCE)0;
   static GetMemInfo       _fcn  = (GetMemInfo)0;

   // Once
   {
      Locker lck( _libMtx );

      if ( !_hLib && (_hLib=::LoadLibrary( "psapi.dll" )) ) {
         pf   = "GetProcessMemoryInfo";
         _fcn = (BOOL (WINAPI *)( Win32MemArgs ))::GetProcAddress( _hLib, pf );
      }
   }
   wSz   = 0;
   pSz   = sizeof( pmc );
   hProc = ::GetCurrentProcess();
   if ( _fcn && (*_fcn)( hProc, &pmc, pSz ) )
      wSz = pmc.WorkingSetSize / K;
   v.InsertAt( (int)wSz, 0 );
   v.InsertAt( (int)wSz, 1 );
#endif // !defined(WIN32)
   return v[0];
}

bool GLcpuTime::KillProcess( char *pKill )
{
#ifdef WIN32
   BOOL           bKill, bOK;
   HANDLE         hSnap, hKill;
   PROCESSENTRY32 pe;
   DWORD          pid;
   TCHAR         *exe;

   hSnap     = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );
   bKill     = false;
   pe.dwSize = sizeof( pe );
   for ( bOK=::Process32First( hSnap, &pe ); !bKill && bOK; ) {
      pid = pe.th32ProcessID;
      exe = pe.szExeFile;
      if ( ::strstr( exe, pKill ) ) {
         hKill = ::OpenProcess( PROCESS_TERMINATE, false, pid );
         bKill = hKill ? ::TerminateProcess( hKill, -1 ) : FALSE;
      }
      pe.dwSize = sizeof( pe );
      bOK       = ::Process32Next( hSnap, &pe );
   }
   ::CloseHandle( hSnap );
   return bKill ? true : false;
#else
   return false;
#endif // WIN32
}


////////////////////////////////////////////////////////////////////////////
// 
//               c l a s s      C P U s t a t
// 
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
CPUstat::CPUstat( char *def ) 
{
   string s( def );
   char  *sp, *pv, *rp;
   double dv;
   int    i;

   _us = 0.0;
   _ni = 0.0;
   _sy = 0.0;
   _id = 0.0;
   _wa = 0.0;
   _si = 0.0;
   _st = 0.0;
   sp  = (char *)s.data();
   pv  = ::strtok_r( sp, _SEP, &rp );
   for ( i=0; pv; pv=::strtok_r( NULL, _SEP, &rp ), i++ ) {
      dv = atof( pv ) * _dClk;
      switch( i ) {
         case 1: _us = dv; break;
         case 2: _ni = dv; break;
         case 3: _sy = dv; break;
         case 4: _id = dv; break;
         case 5: _wa = dv; break;
         case 7: _si = dv; break;
         case 6: _st = dv; break;
      }
   }
}

CPUstat::CPUstat( CPUstat &c0, CPUstat &c1 )
{
   _us = c0._us - c1._us;
   _ni = c0._ni - c1._ni;
   _sy = c0._sy - c1._sy;
   _id = c0._id - c1._id;
   _wa = c0._wa - c1._wa;
   _si = c0._si - c1._si;
   _st = c0._st - c1._st;
}


////////////////////////////////////////////
// Mutator
////////////////////////////////////////////
void CPUstat::SetTop( const CPUstat &c, double dd )
{
   _us = ( 100.0 * c._us ) / dd;
   _ni = ( 100.0 * c._ni ) / dd;
   _sy = ( 100.0 * c._sy ) / dd;
   _id = ( 100.0 * c._id ) / dd;
   _wa = ( 100.0 * c._wa ) / dd;
   _si = ( 100.0 * c._si ) / dd;
   _st = ( 100.0 * c._st ) / dd;
}


////////////////////////////////////////////
// Assignment Operators
////////////////////////////////////////////
CPUstat &CPUstat::operator=( const CPUstat &c )
{
   _us = c._us;
   _ni = c._ni;
   _sy = c._sy;
   _id = c._id;
   _wa = c._wa;
   _si = c._si;
   _st = c._st;
   return *this;
}



////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L p r o c S t a t
//
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLprocStat::GLprocStat( const char *pFile ) :
   string( pFile ),
   _cpus(),
   _tops(),
   _d0( dNow() ),
   _qry( (PDH_HQUERY)0 ),
   _UserTime(),
   _PrivilegedTime(),
   _ProcessorTime(),
   _IdleTime(),
   _InterruptTime()
{
#ifdef WIN32
   GLcpuTime  cpu( true );
   PDH_STATUS sts;
   int        i, n;

   // 0-based _cpus and _tops

   sts = ::PdhOpenQuery( NULL, NULL, &_qry );
   for ( i=0,n=cpu.nCpu(); i<n; i++ ) {
      _cpus.InsertAt( new CPUstat( "" ), i );
      _tops.InsertAt( new CPUstat( "" ), i );
      _UserTime.InsertAt( _AddCounter( "User Time", i ), i );
      _PrivilegedTime.InsertAt( _AddCounter( "Privileged Time", i ), i );
      _ProcessorTime.InsertAt( _AddCounter( "Processor Time", i ), i );
      _IdleTime.InsertAt( _AddCounter( "Idle Time", i ), i );
      _InterruptTime.InsertAt( _AddCounter( "Interrupt Time", i ), i );
   }
#endif // WIN32
   snap();
}

GLprocStat::~GLprocStat()
{
#ifdef WIN32
   ::PdhCloseQuery( _qry );
#endif // WIN32
   _cpus.clearAndDestroy();
   _tops.clearAndDestroy();
}


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
GLvecCPUstat &GLprocStat::cpus()
{
   return _cpus;
}

GLvecCPUstat &GLprocStat::tops()
{
   return _tops;
}

int GLprocStat::nCpu()
{
   return _cpus.size();
}

int GLprocStat::snap( bool bDump )
{
   FILE      *fp;
   double     dn, dd;
   CPUstat   *cd;
   PDH_STATUS sts;
   char      *cp;
   char       buf[K];
   int        i;

   // Snap 'em

   dn  = dNow();
   dd  = dn - _d0;
   dd  = dn - _d0;
   _d0 = dn;
   sts = (PDH_STATUS)0;
#ifdef WIN32
   fp = (FILE *)0;
   sts = ::PdhCollectQueryData( _qry );
   for ( i=0; i<nCpu(); i++ ) {
      cd      = _tops[i];
      cd->_us = _GetCounter( _UserTime[i] );
      cd->_ni = 0.0;
      cd->_sy = _GetCounter( _PrivilegedTime[i] );
      cd->_id = _GetCounter( _IdleTime[i] );
      cd->_wa = 0.0;
      cd->_si = _GetCounter( _InterruptTime[i] );
      cd->_st = 0.0;
dd = _GetCounter( _ProcessorTime[i] ) - ( cd->_us + cd->_si + cd->_id );
   }
#else
   CPUstat *c1;
   char    *cpu;
   int      n;

   fp = ::fopen( data(), "r" );
   for ( i=0; ::fgets( buf, K, fp ); i++ ) {
      chop( buf );
      if ( !(cpu=::strstr( buf, "cpu" )) )
         continue; // for-i
      if ( !InRange( '0', cpu[3], '9' ) )
         continue; // for-i
      n = cpu[3] - '0';
      if ( InRange( '0', cpu[4], '9' ) ) {
         n *= 10;
         n += ( cpu[4] - '0' );
      }

      CPUstat c0( buf );

      if ( ( n+1 > nCpu() ) || !_cpus[n] ) {
         _cpus.InsertAt( new CPUstat( buf ), n );
         _tops.InsertAt( new CPUstat( "" ), n );
      }
      c1 = _cpus[n];
      cd = _tops[n];

      CPUstat d( c0, *c1 );

      *c1 = c0;
      cd->SetTop( d, dd );
   }
#endif // !defined(WIN32)

   // Dump 'em

   for ( i=0; bDump && i<nCpu(); i++ ) {
      cd = _tops[i];
      cp = buf;
#if !defined(WIN32) 
      cp += !i ? sprintf( cp, HOME ) : 0;
#endif // !defined(WIN32) 
      cp += sprintf( cp, "Cpu%2d :", i );
      cp += sprintf( cp, "%5.1f%%us,", cd->_us );
      cp += sprintf( cp, "%5.1f%%sy,", cd->_sy );
      cp += sprintf( cp, "%5.1f%%ni,", cd->_ni );
      cp += sprintf( cp, "%5.1f%%id,", cd->_id );
      cp += sprintf( cp, "%5.1f%%wa,", cd->_wa );
      cp += sprintf( cp, "%5.1f%%si,", cd->_si );
      cp += sprintf( cp, "%5.1f%%st,", cd->_st );
      cp += sprintf( cp, "\n" );
      ::fwrite( buf, strlen( buf ), 1, stdout );
      ::fflush( stdout );
   }
   if ( fp )
      ::fclose( fp );
   return nCpu();
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
PDH_HCOUNTER GLprocStat::_AddCounter( const char *stat, int nCPU )
{
#ifdef WIN32
   PDH_HCOUNTER c;
   PDH_STATUS   sts;
   char         buf[K];
   WCHAR        wbuf[K];

   c   = (PDH_HCOUNTER)0;
   sprintf( buf, "\\Processor(%d)\\%% %s", nCPU, stat );
   ::mbstowcs( wbuf, buf, strlen( buf ) ); 
   sts = ::PdhAddCounter( _qry, buf /* wbuf */, NULL, &c );
   return c;
#endif // WIN32
   return (PDH_HCOUNTER)0;
}

double GLprocStat::_GetCounter( PDH_HCOUNTER c )
{
#ifdef WIN32
   PDH_FMT_COUNTERVALUE val;
   PDH_STATUS           sts;

   sts = ::PdhGetFormattedCounterValue( c, PDH_FMT_DOUBLE, NULL, &val );
   return val.doubleValue;
#endif // WIN32
   return 0.0;
}
