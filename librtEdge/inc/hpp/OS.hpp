/******************************************************************************
*
*  OS.hpp
*     librtEdge - Operating System Stats
*
*  REVISION HISTORY:
*     13 JUL 2017 jcs  Created.
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_OS_H
#define __RTEDGE_OS_H

namespace RTEDGE
{


////////////////////////////////////////////////
//
//        c l a s s   C P U S t a t s
//
////////////////////////////////////////////////

/**
 * \class CPUStats
 * \brief Snap system-wide CPU statistics
 */
class CPUStats
{
	////////////////////////////////////
	// Constructor
	////////////////////////////////////
public:
	/** \brief Constructor */
	CPUStats()
	{
	   Snap();
	}


	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return OSCpuStat for specific CPU
	 *
	 * \param num - CPU number 
	 * \return OSCpuStat for CPU 'num'
	 */
	::OSCpuStat Get( int num )
	{
	   ::OSCpuStat rtn;

	   ::memset( &rtn, 0, sizeof( rtn ) );
	   if ( InRange( 0, num, Size() ) )
	      rtn = _cpus[num];
	   return rtn;
	}

	/**
	 * \brief Return number of CPUs in system
	 *
	 * \return Number of CPUs in system
	 */
	int Size()
	{
	   return _num;
	}

	/**
	 * \brief Snap system-wide CPU statistics
	 *
	 * \return Number of CPU's snapped
	 */
	int Snap()
	{
	   _num = ::OS_GetCpuStats( _cpus, K );
	   return _num;
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	::OSCpuStat _cpus[K];
	int         _num;

}; // class CPUStats



////////////////////////////////////////////////
//
//        c l a s s   D i s k S t a t s
//
////////////////////////////////////////////////

/**
 * \class DiskStats
 * \brief Snap system-wide Disk statistics
 */
class DiskStats
{
	////////////////////////////////////
	// Constructor
	////////////////////////////////////
public:
	/** \brief Constructor */
	DiskStats()
	{
	   Snap();
	}


	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return OSDiskStat for specific disk
	 *
	 * \param num - Disk number 
	 * \return OSDiskStat for Disk 'num'
	 */
	::OSDiskStat Get( int num )
	{
	   ::OSDiskStat rtn;

	   ::memset( &rtn, 0, sizeof( rtn ) );
	   if ( InRange( 0, num, Size() ) )
	      rtn = _disks[num];
	   return rtn;
	}

	/**
	 * \brief Return number of Disks in system
	 *
	 * \return Number of Disks in system
	 */
	int Size()
	{
	   return _num;
	}

	/**
	 * \brief Snap system-wide Disk statistics
	 *
	 * \return Number of Disk's snapped
	 */
	int Snap()
	{
	   _num = ::OS_GetDiskStats( _disks, K );
	   return _num;
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	::OSDiskStat _disks[K];
	int          _num;

}; // class DiskStats


} // namespace RTEDGE

#endif // __RTEDGE_OS_H 
