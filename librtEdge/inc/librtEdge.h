/******************************************************************************
*
*  librtEdge.h
*     'C' interface library to rtEdgeCache
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     . . .
*     18 NOV 2014 jcs  Build 28: rtFld_bytestream; rtEdge_MapFile(); inc/hpp
*      6 JAN 2015 jcs  Build 29: ByteStream / Chain; rtEdgeData._StreamID
*     20 MAR 2015 jcs  Build 30: ioctl_setHeartbeat
*      6 JUL 2015 jcs  Build 31: ioctl_getFd; ioctl_setPubDataPayload
*     15 APR 2016 jcs  Build 32: ioctl_isSnapChan; ioctl_setUserPubMsgTy
*     12 SEP 2016 jcs  Build 33: ioctl_userDefStreamID
*     26 MAY 2017 jcs  Build 34: rtEdgePubAttr._bConnectionless
*     24 SEP 2017 jcs  Build 35: Cockpit; No mo LVC_XxxTicker()
*     12 OCT 2017 jcs  Build 36: rtBuf64
*      7 NOV 2017 jcs  Build 38: rtEdge_RemapFile()
*     21 JAN 2018 jcs  Build 39: CockpitAttr._cxtLVC
*      6 MAR 2018 jcs  Build 40: OS_StartThread / OS_StopThread()
*      6 DEC 2018 jcs  Build 41: VOID_PTR
*      7 SEP 2020 jcs  Build 44: ioctl_normalTapeDir; ioctl_xxxThreadName
*     16 SEP 2020 jcs  Build 45: rtEdge_Parse()
*     22 OCT 2020 jcs  Build 46: rtEdge_StartPumpFullTape()
*      3 DEC 2020 jcs  Build 47: rtEdge_Data._TapePos
*      6 OCT 2021 jcs  Build 50: doxygen de-lint
*      7 MAR 2022 jcs  Build 51: doxygen
*     29 MAR 2022 jcs  Build 52: mddIoctl_unpacked
*      3 MAY 2022 jcs  Build 53: DataDog.hpp
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/

/**
 * \mainpage librtEdge API Reference Manual
 *
 * librtEdge allows you to consume and publish real-time and historical data 
 * from the MD-Direct platform via C or C++ through the following components:
 * Function | Component | Description
 * ---- | --- | ----
 * Subscribe | rtEdgeCache3 | Consume real-time data
 * Publish | rtEdgeCache3 | Publish real-time data
 * Tick-by-Tick | gateaRecorder | Pull out every recorded message for a day
 * Bulk Snapshot | LVC | Bulk snapshot (5000 tickers in under 1 millisecond)
 * Chartable Time-Series | ChartDb | Snap chartable time-series historical data
 */

#ifndef __LIB_RTEDGE_H
#define __LIB_RTEDGE_H
#ifdef __cplusplus
#define _CPP_BEGIN extern "C" {
#define _CPP_END   }
#else
#define _CPP_BEGIN
#define _CPP_END
#endif /* __cplusplus */

_CPP_BEGIN
#include <libmddWire.h>
_CPP_END
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

/* Platform dependent : COM / .NET is __stdcall */

/**
 * \brief Handle different callback calling conventions on all platforms
 *
 * On Windows, COM / .NET is __stdcall
 */

#ifdef WIN32
#define EDGAPI __stdcall
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
#define EDGAPI
#endif /* WIN32 */

#define VOID_PTR void *)(size_t

_CPP_BEGIN

/* Data Structures */

/** \brief The context number of an initialized pub/sub rtEdgeCache3 channel */
typedef long rtEdge_Context;
/** \brief The context number of an opened view to a Last Value Cache */
typedef long LVC_Context;
/** \brief The context number of an opened view to a ChartDB process */
typedef long CDB_Context;
/** \brief The context number of an initialized Cockpit chanel */
typedef long Cockpit_Context;
/** \brief The context number of an initialized OS Thread */
typedef long Thread_Context;

/************************
 * libmddWire.h
 ************************/

/**
 * \enum rtEdgeIoctl
 * \brief Wire protocol control commands passed to rtEdge_ioctl()
 */
