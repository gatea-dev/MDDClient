/******************************************************************************
*
*  Logger.cpp
*
*  REVISION HISTORY:
*     30 JUL 2009 jcs  Created.
*      6 AUG 2009 jcs  Build  5: tmNow()
*      4 SEP 2009 jcs  Build  6: Write()
*     23 SEP 2009 jcs  Build  8: Time2dbl()
*      1 OCT 2010 jcs  Build  8a:time_t, not long
*     15 NOV 2010 jcs  Build  9: QueryPerformanceCounter()
*     25 JAN 2011 jcs  Build 11: time_t, not long in LOCALTIME()
*     24 JAN 2012 jcs  Build 17: HexLog() 
*     12 NOV 2014 jcs  Build 28: CanLog(); -Wall
*     21 MAR 2016 jcs  Build 32: Linux compatibility in libmddWire; 2 logT()'s
*      3 JUN 2023 jcs  Build 63: HexDump()
*      5 OCT 2023 jcs  Build 65: dbl2ttime()
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;


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

void Logger::HexDump( int lvl, const char *data, int dLen )
{
   Locker lck( _mtx );
   char  *obuf;
   int    nOut;

   if ( CanLog( lvl ) ) {
      obuf = new char[dLen*8];
      nOut = rtEdge_hexDump( (char *)data, dLen, obuf );
      ::fwrite( obuf, nOut, 1, _log );
      ::fflush( _log );
      delete[] obuf;
   }
}

void Logger::HexLog( int lvl, const char *data, int dLen )
{
   Locker lck( _mtx );
   char  *obuf;
   int    nOut;

   if ( CanLog( lvl ) ) {
      obuf = new char[dLen*8];
      nOut = rtEdge_hexMsg( (char *)data, dLen, obuf );
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

static double _uSnum = 1000000.0;
static double _uSden = 1.0 / _uSnum;

double Logger::Time2dbl( struct timeval tv )
{
   double dr;

   dr = ( _uSden * tv.tv_usec ) + tv.tv_sec;
   return dr;
}

struct timeval Logger::dbl2time( double dv )
{
   struct timeval tv;

   tv.tv_sec  = (int)dv;
   tv.tv_usec = (int)( ( dv - tv.tv_sec ) * _uSnum );
   return tv;
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
