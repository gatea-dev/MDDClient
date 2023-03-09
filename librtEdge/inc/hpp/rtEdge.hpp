/******************************************************************************
*
*  rtEdge.hpp
*     librtEdge rtEdge base class - static members
*
*  REVISION HISTORY:
*     11 DEC 2014 jcs  Created.
*      8 JAN 2015 jcs  Build 29: TrimString()
*     19 JUN 2015 jcs  Build 31: Channel; _bCache
*     15 APR 2016 jcs  Build 32: OnIdle(); GetThreadID()
*     29 SEP 2016 jcs  Build 33: rtDate / rtTime; unix2rtTime()
*     13 JUL 2017 jcs  Build 34: unix2rtDateTime
*     12 OCT 2017 jcs  Build 36: hash_map
*      7 NOV 2017 jcs  Build 38: RemapFile()
*      7 JUN 2018 jcs  Build 40: breakpoint()
*      6 DEC 2018 jcs  Build 41: VOID_PTR
*      9 FEB 2020 jcs  Build 42: Full namespace; Channel.SetHeartbeat()
*     29 APR 2020 jcs  Build 43: _BDS_PFX
*      7 SEP 2020 jcs  Build 44: XxxThreadName()
*     21 NOV 2020 jcs  Build 46: IsStopping()
*     23 APR 2021 jcs  Build 48: GetDstConn()
*     26 APR 2022 jcs  Build 53: Channel.SetMDDirectMon()
*     23 MAY 2022 jcs  Build 54: Channel.OnError()
*      6 SEP 2022 jcs  Build 56: GetMaxTxBufSize()
*     26 OCT 2022 jcs  Build 58: CxtMap
*     29 OCT 2022 jcs  Build 60: DoubleList
*     26 NOV 2022 jcs  Build 61: DateTimeList; DoubleXY
*      9 MAR 2023 jcs  Build 62: Dump( DoubleGrid & ); static GetThreadID()
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_rtEdge_H
#define __RTEDGE_rtEdge_H
#ifdef WIN32
#pragma warning(disable:4190)
#define socklen_t int
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif // WIN32

#include <string>
#include <map>
#include <vector>
#include <hpp/Mutex.hpp>

#undef K
#if !defined(hash_map)
#if !defined(_GLIBCXX_UNORDERED_MAP) && !defined(WIN32)
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define hash_map std::tr1::unordered_map
#define hash_set std::tr1::unordered_set
#else
#include <unordered_map>
#include <unordered_set>
#define hash_map std::unordered_map
#define hash_set std::unordered_set
#endif // !defined(_GLIBCXX_UNORDERED_MAP)
#endif // !defined(hash_map)
#define K 1024

#ifndef DOXYGEN_OMIT
static double e_MIL     =  1000000.0;

#if !defined(VOID_PTR)
#define VOID_PTR       void *)(size_t
#endif // !defined(VOID_PTR)

#define _BDS_PFX       "~~SYM_LIST~~"

#endif // DOXYGEN_OMIT

namespace RTEDGE
{


////////////////////////////////////////////////
//
//        c l a s s   r t D a t e
//
////////////////////////////////////////////////

/**
 * \class rtDate
 * \brief Calendar date - YMD
 */
class rtDate
{
public:
	/** \brief 4-digit year */
	int _year;
	/** \brief Month : { 0 = Jan, ..., 11 = Dec } */
	int _month;
	/** \brief Day of month : { 0 .. 31 } */
	int _mday;

};  // class rtDate


////////////////////////////////////////////////
//
//        c l a s s   r t T i m e
//
////////////////////////////////////////////////

/**
 * \class rtTime
 * \brief Calendar time - HMS.uuuuuu
 */
class rtTime
{
public:
	/** \brief Hour : { 0, ..., 23 } */
	int _hour;
	/** \brief Minute : { 0, ..., 60 } */
	int _minute;
	/** \brief Second : { 0, ..., 60 } */
	int _second;
	/** \brief MicroSecond : { 0, ..., 999999 } */
	int _micros;

}; // class rtTime


////////////////////////////////////////////////
//
//      c l a s s   r t D a t e T i m e
//
////////////////////////////////////////////////

