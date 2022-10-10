/******************************************************************************
*
*  rtEdge.h
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*      7 JUL 2015 jcs  Build 31: More ioctl's
*     14 JUL 2017 jcs  Build 34: class Channel
*     12 OCT 2017 jcs  Build 36: rtBuf64
*     14 JAN 2018 jcs  Build 39: _nObjCLI
*     10 DEC 2018 jcs  Build 41: Sleep()
*      7 MAR 2022 jcs  Build 51: doxygen 
*     30 MAR 2022 jcs  Build 52: ioctl_unpacked 
*      7 MAR 2022 jcs  Build 53: IsValid(); Channel.SetMDDirectMon(); GetStats()
*     23 MAY 2022 jcs  Build 54: _iVal / _dVal
*     23 SEP 2022 jcs  Build 56: FromUnixTime() : DateTime Reference
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <librtEdge.h>
#endif // DOXYGEN_OMIT
#include <vcclr.h>  // gcroot

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;

/**
 * \mainpage librtEdgeCLI API Reference Manual
 *
 * librtEdge allows you to consume and publish real-time and historical data
 * from the MD-Direct platform via .NET through the following components:
 * Function | Component | Description
 * ---- | --- | ----
 * Subscribe | rtEdgeCache3 | Consume real-time data
 * Publish | rtEdgeCache3 | Publish real-time data
 * Tick-by-Tick | gateaRecorder | Pull out every recorded message for a day
 * Bulk Snapshot | LVC | Bulk snapshot (5000 tickers in under 1 millisecond)
 * Chartable Time-Series | ChartDb | Snap chartable time-series historical data
 */
