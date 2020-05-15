/******************************************************************************
*
*  libyamr.h
*     'C' interface library to yamr : Yet Another Message Recorder
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/

/**
 * \mainpage libyamr API Reference Manual
 *
 * YAMR : Yet Another Messaging Recorder.  libyamr allows you to send 
 * unstructured messages to the yamRecorder server farm for persistence.
 *
 * Multiple protocols are supported, allowing you to record any type of 
 * data:
 * + Real-Time Market Data
 * + Order Flow : Trades and Fills
 * + Web Traffic
 * + Usage Traffic
 * + Text Messages
 * + Encrypted Data
 * + etc.
 *
 * You are free to define your own structured data atop the unstructured 
 * message recording platform.  To do this, you define an extension of 
 * the YAMR::Data::Codec class
 *
 * Several native protocol types are already defined for you:
 * Description | Class
 * --- | ---
 * Integer List | YAMR::Data::IntList
 * Single Precision List | YAMR::Data::FloatList
 * Double Precision List | YAMR::Data::DoubleList
 * Field List | YAMR::Data::FieldList
 * String List | YAMR::Data::StringList
 * String ( Key, Value ) Map | YAMR::Data::StringMap
 */
#ifndef __LIB_YAMR_H
#define __LIB_YAMR_H
#ifdef __cplusplus
#define _CPP_BEGIN extern "C" {
#define _CPP_END   }
#else
#define _CPP_BEGIN
#define _CPP_END
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#if !defined(WIN32)
#include <arpa/inet.h>
#endif /* !defined(WIN32) */

/* Platform dependent : COM / .NET is __stdcall */

/**
 * \brief Handle different callback calling conventions on all platforms
 *
 * On Windows, COM / .NET is __stdcall
 */

#ifdef WIN32
typedef __int64        int64_t;
typedef __int64        u_int64_t;
typedef unsigned int   u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char  u_int8_t;
#define YAMRAPI        __stdcall
#define yamr_PRId64     "%lld"
#if !defined(__MINGW32__)
#pragma warning(disable:4018)
#pragma warning(disable:4101)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4355)
#pragma warning(disable:4786)
#pragma warning(disable:4996)
#endif /* !defined(__MINGW32__) */
#else
#define YAMRAPI
#define yamr_PRId64     "%ld"
#endif /* WIN32 */

/* Useful Macros */

/** \brief Find min of 2 values */
#define gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
/** \brief Find max of 2 values */
#define gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )
/** \brief true if b is in the range [ a,c ] */
#define InRange( a,b,c )     ( ((a)<=(b)) && ((b)<=(c)) )
/** \brief Too lazy to type in 1024 sometimes */
#define K                    1024
/** \brief Max Protocol */
#define YAMR_PROTO_MAX       0x7fff
/** \brief Max Data Type */
#define YAMR_DATATYPE_MAX    0x7fff
/** \brief Floats on wire at 4 sig figs */
#define YAMR_FLOAT_PRECISION 10000.0
/** \brief Doubles on wire at 10 sig figs */
#define YAMR_DBL_PRECISION   10000000000.0
/** \brief Max Packet (Message) Size : UDP */
#define YAMR_MAX_MTU         1500


_CPP_BEGIN

/* Data Structures */

/** \brief The context number of an initialized yamRecorder channel */
typedef long yamr_Context;
/** \brief The context number of an initialized yamRecorder tape */
typedef long yamrTape_Context;
/** \brief The context number of an initialized OS Thread */
typedef long Thread_Context;

/**
 * \enum yamrIoctl
 * \brief Channel control commands passed to yamr_ioctl()
 */