/**
 * \class rtDateTime
 * \brief Calendar date/time - YMD, HMS.uuuuuu
 */
class rtDateTime
{
public:
	/** \brief Date : YMD */
	rtDate _date;
	/** \brief Time : HMS.uuuuuu */
	rtTime _time;

}; // class rtDateTime


////////////////////////////////////////////////
//
//      c l a s s   D o u b l e X Y
//
////////////////////////////////////////////////

/**
 * \class DoubleXY
 * \brief ( x,y ) tuple
 */
class DoubleXY
{
public:
	/** \brief X value */
	double _x;
	/** \brief Y value */
	double _y;

}; // class DoubleXY


////////////////////////////////////////////////
//
//      c l a s s   D o u b l e X Y Z
//
////////////////////////////////////////////////

/**
 * \class DoubleXYZ
 * \brief ( x,y ) tuple
 */
class DoubleXYZ : public DoubleXY
{
public:
	/** \brief Z value */
	double _z;

}; // class DoubleXYZ

typedef std::vector<std::string>  Strings;
typedef std::vector<double>       DoubleList;
typedef std::vector<DoubleXY>     DoubleXYList;
typedef std::vector<DoubleXYZ>    DoubleXYZList;
typedef std::vector<DoubleList>   DoubleGrid;
typedef std::vector<rtDateTime>   DateTimeList;


////////////////////////////////////////////////
//
//        c l a s s   r t E d g e
//
////////////////////////////////////////////////

/**
 * \class rtEdge
 * \brief Library-wide base class
 *
 * Contains common class-wide methods and variables
 */
class rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor. */
	rtEdge() :
	   RDNDISPLAY( 2 ),
	   DSPLY_NAME( 3 ),
	   REF_COUNT( 239 ),
	   RECORDTYPE( 259 ),
	   PREV_LR( 237 ),
	   NEXT_LR( 238 ),
	   LINK_1( 240 ),
	   LONGLINK1( 800 ),
	   LONGPREVLR( 814 ),
	   LONGNEXTLR( 815 ),
	   _NUM_LINK( 14 ),
	   _CHAIN_REC( 120 )
	{ ; }

	/** \brief Destructor. */
	virtual ~rtEdge() { ; }

	////////////////////////////////////
	// Class-wide public methods - librtEdge
	////////////////////////////////////