namespace librtEdge
{
////////////////////////////////////////////////
//
//      e n u m e r a t e d   t y p e s
//
////////////////////////////////////////////////
public enum class rtEdgeIoctl
{
	rtIoctl_unpacked            = ::ioctl_unpacked,
	rtIoctl_nativeField         = ::ioctl_nativeField,
	rtIoctl_svcTkrName          = ::ioctl_svcTkrName,
	rtIoctl_rawData             = ::ioctl_rawData,
	rtIoctl_getStats            = ::ioctl_getStats,
	rtIoctl_binary              = ::ioctl_binary,
	rtIoctl_fixedLibrary        = ::ioctl_fixedLibrary,
	rtIoctl_measureLatency      = ::ioctl_measureLatency,
	rtIoctl_enableCache         = ::ioctl_enableCache,
	rtIoctl_getProtocol         = ::ioctl_getProtocol,
	rtIoctl_randomize           = ::ioctl_randomize,
	rtIoctl_setHeartbeat        = ::ioctl_setHeartbeat,
	rtIoctl_getFd               = ::ioctl_getFd,
	rtIoctl_setRxBufSize        = ::ioctl_setRxBufSize,
	rtIoctl_getRxBufSize        = ::ioctl_getRxBufSize,
	rtIoctl_setTxBufSize        = ::ioctl_setTxBufSize,
	rtIoctl_getTxBufSize        = ::ioctl_getTxBufSize,
	rtIoctl_setPubHopCount      = ::ioctl_setPubHopCount,
	rtIoctl_getPubHopCount      = ::ioctl_getPubHopCount,
	rtIoctl_setThreadProcessor  = ::ioctl_setThreadProcessor,
	rtIoctl_getThreadProcessor  = ::ioctl_getThreadProcessor,
	rtIoctl_setPubDataPayload   = ::ioctl_setPubDataPayload,
	rtIoctl_getInputBuffer      = ::ioctl_getInputBuffer,
	rtIoctl_isSnapChan          = ::ioctl_isSnapChan,
	rtIoctl_setUserPubMsgTy     = ::ioctl_setUserPubMsgTy,
	rtIoctl_setIdleTimer        = ::ioctl_setIdleTimer,
	rtIoctl_setPubAuthToken     = ::ioctl_setPubAuthToken,
	rtIoctl_getPubAuthToken     = ::ioctl_getPubAuthToken,
	rtIoctl_enablePerm          = ::ioctl_enablePerm,
	rtIoctl_getThreadId         = ::ioctl_getThreadId,
	rtIoctl_userDefStreamID     = ::ioctl_userDefStreamID 
};

public enum class rtEdgeState
{
	edg_up   = 0, // ::edg_up,
	edg_down = ::edg_down
};

public enum class rtEdgeType
{
	edg_image      = ::edg_image,
	edg_update     = ::edg_update,
	edg_stale      = ::edg_stale,
	edg_recovering = ::edg_recovering,
	edg_dead       = ::edg_dead
};

public enum class rtFldType
{
	rtFld_undef      = ::rtFld_undef,
	rtFld_string     = ::rtFld_string,
	rtFld_int        = ::rtFld_int,
	rtFld_double     = ::rtFld_double,
	rtFld_date       = ::rtFld_date,
	rtFld_time       = ::rtFld_time,
	rtFld_timeSec    = ::rtFld_timeSec,
	rtFld_float      = ::rtFld_float,
	rtFld_int8       = ::rtFld_int8,
	rtFld_int16      = ::rtFld_int16,
	rtFld_int64      = ::rtFld_int64,
	rtFld_real       = ::rtFld_real,
	rtFld_bytestream = ::rtFld_bytestream,
	rtFld_unixTime   = ::rtFld_unixTime
};

//////////////////////
// Forwards
//////////////////////
ref class rtEdgeChanStats;

////////////////////////////////////////////////
//
//         c l a s s   r t E d g e 
//
////////////////////////////////////////////////
/**
 * \class rtEdge
 * \brief Common base class for publication and subscription channels
 */
public ref class rtEdge
{
protected:
	String        ^_con;
	int            _nUpd;
	Timer         ^_snapTmr;
	TimerCallback ^_snapCbk;

	/////////////////////////////////
	// Constructor
	/////////////////////////////////
protected:
	rtEdge();

	/////////////////////////////////
	// Access 
	/////////////////////////////////
	/**
	 * \brief Returns connection string
	 *
	 * \return Connection string
	 */
public:
	String ^pConn();


	/////////////////////////////////
	// Mesasge Rate
	/////////////////////////////////
	/** \brief Returns true if DumpMsgRate() has been called */
	bool MsgRateOn();

	/**
	 * \brief Dump message rate every nSec
	 *
	 * \param nSec - Interval to dump msg rates
	 */
	void DumpMsgRate( int nSec );

	/** \brief Turn message rate dumping off */
	void DisableMsgRate();


	/////////////////////////////////
	// Timer Handler
	/////////////////////////////////
private:
	static void SnapCPU( Object ^ );


	/////////////////////////////////
	// Class-wide - librtEdge Utilities
	/////////////////////////////////
public:
#ifndef DOXYGEN_OMIT
	static void _IncObj();
	static void _DecObj();
#endif // DOXYGEN_OMIT

	/**
	 * \brief Return number of CLI objects created by librtEdgeCLI
	 *
	 * \return Number of CLI objects created by librtEdgeCLI
	 */
	static int  NumObj();

	/**
	 * \brief Set library logging level
	 *
	 * \param fileName - Log file name
	 * \param debugLevel - Debug level [0-5]
	 */
	static void Log( String ^fileName, int debugLevel );

	/**
	 * \brief Return unix time in YYYY-MM-DD HH:MM:SS.mmm
	 *
	 * \param tm - Unix Time in seconds since epoch 1970-01-01
	 * \return Current time in YYYY-MM-DD HH:MM:SS.mmm
	 */
	static String ^DateTimeMs( long tm );

	/**
	 * \brief Return current time in YYYY-MM-DD HH:MM:SS.mmm
	 *
	 * \return Current time in YYYY-MM-DD HH:MM:SS.mmm
	 */
	static String ^DateTimeMs();

	/**
	 * \brief Return current time in HH:MM:SS.mmm
	 *
	 * \return Current time in HH:MM:SS.mmm
	 */
	static String ^TimeMs();

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
	 * \brief Returns total CPU seconds used by application
	 *
	 * \return Total CPU seconds used by application
	 */
	static double CPU();

	/**
	 * \brief Returns total memory size of this process
	 *
	 * \return Total memory size of this process
	 */
	static int MemSize();

	/**
	 * \brief Returns the library build version
	 *
	 * \return Build version of the library
	 */
	static String ^Version();

	/**
	 * \brief Sleeps for requested period of time.
	 *
	 * \param dSlp - Number of \<seconds\>.\<microseconds\> to sleep
	 */
	static void Sleep( double dSlp );
 
	/**
	 * \brief Reads file contents into byte[] array
	 *
	 * \param pFile - Name of file to map
	 * \return File contents
	 */
	static array<Byte> ^ReadFile( String ^pFile );


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
	 * \brief Marshall a C rtBUF into a C# byte[] array
	 *
	 * \param buf - Memory buffer from unmanaged-land
	 * \return Allocated array<Byte> in managed-land
	 */
	static array<Byte> ^_memcpy( ::rtBUF buf );

	/**
	 * \brief Marshall a C rtBuf64 into a C# byte[] array
	 *
	 * \param buf - Memory buffer from unmanaged-land
	 * \return Allocated array<Byte> in managed-land
	 */
	static array<Byte> ^_memcpy( ::rtBuf64 buf );

	/**
	 * \brief Marshall a C# byte[] array into a C rtBUF
	 *
	 * \param buf - Allocated array<Byte> in managed-land
	 * \return Memory buffer for unmanaged-land
	 */
	static ::rtBUF _memcpy( array<Byte> ^buf );

	/**
	 * \brief Convert from Unix Time to .NET DateTime
	 *
	 * \param tv_sec - Unix Time in seconds
	 * \param tv_usec - Microseconds
	 * \return DateTime
	 */
	static DateTime FromUnixTime( long tv_sec, long tv_usec );

	/**
	 * \brief Convert DateTime to string-ified Tape Time
	 *
	 * \param dtTm - DateTime
	 * \return YYYYMMDD HH:MM:SS
	 */
	static String ^TapeTimeString( DateTime ^dtTm );

	/**
	 * \brief Convert string-ified Tape Time to DateTime
	 *
	 * \param dtTm = string-ified time as YYYYMMDD HH:MM:SS
	 * \return DateTime
	 */
	static DateTime StringToTapeTime( String ^dtTm );

};  // class rtEdge


////////////////////////////////////////////////
//
//         c l a s s   C h a n n e l
//
////////////////////////////////////////////////
/**
 * \class Channel
 * \brief Abstract Publication / Subscription base class
 */
public ref class Channel : public librtEdge::rtEdge
{
protected:
	RTEDGE::Channel *_chan;

	/////////////////////////////////
	// Constructor
	/////////////////////////////////
protected:
	Channel();
	~Channel();

	/////////////////////////////////
	// librtEdge Operations 
	/////////////////////////////////
	/**
	 * \brief Return true if this Channel has been initialized and not Stop()'ed
	 *
	 * \return true if this Channel has been initialized and not Stop()'ed
	 */
public:
	bool IsValid();

	/**
	 * \brief Configure the publication or subscription channel
	 *
	 * \param cmd - Command from rtEdgeIoctl
	 * \param val - Command value
	 */
	void Ioctl( rtEdgeIoctl cmd, IntPtr val );

	/**
	 * \brief Retrieve channel run-time stats
	 *
	 * \return Channel run-time stats
	 */
	rtEdgeChanStats ^GetStats();

	/**
	 * \brief Set SO_RCVBUF for this rtEdgeCache3 channel
	 *
	 * \param bufSiz - SO_RCVBUF size
	 * \return  GetRxBufSize()
	 */
	int SetRxBufSize( int bufSiz );

	/**
	 * \brief Get SO_RCVBUF for this rtEdgeCache3 channel
	 *
	 * \return SO_RCVBUF size
	 */
	int GetRxBufSize();

	/**
	 * \brief Sets the max queue size on outbound channel to Edge2
	 *
	 * \param bufSiz - Max queue size
	 * \return  GetTxBufSize()
	 */
	int SetTxBufSize( int bufSiz );

	/**
	 * \brief Gets current outbound channel queue size.   Max queue
	 * size had been set via SetTxBufSize()
	 *
	 * \return Current queue size on outbound channel to Edge2
	 */
	int GetTxBufSize();

	/**
	 * \brief Tie this channel thread to a specific CPU
	 *
	 * \param cpu - CPU core to attach this channel thread to
	 * \return  GetThreadProcessor()
	 */
	int SetThreadProcessor( int cpu );

	/**
	 * \brief Get the CPU this channel is tied to
	 *
	 * \return  CPU this channel is tied to
	 */
	int GetThreadProcessor();

	/**
	 * \brief Get the thread ID of the library thread
	 *
	 * \return Thread ID of the library thread
	 */
	long GetThreadID();

	/**
	 * \brief Returns true if channel is Binary
	 *
	 * \return true if channel is Binary
	 */
	bool IsBinary();

	/**
	 * \brief Returns true if channel is MF
	 *
	 * \return true if channel is MF
	 */
	bool IsMF();

	/**
	 * \brief Return name of destination connection
	 *
	 * \return Name of destination connection
	 */
	String ^DstConnName();

	/**
	 * \brief Creates a shared memory file for the run-time statistics for
	 * your publication and/or subscription channel(s).
	 *
	 * Exposing your stats in shared memory file allows the FeedMon.exe
	 * agent to monitor your application and optionally stuff stats into DataDog.
	 *
	 * Usage:
	 * -# Up to one subscriber and one publisher may use same file 
	 * -# The 3 string parameters - file, exe, bld - must all be defined  
	 *
	 * \param cxt - Context from rtEdge_Initialize() or rtEdge_PubInit()
	 * \param fileName - Run-time stats filename
	 * \param exeName - Application executable name
	 * \param buildName - Application build name
	 * \return true if successful
	 */
	bool SetMDDirectMon( String ^fileName, 
	                     String ^exeName, 
	                     String ^buildName );

};  // class Channel


////////////////////////////////////////////////
//
//   c l a s s   r t E d g e C h a n S t a t s
//
////////////////////////////////////////////////
/**
 * \struct rtEdgeChanStats
 * \brief Run-time channel statistics returned from rtEdge_GetStats().
 *
 * May be used for either subscription or publication channels
 */
public ref class rtEdgeChanStats
{
private:
	::rtEdgeChanStats *_stC;
	long               _NANOcli;

	/////////////////////////////////
	// Constructor
	/////////////////////////////////
public:
	/** 
	 * \brief Constructor for native rtEdgeChanStats container
	 *
	 * \param stC : Pointer to native C/C++ rtEdgeChanStats
	 */
	rtEdgeChanStats( ::rtEdgeChanStats *stC ) :
	   _stC( stC ),
	   _NANOcli( 1000000000 )
	{ ; }

	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Num msgs read (SUBSCRIBE) or written (PUBLISH) */
	property long long _nMsg
	{
	   long long get() { return _stC->_nMsg; }
	}

	/** \brief Num bytes read (SUBSCRIBE) or written (PUBLISH) */
	property long long _nByte
	{
	   long long get() { return _stC->_nByte; }
	}

	/** \brief Unix time of last msg read (SUBSCRIBE) or written (PUBLISH) */
	property DateTime ^_lastMsg
	{
	   DateTime ^get() {
	      long tTickNs;

	      tTickNs  = _stC->_lastMsg; // seconds
	      tTickNs *= _NANOcli;          // Nanos
	      tTickNs += ( _stC->_lastMsgUs * 1000 );
	      return gcnew DateTime( tTickNs );
	   }
	}

	/** \brief Total Num of subscription streams opened since startup  */
	property int    _nOpen
	{
	   int get() { return _stC->_nOpen; }
	}

	/** \brief Total Num of subscription streams closed since startup  */
	property int    _nClose
	{
	   int get() { return _stC->_nClose; }
	}

	/** \brief Total Num of Images consumed (SUBSRIBE) or published (PUBLISH) */
	property int    _nImg
	{
	   int get() { return _stC->_nImg; }
	}

	/** \brief Total Num of Updates consumed (SUBSRIBE) or published (PUBLISH) */
	property int    _nUpd
	{
	   int get() { return _stC->_nUpd; }
	}

	/** \brief Total Num of DEAD streams */
	property int    _nDead
	{
	   int get() { return _stC->_nDead; }
	}

	/** \brief Current outbound queue size */
	property int    _qSiz
	{
	   int get() { return _stC->_qSiz; }
	}

	/** \brief Max outbound queue size */
	property int    _qSizMax
	{
	   int get() { return _stC->_qSizMax; }
	}

	/** \brief DateTime of last connection */
	property DateTime ^_lastConn
	{
	   DateTime ^get() {
	      long tTickNs;

	      tTickNs  = _stC->_lastConn; // seconds
	      tTickNs *= _NANOcli;  
	      return gcnew DateTime( tTickNs );
	   }
	}

	/** \brief DateTime of last disconnect */
	property DateTime ^_lastDisco
	{
	   DateTime ^get() {
	      long tTickNs;

	      tTickNs  = _stC->_lastDisco; // seconds
	      tTickNs *= _NANOcli;  
	      return gcnew DateTime( tTickNs );
	   }
	}

	/** \brief Total number of connections since startup */
	property int    _nConn
	{
	   int get() { return _stC->_nConn; }
	}

	/** \brief Channel name - SUBSCRIBE or PUBLISH */
	property String ^_chanName
	{
	   String ^get() { return gcnew String( _stC->_chanName ); }
	}

	/** \brief Destination connection as \<host\>:\<port\> */
	property String   ^_dstConn
	{
	   String ^get() { return gcnew String( _stC->_dstConn ); }
	}

	/** \brief 1 if channel is connected; 0 if not */
	property bool   _bUp
	{
	   bool get() { return _stC->_bUp ? true : false; }
	}

	/** \brief User-defined array of integer statistics */
	property int _iVal[int]
	{
	   int  get( int idx ) { return _stC->_iVal[idx]; }
	   void set( int idx, int val ) { _stC->_iVal[idx] = val; }
	}

	/** \brief User-defined array of double statistics */
	property double _dVal[int]
	{
	   double get( int idx ) { return _stC->_dVal[idx]; }
	   void   set( int idx, double val ) { _stC->_dVal[idx] = val; }
	}


};  // rtEdgeChanStats

} // namespace librtEdge