typedef enum {
   /**
    * \brief Retrieve run-time stats.
    *
    * The 3rd argument to yamr_ioctl() is the address of a yamrChanStats
    * pointer which points to the real-time stats for the channel.  In 
    * other words,
    *    \code{.cpp}
    *    yamrChanStats *st;
    *
    *    ::yamr_ioctl( cxt, ioctl_getStats, (void *)&st );
    *    printf( "%ld msgs processed\n", st->_nMsg );
    *    \endcode
    *
    * \param (void *)val - Address of yamrChanStats pointer
    */
   ioctl_getStats           = 1,
   /**
    * \brief Randomize yamrAttr._pSvrHosts when connecting; This allows you 
    * to configure a bunch of yamr client applications with a common config
    * file, yet have the load evenly (stochastically) distributed across 
    * available yamRrecorder servers.
    *
    * \param (void *)val - 1 to enable; 0 to disable; Default = 0
    */
   ioctl_randomize          = 2,
   /**
    * \brief Retrieves socket file descriptor
    *
    * \param (void *)val - Pointer to integer to receive fd 
    */
   ioctl_getFd              = 3,
   /**
    * \brief Sets the SO_RCVBUF.
    *
    * This must be called AFTER you all yamr_Start()
    *
    * \param (void *)val - Pointer to integer.  Default is system
    * dependent, e.g. /proc/sys/net/ipv4/tcp_rmem on Linux.
    */
   ioctl_setRxBufSize       = 4,
   /**
    * \brief Gets the SO_RCVBUF set via ioctl_setRxBufSize
    *
    * \param (void *)val - Pointer to integer to receive SO_RCVBUF size.
    */
   ioctl_getRxBufSize       = 5,
   /**
    * \brief Sets the max queue size on outbound channel to yamRecorder 
    * depending on channel type as follows:
    *
    * Type | BufSiz
    * --- | ---
    * TCP | Internal Queue
    * UDP | SO_SNDBUF
    *
    * Default is 10 MB.
    * \param (void *)val - Pointer to integer containing configured size.
    */
   ioctl_setTxBufSize       = 6,
   /**
    * \brief Gets current configured max outbound channel queue size.
    *
    * This must be called AFTER you all yamr_Init()
    *
    * \param (void *)val - Pointer to integer to receive max queue size.
    */
   ioctl_getTxBufSize       = 7,
   /**
    * \brief Gets current outbound channel queue size.
    *
    * \param (void *)val - Pointer to integer to receive current queue size.
    */
   ioctl_getTxQueueSize     = 8,
   /**
    *  \brief Ties this thread to specific CPU core
    *
    * \param (void *)val - Pointer to integer containing CPU to tie to.
    */
   ioctl_setThreadProcessor = 9,
   /**
    * \brief Gets the CPU core we tied to via ioctl_setThreadProcessor
    *
    * \param (void *)val - Pointer to integer to receive CPU we are tied to.
    */
   ioctl_getThreadProcessor = 10,
   /**
    * \brief Retrieves the thread ID of the libyamr thread associated
    * with the yamr_Context
    *
    * \param (void *)val - Pointer to u_int64_t
    */
   ioctl_getThreadId        = 11,
   /**
    * \brief Allow / Disallow the library timer to call out to your
    * application every second or so to yamrAttr::_idleCbk
    *
    * This is useful for performing tasks in the libyamr thread periodically.
    *
    * \see yamrAttr::_idleCbk
    * \param (void *)val - 1 to fire idle timer; 0 to not fire.  Default is 0.
    */
   ioctl_setIdleTimer       = 12,
   /**
    * \brief Set Hi / Lo Queue Notification Bands (in Percent)
    *
    * You get notified in your yamrAttr::_stsCbk when outbound queue 
    * crosses the following thresholds:
    * Queue State | Threshold | Notification
    * --- | --- | ---
    * Empty | 100 - ioctl_QlimitHiLoBand  | yamr_QhiMark
    * Full| ioctl_QlimitHiLoBand  | yamr_QloMark
    *
    * So if full, you get notified the 1st time it crosses the hi mark.
    * If the queue 'sawtooths' above and below hi mark, you only get 
    * notified once.  Same logic if the queue 'sawtooths' around lo mark.
    *
    * \see yamrAttr::_stsCbk
    * \param (void *)val - Percentage Bands in range ( 5 to 45 )
    */
   ioctl_QlimitHiLoBand     = 13,
   /**
    * \brief Retrieves connection type : TCP (1) or UDP (2)
    *
    * \param (void *)val - Pointer to int
    */
   ioctl_getSocketType      = 14
} yamrIoctl;

/**
 * \struct yamrBuf
 * \brief A 64-bit memory buffer - Not allocated
 */
typedef struct {
   /** \brief The data */
   char     *_data;
   /** \brief Data length */
   u_int64_t _dLen;
   /** \brief Opaque Value Used by Library : DO NOT MODIFY */
   void     *_opaque;
} yamrBuf;

/**
 * \struct yamrMsg
 * \brief A single unstructured message from the tape
 */