public:
	/**
	 * \brief No-Op
	 */
	static void breakpoint()
	{
	}

	/**
	 * \brief Get the thread ID of the current thread
	 *
	 * \return Thread ID of the current thread
	 */
	static u_int64_t GetCurrentThreadID()
	{
	   u_int64_t tid;

	   tid = 0;
	   ::rtEdge_ioctl( 0, ioctl_getThreadId, &tid );
	   return tid;
	}

	/**
	 * \brief Returns the library build version.
	 *
	 * \return Build version of the library
	 */
	static const char *Version()
	{
	   return ::rtEdge_Version();
	}

	/**
	 * \brief Sets the library debug level; Initiates logging. 
	 *
	 * \param pLog - Log filename
	 * \param dbgLvl - Debug level
	 */
	static void Log( const char *pLog, int dbgLvl )
	{
	   ::rtEdge_Log( pLog, dbgLvl );
	}

	/**
	 * \brief Returns clock counter to nanosecond granularity
	 *
	 * \return Clock counter to nanosecond granularity
	 */
	static double TimeNs()
	{
	   return ::rtEdge_TimeNs();
	}

	/**
	 * \brief Returns current time in Unixtime
	 *
	 * \return Current time in Unixtime
	 */
	static time_t TimeSec()
	{
	   return ::rtEdge_TimeSec();
	}

	/**
	 * \brief Returns  time in YYYY-MM-DD HH:MM:SS.mmm
	 *
	 * \see ::rtEdge_pDateTimeMs()
	 * \param rtn - std::string to hold return value
	 * \param dTime - 0 for current time
	 * \return Current time in YYYY-MM-DD HH:MM:SS.mmm 
	 */
	static const char *pDateTimeMs( std::string &rtn, double dTime=0.0 )
	{
	   char buf[K];

	   rtn = ::rtEdge_pDateTimeMs( buf, dTime );
	   return rtn.c_str();
	}

	/**
	 * \brief Returns current time in HH:MM:SS.mmm
	 *
	 * \see ::rtEdge_pTimeMs()
	 * \param rtn - std::string to hold return value
	 * \param dTime - 0 for current time
	 * \return Current time in HH:MM:SS.mmm 
	 */
	static const char *pTimeMs( std::string &rtn, double dTime=0.0 )
	{
	   char buf[K];

	   rtn = ::rtEdge_pTimeMs( buf, dTime );
	   return rtn.c_str();
	}

	/**
	 * \brief Returns number of seconds to the requested mark.
	 *
	 * \param h - Hour
	 * \param m - Minute
	 * \param s - Second
	 * \return Number of seconds to requested mark
	 */
	static int Time2Mark( int h, int m, int s )
	{
	   return ::rtEdge_Time2Mark( h,m,s );
	}

	/**
	 * \brief Converts rtDateTime to Unix time as double
	 *
	 * \param dtTm - Date/Time to convert
	 * \return Unix time as double
	 */
	static double rtEdgeDateTime2unix( rtDateTime dtTm )
	{
	   time_t    now = TimeSec();
	   rtDate   &dt  = dtTm._date;
	   rtTime   &tm  = dtTm._time;
	   struct tm lt;
	   double    rc;

	   ::localtime_r( &now, &lt );
	   lt.tm_year = dt._year - 1900;
	   lt.tm_mon  = dt._month;
	   lt.tm_mday = dt._mday;
	   lt.tm_hour = tm._hour;
	   lt.tm_min  = tm._minute;
	   lt.tm_sec  = tm._second;
	   rc         = ::mktime( &lt ); 
	   rc        += ( 1.0 / e_MIL ) * tm._micros;
	   return rc;
	}

	/**
	 * \brief Converts Unix time to rtDateTime
	 *
	 * \param tv - Unix time in struct timeval
	 * \return rtDateTime
	 */
	static rtDateTime unix2rtDateTime( struct timeval tv )
	{
	   struct tm *tm, lt;
	   time_t    *t;
	   rtDateTime dtTm;
	   rtDate    &rd = dtTm._date;
	   rtTime    &rt = dtTm._time;

	   t          = (time_t *)&tv.tv_sec;
	   tm         = ::localtime_r( t, &lt );
	   rd._year   = tm->tm_year + 1900;
	   rd._month  = tm->tm_mon;
	   rd._mday   = tm->tm_mday;
	   rt._hour   = tm->tm_hour;
	   rt._minute = tm->tm_min;
	   rt._second = tm->tm_sec;
	   rt._micros = tv.tv_usec;
	   return dtTm;
	}

	/**
	 * \brief Converts Unix time to rtDateTime
	 *
	 * \param dtTm - Unix time as double
	 * \return rtDateTime
	 */
	static rtDateTime unix2rtDateTime( double dtTm ) 
	{
	   struct timeval tv;
	   double         uS;

	   tv.tv_sec  = (time_t)dtTm;
	   uS         = ( dtTm - tv.tv_sec ) * e_MIL;
	   tv.tv_usec = uS;
	   return unix2rtDateTime( tv );
	}

	/**
	 * \brief Converts Unix time to rtTime
	 *
	 * \param tv - Unix time in struct timeval
	 * \return rtTime
	 */
	static rtTime unix2rtTime( struct timeval tv )
	{
	   struct tm *tm, lt;
	   time_t    *t;
	   rtTime     rt;

	   t          = (time_t *)&tv.tv_sec;
	   tm         = ::localtime_r( t, &lt );
	   rt._hour   = tm->tm_hour;
	   rt._minute = tm->tm_min;
	   rt._second = tm->tm_sec;
	   rt._micros = tv.tv_usec;
	   return rt;
	}

	/**
	 * \brief Sleeps for requested period of time.
	 *
	 * \param dSlp - Number of \<seconds\>.\<microseconds\> to sleep
	 */
	static void Sleep( double dSlp )
	{
	   ::rtEdge_Sleep( dSlp );
	}

	/**
	 * \brief Hex dump a message into an output buffer
	 *
	 * \param msg - Message to dump
	 * \param len - Message length
	 * \return Hex dump of msg in std::string
	 */
	std::string HexMsg( char *msg, int len )
	{
	   std::string tmp;
	   char       *obuf;
	   int         sz;

	   obuf     = new char[len*8];
	   sz       = ::rtEdge_hexMsg( msg, len, obuf );
	   obuf[sz] = '\0';
	   tmp      = obuf;
	   delete[] obuf;
	   return std::string( tmp );
	}

	/**
	 * \brief Returns total CPU seconds used by application
	 *
	 * \return Total CPU seconds used by application
	 */
	static double CPU()
	{
	   return ::rtEdge_CPU();
	}

	/**
	 * \brief Returns total memory size of this process
	 *
	 * \return Total memory size for this process
	 */
	static int MemSize()
	{
	   return ::rtEdge_MemSize();
	}

	/**
	 * \brief Query file stats
	 *
	 * \param pFile - Filename
	 * \return OSFileStat results
	 */
	OSFileStat GetFileStats( const char *pFile )
	{
	   return ::OS_GetFileStats( pFile );
	}

	/**
	 * \brief Creates a memory-mapped view of the file
	 *
	 * Call UnmapFile() to unmap the view.
	 *
	 * \param pFile - Name of file to map
	 * \param bCopy - true to make copy; false for view-only
	 * \return Mapped view of a file in an rtBUF struct
	 */
	static rtBuf64 MapFile( const char *pFile, bool bCopy=true )
	{
	   return ::rtEdge_MapFile( (char *)pFile, bCopy ? 1 : 0 );
	}

	/**
	 * \brief Re-maps view of file previously mapped via MapFile()
	 *
	 * This works as follows:
	 * + Current view passed in as 1st argument must be a view, not copy
	 * + File is re-mapped if the file size has grown
	 * + If file size has not changed, then view is returned
	 *
	 * \param view - Current view of file from MapFile()
	 * \return Mapped view of a file in an rtBUF struct
	 * \see MapFile()
	 */
	static rtBuf64 RemapFile( rtBuf64 view )
	{
	   return ::rtEdge_RemapFile( view );
	}

	/**
	 * \brief Unmaps a memory-mapped view of a file
	 *
	 * \param bMap - Memory-mapped view of file from MapFile()
	 */
	static void UnmapFile( rtBuf64 bMap )
	{
	   ::rtEdge_UnmapFile( bMap );
	}

	/**
	 * \brief Returns true if 64-bit build; false if 32-bit
	 *
	 * \return true if 64-bit build; false if 32-bit
	 */
	static bool Is64Bit()
	{
	   return ::rtEdge_Is64bit() ? true : false;
	}


	////////////////////////////////////
	// Class-wide public utilities
	////////////////////////////////////
	/**
	 * \brief Trim leading / trailing spaces from a string
	 *
	 * \param str - String to trim
	 * \return Trimmed string
	 */
	static char *TrimString( char *str )
	{
	   char *cp, *rtn;
	   int   i, len;

	   // Pre-condition

	   len = str ? strlen( str ) : 0;
	   if ( !len )
	      return str;

	   // Trim leading fleh ...

	   rtn = cp = str;
	   for ( i=0; !InRange( '!',*cp,'~') && i<len; i++,cp++ );
	   if ( i == len ) {   // All fleh??
	      *str = '\0';
	      return str;
	   }
	   rtn = cp;

	   // ... and trailing fleh

	   cp = str+len-1;
	   for ( i=len-1; !InRange(  '!',*cp,'~') && i>=0; i--,cp-- );
	   *(cp+1) = '\0';

	   return rtn;
	}

	/**
	 * \brief Dump vector as CSV 
	 *
	 * \param g : Grid of doubles
	 * \param obuf : Destination string
	 * \return obuf.data()
	 */
	static const char *Dump( DoubleGrid &g, std::string &obuf )
	{
	   size_t      i, n;
	   std::string r;

	   obuf.clear();
	   n = g.size();
	   for ( i=0; i<n; i++ ) {
	      obuf += "[ ";
	      obuf += Dump( g[i], r );
	      obuf += "]\n";
	   }
	   return obuf.data();
	}

	/**
	 * \brief Dump vector as CSV 
	 *
	 * \param v : List of doubles to dump
	 * \param obuf : Output buffer
	 * \return obuf.data()
	 */
	static const char *Dump( DoubleList &v, std::string &obuf )
	{
	   char  *bp, *cp;
	   size_t i, n;

	   obuf.clear();
	   if ( (n=v.size()) ) {
	      bp = new char[(n+4)*100];
	      cp = bp;
	      for ( i=0; i<n; i++ ) {
	         cp += i ? sprintf( cp, "," ) : 0;
	         cp += sprintf( cp, "%.4f", v[i] );
	      }
	      obuf = bp;
	      delete[] bp;
	   }
	   return obuf.data();
	}

	////////////////////////////////////
	// Protected Members
	////////////////////////////////////
