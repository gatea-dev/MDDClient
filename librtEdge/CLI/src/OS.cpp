/******************************************************************************
*
*  OS.cpp
*     librtEdge - Operating System Stats
*
*  REVISION HISTORY:
*     13 JUL 2017 jcs  Created.
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <OS.h>

namespace librtEdge 
{

////////////////////////////////////////////////
//
//       c l a s s   O S C p u S t a t
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
OSCpuStat::OSCpuStat( ::OSCpuStat st ) :
   _stC( new ::OSCpuStat() )
{
   *_stC = st;
}

OSCpuStat::~OSCpuStat()
{
   delete _stC;
}


////////////////////////////////////////////////
//
//       c l a s s   C P U S t a t s
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
CPUStats::CPUStats() :
   _cpu( new RTEDGE::CPUStats() )
{
}

CPUStats::~CPUStats()
{
   delete _cpu;
}


////////////////////////////////////
// Access / Operations
////////////////////////////////////
OSCpuStat ^CPUStats::Get( int num )
{
   ::OSCpuStat c;

   c = _cpu->Get( num );
   return gcnew OSCpuStat( c );
}

int CPUStats::Size()
{
   return _cpu->Size();
}

int CPUStats::Snap()
{
   return _cpu->Snap();
}



////////////////////////////////////////////////
//
//      c l a s s   O S D i s k S t a t
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
OSDiskStat::OSDiskStat( ::OSDiskStat st ) :
   _stD( new ::OSDiskStat() )
{
   *_stD = st;
}

OSDiskStat::~OSDiskStat()
{
   delete _stD;
}


////////////////////////////////////
// Access
////////////////////////////////////
String ^OSDiskStat::Name()
{
   std::string s;

   return gcnew String( _stD->_diskName );
}


////////////////////////////////////////////////
//
//      c l a s s   D i s k S t a t s
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
DiskStats::DiskStats() :
   _disk( new RTEDGE::DiskStats() )
{
}

DiskStats::~DiskStats()
{
   delete _disk;
}


////////////////////////////////////
// Access / Operations
////////////////////////////////////
OSDiskStat ^DiskStats::Get( int num )
{
   ::OSDiskStat d;

   d = _disk->Get( num );
   return gcnew OSDiskStat( d );
}

int DiskStats::Size()
{
   return _disk->Size();
}

int DiskStats::Snap()
{
   return _disk->Snap();
}


} // namespace librtEdge
