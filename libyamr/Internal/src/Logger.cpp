/******************************************************************************
*
*  Logger.cpp
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <Internal.h>

using namespace YAMR_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s       L o g g e r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Logger::Logger( const char *pFile, int debugLevel ) :
   _log( (FILE *)0 ),
   _mtx(),
   _name( pFile ),
   _debugLevel( debugLevel )
{
   bool bStdout;

   bStdout = !::strcmp( pFile, "stdout" );
   _log    = bStdout ? stdout : ::fopen( pFile, "a+" );
}

Logger::~Logger()
{
   if ( _log && ( _log != stdout ) )
      ::fclose( _log );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
Mutex &Logger::mtx()
{
   return _mtx;
}

bool Logger::CanLog( int lvl )
{
   return( lvl <= _debugLevel );
}



////////////////////////////////////////////
// Operations
////////////////////////////////////////////
void Logger::log( int lvl, char *fmt, ... )
{
   Locker  lck( _mtx );
   char    sText[8*K];
   char   *sp;
   va_list ap;

   // Pre-condition

   if ( !CanLog( lvl ) )
      return;

   // varargs ... Tried and True

   va_start( ap,fmt );
   sp  = sText;
   sp += vsprintf( sp, fmt, ap );
   va_end( ap );

   // Flush ...

   ::fprintf( _log, sText );
   ::fflush( _log );
}

void Logger::logT( int lvl, char *fmt, ... )
{
   Locker  lck( _mtx );
   char    sText[8*K];
   char   *sp;
   va_list ap;
   string  s;

   // Pre-condition

   if ( !CanLog( lvl ) )
      return;

   // varargs ... Tried and True

   va_start( ap,fmt );
   sp  = sText;
   sp += vsprintf( sp, fmt, ap );
   va_end( ap );

   // Flush ...

   ::fprintf( _log, "[%s] %s", GetTime( s ), sText );
   ::fflush( _log );
}

void Logger::logT( int lvl, const char *fmt, ... )
{
   Locker  lck( _mtx );
   char    sText[8*K];
   char   *sp;
   va_list ap;
   string  s;

   // Pre-condition

   if ( !CanLog( lvl ) )
      return;

   // varargs ... Tried and True

   va_start( ap,fmt );
   sp  = sText;
   sp += vsprintf( sp, fmt, ap );
   va_end( ap );

   // Flush ...

   ::fprintf( _log, "[%s] %s", GetTime( s ), sText );
   ::fflush( _log );
}

void Logger::Write( int lvl, const char *data, int dLen )
{
   Locker  lck( _mtx );

   if ( CanLog( lvl ) ) {
      ::fwrite( data, dLen, 1, _log );
      ::fflush( _log );
   }
}

void Logger::HexLog( int lvl, const char *data, int dLen )
{
   Locker lck( _mtx );
   char  *obuf;
   int    nOut;

   if ( CanLog( lvl ) ) {
      obuf = new char[dLen*4];
      nOut = yamr_hexMsg( (char *)data, dLen, obuf );
      ::fwrite( obuf, nOut, 1, _log );
      ::fflush( _log );
      delete[] obuf;
   }
}



////////////////////////////////////////////
// Class-wide
////////////////////////////////////////////
struct timeval Logger::tvNow()
{
   struct timeval tv;

   // Platform-dependent
#ifdef WIN32
   struct _timeb tb;

   ::_ftime( &tb );
   tv.tv_sec  = tb.time;
   tv.tv_usec = tb.millitm * 1000;
#else
   ::gettimeofday( &tv, (struct timezone *)0 );
#endif // WIN32
   return tv;
}

time_t Logger::tmNow()
{
   return (time_t)tvNow().tv_sec;
}

double Logger::dblNow()
{
   double dRtn;

   // Platform-dependent
#ifdef WIN32
   struct _timeb         tb;
   static bool           _bInit = false;
   static struct timeval _t0;
   static double         _freq;
   LARGE_INTEGER         q;

   // Once

   if ( !_bInit ) {
      ::_ftime( &tb );
      _t0.tv_sec  = tb.time;
      _t0.tv_usec = tb.millitm * 1000;
      ::QueryPerformanceFrequency( &q );
      _freq = 1.0 / q.QuadPart;
   }
   _bInit = true;
   ::QueryPerformanceCounter( &q );
   dRtn   = Time2dbl( _t0 );
   dRtn  += ( _freq * q.QuadPart );
#else
   dRtn = Time2dbl( tvNow() );
#endif // WIN32
   return dRtn;
}

u_int64_t Logger::NanoNow()
{
   u_int64_t tm;
   double    dv;
   double    _NANOS = 1000000000.0;

   dv = dblNow() * _NANOS;
   tm = (u_int64_t)dv;
   return tm;
}

double Logger::Time2dbl( struct timeval t0 )
{
   double rtn;
   static double _num = 1.0 / ZMIL;

   rtn = t0.tv_sec + ( t0.tv_usec *_num );
   return rtn;
}

const char *Logger::GetTime( string &s )
{
   struct timeval tv;
   struct tm     *tm, t;
   time_t         t64;
   char           buf[K];

   // Get it

   s   = "Undefined";
   tv  = tvNow();
   t64 = (time_t)tv.tv_sec;
   if ( !(tm=::localtime_r( &t64, &t )) )
      return s.c_str();

   // Format it

   sprintf( buf, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
      tm->tm_year + 1900,
      tm->tm_mon + 1,
      tm->tm_mday,
      tm->tm_hour,
      tm->tm_min,
      tm->tm_sec,
      tv.tv_usec );
   s = buf;
   return s.c_str();
}