protected:
	   int RDNDISPLAY;
	   int DSPLY_NAME;
	   int REF_COUNT;
	   int RECORDTYPE;
	   int PREV_LR;
	   int NEXT_LR;
	   int LINK_1;
	   int LONGLINK1;
	   int LONGPREVLR;
	   int LONGNEXTLR;
	   int _NUM_LINK;
	   int _CHAIN_REC;

};  // class rtEdge



////////////////////////////////////////////////
//
//        c l a s s    C h a n n e l
//
////////////////////////////////////////////////

/**
 * \class Channel
 * \brief Abstract Publication / Subscription base class
 */

class Channel : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
protected:
	/** \brief Constructor. */
	Channel( bool bPub ) :
	   _bPub( bPub ),
	   _cxt( (rtEdge_Context)0 ),
	   _cxtThr( (Thread_Context)0 ),
	   _tHbeat( 60 ),
	   _bIdleCbk( false ),
	   _bCache( false ),
	   _bStopping( false ),
	   _dstConn()
	{ ; }

	/** \brief Destructor. */
public:
	virtual ~Channel()
	{
	   Stop();
	}


	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Returns rtEdge_Context associated with this channel
	 *
	 * \return rtEdge_Context associated with this channel
	 */
	rtEdge_Context cxt()
	{
	   return _cxt;
	}

	/**
	 * \brief Return true if this Channel has been initialized and not Stop()'ed
	 *
	 * \return true if this Channel has been initialized and not Stop()'ed
	 */
	bool IsValid()
	{
	   return( _cxt != (rtEdge_Context)0 );
	}

	/**
	 * \brief Return true if Stop() has been or is being called
	 *
	 * \return true if Stop() has been or is being called
	 */
	bool IsStopping()
	{
	   return _bStopping;
	}

	/**
	 * \brief Configure pub / sub channel
	 *
	 * \param cmd - Command from rtEdgeIoctl
	 * \param val - Command value
	 * \see ::rtEdge_ioctl()
	 */
	void Ioctl( rtEdgeIoctl cmd, void *val )
	{
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, cmd, val );
	}

	/**
	 * \brief Return socket file descriptor for this Channel
	 *
	 * \return Socket file descriptor for this Channel
	 */
	int GetSocket()
	{
	   int fd;

	   fd = 0;
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, ioctl_getFd, &fd );
	   return fd;
	}

	/**
	 * \brief Return name of destination connection
	 *
	 * \return Name of destination connection
	 */
	const char *DstConnName()
	{
	   struct sockaddr_in dst;
	   struct hostent    *h;
	   struct in_addr     ip;
	   uint32_t           addr;
	   uint16_t           port;
	   socklen_t          dSz;
	   const char        *pd;
	   char               sPort[K];
	   int                fd;

	   // Set once; Empty _dstConn implies disconnected

	   if ( !(fd=GetSocket()) )
	      _dstConn = "";
	   else if ( !_dstConn.length() ) {
	      dSz = sizeof( struct sockaddr );
	      ::getpeername( fd, (struct sockaddr *)&dst, &dSz );
	      addr = dst.sin_addr.s_addr;
	      port = ntohs( dst.sin_port );
	      h    = ::gethostbyaddr( (char *)&addr, sizeof( addr ), AF_INET );
	      if ( h && h->h_name )
	         _dstConn = h->h_name;
	      else {
	         ip.s_addr = addr;
	         _dstConn  = ::inet_ntoa( ip );
	      }
	      sprintf( sPort, ":%d", port );
	      _dstConn += sPort;
	   }

	   // Empty _dstConn implies disconnected

	   pd = _dstConn.length() ? _dstConn.data() : "Not connected";
	   return pd;
	}

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
	 * \param fileName - Run-time stats filename
	 * \param exeName - Application executable name
	 * \param buildName - Application build name
	 * \return true if successful
	 */
	bool SetMDDirectMon( const char *fileName,
	                     const char *exeName,
	                     const char *buildName )
	{
	   char rc;

	   rc = 0;
	   if ( IsValid() )
	      rc = ::rtEdge_SetMDDirectMon( _cxt, fileName, exeName, buildName );
	   return rc ? true : false;
	}

	/**
	 * \brief Set channel run-time stats
	 *
	 * \param stat - Run-time stats to set
	 * \return true if successful; false otherwise
	 */
	bool SetStats( rtEdgeChanStats *stat )
	{
	   return ::rtEdge_SetStats( _cxt, stat ) ? true : false;
	}

	/**
	 * \brief Retrieve channel run-time stats
	 *
	 * \return Channel run-time stats
	 */
	rtEdgeChanStats *GetStats()
	{
	   rtEdgeChanStats *rtn;

	   rtn = (rtEdgeChanStats *)0;
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, ioctl_getStats, &rtn );
	   return rtn;
	}

	/**
	 * \brief Set SO_RCVBUF for this rtEdgeCache3 channel
	 *
	 * \param bufSiz - SO_RCVBUF size
	 * \return  GetRxBufSize()
	 */
	int SetRxBufSize( int bufSiz )
	{
	   ::rtEdge_ioctl( _cxt, ioctl_setRxBufSize, &bufSiz );
	   return GetRxBufSize();
	}

	/**
	 * \brief Get SO_RCVBUF for this rtEdgeCache3 channel
	 *
	 * \return SO_RCVBUF size
	 */
	int GetRxBufSize()
	{
	   int bufSiz;

	   bufSiz = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_getRxBufSize, &bufSiz );
	   return bufSiz;
	}

	/**
	 * \brief Sets the max queue size on outbound channel to Edge2
	 *
	 * \param bufSiz - Max queue size
	 * \return  GetTxBufSize()
	 */
	int SetTxBufSize( int bufSiz )
	{
	   ::rtEdge_ioctl( _cxt, ioctl_setTxBufSize, &bufSiz );
	   return GetTxBufSize();
	}

	/**
	 * \brief Gets current outbound channel queue size.   Max queue
	 * size had been set via SetTxBufSize()
	 *
	 * \return Current queue size on outbound channel to Edge2
	 */
	int GetTxBufSize()
	{
	   int bufSiz;

	   bufSiz = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_getTxBufSize, &bufSiz );
	   return bufSiz;
	}

	/**
	 * \brief Gets MAX outbound channel queue size.  Max queue size
	 * is set via SetTxBufSize(); Current queue size is GetTxBufSize()
	 *
	 * \return Max queue size on outbound channel to Edge3
	 * \see SetTxBufSize()
	 */
	int GetTxMaxBufSize()
	{
	   int bufSiz;

	   bufSiz = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_getTxMaxSize, &bufSiz );
	   return bufSiz;
	}

	/**
	 * \brief Get the thread ID of the library thread
	 *
	 * \return Thread ID of the library thread
	 */
	u_int64_t GetThreadID()
	{
	   u_int64_t tid;

	   tid = 0;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_getThreadId, &tid );
	   return tid;
	}

	/**
	 * \brief Tie this channel thread to a specific CPU
	 *
	 * \param cpu - CPU core to attach this channel thread to
	 * \return  GetThreadProcessor()
	 */
	int SetThreadProcessor( int cpu )
	{
	   ::rtEdge_ioctl( _cxt, ioctl_setThreadProcessor, &cpu );
	   return GetThreadProcessor();
	}

	/**
	 * \brief Get the CPU this channel is tied to
	 *
	 * \return  CPU this channel is tied to
	 */
	int GetThreadProcessor()
	{
	   int cpu;

	   cpu = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_getThreadProcessor, &cpu );
	   return cpu;
	}

	/**
	 * \brief Set thread name : viewable in top
	 *
	 * Only valid if this is called from the pub/sub Channel thread
	 *
	 * \param name - Thread name
	 * \see GetThreadName()
	 */
	const char *SetThreadName( const char *name )
	{
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, ioctl_setThreadName, (void *)name );
	   return GetThreadName();
	}

	/**
	 * \brief Get thread name set from SetThreadName()
	 *
	 * \return Thread name, or empty string if not set
	 * \see SetThreadName()
	 */
	const char *GetThreadName()
	{
	   const char *name;

	   name = "";
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, ioctl_getThreadName, (void *)name );
	   return name;
	}

	/**
	 * \brief Returns channel protocol
	 *
	 * \return Channel protocol
	 */
	mddProtocol GetProtocol()
	{
	   mddProtocol pro;

	   pro = mddProto_Undef;
	   ::rtEdge_ioctl( _cxt, ioctl_getProtocol, (void *)&pro );
	   return pro;
	}

	/**
	 * \brief Returns channel protocol
	 *
	 * \return Channel protocol
	 */
	rtBUF GetInputBuffer()
	{
	   rtBUF b;

	   b._data = NULL;
	   b._dLen = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_getInputBuffer, (void *)&b );
	   return b;
	}

	/**
	 * \brief Returns true if channel is Binary
	 *
	 * \return true if channel is Binary
	 */
	bool IsBinary()
	{
	   mddProtocol pro;

	   pro = GetProtocol();
	   return( pro == mddProto_Binary );
	}

	/**
	 * \brief Returns true if channel is MF
	 *
	 * \return true if channel is MF
	 */
	bool IsMF()
	{
	   mddProtocol pro;

	   pro = GetProtocol();
	   return( pro == mddProto_MF );
	}

	/**
	 * \brief Enables cache-ing in the library.
	 *
	 * This must be called BEFORE calling Start().
	 */
	void SetCache( bool bCache )
	{
	   _bCache = !_cxt ? bCache : _bCache;
	}

	/**
	 * \brief Return true if cache-ing
	 *
	 * \return true if cache-ing
	 */
	bool IsCache()
	{
	   return _bCache;
	}

	/**
	 * \brief Allow / Disallow the library timer to call out to OnIdle()
	 * every second or so.
	 *
	 * This is useful for performing tasks in the librtEdge thread
	 * periodically, for example if you need to ensure that you call
	 * Subscribe() ONLY from the subscription channel thread.
	 *
	 * \param bIdleCbk - true to receive OnIdle(); false to disable
	 */
	void SetIdleCallback( bool bIdleCbk )
	{
	   _bIdleCbk = bIdleCbk;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_setIdleTimer, (void *)_bIdleCbk );
	}

	/**
	 * \brief Sets channel heartbeat / ping interval in seconds.
	 *
	 * At connect time, this value is passed to the rtEdgeCache3 server.
	 * The server sends a Ping request at this interval, which the library
	 * returns.  If the server does not get a message from the library
	 * in twice this interval, the connection is terminated by the
	 * server.
	 *
	 * This must be called BEFORE calling Start() for it to take effect.
	 * If not called, the library default of 3600 (1 hour) is used.
	 *
	 * \param tHbeat - Heartbeat interval in seconds.
	 */
	void SetHeartbeat( int tHbeat )
	{
	   _tHbeat = tHbeat;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_setHeartbeat, (VOID_PTR)_tHbeat );
	}


	////////////////////////////////////
	// Access / Operations - Worker Thread
	////////////////////////////////////