typedef struct {
   /** \brief Time in Nanos msg was written to tape */
   u_int64_t _Timestamp;
   /** \brief IP Address of Sending Client */
   u_int32_t _Host;
   /** \brief Client Session ID */
   u_int16_t _SessionID;
   /** \brief Data Type */
   u_int16_t _DataType;
   /** \brief Message Protocol */
   u_int16_t _MsgProtocol;
   /** \brief Wire Protocol (e.g., StrinMap is IntList on Wire) */
   u_int16_t _WireProtocol;
   /** \brief Message Sequence Number */
   u_int64_t _SeqNum;
   /** \brief Unstructured Data; Volatile so deep copy if you need it */
   yamrBuf   _Data;
} yamrMsg;

/**
 * \enum yamrState
 * \brief The state - UP or DOWN - of the channel
 */
typedef enum {
   /** \brief The Channel is UP */
   yamr_up    = 0,
   /** \brief The Channel is DOWN */
   yamr_down  = 1
} yamrState;

/**
 * \enum yamrStatus
 * \brief Channel status - Queue full, etc
 */
typedef enum {
   /** \brief Channel queue < Lo-Water Mark (20%) */
   yamr_QloMark = 0,
   /** \brief Channel queue > Hi-Water Mark (80%) */
   yamr_QhiMark = 1
} yamrStatus;

/**
 * \struct yamrChanStats
 * \brief Run-time channel statistics returned from yamr_GetStats().  
 */
typedef struct {
   /** \brief Num msgs sent to yamRecorder */
   u_int64_t _nMsg;
   /** \brief Num bytes sent to yamRecorder */
   u_int64_t _nByte;
   /** \brief Unix time in nanos of last msg sent to yamRecorder */
   u_int64_t _lastMsg;
   /** \brief Unix time in nanos of last heartbeat read from yamRecorder */
   u_int64_t _lastHB;
   /** \brief Current outbound queue size */
   int       _qSiz;
   /** \brief Max outbound queue size */
   int       _qSizMax;
   /** \brief Unix time in nanos of last connection */
   u_int64_t _lastConn;
   /** \brief Unix time in nanos of last disconnect */
   u_int64_t _lastDisco;
   /** \brief Total number of connections since startup */
   int    _nConn;
   /** \brief Reserved for future use */
   int    _iVal[20];
   /** \brief Destination connection as \<host\>:\<port\> */
   char   _dstConn[128];
   /** \brief 1 if channel is connected; 0 if not */
   char   _bUp;
} yamrChanStats;


/*************
 * API Calls *
 *************/
/**
 * \typedef yamrConnFcn
 * \brief Connection callback definition
 *
 * This is called when channel connects / disconnects or when session
 * is accepted / rejected.
 *
 * \see yamrAttr::_connCbk
 * \param cxt - yamRecorder channel context
 * \param msg - Textual description of event
 * \param state - yamrState : yamr_up if connected
 */
typedef void (YAMRAPI *yamrConnFcn)( yamr_Context cxt, 
                                     const char  *msg, 
                                     yamrState    state );

/**
 * \typedef yamrStatusFcn
 * \brief Channel status callback definition
 *
 * This is called when channel status changes : Queue full, etc.
 *
 * \see yamrAttr::_stsCbk
 * \param cxt - yamRecorder channel context
 * \param sts - yamrStatus
 */
typedef void (YAMRAPI *yamrStatusFcn)( yamr_Context cxt, yamrStatus sts );

/**
 * \typedef yamrIdleFcn
 * \brief Idle callback definition
 *
 * This is called on your channel every second or so when the library is idle
 *
 * \param cxt - yamRecorder subscription channel context
 */
typedef void (YAMRAPI *yamrIdleFcn)( yamr_Context cxt );

/**
 * \typedef yamrThreadFcn
 * \brief Worker thread callback definition
 *
 * This is called repeatedly after you call OS_StartThread() up until you
 * call OS_StopThread()
 *
 * \param arg - User supplied argument
 * \see OS_StartThread()
 * \see OS_StopThread()
 */
typedef void (YAMRAPI *yamrThreadFcn)( void *arg );

/**
 * \struct yamrAttr
 * \brief Channel configuration passed to yamr_Initialize()
 */