typedef enum {
   /**
    * \brief Publish mddProto_Binary FieldLists as unpacked values
    *
    * + Only used for Binary Protocol
    * + Only used for publishing
    * + Subscription
    * + Default is 0
    * \param (void *)val - 1 for unpacked; 0 for packed; Default = 0
    */
   ioctl_unpacked           = mddIoctl_unpacked,
   /**
    * \brief Convert string fields to native fields based on schema.
    *
    * \param (void *)val - 1 to convert; 0 to not convert; Default = 0
    */
   ioctl_nativeField        = mddIoctl_nativeField,
   /**
    * \brief Populate rtEdgeData::_svc and rtEdgeData::_tkr
    *
    * Strings are expensive to push into .NET.  If you can determine 
    * the ticker based on rtEdgeData::_arg which you passed in via
    * rtEdge_Subscribe(), then best performance is achieved by setting 
    * this to 0.
    *
    * \param (void *)val - 1 to populate; 0 to leave empty; Default = 1
    */
   ioctl_svcTkrName         = 2,
   /**
    * \brief Populate rtEdgeData::_rawData
    *  
    * Strings are expensive to push into .NET.  If you have the library
    * return a parsed field list via ioctl_parse and better yet have
    * the library return a parsed field list containing native fields 
    * via ioctl_parse and ioctl_nativeField, then you do not need the 
    * raw message returned in rtEdgeData::_rawData.  If this is the case,
    * then for best performance set this to 0.
    *
    * \param (void *)val - 1 to populate; 0 to leave empty; Default = 1
    */
   ioctl_rawData            = 3,
   /**
    * \brief Retrieve run-time stats.
    *
    * The 3rd argument to rtEdge_ioctl() is the address of a rtEdgeChanStats
    * pointer which points to the real-time stats for the channel.  In 
    * other words,
    *    \code{.cpp}
    *    rtEdgeChanStats *st;
    *
    *    ::rtEdge_ioctl( cxt, ioctl_getStats, (void *)&st );
    *    printf( "%ld msgs processed\n", st->_nMsg );
    *    \endcode
    *
    * \param (void *)val - Address of rtEdgeChanStats pointer
    */
   ioctl_getStats           = 4,
   /**
    * \brief Configure the channel to be binary
    *
    * This must be called before rtEdge_Start() or rtEdge_PubStart()
    *
    * \param (void *)val - 1 for binary; 0 for ASCII field list; Default = 0
    */
   ioctl_binary             = 5,
   /**
    * \brief Define the name of the loadable library to use to
    * convert fixed-sized messages to an mddFieldList.
    *
    * + Default is 0 (no loadable driver)
    * + Publisher and subscriber must agree on the fixed-size data format
    * + The fixed-size messages are passed through unmodified through the
    * MD-Direct infrastructure from publisher to subscriber.
    *
    * The library must export 2 functions:
    * + mddDriverFcn named "ConvertFixedToFieldList"
    * + mddDvrVerFcn named "Version"
    *
    * \param (void *)val - String = Name of library to load 
    */
   ioctl_fixedLibrary       = mddIoctl_fixedLibrary,
   /**
    * \brief Used for debugging to determine read / write latency
    *
    * \param (void *)val - 1 to enable; 0 to disable; Default = 0
    */
   ioctl_measureLatency     = 7,
   /**
    * \brief Allow / Disallow cache-ing:
    * + Subscriber channel : Maintain cache in librtEdge
    * + Publisher channel : Maintain cache in rtEdgeCache3
    * 
    * For subscriber channels, this must be called AFTER rtEdge_Initialize() 
    * and BEFORE rtEdge_Start().  For publisher channels, this must be 
    * called AFTER rtEdge_PubInit() and BEFORE rtEdge_PubStart().
    *
    * \param (void *)val - 1 to enable; 0 to disable; Default = 0 for
    * subscribers and 1 for publishers (cache in rtEdgeCache3)
    */
   ioctl_enableCache        = 8,
   /**
    * \brief Retrieve mddProtocol for channel
    *
    * \param (void *)val - Pointer to mddProtocol to receive protocol
    */
   ioctl_getProtocol        = 9,
   /**
    * \brief Randomize rtEdgeAttr._pSvrHosts / rtEdgePubAttr._pSvrHosts 
    * when connecting; This allows you to configure a bunch of subscriber 
    * and/or publisher applications with a common configuration file, yet 
    * have the load evenly (stochastically) distributed across available
    * Edge2 servers.
    *
    * \param (void *)val - 1 to enable; 0 to disable; Default = 0
    */
   ioctl_randomize          = 10,
   /**
    * \brief Sets the channel heartbeat for publication channel created
    * via rtEdge_PubInit().  Argument is number of seconds.  Default is 3600.
    *
    * \param (void *)val - integer heartbeat
    */
   ioctl_setHeartbeat       = 11,
   /**
    * \brief Retrieves socket file descriptor
    *
    * \param (void *)val - Pointer to integer to receive fd 
    */
   ioctl_getFd              = 12,
   /**
    * \brief Sets the SO_RCVBUF.
    *
    * This must be called AFTER you all rtEdge_Start() / rtEdge_PubStart().
    *
    * \param (void *)val - Pointer to integer.  Default is system
    * dependent, e.g. /proc/sys/net/ipv4/tcp_rmem on Linux.
    */
   ioctl_setRxBufSize       = 13,
   /**
    * \brief Gets the SO_RCVBUF set via ioctl_setRxBufSize
    *
    * \param (void *)val - Pointer to integer to receive SO_RCVBUF size.
    */
   ioctl_getRxBufSize       = 14,
   /**
    * \brief Sets the max queue size on outbound channel to Edge2.
    * Default is 10 MB.
    *
    * \param (void *)val - Pointer to integer containing configured size.
    */
   ioctl_setTxBufSize       = 15,
   /**
    * \brief Gets current outbound channel queue size.
    *
    * This must be called AFTER you all rtEdge_Init() / rtEdge_PubInit().
    *
    * \param (void *)val - Pointer to integer to receive current queue size.
    */
   ioctl_getTxBufSize       = 16,
   /**
    * \brief Sets the minimum hop count for the publication channel 
    * created by rtEdge_PubInit().  Default is 0
    *
    * This must be called before you all rtEdge_PubInit().
    *
    * \param (void *)val - integer min hop count; Default is 0
    */
   ioctl_setPubHopCount     = 17,
   /**
    * \brief Gets the minimum hop count set by ioctl_setPubHopCount.
    *
    * \param (void *)val - Pointer to integer to receive current hop count.
    */
   ioctl_getPubHopCount     = 18,
   /**
    *  \brief Ties this thread to specific CPU core
    *
    * \param (void *)val - Pointer to integer containing CPU to tie to.
    */
   ioctl_setThreadProcessor = 19,
   /**
    * \brief Gets the CPU core we tied to via ioctl_setThreadProcessor
    *
    * \param (void *)val - Pointer to integer to receive CPU we are tied to.
    */
   ioctl_getThreadProcessor = 20,
   /**
    * \brief Ties this thread / channel to specific CPU core
    *
    * \param (void *)val - Pointer to integer containing CPU core to tie to.
    * Default is 0 - Don't tie to any CPU.
    */
   ioctl_setThreadName      = 21,
   /**
    * \brief Gets the CPU core we tied to via ioctl_setThreadProcessor
    *
    * \param (void *)val - Pointer to integer to receive CPU we are tied to.
    */
   ioctl_getThreadName      = 22,
   /**
    * \brief Loads pre-built data payload to be published via next call
    * to rtEdge_Publish().
    *
    * If your application has a pre-built data payload (e.g., Pipe-like
    * application where input and output channels are same protocol), 
    * this helps improve performance as it saves the library from 
    * building the payload.
    *
    * NOTE: There is one pre-built publication buffer per context.  If 
    * your application publishes from 2 or more threads into the same channel,
    * it is possible that calling ioctl_setPubDataPayload to set the 
    * pre-built payload, then immediately calling rtEdge_Publish() may 
    * NOT use the pre-built payload.  Therefore, YOU MUST SYNCHRONIZE 
    * ACCESS TO THIS PRE-BUILT BUFFER if you are publishing from 2+
    * threads to the same publication channel.
    *
    * \param (void *)val - Pointer to pre-build payload in rtPreBuiltBUF
    */
   ioctl_setPubDataPayload  = 23,
   /**
    * \brief Gets view on current contents of input buffer for debugging.
    *
    * \param (void *)val - Pointer to rtBUF to view input buffer
    */
   ioctl_getInputBuffer     = 24,
   /**
    * \brief Retrieves bool describing if channel is snapshot (true) or 
    * streaming (false)
    *
    * NOTE: You do not specifically define a subscription channel that you 
    * create via rtEdge_Initialize() to be snapshot or streaming.  Rather, 
    * this is defined on the rtEdgeCache3 server.  There is a streaming 
    * port and (optional) snapshot port.  After connecting, you may query 
    * via ioctl_isSnapChan to determine if the channel is snapshot or
    * streaming.
    *
    * \param (void *)val - Pointer to bool; true if snap; false if streaming
    */
   ioctl_isSnapChan         = 25,
   /**
    * \brief Allow / Disallow strict interpretation of publisher message 
    * type in rtEdge_Publish():
    * + true : Use rtEdgeData::_ty supplied by user in call to rtEdge_Publish()
    * + false : Determine if published message type should be edg_image or
    * edg_update based on what has been published for the stream.
    *
    * If false, the library allows you to publish edg_image after edg_image,
    * but puts the 2nd+ messages on the wire as edg_images.  If true, every
    * message goes onto the wire as you specify in the call to rtEdge_Publish()
    *
    * \param (void *)val - 1 to use user-supplied message type; 0 to have 
    * library determine edg_image or edg_update.  Default is 0.
    */
   ioctl_setUserPubMsgTy    = 26,
   /**
    * \brief Allow / Disallow the library timer to call out to your
    * application every second or so via the following callbacks:
    *
    * Channel | Callback | Identifier
    * ---- | ---- | ----
    * Subscription | rtEdgeAttr::_dataCbk | rtEdgeData::_ty == 0
    * Publication | rtEdgePubAttr::_openCbk | tkr == NULL
    *
    * This is useful for performing tasks in the librtEdge thread 
    * periodically, for example if you need to ensure that you call
    * rtEdge_Subscribe() ONLY from the subscription channel thread or 
    * handle publication overflow events ONLY from the publication channel
    * thread when it is dormant (not dispatching events).
    *
    * When you enable the idle timer, your standard callback handler is 
    * called as described above:
    * + Subscription Channel : rtEdgeAttr::_dataCbk
    * + Publication Channel : rtEdgePubAttr::_openCbk
    *
    * The callback contains an indicator telling you it's idle timer:
    * + Subscription Channel : rtEdgeData::_ty == 0
    * + Publication Channel : tkr (1st argument) == NULL
    *
    * \see rtEdgeAttr::_dataCbk
    * \see rtEdgePubAttr::_openCbk
    * \param (void *)val - 1 to fire idle timer; 0 to not fire.  Default is 0.
    */
   ioctl_setIdleTimer       = 27,
   /**
    * \brief Defines the publication channel authentication string required 
    * from rtEdgeCache3.
    *
    * rtEdgeCache3 Build 21 and later sends an authentication string which 
    * the library may match against via ioctl_getPubAuthToken.  This
    * ensures the publisher is sending data into a controlled appliance
    * (i.e., rtEdgeCache3 at a client-site configured from a centrally-
    * controlled d/b.)
    *
    * \param (void *)val - String = Authentication token
    */
   ioctl_setPubAuthToken    = 28,
   /**
    * \brief Retrieves the authentication token that was sent by the 
    * rtEdgeCache3 configured as a controlled device to this publication 
    * channel.
    *
    * You compare this against the value you set in ioctl_setPubAuthToken 
    * to determine if you are authenticated to publish into the rtEdgeCache3
    * configured as a controlled devies.
    *
    * \param (void *)val - String = Authentication string
    */
   ioctl_getPubAuthToken    = 29,
   /**
    * \brief Enable / Disable supplying perms for this publication channel
    * set via rtEdge_PubInit().
    *
    * This must be called BEFORE calling rtEdge_PubStart()
    *
    * \param (void *)val - 1 to enable; 0 to disable; Default = 0
    */
   ioctl_enablePerm         = 30,
   /**
    * \brief Retrieves the thread ID of the librtEdge thread associated
    * with the rtEdge_Context
    *
    * \param (void *)val - Pointer to u_int64_t
    */
   ioctl_getThreadId        = 31,
   /**
    * \brief Enable / Disable user-defined StreamID; If enabled, the 
    * user-defined argument passed to rtEdge_Subscribe() is interpreted
    * as an integer and passed to the rtEdgeCache3 as the Stream ID.
    *
    * \param (void *)val - 1 to enable; 0 to disable; Default = 0
    */
   ioctl_userDefStreamID    = 32,
   /**
    * \brief Sets the direction messages - tape (reverse) or chronological -
    * when pumping from tape.
    *
    * Messages are stored on the tape in reverse order as each messsage has
    * a backward-, not forward-pointer.  As such, when Subscribing from tape,
    * the messages are read from tape in reverse order.
    *
    * \param (void *)val - 1 to pump in tape (reverse) order; 0 in chronological; Default is 1
    */
   ioctl_tapeDirection     = 33
} rtEdgeIoctl;

/**
 * \enum rtFldType
 * \brief rtFIELD type
 */
typedef enum {
   /** \brief Undefined field type */
   rtFld_undef      = mddFld_undef,
   /** \brief String field; Value in rtVALUE::_buf */
   rtFld_string     = mddFld_string,
   /** \brief 32-bit int field; Value in rtVALUE::_i32 */
   rtFld_int        = mddFld_int32,
   /** \brief double field; Value in rtVALUE::_r64 */
   rtFld_double     = mddFld_double,
   /** \brief Date field; Value in rtVALUE::_r64 */
   rtFld_date       = mddFld_date,
   /** \brief Time field; Value in rtVALUE::_r64 */
   rtFld_time       = mddFld_time,
   /** \brief Time-seconds field; Value in rtVALUE::_r64 */
   rtFld_timeSec    = mddFld_timeSec,
   /** \brief float field; Value in rtVALUE::_r32 */
   rtFld_float      = mddFld_float,
   /** \brief 8-bit int field; Value in rtVALUE::_i8 */
   rtFld_int8       = mddFld_int8,
   /** \brief 16-bit int field; Value in rtVALUE::_i16 */
   rtFld_int16      = mddFld_int16,
   /** \brief 64-bit int field; Value in rtVALUE::_i64 */
   rtFld_int64      = mddFld_int64,
   /** \brief Real field; Value in rtVALUE::_real */
   rtFld_real       = mddFld_real,
   /** \brief Bytestream field; Value in rtVALUE::_buf */
   rtFld_bytestream = mddFld_bytestream
} rtFldType;