public:
	/**
	 * \brief Starts the (one) worker thread for this channel
	 *
	 * The library will call OnWorkerThread() until you call StopThread().
	 * In order to stop and cleanly terminate this worker thread, your
	 * OnWorkerThread() must periodically do one of the following:
	 * Thread Type | Requirement
	 * --- | ---
	 * + Cooperative | Return control back to the library
	 * + Greedy | Call ThreadIsRunning() and if false return control
	 *
	 * \see ThreadIsRunning()
	 * \see StopThread()
	 * \see OnWorkerThread()
	 */
	void StartThread()
	{
	   StopThread();
	   _cxtThr = ::OS_StartThread(  Channel::_thrCbk, this );
	}

	/**
	 * \brief Returns true if the (one) worker thread is running
	 *
	 * \return true if the (one) worker thread is running
	 */
	bool ThreadIsRunning()
	{
	   return ::OS_ThreadIsRunning( _cxtThr ) ? true : false;
	}

	/**
	 * \brief Stop the (one) worker thread for this channel
	 */
	void StopThread()
	{
	   if ( _cxtThr )
	      ::OS_StopThread( _cxtThr );
	   _cxtThr = (Thread_Context)0;
	}


	////////////////////////////////////
	// RTEDGE::Channel Interface
	////////////////////////////////////
	/**
	 * \brief Destroy connection to the rtEdgeCache3
	 *
	 * Calls ::rtEdge_Destroy() to disconnect from the rtEdgeCache3 server.
	 */
	virtual void Stop()
	{
	   _bStopping = true;
	   StopThread();
	   if ( _cxt ) {
	      if ( _bPub )
	      ::rtEdge_PubDestroy( _cxt );
	      else
	      ::rtEdge_Destroy( _cxt );
	   }
	   _cxt  = (rtEdge_Context)0;
	}


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously - roughly once per second - when the
	 * library is idle.
	 *
	 * This is only called if you enable via SetIdleCallback()
	 *
	 * Override this method in your application to perform tasks
	 * in the subscription channel thread.
	 */
	virtual void OnIdle()
	{ ; }

	/**
	 * \brief Called repeatedly by the library between the time you
	 * call StartThread() and StopThread()
	 *
	 * If your Worker Thread is an infinite loop, you must periodically
	 * call ThreadIsRunning() and return control if false
	 */
	virtual void OnWorkerThread()
	{ ; }

