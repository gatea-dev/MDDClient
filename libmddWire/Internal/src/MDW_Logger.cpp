/******************************************************************************
*
*  MDW_Logger.cpp
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     16 OCT 2013 jcs  Build  3: clock_gettime()
*     11 MAR 2014 jcs  Build  6: No mo _ftime()
*     12 NOV 2014 jcs  Build  8: -Wall
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>

using namespace MDDWIRE_PRIVATE;


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

   if ( lvl > _debugLevel )
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

   if ( lvl > _debugLevel )
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

   if ( lvl <= _debugLevel ) {
      ::fwrite( data, dLen, 1, _log );
      ::fflush( _log );
   }
}

void Logger::HexLog( int lvl, const char *data, int dLen )
{
   Locker lck( _mtx );
   char  *obuf;
   int    nOut;

   if ( lvl <= _debugLevel ) {
      obuf = new char[dLen*4];
      nOut = mddWire_hexMsg( (char *)data, dLen, obuf );
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
   SYSTEMTIME       w32;
   LARGE_INTEGER    utcFT     = { 0 };
   LARGE_INTEGER    jan1970FT = { 0 };
   unsigned __int64 utcDosTime;

   // http://blogs.msdn.com/b/joshpoley/archive/2007/12/19/date-time-formats-and-conversions.aspx

   ::GetSystemTime( &w32 );
   jan1970FT.QuadPart = 116444736000000000I64; // january 1st 1970
   ::SystemTimeToFileTime( &w32, (FILETIME*)&utcFT );
   utcDosTime = ( utcFT.QuadPart - jan1970FT.QuadPart) / 10000000;
   tv.tv_sec  = (time_t)utcDosTime;;
   tv.tv_usec = w32.wMilliseconds * 1000;
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
   static bool           _bInit = false;
   static struct timeval _t0;
   static double         _freq;
   LARGE_INTEGER         q;

   // Once

   if ( !_bInit ) {
      _t0 = tvNow();
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
      tm->tm_mon,
      tm->tm_mday,
      tm->tm_hour,
      tm->tm_min,
      tm->tm_sec,
      tv.tv_usec );
   s = buf;
   return s.c_str();
}