/**
 * \typedef rtBUF
 * \brief A memory buffer - Not allocated
 */
typedef mddBuf   rtBUF;

/**
 * \struct rtBuf64
 * \brief A 64-bit memory buffer - Not allocated
 */
typedef struct {
   /** \brief The data */
   char     *_data;
   /** \brief Data length */
   u_int64_t _dLen;
   /** \brief Opaque Value Used by Library : DO NOT MODIFY */
   void     *_opaque;
} rtBuf64;

/**
 * \struct rtPreBuiltBUF
 * \brief A memory buffer - Not allocated - containing pre-built 
 * data payload and data type.
 */
typedef struct {
   /** \brief Data Payload */
   rtBUF       _payload;
   /** \brief Data Type - mddDt_FieldList or mddDt_FixedMsg */ 
   mddDataType _dataType;
} rtPreBuiltBUF;

/**
 * \typedef rtVALUE
 * \brief Polymorphic field value
 *
 * Type defined in rtFIELD::_type
 */
typedef mddValue rtVALUE;

/**
 * \struct rtFIELD;
 * \brief A single mddWire field in a field list
 */
typedef struct {
   /** \brief Field ID */
   u_int       _fid;
   /** \brief Field value */
   rtVALUE     _val;
   /** \brief Field name (from Schema) */
   const char *_name;
   /** \brief Field type */
   rtFldType   _type;
} rtFIELD;

/**
 * \enum rtEdgeType
 * \brief Type of message in rtEdgeData
 */
typedef enum {
   /** \brief Initial Image */
   edg_image      = mddMt_image,
   /** \brief Update */
   edg_update     = mddMt_update,
   /** \brief Stream is Stale */
   edg_stale      = mddMt_stale,
   /** \brief Stream is Recovering */
   edg_recovering = mddMt_recovering,
   /** \brief Dead Stream */
   edg_dead       = mddMt_dead,
   /** \brief Perm Query */
   edg_permQuery  = mddMt_dbQry,
   /** \brief Stream Complete */
   edg_streamDone = mddMt_close
} rtEdgeType;

/**
 * \enum rtEdgeState
 * \brief The state - UP or DOWN - of a wire resource such as a Channel 
 * or Service
 */
typedef enum {
   /** \brief The wire resource (Channel or Service) is UP */
   edg_up    = mdd_up,
   /** \brief The wire resource (Channel or Service) is DOWN */
   edg_down  = mdd_down
} rtEdgeState;

/********************
 * end libmddWire.h
 *******************/

/**
 * \struct rtEdgeData
 * \brief An MD-Direct message received from or published to the rtEdgeCache3 
 * server.
 *
 * This structure is used to both consume and publish data:
 * + If consumed, this message is driven into your application in the 
 * rtEdgeAttr::_dataCbk you registered in rtEdge_Initialize().
 * + If published, you populate this struct then call rtEdge_Publish().   
 */
typedef struct {
   /** \brief Message Time */
   double      _tMsg;
   /** \brief Real-time service name */
   const char *_pSvc;
   /** \brief Ticker name */
   const char *_pTkr;
   /** \brief Error string */
   const char *_pErr;
   /** \brief User-supplied argument */
   void       *_arg;
   /** \brief Message Type */
   rtEdgeType  _ty;
   /** \brief Field list */
   rtFIELD    *_flds;
   /** \brief Number of fields */
   int         _nFld;
   /** \brief Raw message data */
   const char *_rawData;
   /** \brief Raw message data lngth */
   int         _rawLen;
   /** \brief Unique Stream ID */
   int         _StreamID;
   /** \brief Tape Offset, if from tape */
   u_int64_t   _TapePos;
} rtEdgeData;

/**
 * \struct rtEdgeRead
 * \brief x* Used by rtEdge_Read() to read conflated updates at your leisure.
 */

typedef struct {
   rtEdgeData  _d;
   rtEdgeState _state;
   char        _msg[K];
} rtEdgeRead;

/**
 * \struct rtEdgeChanStats
 * \brief Run-time channel statistics returned from rtEdge_GetStats().  
 *
 * May be used for either subscription or publication channels created by:
 * +  rtEdge_Initialize()
 * +  rtEdge_PubInit()
 */
typedef struct {
   /** \brief Num msgs read (SUBSCRIBE) or written (PUBLISH) */
   u_int64_t _nMsg;
   /** \brief Num bytes read (SUBSCRIBE) or written (PUBLISH) */
   u_int64_t _nByte;
   /** \brief Unix time of last msg read (SUBSCRIBE) or written (PUBLISH) */
   u_int  _lastMsg;
   /** \brief Microseconds of last msg read (SUBSCRIBE) or written (PUBLISH) */
   u_int  _lastMsgUs;
   /** \brief Total Num of subscription streams opened since startup  */
   int    _nOpen;
   /** \brief Total Num of subscription streams closed since startup  */
   int    _nClose;
   /** \brief Total Num of Images consumed (SUBSRIBE) or published (PUBLISH) */
   int    _nImg;
   /** \brief Total Num of Updates consumed (SUBSRIBE) or published (PUBLISH) */
   int    _nUpd;
   /** \brief Total Num of DEAD streams */
   int    _nDead;
   /** \brief Current outbound queue size */
   int    _qSiz;
   /** \brief Max outbound queue size */
   int    _qSizMax;
   /** \brief Unix time of last connection */
   int    _lastConn;
   /** \brief Unix time of last disconnect */
   int    _lastDisco;
   /** \brief Total number of connections since startup */
   int    _nConn;
   /** \brief Reserved for future use */
   u_int64_t _iVal[20];
   /** \brief Reserved for future use */
   double _dVal[20];
   /** \brief Channel name - SUBSCRIBE or PUBLISH */
   char   _chanName[MAXLEN];
   /** \brief Destination connection as \<host\>:\<port\> */
   char   _dstConn[MAXLEN];
   /** \brief 1 if channel is connected; 0 if not */
   char   _bUp;
   /** \brief Padding */
   char   _pad[7];
} rtEdgeChanStats;

/**
 * \struct OSCpuStat
 * \brief Current CPU usage for a given CPU on the system
 *
 * \see OS_GetCpuStats()
 */
typedef struct {
   /** \brief CPU Number */
   int    _CPUnum;
   /** \brief Pct CPU utilized in User mode */
   double _us;
   /** \brief Pct CPU utilized in User mode w/ low priority (nice) */
   double _ni;
   /** \brief Pct CPU utilized in System mode */
   double _sy;
   /** \brief Pct CPU utilized in Idle */
   double _id;
   /** \brief Pct CPU utilized in iowait : Time waiting for I/O to complete */
   double _wa;
   /** \brief Pct CPU utilized in irq : Time service-ing interrupts */
   double _si;
   /** \brief Pct CPU utilized in softirq : Time service-ing softirqs */
   double _st;
} OSCpuStat;

/**
 * \struct OSDiskStat
 * \brief Current usage for a given disk on the system
 *
 * \see OS_GetDiskStats()
 */
typedef struct {
   /** \brief Name of disk */
   const char *_diskName;
   /** \brief Number of reads completed */
   double      _nRd;
   /** \brief Number of reads merged (2 adjacent reads = 1 read) */
   double      _nRdM;
   /** \brief Number of sectors read */
   double      _nRdSec;
   /** \brief Milliseconds spent reading */
   double      _nRdMs;
   /** \brief Number of writes completed */
   double      _nWr;
   /** \brief Number of writes merged (2 adjacent writes = 1 write) */
   double      _nWrM;
   /** \brief Number of sectors written */
   double      _nWrSec;
   /** \brief Milliseconds spent writing */
   double      _nWrMs;
   /** \brief Number of I/O operations currently in progress */
   double      _nIo;
   /** \brief Milliseconds spent doing I/O */
   double      _nIoMs;
   /** \brief Weighted number of milliseconds spent doing I/O
    *
    * This field is incremented at each I/O start, I/O completion, 
    * I/O merge, or read of these stats by the number of I/Os in 
    * progress (field 9) times the number of milliseconds spent 
    * doing I/O since the last update of this field.  This can 
    * provide an easy measure of both I/O completion time and 
    * the backlog that may be accumulating.
    */
   double      _wIoMs;
} OSDiskStat;

/**
 * \struct OSFileStat
 * \brief Current stats for a specific file
 *
 * \see OS_GetFileStats()
 */
typedef struct {
   /** \brief Total size in bytes */
   u_int64_t _Size;
   /** \brief Time of last access */
   time_t    _tAccess;
   /** \brief Time of last modification */
   time_t    _tMod;
} OSFileStat;

/*******
 * LVC *
 *******/
/**
 * \struct LVCData
 * \brief Current (volatile) data for a single ticker from the Last Value 
 * Cache returned from LVC_View() or LVC_Snapshot() and freed via LVC_Free().
 */
