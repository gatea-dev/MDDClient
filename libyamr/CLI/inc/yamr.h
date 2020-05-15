/******************************************************************************
*
*  yamr.h
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#pragma once
#include <yamrCLI.h>

namespace libyamr
{
////////////////////////////////////////////////
//
//      e n u m e r a t e d   t y p e s
//
////////////////////////////////////////////////
/**
 * \enum yamrIoctl
 * \brief Channel control commands passed to Writer::Ioctl()
 */
public enum class yamrIoctl
{
   ioctl_getStats           = ::ioctl_getStats,
   ioctl_randomize          = ::ioctl_randomize,
   ioctl_getFd              = ::ioctl_getFd,
   ioctl_setRxBufSize       = ::ioctl_setRxBufSize,
   ioctl_getRxBufSize       = ::ioctl_getRxBufSize,
   ioctl_setTxBufSize       = ::ioctl_setTxBufSize,
   ioctl_getTxBufSize       = ::ioctl_getTxBufSize,
   ioctl_getTxQueueSize     = ::ioctl_getTxQueueSize,
   ioctl_setThreadProcessor = ::ioctl_setThreadProcessor,
   ioctl_getThreadProcessor = ::ioctl_getThreadProcessor,
   ioctl_getThreadId        = ::ioctl_getThreadId,
   ioctl_setIdleTimer       = ::ioctl_setIdleTimer,
   ioctl_QlimitHiLoBand     = ::ioctl_QlimitHiLoBand,
   ioctl_getSocketType      = ::ioctl_getSocketType 

}; // class yamrIoctl

/**
 * \enum yamrStatus
 * \brief Channel status - Queue full, etc
 */
public enum class yamrStatus
{
   yamr_QloMark = ::yamr_QloMark,
   yamr_QhiMark = ::yamr_QhiMark
};

////////////////////////////////////////////////
//
//         c l a s s   y a m r
//
////////////////////////////////////////////////
/**
 * \class yamr
 * \brief yamrRecorder base class
 */
public ref class yamr
{
	/////////////////////////////////
	// Constructor
	/////////////////////////////////
protected:
	yamr();

	/////////////////////////////////
	// Class-wide - libyamr Utilities
	/////////////////////////////////
public:
public:
	/**
	 * \brief No-Op
	 */
	static void breakpoint();

	/**
	 * \brief Returns the library build version.
	 *
	 * \return Build version of the library
	 */
	static String ^Version();

	/**
	 * \brief Sets the library debug level; Initiates logging. 
	 *
	 * \param pLog - Log filename
	 * \param dbgLvl - Debug level
	 */
	static void Log( String ^pLog, int dbgLvl );

	/**
	 * \brief Returns clock counter to nanosecond granularity
	 *
	 * \return Clock counter to nanosecond granularity
	 */
	static double TimeNs();

	/**
	 * \brief Returns current time in Unixtime
	 *
	 * \return Current time in Unixtime
	 */
	static long TimeSec();

	/**
	 * \brief Returns time Unix time in Nanos from time string formatted
	 * as YYYY-MM-DD HH:MM:SS.uuuuuu
	 *
	 * Any combination of the following are supported:
	 * Type | Supported | Value
	 * --- | --- | ---
	 * Date | YYYY-MM-DD | As is
	 * Date | YYYYMMDD | As is
	 * Date | \<empty\> | Current Date
	 * Time | HH:MM:SS | As is
	 * Time | HHMMSS | As is
	 * Sub-Second | uuuuuu | Micros (6 digits)
	 * Sub-Second | mmm | Millis (3 digits)
	 * Sub-Second | \<empty\> | 0
	 *
	 * \param pTime - String-formatted time
	 * \return Unix Time in nanos
	 */
	static long TimeFromString( String ^pTime );

	/**
	 * \brief Returns  time in YYYY-MM-DD HH:MM:SS.mmm
	 *
	 * \param tNano - Time in nanos; Set to 0 to return current time
	 * \return Current time in YYYY-MM-DD HH:MM:SS.mmm 
	 */
	static String ^pDateTimeMs( long tNano );

	/**
	 * \brief Returns current time in HH:MM:SS.mmm
	 *
	 * \param tNano - Time in nanos; Set to 0 to return current time
	 * \return Current time in HH:MM:SS.mmm 
	 */
	static String ^pTimeMs( long tNano );

	/**
	 * \brief Sleeps for requested period of time.
	 *
	 * \param dSlp - Number of \<seconds\>.\<microseconds\> to sleep
	 */
	static void Sleep( double dSlp );

	/**
	 * \brief Hex dump a message into an output buffer
	 *
	 * \param msg - Message to dump
	 * \return Hex dump of msg in string
	 */
	String ^HexMsg( String ^msg );

	/**
	 * \brief Returns total CPU seconds used by application
	 *
	 * \return Total CPU seconds used by application
	 */
	static double CPU();

	/**
	 * \brief Returns total memory size of this process
	 *
	 * \return Total memory size for this process
	 */
	static int MemSize();

	/**
	 * \brief Reads file contents into byte[] array
	 *
	 * \param pFile - Name of file to map
	 * \return File contents
	 */
	static array<Byte> ^ReadFile( String ^pFile );

	/**
	 * \brief Trim leading / trailing spaces from a string
	 *
	 * \param str - String to trim
	 * \return Trimmed string
	 */
	static String ^TrimString( String ^str );


	/////////////////////////////////
	// Class-wide - Data Conversion
	/////////////////////////////////
public:
	/**
	 * \brief Marshall a CLI String to a C char * array
	 *
	 * \param str - CLI String
	 * \return char * array
	 */
	static const char *_pStr( String ^str );

	/**
	 * \brief Marshall a C yamrBuf into a C# byte[] array
	 *
	 * \param buf - Memory buffer from unmanaged-land
	 * \return Allocated array<Byte> in managed-land
	 */
	static array<Byte> ^_memcpy( ::yamrBuf buf );

	/**
	 * \brief Marshall a C# byte[] array into a C yamrBuf
	 *
	 * \param buf - Allocated array<Byte> in managed-land
	 * \return Memory buffer for unmanaged-land
	 */
	static ::yamrBuf _memcpy( array<Byte> ^buf );

	/**
	 * \brief Convert from Unix Time to .NET DateTime
	 *
	 * \param tv_sec - Unix Time in seconds
	 * \param tv_usec - Microseconds
	 * \return DateTime
	 */
	static DateTime FromUnixTime( long tv_sec, long tv_usec );

};  // class yamr

} // namespace libyamr
