/******************************************************************************
*
*  rtEdge.cpp
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*      7 JUL 2015 jcs  Build 31: rtBUF _memcpy()
*     14 JUL 2017 jcs  Build 34: class Channel
*     12 OCT 2017 jcs  Build 36: rtBuf64
*     14 JAN 2018 jcs  Build 39: _nObjCLI
*     10 DEC 2018 jcs  Build 41: VS2017; Sleep()
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <rtEdge.h>

#if defined(__LP64__) || defined(_WIN64)
#define GL64 "(64-bit)"
#else
#define GL64 "(32-bit)"
#endif // __LP64__

char *librtEdgeCLIID()
{
   static std::string s;
   static char       *sccsid = (char *)0;

   // Once

   if ( !sccsid ) {
      char bp[K], *cp;

      cp     = bp;
      cp    += sprintf( cp, "@(#)librtEdgeCLI %s Build 51 ", GL64 );
      cp    += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s      = bp;
      sccsid = (char *)s.data();
   }
   return sccsid+4;
}

static long _nObjCLI = 0;

namespace librtEdge
{

////////////////////////////////////////////////
//
//     c l a s s   r t E d g e
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor 
/////////////////////////////////
rtEdge::rtEdge() :
   _con( nullptr ),
   _nUpd( 0 ),
   _snapTmr( nullptr ),
   _snapCbk( nullptr )
{
}


/////////////////////////////////
// Access
/////////////////////////////////
String ^rtEdge::pConn()
{
   return _con;
}


/////////////////////////////////
// Mesasge Rate
/////////////////////////////////
bool rtEdge::MsgRateOn()
{
   return( _snapTmr != nullptr );
}

void rtEdge::DumpMsgRate( int nSec )
{
   int tPer;

   DisableMsgRate();
   tPer     = nSec * 1000;
   _snapCbk = gcnew TimerCallback( SnapCPU );
   _snapTmr = gcnew Timer(_snapCbk, this, tPer, tPer);
}

void rtEdge::DisableMsgRate()
{
   if ( MsgRateOn() )
      _snapTmr->Dispose( nullptr );
   _snapTmr = nullptr;
   _snapCbk = nullptr;
}



/////////////////////////////////
// Timer Handler
/////////////////////////////////
void rtEdge::SnapCPU( Object ^data )
{
   rtEdge ^edg;
   String ^tm;

   edg = (rtEdge ^)data;
   tm  = DateTimeMs();
   Console::WriteLine( "[{0}] SnapCPU() : {1},{2}", tm, edg->_nUpd, CPU() );
}


/////////////////////////////////
// Class-wide - librtEdgeXX.dll
/////////////////////////////////
void rtEdge::_IncObj()
{
   InterlockedIncrement( &_nObjCLI );
}

void rtEdge::_DecObj()
{
   InterlockedDecrement( &_nObjCLI );
}

int rtEdge::NumObj()
{
   return _nObjCLI;
}

void rtEdge::Log( String ^fileName, int debugLevel )
{
   RTEDGE::rtEdge::Log( _pStr( fileName ), debugLevel );
}

bool rtEdge::SetMDDirectMon( String ^fileName, 
                             String ^exeName, 
                             String ^buildName )
{
   const char *pFile = _pStr( fileName );
   const char *pExe  = _pStr( exeName );
   const char *pBld  = _pStr( buildName );

   return RTEDGE::rtEdge::SetMDDirectMon( pFile, pExe, pBld );
}

String ^rtEdge::DateTimeMs( long tm )
{
   std::string s;
   double      dt;

   dt = tm;
   return gcnew String( RTEDGE::rtEdge::pDateTimeMs( s, dt ) );
}

String ^rtEdge::DateTimeMs()
{
   std::string s;

   return gcnew String( RTEDGE::rtEdge::pDateTimeMs( s ) );
}

String ^rtEdge::TimeMs()
{
   std::string s;

   return gcnew String( RTEDGE::rtEdge::pTimeMs( s ) );
}

double rtEdge::TimeNs()
{
   return RTEDGE::rtEdge::TimeNs();
}

long rtEdge::TimeSec()
{
   return (long)RTEDGE::rtEdge::TimeSec();
}

double rtEdge::CPU()
{
   return RTEDGE::rtEdge::CPU();
}

int rtEdge::MemSize()
{
   return RTEDGE::rtEdge::MemSize();
}

String ^rtEdge::Version()
{
   std::string s;

   s  = RTEDGE::rtEdge::Version();
   s += "\n";
   s += librtEdgeCLIID();

   return gcnew String( s.data() );
}

void rtEdge::Sleep( double dSlp )
{
   RTEDGE::rtEdge::Sleep( dSlp );
}

array<Byte> ^rtEdge::ReadFile( String ^pFile )
{
   ::rtBuf64    vw;
   array<Byte> ^rtn;

   vw  = RTEDGE::rtEdge::MapFile( _pStr( pFile ) );
   rtn = _memcpy( vw );
   RTEDGE::rtEdge::UnmapFile( vw );
   return rtn;
}


/////////////////////////////////
// Class-wide - Data Conversion
/////////////////////////////////
const char *rtEdge::_pStr( String ^str )
{
   IntPtr ptr;

   ptr = Marshal::StringToHGlobalAnsi( str );
   return (const char *)ptr.ToPointer();
}

array<Byte> ^rtEdge::_memcpy( ::rtBUF b )
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

array<Byte> ^rtEdge::_memcpy( ::rtBuf64 b )
{
   array<Byte> ^rtn;
   IntPtr       ptr;
   long         dSz;

   rtn = nullptr;
   ptr = IntPtr( (void *)b._data );
   dSz = b._dLen;
   if ( dSz ) {
      rtn = gcnew array<Byte>( dSz );
      Marshal::Copy( ptr, rtn, 0, dSz );
   }
   return rtn;
}

::rtBUF rtEdge::_memcpy( array<Byte> ^b )
{
   ::rtBUF       rtn;
   pin_ptr<Byte> p  = &b[0];
   u_char       *bp = p;

   rtn._data = reinterpret_cast<char *>(bp);
   rtn._dLen = b->Length;
   return rtn;
}

DateTime rtEdge::FromUnixTime( long tv_sec, long tv_usec )
{
   DateTime ^epoch, ^rtn;
   Double    dSec;

   epoch = gcnew DateTime( 1970,1,1,0,0,0,0, DateTimeKind::Utc );
   dSec  = (double)tv_sec;
   dSec += (double)tv_usec / 1000000.0;
   return epoch->AddSeconds( dSec ).ToLocalTime();
}



////////////////////////////////////////////////
//
//     c l a s s   C h a n n e l
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor 
/////////////////////////////////
Channel::Channel() :
   _chan( (RTEDGE::Channel *)0 )
{
}

Channel::~Channel()
{
}


/////////////////////////////////
// librtEdge Operations
/////////////////////////////////
void Channel::Ioctl( rtEdgeIoctl cmd, IntPtr val )
{
   _chan->Ioctl( (::rtEdgeIoctl)cmd, (void *)val );
}

int Channel::SetRxBufSize( int bufSiz )
{
   return _chan->SetRxBufSize( bufSiz );
}

int Channel::GetRxBufSize()
{
   return _chan->GetRxBufSize();
}

int Channel::SetTxBufSize( int bufSiz )
{
   return _chan->SetTxBufSize( bufSiz );
}

int Channel::GetTxBufSize()
{
   return _chan->GetTxBufSize();
}

int Channel::SetThreadProcessor( int cpu )
{
   return _chan->SetThreadProcessor( cpu );
}

int Channel::GetThreadProcessor()
{
   return _chan->GetThreadProcessor();
}

long Channel::GetThreadID()
{
   return (long)_chan->GetThreadID();
}

bool Channel::IsBinary()
{
   return _chan->IsBinary();
}

bool Channel::IsMF()
{
   return _chan->IsMF();
}

} // namespace librtEdge