typedef struct {
   /** \brief DB record Service name (e.g. BLOOMBERG or IDN_RDF) */
   const char *_pSvc;
   /** \brief DB record Ticker name (e.g., EUR CURNCY or CSCO.OQ */
   const char *_pTkr;
   /** \brief Error message */
   const char *_pErr;
   /** \brief State of DB record - edg_image, edg_stale or edg_dead */
   rtEdgeType  _ty;
   /** \brief Unix time when DB record was created */
   int         _tCreate;
   /** \brief Unix time of last update */
   int         _tUpd;
   /** \brief Microseconds of last update; Use with _tUpd */
   int         _tUpdUs;
   /** \brief DB record age - Time of last update */
   double      _dAge;
   /** \brief Unix time when DB record stream became edg_dead */
   int         _tDead;
   /** \brief Number of updates received */
   int         _nUpd;
   /** \brief List of (volatile) field data; List size = _nFld */
   rtFIELD    *_flds;
   /** \brief List size of _flds */
   int         _nFld;
   /** \brief Time required to snap all field list into _flds */
   double      _dSnap;
   /**
    * \brief 1 if shallow (volatile) copy; 0 if deep copy
    *
    * Set with LVC_SetCopyType()
    */
   char        _bShallow;
   /** \brief Raw Image of DB record if full, deep copy */
   char       *_copy;
   /** \brief 1 if binary LVC file; 0 if MF (ASCII Field List) */
   char     _bBinary;
} LVCData;

/**
 * \struct LVCDataAll
 * \brief Current (volatile) data for a ALL tickers from the Last Value
 * Cache returned from LVC_ViewAll() or LVC_SnapAll() and freed via
 * LVC_FreeAll().
 */
typedef struct {
   /** \brief List of all DB record data; List size = _nTkr */
   LVCData *_tkrs;
   /** \brief List size of _tkrs */
   int      _nTkr;
   /** \brief Time required to snap entire DB into _tkrs */
   double   _dSnap;
   /** \brief 1 if binary LVC file; 0 if MF (ASCII Field List) */
   char     _bBinary;
} LVCDataAll;


/**********************
 * Chart / Tape Query *
 **********************/
/**
 * \struct MDDRecDef
 * \brief Database record definition returned in a MDDResult.
 *
 * MD-Direct has 2 components that return a record definitions which you
 * may query via MDD_Query():
 * + ChartDB 
 * + Tape 
 *
 * Once completed, you must free the result via MDD_FreeResult()
 */
typedef struct {
   /** \brief DB record Service name (e.g. BLOOMBERG or IDN_RDF) */
   const char *_pSvc;
   /** \brief DB record Ticker name (e.g., EUR CURNCY or CSCO.OQ */
   const char *_pTkr;
   /** \brief Field ID being recorded if ChartDB (e.g., 6 = LAST; 22 = BID) */
   int         _fid;
   /** \brief ChartDB : Recording interval in secs; Tape : NumMsg */
   int         _interval;
} MDDRecDef;

/**
 * \struct MDDResult
 * \brief Database directory returned from MDD_Query()
 *
 * MD-Direct has 2 components that return a record definitions which you
 * may query via MDD_Query():
 * + ChartDB
 * + Tape
 *
 * Once completed, you must free the result via MDD_FreeResult()  
 */
typedef struct {
   /** \brief List of DB records; List size = _nRec */
   MDDRecDef *_recs;
   /** \brief List size of _recs */
   int        _nRec;
   /** \brief Time required to snap this DB directory */
   double     _dSnap;
} MDDResult;


/***********
 * ChartDB *
 ***********/
/**
 * \struct CDBData
 * \brief Current time-series data for a single ticker from ChartDB returned
 * from CDB_View() and freed via CDB_Free()
 */
typedef struct {
   /** \brief DB record Service name (e.g. BLOOMBERG or IDN_RDF) */
   const char *_pSvc;
   /** \brief DB record Ticker name (e.g., EUR CURNCY or CSCO.OQ */
   const char *_pTkr;
   /** \brief Error message */
   const char *_pErr;
   /** \brief Field ID being recorded (e.g., 6 = LAST; 22 = BID, etc. */
   int         _fid;
   /** \brief Recording interval in seconds */
   int         _interval;
   /** \brief Current write position = List Size of _flds */
   int         _curTick;
   /** \brief Max ticks per day = 86400 / _interval */
   int         _numTick;
   /** \brief Unix time when DB record was created */
   int         _tCreate;
   /** \brief Unix time of last update */
   int         _tUpd;
   /** \brief Microseconds of last update; Use with _tUpd */
   int         _tUpdUs;
   /** \brief DB record age - Time of last update */
   double      _dAge;
   /** \brief Unix time when DB record stream became edg_dead */
   int         _tDead;
   /** \brief Number of updates received */
   int         _nUpd;
   /** \brief Time series; Size = _curTick */
   float      *_flds;
   /** \brief Time required to snap time series into _flds */
   double      _dSnap;
} CDBData;


/***********
 * Cockpit *
 ***********/
/**
 * \struct Tuple
 * \brief ( Name, Value ) tuple
 */
typedef struct {
   /** \brief Tuple Name */
   const char *_name;
   /** \brief Tuple Value */
   const char *_value;
} Tuple;

/**
 * \struct TupleList
 * \brief A list of Tuple's
 */
typedef struct {
   /** \brief List of Tuple's */
   Tuple *_tuples;
   /** \brief Number of _tuples */
   int    _nTuple;
} TupleList;

/**
 * \struct CockpitData
 * \brief Message from Cockpit
 */
typedef struct _CockpitData {
   /** \brief XML Element ( Name, Data ) tuple */
   Tuple                _value;
   /** \brief XML Attributes */
   TupleList            _attrs;
   /** \brief XML Sub-Elements */
   struct _CockpitData *_elems;
   /** \brief Number of _elements */
   int                  _nElem;
} CockpitData;

/*************
 * API Calls *
 *************/
/**
 * \typedef rtEdgeConnFcn
 * \brief Connection callback definition
 *
 * This is called when channel connects / disconnects or when session
 * is accepted / rejected.
 *
 * \see rtEdgeAttr::_connCbk
 * \see rtEdgePubAttr::_connCbk
 * \param cxt - rtEdgeCache3 channel context
 * \param msg - Textual description of event
 * \param state - rtEdgeState : edg_up if connected
 */
typedef void (EDGAPI *rtEdgeConnFcn)( rtEdge_Context cxt, 
                                      const char    *msg, 
                                      rtEdgeState    state );

/**
 * \typedef rtEdgeSvcFcn
 * \brief Service callback definition
 *
 * This is called on your subscription channel when a real-time publisher 
 * goes UP or down.
 *
 * \see rtEdgeAttr::_svcCbk
 * \param cxt - rtEdgeCache3 subscription channel context
 * \param msg - Service name (e.g., BLOOMBERG or IDN_RDF)
 * \param state - rtEdgeState : edg_up if connected
 */
typedef void (EDGAPI *rtEdgeSvcFcn)( rtEdge_Context cxt,
                                     const char    *svc,
                                     rtEdgeState    state );

/**
 * \typedef rtEdgeDataFcn
 * \brief Market data message callback definition
 *
 * This is called on your subscription channel when market data arrives.
 *
 * \see rtEdgeAttr::_dataCbk
 * \see rtEdgeAttr::_schemaCbk
 * \param cxt - rtEdgeCache3 subscription channel context
 * \param data - Market data
 */
typedef void (EDGAPI *rtEdgeDataFcn)( rtEdge_Context cxt, rtEdgeData data );

/**
 * \typedef rtEdgeOpenFcn 
 * \brief Market data open request callback definition
 *
 * This is called on your (interactive) publication channel when a user 
 * requests you publish a new ticker.  This is only called if you defined
 * the publication channel as interactive by setting 
 * rtEdgePubAttr::_bInteractive = 1 when you called rtEdge_PubInit().
 *
 * The 3rd argument in this function is the unique StreamID.  You must 
 * save this and return it in rtEdgeData::_arg for every message you 
 * publish for this stream. 
 *
 * \see rtEdgeAttr::_dataCbk
 * \see rtEdgeAttr::_schemaCbk
 * \param cxt - rtEdgeCache3 publication channel context
 * \param tkr - Requested Ticker name
 * \param StreamID - Unique Stream ID.
 */
typedef void (EDGAPI *rtEdgeOpenFcn)( rtEdge_Context cxt, 
                                      const char    *tkr, 
                                      void          *StreamID );

/**
 * \typedef rtEdgeCloseFcn
 * \brief Market data close request callback definition
 *
 * This is called on your (interactive) publication channel when a user 
 * closes a previously opened stream.  This is only called if you defined
 * the publication channel as interactive by setting 
 * rtEdgePubAttr::_bInteractive = 1 when you called rtEdge_PubInit().
 *
 * \see rtEdgeAttr::_dataCbk
 * \see rtEdgeAttr::_schemaCbk
 * \param cxt - rtEdgeCache3 publication channel context
 * \param tkr - Requested Ticker name
 */
typedef void (EDGAPI *rtEdgeCloseFcn)( rtEdge_Context cxt, const char *tkr );

