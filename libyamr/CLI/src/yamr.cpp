/******************************************************************************
*
*  yamr.cpp
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <yamr.h>

#if defined(__LP64__) || defined(_WIN64)
#define GL64 "(64-bit)"
#else
#define GL64 "(32-bit)"
#endif // __LP64__

char *libyamrCLIID()
{
   char               bp[K], *cp;
   static std::string s;
   static char       *sccsid = (char *)0;

   // Once

   if ( !sccsid ) {
      cp     = bp;
      cp    += sprintf( cp, "@(#)libyamrCLI %s Build 1 ", GL64 );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}

namespace libyamr
{

////////////////////////////////////////////////
//
//     c l a s s   y a m r
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor 
/////////////////////////////////
yamr::yamr()
{
}


/////////////////////////////////
// Class-wide - libyamrXX.dll
/////////////////////////////////
void yamr::breakpoint()
{
   YAMR::yamr::breakpoint();
}

String ^yamr::Version()
{
   std::string s;

   s  = YAMR::yamr::Version();
   s += "\n";
   s += libyamrCLIID();

   return gcnew String( s.data() );
}

void yamr::Log( String ^fileName, int debugLevel )
{
   YAMR::yamr::Log( _pStr( fileName ), debugLevel );
}

double yamr::TimeNs()
{
   return YAMR::yamr::TimeNs();
}

long yamr::TimeSec()
{
   return (long)YAMR::yamr::TimeSec();
}

long yamr::TimeFromString( String ^pTime )
{
   return YAMR::yamr::TimeFromString( _pStr( pTime ) );
}

String ^yamr::pDateTimeMs( long tm )
{
   std::string s;
   double      dt;

   dt = tm;
   return gcnew String( YAMR::yamr::pDateTimeMs( s, dt ) );
}

String ^yamr::pTimeMs( long tm )
{
   std::string s;

   return gcnew String( YAMR::yamr::pTimeMs( s, tm ) );
}

void yamr::Sleep( double dSlp )
{
   YAMR::yamr::Sleep( dSlp );
}

String ^yamr::HexMsg( String ^s )
{
   char       *cp;
   std::string cs;

   cp = (char *)_pStr( s );
   cs = YAMR::yamr::HexMsg( cp, strlen( cp ) );
   return gcnew String( cs.data() );
}

double yamr::CPU()
{
   return YAMR::yamr::CPU();
}

int yamr::MemSize()
{
   return YAMR::yamr::MemSize();
}

array<Byte> ^yamr::ReadFile( String ^pFile )
{
   ::yamrBuf    vw;
   array<Byte> ^rtn;

   vw  = YAMR::yamr::MapFile( _pStr( pFile ) );
   rtn = _memcpy( vw );
   YAMR::yamr::UnmapFile( vw );
   return rtn;
}

String ^yamr::TrimString( String ^s )
{
   char *cp;

   cp = (char *)_pStr( s );
   return gcnew String( YAMR::yamr::TrimString( cp ) );
}


/////////////////////////////////
// Class-wide - Data Conversion
/////////////////////////////////
const char *yamr::_pStr( String ^str )
{
   IntPtr ptr;

   ptr = Marshal::StringToHGlobalAnsi( str );
   return (const char *)ptr.ToPointer();
}

array<Byte> ^yamr::_memcpy( ::yamrBuf b )
{
   array<Byte> ^rtn;
   IntPtr       ptr;
   u_int        dSz;

   rtn = nullptr;
   ptr = IntPtr( (void *)b._data );
   dSz = b._dLen;
   if ( dSz ) {
      rtn = gcnew array<Byte>( dSz );
      Marshal::Copy( ptr, rtn, 0, dSz );
   }
   return rtn;
}

::yamrBuf yamr::_memcpy( array<Byte> ^b )
{
   ::yamrBuf       rtn;
   pin_ptr<Byte> p  = &b[0];
   u_char       *bp = p;

   rtn._data = reinterpret_cast<char *>(bp);
   rtn._dLen = b->Length;
   return rtn;
}

DateTime yamr::FromUnixTime( long tv_sec, long tv_usec )
{
   DateTime ^epoch, ^rtn;
   Double    dSec;

   epoch = gcnew DateTime( 1970,1,1,0,0,0,0, DateTimeKind::Utc );
   dSec  = (double)tv_sec;
   dSec += (double)tv_usec / 1000000.0;
   return epoch->AddSeconds( dSec ).ToLocalTime();
}

} // namespace libyamr