public:
	/**
	 * \brief Called when a General Purpose error occurs on the channel
	 *
	 * \param err - Error message
	 */
	virtual void OnError( const char *err )
	{ ; }


	////////////////////////////////////
	// Protected Members
	////////////////////////////////////
protected:
	bool           _bPub;
	rtEdge_Context _cxt;
	Thread_Context _cxtThr;
	int            _tHbeat;
	bool           _bIdleCbk;
	bool           _bCache;
	bool           _bStopping;
	std::string    _dstConn;


	////////////////////////////////////
	// Class-wide (private) callbacks
	////////////////////////////////////
private:
	static void EDGAPI _thrCbk( void *arg )
	{
	   Channel *us;

	   us = (Channel *)arg;
	   us->OnWorkerThread();
	}

};  // class Channel

#ifndef DOXYGEN_OMIT

////////////////////////////////////////////////
//
//        c l a s s    C x t M a p
//
////////////////////////////////////////////////

/**
 * \class CxtMap
 * \brief Templatized channel lookup by context (integer)
 */

#define _CxtMap         hash_map<int, T>
#define _CxtMapIterator hash_map<int, T>::iterator

template <class T>
class CxtMap
{
private:
	_CxtMap _DB;
	Mutex   _mtx;

	/////////////////////
	// Constructor
	/////////////////////
public:
	CxtMap<T>() :
	  _DB(),
	  _mtx()
	{ ; }

	/////////////////////
	// Operations
	/////////////////////
public:
	T Get( int cxt )
	{    
	   Locker                     lck( _mtx );
	   typename _CxtMap::iterator it;
	   T                          rc;  

	   it = _DB.find( cxt );
	   rc = ( it != _DB.end() ) ? (*it).second : (T)0;
	   return rc;
	}    

	void Add( int cxt, T chan )
	{    
	   Locker  lck( _mtx );

	   _DB[cxt] = chan;
	}

	void Remove( int cxt )
	{
	   Locker                     lck( _mtx );
	   typename _CxtMap::iterator it;

	   if ( (it=_DB.find( cxt )) != _DB.end() )
	      _DB.erase( it );
	}

}; // class CxtMap<T>

#endif // DOXYGEN_OMIT

} // namespace RTEDGE

#endif // __RTEDGE_rtEdge_H 