/**
 * \typedef rtEdgePermQryFcn
 * \brief Perm Query callback definition
 *
 * This is called on your (interactive) publication channel when the 
 * rtEdgeCache3 data distributor requires permissioning information 
 * for a given ( Service, Ticker, User, Location ) tuple.  For example, 
 * Bloomberg permissions by ( UUID, IP Address ).  You respond with
 * ... TODO ...
 *
 * \see rtEdgeAttr::_dataCbk
 * \see rtEdgeAttr::_schemaCbk
 * \param cxt - rtEdgeCache3 subscription channel context
 * \param tuple - ( Service, Ticker, User, Location ) tuple
 * \param reqID - Unique query / request ID
 */
typedef void (EDGAPI *rtEdgePermQryFcn)( rtEdge_Context cxt, 
                                         const char   **tuple,
                                         int            reqID );

/**
 * \typedef rtEdgeSymQryFcn
 * \brief Connectionless Publisher Query callback definition
 *
 * This is called on your connectionless UDP publication channel 
 * when the rtEdgeCache3 data distributor queries your connectionless 
 * publisher for the symbol list.  The library automatically responds 
 * with the Symbol List.
 *
 * \param cxt - rtEdgeCache3 subscription channel context
 * \param nSym - Number of symbols returned by librtEdge
 */
typedef void (EDGAPI *rtEdgeSymQryFcn)( rtEdge_Context cxt, 
                                        int            nSym );

/**
 * \typedef CockpitDataFcn
 * \brief Cockpit message callback definition
 *
 * This is called on your Cockpit channel when data arrives.
 *
 * \see CockpitAttr::_dataCbk
 * \param cxt - Cockpit channel context
 * \param data - Cockpit data
 */
typedef void (EDGAPI *CockpitDataFcn)( Cockpit_Context cxt, 
                                       CockpitData     data );

/**
 * \typedef rtEdgeThreadFcn
 * \brief Worker thread callback definition
 *
 * This is called repeatedly after you call OS_StartThread() up until you
 * call OS_StopThread()
 *
 * \param arg - User supplied argument
 * \see OS_StartThread()
 * \see OS_StopThread()
 */
typedef void (EDGAPI *rtEdgeThreadFcn)( void *arg );

/**
 * \struct rtEdgeAttr
 * \brief Subscription channel configuration passed to rtEdge_Initialize()
 */
typedef struct {
   /** 
     * \brief 1 to pump from file; 0 to pump from rtEdgeCache3 server
     *
     * _bTape | _pSvrHosts
     * --- | ---
     * 0 | Comma separated list of rtEdgeCache3 servers
     * 1 | Filename of gateaRecorder tape
     */
   char          _bTape;
   /** 
    * \brief Data source, based on _bTape
    *
     * _bTape | _pSvrHosts
     * --- | ---
     * 0 | Comma separated list of rtEdgeCache3 servers as host1:port1,host2:port2,...
     * 1 | Filename of gateaRecorder tape
    */
   const char   *_pSvrHosts;
   /** \brief rtEdgeCache3 Username */ 
   const char   *_pUsername;
   /** \brief Callback when subscription channel connects or disconnects */
   rtEdgeConnFcn _connCbk;
   /** \brief Callback when a service such as BLOOMBERG goes UP or DOWN */
   rtEdgeSvcFcn  _svcCbk;
   /** \brief Callback to receive real-time market data updates */
   rtEdgeDataFcn _dataCbk;
   /** \brief Callback to receive the schema (data dictionary) */
   rtEdgeDataFcn _schemaCbk;
} rtEdgeAttr;

/**
 * \struct rtEdgePubAttr
 * \brief Publication channel configuration passed to rtEdge_PubInit()
 */
typedef struct {
   /** 
    * \brief Comma-separated list of rtEdgeCache3 servers to connect this
    * subscription channel to formatted as host1:port1,host2:port2,...
    */
   const char      *_pSvrHosts;
   /** \brief Publication service name (e.g., MYSOURCE) */
   const char      *_pPubName;
   /** \brief 1 if interactive (client-driven) publisher; 0 if broadcast */
   char             _bInteractive;
   /** \brief 1 if connectionless (UDP) publisher; 0 if connected (TCP) */
   char             _bConnectionless;
   /** \brief If _bConnectionless, bind this local port; 0 = ephemeral */
   int              _udpPort;
   /** \brief Callback when subscription channel connects or disconnects */
   rtEdgeConnFcn    _connCbk;
   /** \brief Callback when user opens a data stream (e.g., MYITEM) */
   rtEdgeOpenFcn    _openCbk;
   /** \brief Callback when user closes a data stream (e.g., MYITEM) */
   rtEdgeCloseFcn   _closeCbk;
   /** \brief Callback when Edge3 queries for user permission info */
   rtEdgePermQryFcn _permQryCbk;
   /** \brief Callback when Edge3 queries Connectionless for Sym List */
   rtEdgeSymQryFcn  _symQryCbk;
   /** \brief Callback when Edge3 queries Connectionless for Refresh Image */
   rtEdgeOpenFcn    _imgQryCbk;
} rtEdgePubAttr;

/**
 * \struct CockpitAttr
 * \brief Cockpit channel configuration passed to Cockpit_Initialize()
 */
typedef struct {
   /** 
    * \brief Comma-separated list of Cockpi servers to connect this
    * channel to formatted as host1:port1,host2:port2,... 
    */
   const char    *_pSvrHosts;
   /** 
    * \brief Opened LVC from LVC_Initialize().
    *
    * The LVC is locked during Cockpit activity if the following are 
    * supplied and non-zero:
    * + CockpitAttr._cxtLVC
    * + CockpitAttr._lockWaitSec
    *
    * librtEdge will LOCK the LVC while you communicate with it via the 
    * Cockpit.  While locked, the following API's are held up:
    * + LVC_Snapshot() 
    * + LVC_View() 
    * + LVC_SnapAll() 
    * + LVC_ViewAll() 
    *
    * The LVC is UNLOCKED when either:
    * + LVC responds to Cockpit cmd - ACK or NAK
    * + CockpitAttr._lockWaitSec timer expires
    */
   LVC_Context    _cxtLVC;
   /** \brief Max time to Lock LVC */
   int            _lockWaitSec;
   /** \brief Callback when subscription channel connects or disconnects */
   rtEdgeConnFcn  _connCbk;
   /** \brief Callback to receive data from Cockpit */
   CockpitDataFcn _dataCbk;
} CockpitAttr;


/***********************
 * Initialization, etc *
 **********************/

/**
 * \brief Initialize the rtEdgeCache3 subscription channel connection
 *
 * This initializes the subscription channel to the rtEdgeCache3.  You 
 * connect by calling rtEdge_Start().
 *
 * \param attr - rtEdgeAttr configuration
 * \return Initialized context for rtEdge_Start(), rtEdge_Destroy(), etc.
 */
rtEdge_Context rtEdge_Initialize( rtEdgeAttr attr );

/**
 * \brief Connect subscription channel to the rtEdgeCache3 server.
 *
 * \param cxt - Context from rtEdge_Initialize()
 * \return Description of connection
 */
const char *rtEdge_Start( rtEdge_Context cxt );

/**
 * \brief Destroy subscription channel connection to rtEdgeCache3 server.
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 */
void rtEdge_Destroy( rtEdge_Context cxt );


/**
 * \brief Configures the publication or subscription channel
 *
 * See rtEdgeIoctl for list of command and values
 *
 * \param cxt - Channel Context from rtEdge_Initialize() or rtEdge_PubInit()
 * \param cmd - Command from rtEdgeIoctl
 * \param val - Command value
 */
void rtEdge_ioctl( rtEdge_Context cxt, rtEdgeIoctl cmd, void *val );

/**
 * \brief Set run-time stats from the publication or subscription channel
 *
 * \param cxt - Context from rtEdge_Initialize() or rtEdge_PubInit()
 * \param st - Pointer to rtEdgeChanStats to set
 * \return 1 if successful; 0 otherwise
 */
char rtEdge_SetStats( rtEdge_Context cxt, rtEdgeChanStats *st );

/**
 * \brief Return library build description
 *
 * \return Library build description
 */
const char *rtEdge_Version( void );

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
 * \param file - Run-time stats filename
 * \param exe - Application executable name
 * \param bld - Application build name
 * \return 1 if successful; 0 otherwise
 */
char rtEdge_SetMDDirectMon( rtEdge_Context cxt,
                            const char    *file, 
                            const char    *exe, 
                            const char    *bld );


/*******************************
 * Subscription - rtEdgeCache  *
 ******************************/

/**
 * \brief Open a ( Service, Ticker ) subscription stream
 *
 * You receive asynchronous market data updates in the rtEdgeAttr::_dataCbk
 * function you passed into rtEdge_Initialize().
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name
 * \param arg - User defined argument returned in rtEdgeData struct
 * \return Unique Stream ID
 */
int rtEdge_Subscribe( rtEdge_Context cxt, 
                      const char    *svc, 
                      const char    *tkr, 
                      void          *arg ); 

/**
 * \brief Retrieve schema from subscription channel
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param data - Schema, if available
 * \return 1 if found; 0 if not
 */
char    rtEdge_GetSchema( rtEdge_Context cxt, rtEdgeData *data );