typedef struct {
   /** 
    * \brief yamRecorder servers as host1:port1,host2:port2,...
    *
    * libyamr support both connected (TCP) and connectionless (UDP) access
    * to the yamRecorder:
    * pSvrHosts | Type
    * --- | ---
    * host1:port,... | TCP
    * tcp:host1:port,... | TCP
    * udp:host1:port | UDP multicast (one destination only)
    */
   const char   *_pSvrHosts;
   /** \brief Session or App ID : Must be unique on client box */
   int           _SessionID;
   /** \brief Callback when yamRecorder channel connects or disconnects */
   yamrConnFcn   _connCbk;
   /** \brief Callback when yamRecorder channel status changes */
   yamrStatusFcn _stsCbk;
   /** \brief Callback to receive real-time market data updates */
   yamrIdleFcn   _idleCbk;
} yamrAttr;


/************************
 * Client Channel Stuff *
 ***********************/
/**
 * \brief Initialize the yamRecorder channel connection
 *
 * This initializes the channel to the yamRecorder.  You connect by calling 
 * yamr_Start().
 *
 * \param attr - yamrAttr configuration
 * \return Initialized context for yamr_Start(), yamr_Destroy(), etc.
 */
yamr_Context yamr_Initialize( yamrAttr attr );

/**
 * \brief Connect channel to the yamRecorder server.
 *
 * \param cxt - Context from yamr_Initialize()
 * \return Description of connection
 */
const char *yamr_Start( yamr_Context cxt );

/**
 * \brief Destroy channel connection to yamRecorder server.
 *
 * \param cxt - Channel Context from yamr_Initialize()
 */
void yamr_Destroy( yamr_Context cxt );

/**
 * \brief Configures the yamRecorder channel
 *
 * See yamrIoctl for list of command and values
 *
 * \param cxt - Channel Context from yamr_Initialize()
 * \param cmd - Command from yamrIoctl
 * \param val - Command value
 */
void yamr_ioctl( yamr_Context cxt, yamrIoctl cmd, void *val );

/**
 * \brief Set run-time stats from the yamRecorder channel
 *
 * \param cxt - Context from yamr_Initialize()
 * \param st - Pointer to yamrChanStats to set
 * \return 1 if successful; 0 otherwise
 */
char yamr_SetStats( yamr_Context cxt, yamrChanStats *st );

/**
 * \brief Return library build description
 *
 * \return Library build description
 */
const char *yamr_Version( void );

/**
 * \brief Send data message to yamRecorder
 *
 * \param cxt - Channel Context from yamr_Initialize()
 * \param data - Buffer containing data to sent
 * \param WireProtocol - Wire Protocol : < YAMR_PROTO_MAX
 * \param MsgProtocol - Message Protocol : < YAMR_PROTO_MAX
 * \return 1 if message consumed; 0 if not
 */
char yamr_Send( yamr_Context cxt, 
                yamrBuf      data, 
                u_int16_t    WireProtocol,
                u_int16_t    MsgProtocol );


/************************
 * Tape Reader Stuff    *
 ***********************/
/**
 * \brief Open and map existing tape from yamRecorder
 *
 * \param tapeName - Tape filename to read
 * \return Initialized context for tape reading
 *
 * \see yamrTape_Rewind()
 * \see yamrTape_RewindTo()
 * \see yamrTape_Read()
 * \see yamrTape_Close()
 */
yamrTape_Context yamrTape_Open( const char *tapeName );

/**
 * \brief Close tape that you opened from yamrTape_Open()
 *
 * \param cxt - Channel Context from yamrTape_Open()
 */
void yamrTape_Close( yamrTape_Context cxt );

/**
 * \brief Rewind tape to beginning that you opened yamrTape_Open()
 *
 * \param cxt - Context from yamrTape_Open()
 * \return Unix Time in Nanos of next message; 0 if empty tape
 */
u_int64_t yamrTape_Rewind( yamrTape_Context cxt );

/**
 * \brief Rewind tape to specific location that you opened yamrTape_Open()
 *
 * \param cxt - Context from yamrTape_Open()
 * \param pos - Rewind position in Nanos since epoch
 * \return Unix Time in Nanos of next message; 0 if empty tape
 */
u_int64_t yamrTape_RewindTo( yamrTape_Context cxt, u_int64_t pos );

/**
 * \brief Send data message to yamRecorder
 *
 * \param cxt - Channel Context from yamrTape_Open()
 * \param msg - Buffer containing message read from tape
 * \return 1 if message read; 0 if end of tape
 */
char yamrTape_Read( yamrTape_Context cxt, yamrMsg *msg );


