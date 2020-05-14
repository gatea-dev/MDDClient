/******************************************************************************
*
*  OS.h
*     librtEdge - Operating System Stats
*
*  REVISION HISTORY:
*     13 JUL 2017 jcs  Created.
*     12 SEP 2017 jcs  Build 35
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <librtEdge.h>
#endif // DOXYGEN_OMIT
#include <vcclr.h>  // gcroot

using namespace System;

namespace librtEdge
{
////////////////////////////////////////////////
//
//      c l a s s   O S C p u S t a t
//
////////////////////////////////////////////////
/**
 * \class OSCpuStat
 * \brief Current CPU usage for a given CPU on the system
 *
 * \see CPUStats
 */
public ref class OSCpuStat
{
private:
   ::OSCpuStat *_stC;


   /////////////////////////////////
   // Constructor
   /////////////////////////////////
public:
   /** \brief Constructor for native OSCpuStat container */
   OSCpuStat( ::OSCpuStat );
   ~OSCpuStat();


   /////////////////////////////////
   // Properties
   /////////////////////////////////
public:
   /** \brief Returns CPU number */
   property int _CPUnum
   {
      int get() { return _stC->_CPUnum; }
   }

   /** \brief Returns Pct CPU utilized in user mode */
   property double _us
   {
      double get() { return _stC->_us; }
   }

   /** \brief Returns Pct CPU utilized in user mode w/ nice */
   property double _ni
   {
      double get() { return _stC->_ni; }
   }

   /** \brief Returns Pct CPU utilized in System mode */
   property double _sy
   {
      double get() { return _stC->_sy; }
   }

   /** \brief Returns Pct CPU utilized in Idle mode */
   property double _id
   {
      double get() { return _stC->_id; }
   }

   /** \brief Returns Pct CPU utilized in iowait : Waiting for I/O */
   property double _wa
   {
      double get() { return _stC->_wa; }
   }

   /** \brief Returns Pct CPU utilized in irq : Service interrupts */
   property double _si
   {
      double get() { return _stC->_si; }
   }

   /** \brief Returns Pct CPU utilized in softirq : Service softirqs */
   property double _st
   {
      double get() { return _stC->_st; }
   }

};  // OSCpuStat


////////////////////////////////////////////////
//
//         c l a s s   C P U S t a t s
//
////////////////////////////////////////////////
/**
 * \class CPUStats
 * \brief Snap system-wide CPU statistics
 */
public ref class CPUStats
{
protected:
   RTEDGE::CPUStats *_cpu;

   /////////////////////////////////
   // Constructor
   /////////////////////////////////
public:
   CPUStats();
   ~CPUStats();


   /////////////////////////////////
   // Access / Operations 
   /////////////////////////////////
   /**
    * \brief Return OSCpuStat for specific CPU
    *
    * \param num - CPU number
    * \return OSCpuStat for CPU 'num'
    */
public:
   OSCpuStat ^Get( int num );

   /**
    * \brief Return number of CPUs in system
    *
    * \return Number of CPUs in system
    */
   int Size();

   /**
    * \brief Snap system-wide CPU statistics
    *
    * \return Number of CPU's snapped
    */
   int Snap();

};  // class CPUStats


////////////////////////////////////////////////
//
//      c l a s s   O S D i s k S t a t
//
////////////////////////////////////////////////
/**
 * \class OSDiskStat
 * \brief Current usage for a given disk on the system
 *
 * \see DiskStats()
 */
public ref class OSDiskStat
{
private:
   ::OSDiskStat *_stD;


   /////////////////////////////////
   // Constructor
   /////////////////////////////////
public:
   /** \brief Constructor for native OSDiskStat container */
   OSDiskStat( ::OSDiskStat );
   ~OSDiskStat();


   /////////////////////////////////
   // Access
   /////////////////////////////////
public:
   /**
    * \brief Return disk name
    *
    * \return Disk Name
    */
   String ^Name();


   /////////////////////////////////
   // Properties
   /////////////////////////////////
public:
   /** \brief Number of reads completed */
   property double _nRd
   {
      double get() { return _stD->_nRd; }
   }

   /** \brief Number of reads merged (2 adjacent reads = 1 read) */
   property double _nRdM
   {
       double get() { return _stD->_nRdM; }
   }

   /** \brief Number of sectors read */
   property double _nRdSec
   {
       double get() { return _stD->_nRdSec; }
   }

   /** \brief Milliseconds spent reading */
   property double _nRdMs
   {
       double get() { return _stD->_nRdMs; }
   }

   /** \brief Number of writes completed */
   property double _nWr
   {
       double get() { return _stD->_nWr; }
   }

   /** \brief Number of writes merged (2 adjacent writes = 1 write) */
   property double _nWrM
   {
       double get() { return _stD->_nWrM; }
   }

   /** \brief Number of sectors written */
   property double _nWrSec
   {
       double get() { return _stD->_nWrSec; }
   }

   /** \brief Milliseconds spent writing */
   property double _nWrMs
   {
       double get() { return _stD->_nWrMs; }
   }

   /** \brief Number of I/O operations currently in progress */
   property double _nIo
   {
       double get() { return _stD->_nIo; }
   }

   /** \brief Milliseconds spent doing I/O */
   property double _nIoMs
   {
       double get() { return _stD->_nIoMs; }
   }

   /** \brief Weighted number of milliseconds spent doing I/O
    *
    * This field is incremented at each I/O start, I/O completion,
    * I/O merge, or read of these stats by the number of I/Os in
    * progress (field 9) times the number of milliseconds spent
    * doing I/O since the last update of this field.  This can
    * provide an easy measure of both I/O completion time and
    * the backlog that may be accumulating.
    */
   property double _wIoMs
   {
       double get() { return _stD->_wIoMs; }
   }

};  // OSDiskStat


////////////////////////////////////////////////
//
//        c l a s s   D i s k S t a t s
//
////////////////////////////////////////////////
/**
 * \class DiskStats
 * \brief Snap system-wide Disk statistics
 */
public ref class DiskStats
{
protected:
   RTEDGE::DiskStats *_disk;


   /////////////////////////////////
   // Constructor
   /////////////////////////////////
public:
   DiskStats();
   ~DiskStats();


   /////////////////////////////////
   // Access / Operations 
   /////////////////////////////////
   /**
    * \brief Return OSDiskStat for specific Disk
    *
    * \param num - Disk number
    * \return OSDiskStat for Disk 'num'
    */
public:
   OSDiskStat ^Get( int num );

   /**
    * \brief Return number of Disks in system
    *
    * \return Number of Disks in system
    */
   int Size();

   /**
    * \brief Snap system-wide Disk statistics
    *
    * \return Number of Disk's snapped
    */
   int Snap();

};  // class DiskStats

} // namespace librtEdge