/**
 * \brief Retrieve field by name from current update message.
 *
 * The library maintains a single market data message during the life 
 * of the data callback.  Recall that the data callback is the rtEdgeDataFcn 
 * that you registered via rtEdgeAttr::_dataCbk in rtEdge_Initialize().  
 *
 * Since the library must parse the field list in the market data message 
 * before calling your rtEdgeDataFcn, the library goes an extra step and 
 * stores the parsed fields on an internal map / hash table during the life 
 * of your rtEdgeDataFcn callback.  API calls such as rtEdge_GetField() and
 * rtEdge_HasField(), are exposed to allow you to use this map to quickly 
 * find any field in the field list without having to build your own map 
 * to store the fields.
 *
 * The following may only be used in your rtEdgeDataFcn callback:
 * + rtEdge_HasField()
 * + rtEdge_HasFieldByID()
 * + rtEdge_GetField()
 * + rtEdge_GetFieldByID()
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param fldName - Field name (BID, TRDPRC_1, etc.)
 * \return Field value
 */
rtFIELD rtEdge_GetField( rtEdge_Context cxt, const char *fldName );

/**
 * \brief Retrieve field by ID from current update message.
 *
 * The library maintains a single market data message during the life  
 * of the data callback.  Recall that the data callback is the rtEdgeDataFcn 
 * that you registered via rtEdgeAttr::_dataCbk in rtEdge_Initialize(). 
 *
 * Since the library must parse the field list in the market data message  
 * before calling your rtEdgeDataFcn, the library goes an extra step and
 * stores the parsed fields on an internal map / hash table during the life
 * of your rtEdgeDataFcn callback.  API calls such as rtEdge_GetField() and
 * rtEdge_HasField(), are exposed to allow you to use this map to quickly 
 * find any field in the field list without having to build your own map 
 * to store the fields.
 *
 * The following may only be used in your rtEdgeDataFcn callback:
 * + rtEdge_HasField()
 * + rtEdge_HasFieldByID()
 * + rtEdge_GetField()
 * + rtEdge_GetFieldByID()
 * 
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param fid - Field ID
 * \return Field value
 */
rtFIELD rtEdge_GetFieldByID( rtEdge_Context cxt, int fid );

/**
 * \brief Determine if current message has field by name.
 *
 * The library maintains a single market data message during the life  
 * of the data callback.  Recall that the data callback is the rtEdgeDataFcn 
 * that you registered via rtEdgeAttr::_dataCbk in rtEdge_Initialize(). 
 *
 * Since the library must parse the field list in the market data message  
 * before calling your rtEdgeDataFcn, the library goes an extra step and
 * stores the parsed fields on an internal map / hash table during the life
 * of your rtEdgeDataFcn callback.  API calls such as rtEdge_GetField() and
 * rtEdge_HasField(), are exposed to allow you to use this map to quickly 
 * find any field in the field list without having to build your own map 
 * to store the fields.
 *
 * The following may only be used in your rtEdgeDataFcn callback:
 * + rtEdge_HasField()
 * + rtEdge_HasFieldByID()
 * + rtEdge_GetField()
 * + rtEdge_GetFieldByID()
 * 
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param fldName - Field name (BID, TRDPRC_1, etc.) 
 * \return 1 if available; 0 if not
 */
char rtEdge_HasField( rtEdge_Context cxt, const char *fldName );

/**
 * \brief Determine if current message has field by ID. 
 *
 * The library maintains a single market data message during the life
 * of the data callback.  Recall that the data callback is the rtEdgeDataFcn
 * that you registered via rtEdgeAttr::_dataCbk in rtEdge_Initialize().
 *
 * Since the library must parse the field list in the market data message
 * before calling your rtEdgeDataFcn, the library goes an extra step and
 * stores the parsed fields on an internal map / hash table during the life
 * of your rtEdgeDataFcn callback.  API calls such as rtEdge_GetField() and
 * rtEdge_HasField(), are exposed to allow you to use this map to quickly
 * find any field in the field list without having to build your own map
 * to store the fields.
 *
 * The following may only be used in your rtEdgeDataFcn callback:
 * + rtEdge_HasField()
 * + rtEdge_HasFieldByID()
 * + rtEdge_GetField()
 * + rtEdge_GetFieldByID()
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param fid - Field ID
 * \return 1 if available; 0 if not
 */
char rtEdge_HasFieldByID( rtEdge_Context cxt, int fid );

/**
 * \brief Close a ( Service, Ticker ) subscription stream
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name
 * \return Unique Stream ID
 */
int rtEdge_Unsubscribe( rtEdge_Context cxt, 
                        const char    *svc, 
                        const char    *tkr );

/* Subscription Channel Cache */

/**
 * \brief Query the internal subscription channel in-memory cache for 
 * current (volatile) values by ( Service,Ticker ) stream name.
 *
 * By default, the library does not maintain an in-memory last value cache 
 * for the subscription channel.  If you wish to use rtEdge_GetCache(), you 
 * must enable cache-ing as follows:
 * + rtEdge_ioctl( cxt, ioctl_enableCache, 1 );
 * + This must be called BEFORE rtEdge_Start()
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name
 * \return Current cached values in an rtEdgeData struct
 */
rtEdgeData rtEdge_GetCache( rtEdge_Context cxt, 
                            const char    *svc, 
                            const char    *tkr );


/**
 * \brief Query the internal subscription channel in-memory cache for 
 * current (volatile) values by StreamID.
 *
 * By default, the library does not maintain an in-memory last value cache 
 * for the subscription channel.  If you wish to use rtEdge_GetCache(), you 
 * must enable cache-ing as follows:
 * + rtEdge_ioctl( cxt, ioctl_enableCache, 1 );
 * + This must be called BEFORE rtEdge_Start()
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param StreamID - Unique Stream ID returned from rtEdge_Subscribe()
 * \return Current cached values in an rtEdgeData struct
 */
rtEdgeData rtEdge_GetCacheByStreamID( rtEdge_Context cxt, int StreamID );

/* Conflation - Subscription Channel */

/**
 * \brief Enable / Disable conflation in the library.
 *
 * If enabled, you may drive conflated real-time data into your application
 * at a pace you can handle via either:
 * + rtEdge_Dispatch() - Dispatch conflated data into the rtEdgeAttr::_dataCbk
 * function you registered in rtEdge_Initialize(), or
 * + rtEdge_Read() - Read the conflated data directly when you are ready.
 *
 * Again, both API's allow you to control the pace at which you consume 
 * the market data stream.
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param enable - 1 to enable; 0 to disable
 */
void rtEdge_Conflate( rtEdge_Context cxt, char enable );

/**
 * \brief "Push" conflated updates into your application.
 *
 * This only works if conflation has been enabled via rtEdge_Conflate().
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param maxUpd - Max num updates to dispatch during dWait
 * \param dWait - Max seconds to spend dispatching maxUpd conflated updates
 * \return Number of updates dispatched into your app via rtEdgeAttr::_dataCbk
 */
int rtEdge_Dispatch( rtEdge_Context cxt, int maxUpd, double dWait );

/**
 * \brief "Pull" 1 conflated update into your application.
 *
 * This only works if conflation has been enabled via rtEdge_Conflate().
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param dWait - Max seconds to wait for a single conflated update
 * \param data - Market data if successful
 * \return Non-zero if update read; 0 if not
 */
int rtEdge_Read( rtEdge_Context cxt, double dWait, rtEdgeRead *data );


/* Subscription - Parse Only */

/**
 * \brief Parse a raw message
 *
 * You normally call rtEdge_Parse() on Tape channels where you have stored 
 * the raw buffer from your callback.  The data format is implied by the 
 * rtEdge_Context you pass in as 1st argument.
 *
 * You are responsible for passing in an rtEdgeData struct as follows:
 * Field | Value
 * --- | ---
 * _flds | Pre-allocated rtFIELD array to accept parsed results
 * _nFld | Max Number of fields available in _flds
 * _rawData | Raw data buffer
 * _rawLen | Raw data length
 * Others | Zero
 *
 * Upon return, the rtEdgeData buffer is filled as follows:
 * Field | Value
 * --- | ---
 * _flds | Parsed rtFIELD array
 * _nFld | Number of parsed rtFIELD's in _flds
 * _rawData | Unchanged
 * _rawLen | Unchanged
 * Others | Unchanged (zero) 
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param data - Data to parse; Parsed results
 * \return rtEdgeData._nFld
 */
int rtEdge_Parse( rtEdge_Context cxt, rtEdgeData *data );


/* Subscription - Tape only */

/**
 * \brief Pump All Messages from Tape starting at offset
 *
 * You receive asynchronous market data updates in the rtEdgeAttr::_dataCbk
 * function you passed into rtEdge_Initialize() and are notified of completion
 * when rtEdgeData._ty == edg_streamDone.
 *
 * To pump a 'slice', you will need to store the rtEdgeData._TapePos from the 
 * last message received in previous call to rtEdge_StartPumpFullTape(), 
 * then use this value as the off0 argument to next call to 
 * rtEdge_StartPumpFullTape().
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param off0 - Beginning offset; 0 for beginning of tape
 * \param nMsg - Number of msgs to pump; 0 for all
 * \return Unique Tape Pumping ID; Kill pump via rtEdge_StopPumpFullTape()
 * \see rtEdge_StopPumpFullTape()
 */
int rtEdge_StartPumpFullTape( rtEdge_Context cxt, u_int64_t off0, int nMsg );

/**
 * \brief Stop pumping from tape
 *
 * \param cxt - Subscription Channel Context from rtEdge_Initialize()
 * \param pumpID - Pump ID returned from rtEdge_StartPumpFullTape()
 * \return 1 if stopped; 0 if invalid Pump ID
 * \see rtEdge_StartPumpFullTape()
 */