/**********************
 * Library Utilities  *
 *********************/

/**
 * \brief Sets the library debug level; Initiates logging
 *
 * \param pLog - Log filename
 * \param dbgLvl - Debug verbosity level
 */
void yamr_Log( const char *pLog, int dbgLvl );

/**
 * \brief Returns Unix system time in nanos
 *
 * \return Unix system time in nanos
 */
u_int64_t yamr_TimeNs( void );

/**
 * \brief Returns Unix system time
 *
 * \return Unix system time
 */ 
time_t yamr_TimeSec( void );

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
u_int64_t yamr_TimeFromString( const char *pTime );

/**
 * \brief Returns time formatted as ODBC date/time string
 * YYYY-MM-DD HH:MM:SS.mmm
 *
 * \param outbuf - Output buffer to hold formatted result string
 * \param tNano - Time in nanos; Set to 0 to return current time
 * \return Current time formatted as ODBC date/time string
 */ 
char *yamr_pDateTimeMs( char *outbuf, u_int64_t tNano );

/**
 * \brief Returns time as HH:MM:SS.mmm
 *
 * \param outbuf - Output buffer to hold formatted result string
 * \param tNano - Time in nanos; Set to 0 to return current time
 * \return Current time as HH:MM:SS.mmm
 */
char *yamr_pTimeMs( char *outbuf, u_int64_t tNano );

/**
 * \brief Sleeps for requested period of time.
 *
 * \param dSlp - Number of \<seconds\>.\<microseconds\> to sleep
 */
void yamr_Sleep( double dSlp );

/**
 * \brief Hex dump a message into an output buffer
 *
 * \param msg - Message to dump
 * \param len - Message length
 * \param outbuf - Output buffer to hold hex dump
 * \return Length of hex dump in outbuf
 */
int yamr_hexMsg( char *msg, int len, char *outbuf );

/**
 * \brief Returns total CPU usage for this process
 *
 * \return Total CPU usage for this process
 */
double yamr_CPU( void );

/**
 * \brief Returns total memory size of this process
 *
 * \return Total memoroy size for this process
 */
int yamr_MemSize( void );

/**
 * \brief Creates a memory-mapped view of the file
 *
 * Call yamr_UnmapFile() to unmap the view.
 *
 * \param pFile - Name of file to map
 * \param bCopy - 1 to make deep copy; 0 for view only
 * \return Mapped view of a file in an rtBUF struct
 */
yamrBuf yamr_MapFile( char *pFile, char bCopy );

/**
 * \brief Re-maps view of file previously mapped via yamr_MapFile()
 *
 * This works as follows:
 * + Current view passed in as 1st argument must be a view, not copy
 * + File is re-mapped if the file size has grown
 * + If file size has not changed, then view is returned
 *
 * \param view - Current view of file from yamr_MapFile()
 * \return Mapped view of a file in an rtBUF struct
 * \see yamr_MapFile()
 */
yamrBuf yamr_RemapFile( yamrBuf view );

/**
 * \brief Unmaps a memory-mapped view of a file
 *
 * \param bMap - Memory-mapped view of file from yamr_MapFile()
 */
void yamr_UnmapFile( yamrBuf bMap );

/**
 * \brief No-Op
 */
void yamr_breakpoint();

/* Linux Compatibility */

#ifdef WIN32
/**
 * \brief Thread-safe localtime() compatibile with Linux
 *
 * \param tm - Pointer to Unix time (sec since Jan 1, 1970)
 * \param outBuf - Output buffer to receive deep copy of localtime struct
 * \return pointer to localtime struct
 */
struct tm *localtime_r( const time_t *tm, struct tm *outBuf );

/**
 * \brief Thread-safe strtok() compatibile with Linux
 *
 * \param str - String to tokenize
 * \param delim - Set of characters to delimit the tokens in str
 * \param noUsed - Not used; For compatibility with Linux
 * \return pointer to next token or NULL if there are no more tokens
 */
char *strtok_r( char *str, const char *delim, char **notUsed );

#endif // WIN32

_CPP_END
#ifdef __cplusplus
#include <string>
#include <hpp/Mutex.hpp>
#include <hpp/Reader.hpp>
#include <hpp/Writer.hpp>
#include <hpp/data/Data.hpp>
#include <hpp/data/Listener.hpp>
#endif /* __cplusplus */

#endif // __LIB_YAMR_H