int rtEdge_StopPumpFullTape( rtEdge_Context cxt, int pumpID );



/****************
 * Publication  *
 ***************/
/**
 * \brief Initialize the rtEdgeCache3 publication channel connection
 *
 * This initializes the subscription channel to the rtEdgeCache3.  You
 * connect by calling rtEdge_PubStart().
 *
 * \param attr - rtEdgePubAttr configuration
 * \return Initialized context for rtEdge_PubStart(), rtEdge_PubDestroy(), etc.
 */
rtEdge_Context rtEdge_PubInit( rtEdgePubAttr attr );

/**
 * \brief Request publication schema from rtEdgeCache3 server
 *
 * The schema is returned asynchronously to the callback function you pass
 * in as the 2nd argument.
 *
 * \param cxt - Publication channel context from rtEdge_PubInit()
 * \param cbk - Calllback function to receive Schema
 */
void rtEdge_PubInitSchema( rtEdge_Context cxt, rtEdgeDataFcn cbk );

/**
 * \brief Connect publication channel to the rtEdgeCache3 server.
 *
 * \param cxt - Publication channel context from rtEdge_PubInit()
 * \return Description of connection
 */
const char *rtEdge_PubStart( rtEdge_Context cxt );

/**
 * \brief Destroy publication channel connection to rtEdgeCache3 server.
 *
 * \param cxt - Publication Channel Context from rtEdge_PubInit()
 */
void rtEdge_PubDestroy( rtEdge_Context cxt );

/**
 * \brief Publish field list on an opened stream to rtEdgeCache3 server
 *
 * \param cxt - Publication Channel Context from rtEdge_PubInit()
 * \param data - Data stream + field list to publish in an rtEdgeData struct 
 * \return Number of bytes published
 */
int rtEdge_Publish( rtEdge_Context cxt, rtEdgeData data );

/**
 * \brief Publish error on an opened stream to rtEdgeCache3 server
 *
 * \param cxt - Publication Channel Context from rtEdge_PubInit()
 * \param data - Error and stream info to publish
 * \return Number of bytes published
 */
int rtEdge_PubError( rtEdge_Context cxt, rtEdgeData data );

/**
 * \brief Retrieve last published message on this publication channel.
 *
 * There is one publication buffer per channel / context.  Therefore
 * YOU MUST SYNCHRONIZE ACCESS TO THIS INTERNAL BUFFER if you are 
 * publishing from 2+ threades.  If your application only calls 
 * rtEdge_Publish() from one thread, then a call to rtEdge_PubGetData() 
 * is guaranteed to return the message just published. 
 *
 * \param cxt - Publication Channel Context from rtEdge_PubInit()
 * \return rtBUF pointing to last published message.
 */
rtBUF rtEdge_PubGetData( rtEdge_Context cxt );


/******************
 * Snapshot - LVC * 
 *****************/

/**
 * \brief Configure library to acquire lock from LVC before accessing the 
 * read-only data.
 *
 * This API must be in sync with the configuration of the LVC process.
 * Specifically, this is only valid if db.lock.enable is set int the LVC
 * config file.
 *
 * If enabled in both the library and LVC process, 
 * -# Locking ensures that you can change the "shape" of the LVC database - 
 * Add / Del Ticker - without crashing your application.
 * -# Your application locks the LVC process as long as you hold an 
 * LVC_Context.  In other words, from LVC_Initialize() until LVC_Destroy().
 * When locked, the LVC can not change "shape". 
 *
 * \param bLock - 1 to enable locking; 0 to not use locking
 * \param dwWaitMillis - Number of millis to wait on locked LVC before 
 * returning empty result set in:
 * + LVC_Snapshot()
 * + LVC_View()
 * + LVC_SnapAll()
 * + LVC_ViewAll()
 */
void LVC_SetLock( char bLock, long dwWaitMillis );

/**
 * \brief Initialize the view on the Last Value Cache (LVC)
 *
 * \param pFile - Last Value Cache filename
 * \return Initialized context for LVC_ViewAll(), LVC_View(), etc.
 */
LVC_Context LVC_Initialize( const char *pFile );

/**
 * \brief Retrieve the schema from the LVC database.
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \param data - Returned schema in LVCData struct
 * \return Number of field definitions in the schema (data dictionary)
 */
int LVC_GetSchema( LVC_Context cxt, LVCData *data );

/**
 * \brief Set query response field list filter.
 *
 * Use this to improve query performance if you know you only want 
 * to view certain fields but the LVC might contain many more.  For 
 * example, you may only want to see TRDPRC_1, BID and ASK yet the 
 * LVC might be cache-ing over 100 fields per record.  You may 
 * call LVC_SetFilter() to configure the library to only return these 
 * 3 fields.
 *
 * The filter is used on the following APIs:
 * + LVC_Snapshot()
 * + LVC_View()
 * + LVC_SnapAll()
 * + LVC_ViewAll()
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \param pFlds - Comma-separated list of field names or ID's 
 * \return Number of fields in filter
 */
int LVC_SetFilter( LVC_Context cxt, const char *pFlds );

/**
 * \brief Configure your LVC view to be volatile (Copy Type = 0) or 
 * snapshot / deep copy (Copy Type = 1).
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \param bFull - 1 for full (deep) copy; 0 for volatile view 
 */
void LVC_SetCopyType( LVC_Context cxt, char bFull );

/**
 * \brief Query the LVC for current (volatile) values for a single 
 * ( svc,tkr ) record.
 *
 * -# This API is equivalent to LVC_View()
 * -# Call LVC_Free() when done with the data
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name; Comma-separated for multiple tickers
 * \return Current cached values in an LVCData struct
 */
LVCData LVC_Snapshot( LVC_Context cxt, 
                      const char *svc, 
                      const char *tkr );

/**
 * \brief Query the LVC for current (volatile) values for a single 
 * ( svc,tkr ) record.
 *
 * -# This API is equivalent to LVC_Snapshot()
 * -# Call LVC_Free() when done with the data
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \param svc - Service name 
 * \param tkr - Ticker name; Comma-separated for multiple tickers
 * \return Current cached values in an LVCData struct
 */
LVCData LVC_View( LVC_Context cxt, const char *svc, const char *tkr );

/**
 * \brief Release resources associated with last call to LVC_View() 
 * or LVC_Snapshot()
 *
 * \param data - LVCData struct returned from call to LVC_View() 
 * or LVC_Snapshot()
 */
void LVC_Free( LVCData *data );

/**
 * \brief Query the LVC for current (volatile) values of the entire database.
 *
 * -# This API is equivalent to LVC_ViewAll()
 * -# Call LVC_FreeAll() when done with the data
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \return Current cached values of complete DB in LVCDataAll struct
 */
LVCDataAll LVC_SnapAll( LVC_Context cxt );

/**
 * \brief Query the LVC for current (volatile) values of the entire database.
 *
 * -# This API is equivalent to LVC_SnapAll()
 * -# Call LVC_FreeAll() when done with the data
 *
 * \param cxt - LVC Context from LVC_Initialize()
 * \return Current cached values of complete DB in LVCDataAll struct
 */
LVCDataAll LVC_ViewAll( LVC_Context cxt );

/**
 * \brief Release resources associated with last call to LVC_ViewAll() or 
 * LVC_SnapAll().
 *
 * \param data - LVCDataAll struct returned from call to LVC_SnapAll() 
 * or LVC_ViewAll()
 */
void LVC_FreeAll( LVCDataAll *data );

/**
 * \brief Destroy view on LVC
 *
 * \param cxt - LVC Context from LVC_Initialize()
 */
void LVC_Destroy( LVC_Context cxt );


/**********************
 * Chart / Tape Query *
 **********************/
/**
 * \brief Query ChartDB or Tape for directory of all tickers in the DB.
 * 
 * When done with the result, free the MDDResult struct via MDD_FreeResult().
 * 
 * \param cxt - ChartDB context from CDB_Initialize()
 * \return Directory in MDDResult struct
 */
MDDResult MDD_Query( CDB_Context cxt );
   
/**
 * \brief Release resources associated with last call to CDB_Query()
 *
 * \param res - MDDResult struct returned from call to CDB_Query().
 */
void MDD_FreeResult( MDDResult *res );


/*****************************
 * Snapshot Series - ChartDB * 
 ****************************/
/**
 * \brief Initialize the view on the ChartDB time-series database
 *
 * \param pFile - ChartDB time-series file name
 * \param pAdmin - \<host\>:\<port\> of ChartDB admin channel
 * \return Initialized context for CDB_Query(), CDB_View(), etc.
 */
CDB_Context CDB_Initialize( const char *pFile, const char *pAdmin );

/**
 * \brief Retrieve time series from ChartDB database
 *
 * This returns a CDBData struct containing time series data up to and 
 * including the current tick.  The CDBData::_flds array points to the 
 * time-series values.  The timestamp is implied by the following:
 * + CDBData::_interval 
 * + Position in the CDBData::_flds array.  
 *
 * For example, if CDBData::_interval = 15, then 
 * + CDBData::_flds[4] is 00:01:00 and 
 * + CDBData::_flds[240] = 01:00:00.
 *
 * \param cxt - ChartDB context from CDB_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name
 * \param fid - Field ID
 * \return Time-series for ( svc, tkr ) up to current tick.
 */
CDBData CDB_View( CDB_Context cxt, 
                  const char *svc,     
                  const char *tkr,
                  int         fid );

/**
 * \brief Release resources associated with last call to CDB_View().
 *
 * \param data - CDBData struct returned from call to CDB_View()
 */
void CDB_Free( CDBData *data );

/**
 * \brief Add new ( svc,tkr,fid ) time-series stream to ChartDB 
 *
 * \param cxt - ChartDB context from CDB_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name; Comma-separated for multiple tickers
 * \param fid - Field ID 
 */
void CDB_AddTicker( CDB_Context cxt, 
                    const char *svc, 
                    const char *tkr,
                    int         fid );

/**
 * \brief Delete existing ( svc,tkr,fid ) time-series stream to ChartDB 
 *
 * \param cxt - ChartDB context from CDB_Initialize()
 * \param svc - Service name
 * \param tkr - Ticker name; Comma-separated for multiple tickers
 * \param fid - Field ID
 */
void CDB_DelTicker( CDB_Context cxt,
                    const char *svc,
                    const char *tkr, 
                    int         fid ); 

/**
 * \brief Destroy view on ChartDB
 *
 * \param cxt - ChartDB Context from CDB_Initialize()
 */
void CDB_Destroy( CDB_Context cxt );


/*********************
 * Control - Cockpit *
 ********************/
/**
 * \brief Initialize the Cockpit channel
 *
 * \param attr - Cockpit configuration
 * \return Initialized context for Cockpit_Send(), etc.
 */
Cockpit_Context Cockpit_Initialize( CockpitAttr attr );

/**
 * \brief Connect Cockpit channel
 *
 * \param cxt - Context from Cockpit_Initialize()
 * \return Description of connection
 */
const char *Cockpit_Start( Cockpit_Context cxt );

/**
 * \brief Send formatted data to Cockpit
 *
 * \param cxt - Cockpit context from Cockpit_Initialize()
 * \param msg - Formatted Message to Send
 */
void Cockpit_Send( Cockpit_Context cxt, const char *msg );

/**
 * \brief Destroy view on ChartDB
 *
 * \param cxt - Cockpit Context from Cockpit_Initialize()
 */
void Cockpit_Destroy( Cockpit_Context cxt );


/********************
 * Operating System * 
 ********************/

/**
 * \brief Query system CPU statistics
 *
 * \param cpus - Array of OSCpuStat's to hold results
 * \param maxCpu - Max size of cpus array
 * \return Number of CPU's on box, not exceeding maxCpu
 */
int OS_GetCpuStats( OSCpuStat *cpus, int maxCpu );

/**
 * \brief Query system disk statistics
 *
 * \param disks - Array of OSDiskStat's to hold results
 * \param maxDisk - Max size of disks array
 * \return Number of disk's on box, not exceeding maxDisk
 */
int OS_GetDiskStats( OSDiskStat *disks, int maxDisk );

/**
 * \brief Query file stats
 *
 * \param pFile - Filename
 * \return OSFileStat results
 */
OSFileStat OS_GetFileStats( const char *pFile );

/**
 * \brief Start native system thread
 *
 * The library will call your rtEdgeThreadFcn until you call OS_StopThread().
 * In order to stop and cleanly terminate this worker thread, your 
 * rtEdgeThreadFcn must periodically do one of the following:
 * Thread Type | Requirement
 * --- | ---
 * + Cooperative | Return control back to the library
 * + Greedy | Call OS_ThreadIsRunning() and if false return control
 *
 * \param fcn - Worker thread function to call until OS_StopThread() 
 * \param arg - Argument returned to rtEdgeThreadFcn
 * \return Initialized context for OS_StopThread()
 * \see OS_ThreadIsRunning()
 * \see OS_StopThread()
 */
Thread_Context OS_StartThread( rtEdgeThreadFcn fcn, void *arg );

/**
 * \brief Returns 1 if worker thread is running
 *
 * A worker thread is running if OS_StartThread() has been called but 
 * OS_StopThread() has not.
 *
 * \param cxt - Initialized context from OS_StartThread()
 * \return 1 if thread is running; 0 if not found or stopped
 * \see OS_StartThread()
 * \see OS_ThreadIsRunning()
 */
char OS_ThreadIsRunning( Thread_Context cxt );

/**
 * \brief Stop native system thread
 *
 * \param cxt - Initialized context from OS_StartThread()
 * \see OS_StartThread()
 * \see OS_ThreadIsRunning()
 */
void OS_StopThread( Thread_Context cxt );



/**********************
 * Library Management * 
 *********************/

/**
 * \brief Sets the library debug level; Initiates logging
 *
 * \param pLog - Log filename
 * \param dbgLvl - Debug verbosity level
 */
void rtEdge_Log( const char *pLog, int dbgLvl );

/**********************
 * Library Utilities  *
 *********************/

/**
 * \brief Returns Unix system time as a double including microseconds
 *
 * \return Unix system time as a double including microseconds
 */
double rtEdge_TimeNs( void );

/**
 * \brief Returns Unix system time
 *
 * \return Unix system time
 */ 
time_t rtEdge_TimeSec( void );

/**
 * \brief Returns time formatted as ODBC date/time string
 * YYYY-MM-DD HH:MM:SS.mmm
 *
 * \param outbuf - Output buffer to hold formatted result string
 * \param dTime - Set to 0 to return current time
 * \return Current time formatted as ODBC date/time string
 */ 
char *rtEdge_pDateTimeMs( char *outbuf, double dTime );

/**
 * \brief Returns time as HH:MM:SS.mmm
 *
 * \param outbuf - Output buffer to hold formatted result string
 * \param dTime - Set to 0 to return current time
 * \return Current time as HH:MM:SS.mmm
 */
char *rtEdge_pTimeMs( char *outbuf, double dTime );

/**
 * \brief Returns number of seconds to the requested mark.
 *
 * \param h - Hour
 * \param m - Minute
 * \param s - Second
 * \return Number of seconds to requested mark
 */
int rtEdge_Time2Mark( int h, int m, int s );

/**
 * \brief Sleeps for requested period of time.
 *
 * \param dSlp - Number of \<seconds\>.\<microseconds\> to sleep
 */
void rtEdge_Sleep( double dSlp );


/**
 * \brief Hex dump a message into an output buffer
 *
 * \param msg - Message to dump
 * \param len - Message length
 * \param outbuf - Output buffer to hold hex dump
 * \return Length of hex dump in outbuf
 */
int rtEdge_hexMsg( char *msg, int len, char *outbuf );

/**
 * \brief Returns field value as double.
 *
 * Converts ASCII field list value via atof(); Otherwise casts binary 
 * field value to double.
 *
 * \param f - Field Value
 * \return Field value as double
 */
double rtEdge_atof( rtFIELD f );

/**
 * \brief Returns field value as int.
 *
 * Converts ASCII field list value via atoi(); Otherwise casts binary
 * field value to int.
 *
 * \param f - Field Value
 * \return Field value as int
 */
int rtEdge_atoi( rtFIELD f );


/**
 * \brief Returns total CPU usage for this process
 *
 * \return Total CPU usage for this process
 */
double rtEdge_CPU( void );

/**
 * \brief Returns total memory size of this process
 *
 * \return Total memoroy size for this process
 */
int rtEdge_MemSize( void );

/**
 * \brief Creates a memory-mapped view of the file
 *
 * Call rtEdge_UnmapFile() to unmap the view.
 *
 * \param pFile - Name of file to map
 * \param bCopy - 1 to make deep copy; 0 for view only
 * \return Mapped view of a file in an rtBUF struct
 */
rtBuf64 rtEdge_MapFile( char *pFile, char bCopy );

/**
 * \brief Re-maps view of file previously mapped via rtEdge_MapFile()
 *
 * This works as follows:
 * + Current view passed in as 1st argument must be a view, not copy
 * + File is re-mapped if the file size has grown
 * + If file size has not changed, then view is returned
 *
 * \param view - Current view of file from rtEdge_MapFile()
 * \return Mapped view of a file in an rtBUF struct
 * \see rtEdge_MapFile()
 */
rtBuf64 rtEdge_RemapFile( rtBuf64 view );

/**
 * \brief Unmaps a memory-mapped view of a file
 *
 * \param bMap - Memory-mapped view of file from rtEdge_MapFile()
 */
void rtEdge_UnmapFile( rtBuf64 bMap );

/**
 * \brief Returns 1 if 64-bit build; 0 if 32-bit 
 *
 * \return 1 if 64-bit build; 0 if 32-bit
 */
char rtEdge_Is64bit();

_CPP_END
#ifdef __cplusplus
#include <string>
#include <hpp/DataDog.hpp>
#include <hpp/OS.hpp>
#include <hpp/Mutex.hpp>
#include <hpp/Field.hpp>
#include <hpp/rtMessage.hpp>
#include <hpp/Schema.hpp>
#include <hpp/ChartDB.hpp>
#include <hpp/LVC.hpp>
#include <hpp/ByteStream.hpp>
#include <hpp/Chain.hpp>
#include <hpp/PubChannel.hpp>
#include <hpp/SubChannel.hpp>
#include <hpp/Cockpit.hpp>
#include <hpp/Update.hpp>
#include <hpp/XmlParser.hpp>
#include <hpp/admin/LVCAdmin.hpp>
#endif /* __cplusplus */

#endif // __LIB_RTEDGE_H
