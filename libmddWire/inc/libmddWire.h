/******************************************************************************
*
*  libmddWire.h
*     MD-Direct Wire Format - 'C' interface library
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created.
*     12 OCT 2013 jcs  Build  3: mddIoctl_fixedXxxx
*      5 DEC 2013 jcs  Build  4: UNPACKED_BINFLD
*      4 JUN 2014 jcs  Build  7: mddFld_real / mddFld_bytestream
*     13 NOV 2014 jcs  Build  8: mddWire_Alloc() / Free(); XML DTD
*      8 MAY 2015 jcs  Build  9: winsock.h
*     13 SEP 2015 jcs  Build 10: localtime_r / strtok_r
*     12 OCT 2015 jcs  Build 10a:_pUser -> _mdd_pUser; mddWire_dumpField
*     16 APR 2016 jcs  Build 11: _mdd_pAuth, etc.
*     29 MAR 2022 jcs  Build 13: mddIoctl_unpacked, etc.
*     23 MAY 2022 jcs  Build 14: mddFld_unixTime
*     24 OCT 2022 jcs  Build 15: bld.hpp
*      1 NOV 2022 jcs  Build 16: mddFld_vector; mddWire_vectorSize; 64-bit mddReal
*     16 MAR 2024 jcs  Build 20: mddWire_RealToDouble() / mddWire_DoubleToReal()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
/** 
 * \mainpage libmddWire API Reference Manual
 *
 * @ref libmddWire.h "API data structures and functions"
 */

#ifndef __LIB_MDD_WIRE_H
#define __LIB_MDD_WIRE_H
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <bld.hpp>

/* Platform dependent */

#ifdef WIN32
typedef __int64        int64_t;
typedef __int64        u_int64_t;
#define mdd_PRId64     "%lld"
#include <winsock.h>
#else
#define mdd_PRId64     "%ld"
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
#define MAXLEN               200
/** \brief Bitmask for packed binary field (Used internally) */
#define PACKED_BINARY        0x80
/** \brief Bitmask for unpacked binary field (Used internally) */
#define UNPACKED_BINFLD      0x40
/** \brief Nanosecond */
#define _NANO                1000000000
/** \brief Max mddReal Precision - 14 significant digits */
#define _MAX_REAL_HINT       14

/* Data Structures */

/** \brief The context number of an initialized wire  */
typedef int mddWire_Context;

/**
 * \enum mddProtocol
 * \brief The wire protocol for this channel (one per channel)
 */
typedef enum {
   /** \brief Undefined protocol */
   mddProto_Undef  = 'u',
   /** \brief Binary protocol */
   mddProto_Binary = 'b',
   /** \brief ASCII Field List protocol */
   mddProto_MF     = 'm',
   /** \brief XML Field List protocol */
   mddProto_XML    = 'x'
} mddProtocol;

/**
 * \enum mddIoctl
 * \brief Wire protocol control commands passed to mddWire_ioctl()
 */
typedef enum {
   /** 
    * \brief Publish mddProto_Binary FieldLists as unpacked values
    *
    *  + Only used for mddProtocol == mddProto_Binary
    *  + Only used for publishing
    *  + Subscription
    *  + Default is 0
    */
   mddIoctl_unpacked        = 0,
   /** 
    * \brief Convert mddProto_MF string fields to native fields based on schema.
    *  + Only used for mddProtocol == mddProto_MF
    *  + Set to 1 to convert to mddProto_MF string fields to native fields
    *  + Set to 0 to return raw string field 
    *  + Default is 0
    */
   mddIoctl_nativeField    = 1,
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
    */
   mddIoctl_fixedLibrary   = 6
} mddIoctl;

/**
 * \enum mddFldType
 * \brief Field type when mddMsgHdr::_dt == mddDt_FieldList
 */
typedef enum {
   /** \brief Undefined field type */
   mddFld_undef  = 0,
   /** \brief String field; Value in mddValue::_buf */
   mddFld_string,
   /** \brief 32-bit int field; Value in mddValue::_i32 */
   mddFld_int32,
   /** \brief double field; Value in mddValue::_r64 */
   mddFld_double,
   /** \brief Date field; Value in mddValue::_r64 */
   mddFld_date,
   /** \brief Time field; Value in mddValue::_r64 */
   mddFld_time,
   /** \brief Time-seconds field; Value in mddValue::_r64 */
   mddFld_timeSec,
   /** \brief float field; Value in mddValue::_r32 */
   mddFld_float,
   /** \brief 8-bit int field; Value in mddValue::_i8 */
   mddFld_int8,
   /** \brief 16-bit int field; Value in mddValue::_i16 */
   mddFld_int16,
   /** \brief 64-bit int field; Value in mddValue::_i64 */
   mddFld_int64,
   /** \brief Real field; Value in mddValue::_real */
   mddFld_real,
   /** \brief Bytestream field; Value in mddValue::_buf */
   mddFld_bytestream,
   /** \brief Nanos since epoch; Value in mddValue::_i64 */
   mddFld_unixTime,
   /** \brief Vector of doubles in mddValue::_buf; Num = mddWire_vectorSize() */
   mddFld_vector
} mddFldType;

/**
 * \struct mddBuf
 * \brief A memory buffer - Not allocated
 */  
typedef struct {
   /** \brief The data */
   char *_data;
   /** \brief Data length */
   u_int _dLen;
} mddBuf;

/**
 * \struct mddBldBuf
 * \brief Allocated memory buffer use to build mddWireMsg.
 *  
 * You MUST use mddBldBuf_Alloc() and mddBldBuf_Free() to allocate / free  
 */
typedef struct {
   /** \brief The data */
   char *_data;
   /** \brief Data length */
   u_int _dLen;
   /** \brief Allocated size; Always > _dLen */
   u_int _nAlloc;
   /** \brief 1 if _data points to mddMsgHdr; 0 if not */
   char  _bNoHdr;
   /** \brief Pointer to message payload (not including header */
   char *_payload;
} mddBldBuf;

/**
 * \struct mddReal
 * \brief Real field value
 */
typedef struct {
   /** \brief Real data value */
   u_int64_t value;
   /**
    * \brief Multiplier for value
    *
    * Hint | Multiplier
    * --- | ---
    * 0 | 1.0
    * 1 | 0.1
    * 2 | 0.01
    * 3 | 0.001
    * . . . | . . .
    * 10 | 0.000000001
    */
   u_char    hint;
   /** \brief 1 if mddReal contains blank value */
   u_char    isBlank;
} mddReal;

/**
 * \union mddValue
 * \brief Polymorphic field value
 *
 * Type defined in mddField::_type
 */
typedef union {
   /** 
    * \brief String / Binary Buffer
    *
    * When mddField::_type == mddFld_string or mddFld_bytestream
    */
   mddBuf    _buf;
   /** \brief Floating point value when mddField::_type == mddFld_float. */
   float     _r32;
   /** \brief Double value when mddField::_type == mddFld_double. */
   double    _r64;
   /** \brief 8-bit value when mddField::_type == mddFld_int8. */
   u_char    _i8;
   /** \brief 16-bit short int value when mddField::_type == mddFld_int16. */
   u_short   _i16;
   /** \brief 32-bit int value when mddField::_type == mddFld_int32. */
   u_int     _i32;
   /** \brief 64-bit long int value when mddField::_type == mddFld_int64. */
   u_int64_t _i64;
   /** \brief Floating point value when mddField::_type == mddFld_real. */
   mddReal   _real;
} mddValue;

/**
 * \struct mddField;
 * \brief A single mddWire field in an mddFieldList 
 */
typedef struct {
   /** \brief Field ID */
   u_int       _fid;
   /** \brief Field value */
   mddValue    _val;
   /** \brief Field name (from Schema) */
   const char *_name;
   /** \brief Field type */
   mddFldType  _type;
   /** \brief mddFld_vector Precision : 0 to 20; 0xff = 10 */
   char        _vPrecision;
} mddField;

/**
 * \enum mddMsgType
 * \brief Type of wire message in mddWireMsg
 */
typedef enum {
   /** \brief Undefined message */
   mddMt_undef      =  0,
   /** \brief Initial Image */
   mddMt_image      =  1,
   /** \brief Update */
   mddMt_update     =  2,
   /** \brief Stream is Stale */
   mddMt_stale      =  3,
   /** \brief Stream is Recovering */
   mddMt_recovering =  4,
   /** \brief Dead Stream */
   mddMt_dead       =  5,
   /** \brief Mount - Session Establish */
   mddMt_mount      =  6,
   /** \brief Ping */
   mddMt_ping       =  7,
   /** \brief Control - mddIoctl  */
   mddMt_ctl        =  8,
   /** \brief Open Stream (Publisher) */
   mddMt_open       =  9,
   /** \brief Close Stream (Publisher) */
   mddMt_close      = 10,
   /** \brief Query (Publisher) */
   mddMt_query      = 11,
   /** \brief Insert (Send msg upstream) */
   mddMt_insert     = 12,
   /** \brief Insert ACK */
   mddMt_insAck     = 13,
   /** \brief Global Status */
   mddMt_gblStatus  = 14,
   /** \brief History */
   mddMt_history    = 15,
   /** \brief Database Query */
   mddMt_dbQry      = 16,
   /** \brief Database Response - Table */
   mddMt_dbTable    = 17
} mddMsgType;

/**
 * \enum mddDataType
 * \brief Message Data Type - Normally mddDt_FieldList
 */
typedef enum {
   /** \brief Undefined data type */
   mddDt_undef           =  0,
   /** \brief Control Msg; Only for mddProto_Binary */
   mddDt_Control         = 'c',
   /** \brief Field List; All mddProtocol values */
   mddDt_FieldList       = 'f',
   /** \brief Order Book Order; Only for mddProto_Binary */
   mddDt_BookOrder       = 'o',
   /** \brief Order Book Price Level ; Only for mddProto_Binary */
   mddDt_BookPriceLvevel = 'p',
   /** \brief Blob List s; Only for mddProto_Binary */
   mddDt_BlobList        = 'l',
   /** \brief Blob Table; Only for mddProto_Binary */
   mddDt_BlobTable       = 't',
   /** \brief Fixed-size Msg; Only for mddProto_Binary */
   mddDt_FixedMsg        = 'x'
} mddDataType;

/**
 * \enum mddWireState
 * \brief The state - UP or DOWN - of a wire resource such as 
 * a Channel or Service
 */
typedef enum {
   /** \brief The wire resource (Channel or Service) is UP */
   mdd_up = 0,
   /** \brief The wire resource (Channel or Service) is DOWN */
   mdd_down
} mddWireState;

/**
 * \struct mddFieldList
 * \brief A list of mddField's.
 */
typedef struct {
   /** \brief Array of fields; Size = _nFld */
   mddField *_flds;
   /** \brief Number of fields in list - e.g., size of _flds */
   int       _nFld;
   /** \brief Allocated size of _flds; > _nFld */
   int       _nAlloc;
} mddFieldList;

/* TODO : Stuff mddMsgHdr into mddWireMsg */

/**
 * \struct mddMsgHdr
 * \brief The header portion of an MD-Direct wire message, used by:
 * + Subscribe : mddSub_ParseHdr()
 * + Publish   : mddPub_BuildMsg()
 * + Publish   : mddPub_BuildRawMsg()
 */
typedef struct
{
   /** \brief Total Message length */
   int          _len;
   /** \brief Message Protocol */
   mddProtocol  _proto;
   /** \brief true if packed - mddProto_Binary only  */
   char         _bPack;
   /** \brief Message Type */
   mddMsgType   _mt;
   /** \brief Data Type */
   mddDataType  _dt;
   /** \brief Message tag - string  */
   char         _tag[K];
   /** \brief Message tag - int */
   u_int        _iTag;
   /** \brief Record Transaction Level */
   u_int        _RTL;
   /** \brief Message Time - <unixtime>.<micros> */
   double       _time;
   /** \brief Real-time service supplying this message */
   mddBuf       _svc;
   /** \brief Real-time ticker name */
   mddBuf       _tkr;
   /** \brief Message attributes - mddProto_XML only */
   mddFieldList _attrs;
   /** \brief Length of header in raw message */
   int          _hdrLen;
} mddMsgHdr;


/**
 * \struct mddWireMsg
 * \brief A complete MD-Direct wire message, used by:
 * + Subscribe : mddSub_ParseMsg()
 * + Convert   : mddWire_ConvertFieldList()
 */
typedef struct {
   /** \brief State of the _svc */
   mddWireState _state;
   /** \brief Real-time service name */
   mddBuf       _svc;
   /** \brief Real-time ticker name */
   mddBuf       _tkr;
   /** \brief Error message, if any */
   mddBuf       _err;
   /** \brief Unique stream ID */
   u_int        _tag;
   /** \brief Protocol */
   mddProtocol  _proto;
   /** \brief Message Type */
   mddMsgType   _mt;
   /** \brief Data Type */
   mddDataType  _dt;
   /** \brief Field List */
   mddFieldList _flds;
   /** \brief Raw data buffer */
   mddBuf       _rawData;
} mddWireMsg;

/**
 * \struct mddMsgBuf
 * \brief Used by mddSub_ParseMsg() to parse.
 * 
 * This is the raw wire message in an mddBuf plus a pointer to a 
 * previously parsed header.  This allows the library to only parse 
 * the header once if the application code calls mddSub_ParseHdr()
 * before mddSub_ParseMsg().
 */
typedef struct {
   /** \brief Raw MD-Direct wire message */
   char      *_data;
   /** \brief MD-Direct wire message length */
   u_int      _dLen;
   /** \brief mddSub_ParseHdr() result, if called before mddSub_ParseMsg() */
   mddMsgHdr *_hdr;
} mddMsgBuf;

/**
 * \struct mddConvertBuf
 * \brief Structure by mddWire_ConvertFieldList() to convert field lists
 * between different mddProtocol.
 */
typedef struct {
   /** \brief Input message to convert */
   mddWireMsg *_msgIn;
   /** \brief Output protocol */
   mddProtocol _proto;
   /** \brief Output buffer */
   mddBldBuf  *_bufOut;
   /** \brief Set to 1 for Field Name rather than ID; mddProto_XML only */
   char        _bFldNm;
} mddConvertBuf;

/*******************
 * Loadable Driver
 ******************/
#define _pDriverFcn "ConvertFixedToFieldList"

/**
 * \brief Required exported function definition to convert fixed-size 
 * message to an mddFieldList via mddIoctl_fixedLibrary.  The exported 
 * function must be named _pDriverFcn.
 */
typedef char  (*mddDriverFcn)( mddBuf *, mddFieldList * );
/**
 * \brief Required exported function definition to specify loadable 
 * library version if you load library via mddIoctl_fixedLibrary.  The    
 * exported function must be named "Version".
 */
typedef char *(*mddDvrVerFcn)( void );

/*************
 * API Calls *
 *************/

/* Subscription */

/**
 * \brief Initialize a workspace to parse incoming messages.
 *
 * This initializes a workspace that can be identified by the mddWire_Context
 * in subsequent calls to mddSub_ParseMsg() and the like.  Typically you 
 * only need one per subscription channel in your application.  
 *
 * \return Initialized context for mddSub_ParseMsg(), etc.
 */
mddWire_Context mddSub_Initialize();

/**
 * \brief Parse incoming message
 *
 * The mddMsgBuf::_hdr allows you to call mddSub_ParseHdr() before calling
 * mddSub_ParseMsg() and only parse the header once.
 *
 * \param cxt - Context from mddSub_Initialize()
 * \param inp - Raw input message
 * \param msg - Parsed message result
 * \return Message size in bytes
 */
int  mddSub_ParseMsg( mddWire_Context cxt, 
                      mddMsgBuf       inp, 
                      mddWireMsg     *msg );

/**
 * \brief Parse incoming message header
 *
 * The mddMsgBuf::_hdr allows you to call mddSub_ParseHdr() before calling
 * mddSub_ParseMsg() and only parse the header once.
 *
 * \param cxt - Context from mddSub_Initialize()
 * \param inp - Raw input message
 * \param hdr - Parsed header result
 * \return Header size in bytes
 */
int  mddSub_ParseHdr( mddWire_Context cxt, 
                      mddMsgBuf       inp, 
                      mddMsgHdr      *hdr );

/**
 * \brief Destroy workspace used for parsing incoming messages
 *
 * \param cxt - Context from mddSub_Initialize()
 */
void mddSub_Destroy( mddWire_Context );

/* Publication */

/**
 * \brief Initialize a workspace to build outgoing messages.
 *
 * This initializes a workspace that can be identified by the mddWire_Context
 * in subsequent calls to mddSub_BuildMsg() and the like.  Typically you
 * only need one per publication channel in your application.
 *
 * \return Initialized context for mddSub_Buildmsg(), etc.
 */
mddWire_Context mddPub_Initialize();

/**
 * \brief Stores fields from an mddFieldList to be published 
 *
 * This adds all fields from the list to an internal store.  The fields 
 * in the internal store are accessed by mddPub_BuildMsg() and 
 * mddPub_BuildRawMsg() to build the message.  The internal store is 
 * cleared once the message is built.
 *
 * You may call mddPub_AddFieldList() one or more times between calls to 
 * build the message.  The fields from the 2nd+ call to mddPub_AddFieldList()
 * simply append the fields to the internal store.
 *
 * \param cxt - Context from mddSub_PubInitialize()
 * \param fl - mddFieldList to add to the message
 * \return Total fields that have been added since last message was built 
 * in mddPub_BuildMsg() or mddPub_BuildRawMsg().
 */
int mddPub_AddFieldList( mddWire_Context cxt, mddFieldList fl );

/**
 * \brief Builds a message from the supplied header and any field list
 * added via mddPub_AddFieldList().
 *
 * This message clears the internal store of fields from mddPub_AddFieldList().
 *
 * \param cxt - Context from mddSub_PubInitialize()
 * \param hdr - Initialized message header with values set by caller
 * \param outbuf - Reusable buffer used to build the message.
 * \return Buffer containing built message in the protocol defined by 
 * mddWire_SetProtocol() for this context.
 */
mddBuf mddPub_BuildMsg( mddWire_Context cxt, 
                        mddMsgHdr       hdr, 
                        mddBldBuf      *outbuf );

/**
 * \brief Builds a message from the supplied header and preformatted 
 * message payload.
 *
 * \param cxt - Context from mddSub_PubInitialize()
 * \param hdr - Initialized message header with values set by caller
 * \param payload - Pre-built message payload
 * \param outbuf - Reusable buffer used to build the message.
 * \return Buffer containing built message in the protocol defined by   
 * mddWire_SetProtocol() for this context.
 */
mddBuf mddPub_BuildRawMsg( mddWire_Context cxt, 
                           mddMsgHdr       hdr, 
                           mddBuf          raw, 
                           mddBldBuf      *outbuf );

/**
 * \brief Sets the header tag for a pre-built mddProto_Binary message.
 *
 * This allows bridges or distributors such as rtEdgeCache2 to 'pipe' 
 * binary data from binary publisher to binary consumer without unpacking 
 * the message by simply changing the tag (unique stream identifier) 
 * then sending the message to the subscriber.
 *
 * \param cxt - Context from mddPub_Initialize()
 * \param tag - Header tag
 * \param bld - Reusable buffer with mddProto_Binary already built
 * \return Buffer containing built message in the protocol defined by
 * mddWire_SetProtocol() for this context.
 */
mddBuf mddPub_SetHdrTag( mddWire_Context cxt, u_int tag, mddBuf *bld );

/**
 * \brief Destroy workspace used for building outgoing messages
 *
 * \param cxt - Context from mddPub_Initialize()
 */
void mddPub_Destroy( mddWire_Context );

/* Control / Schema */

/**
 * \brief Returns ping message.
 * 
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \return Buffer containing ping message in the protocol defined by
 * mddWire_SetProtocol() for this context.
 */
mddBuf mddWire_Ping( mddWire_Context cxt );

/**
 * \brief Configures the workspace
 * 
 * See mddIoctl for list of command and values
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \param cmd - Command from mddIoctl
 * \param val - Command value
 */
void mddWire_ioctl( mddWire_Context cxt, mddIoctl cmd, void *val );

/**
 * \brief Sets the workspace protocol
 *
 * If cxt is a workspace for inbound messages from mddSub_Initialize(),
 * then messages are parsed using this protocol.  If cxt is a workspace 
 * for outbound messages from mddPub_Initialize(), then messages are 
 * built using this protocol
 *
 * Default is mddProto_XML.
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \param proto - Protocol
 */
void mddWire_SetProtocol( mddWire_Context cxt, mddProtocol proto );

/**
 * \brief Returns the workspace protocol set from mddWire_SetProtocol().
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \return Workspace protocol set from mddWire_SetProtocol().
 */
mddProtocol  mddWire_GetProtocol( mddWire_Context cxt );

/**
 * \brief Sets the workspace schema
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \param schema - Complete schema in format returned by rtEdgeCache2
 * \return Schema size (number of fields)
 */
int mddWire_SetSchema( mddWire_Context cxt, const char *schema );

/**
 * \brief Returns the field list schema set from mddWire_SetSchema().
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \return Schema set by mddWire_SetSchema().
 */
mddFieldList mddWire_GetSchema( mddWire_Context );

/**
 * \brief Copies schema from source workspace to destination workspace
 *
 * The copy is **NOT** a deep copy.  If you destroy the source workspace,
 * then the destination workspace schema will be indeterminate.
 *
 * \param cxtDst - Destination workspace context
 * \param cxtSrc - Source workspace context
 * \return Schema size (number of fields) that was copied
 */
int mddWire_CopySchema( mddWire_Context cxtDst, mddWire_Context cxtSrc );

/**
 * \brief Returns the field definition for a specific FID from the schema
 * set from mddWire_SetSchema(). 
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \param fid - Field ID
 * \return Field definition, if defined
 */
mddField *mddWire_GetFldDefByFid( mddWire_Context cxt, int fid );

/**
 * \brief Returns the field definition for a specific field name from the 
 * schema  set from mddWire_SetSchema().
 *
 * \param cxt - Context from mddSub_Initialize() or mddPub_Initialize()
 * \param fieldName - Field name 
 * \return Field definition, if defined 
 */
mddField *mddWire_GetFldDefByName( mddWire_Context cxt, const char *fielddName );

/* Data Conversion - FieldList */

/**
 * \brief Convert field lists between protocols
 *
 * Convert the field list in mddConvertBuf::_msgIn from the protcool
 * mddConverBuf::_proto to a field list in the protocol in the publication 
 * workspace in cxt.
 *
 * This is equivalent to the following:
 * + Using the field list from the message in mddConvertBuf::_msgIn 
 * + Calling mddPub_AddFieldList() in the cxt workspace 
 * + Populating a new mddMsgHdr with header from mddConvertBuf::_msgIn 
 * + Calling mddPub_BuildMsg() in the cxt workspace 
 *
 * \param cxt - Workspace from mddPub_Initialize() to convert to
 * \param buf - Input buffer containing message and protocol
 * \return Buffer containing built message in the protocol defined in cxt
 */
mddBuf mddWire_ConvertFieldList( mddWire_Context cxt, mddConvertBuf buf );

/* Real Conversion */

/**
 * \brief Convert mddReal to double w/ precision
 *
 * \param r - mddReal to convert
 * \return Converted double
 */
double mddWire_RealToDouble( mddReal r );

/**
 * \brief Convert double to mddReal
 *
 * \param d - double to convert
 * \param hint - Precision from mddReal
 * \return Converted mddReal
 * \see mddReal
 */
mddReal mddWire_DoubleToReal( double d, int hint );

/* Library Management */

/**
 * \brief Return library build description
 *
 * \return Library build description
 */
const char *mddWire_Version( void );

/**
 * \brief Sets the library debug level; Initiates logging
 *
 * \param pLog - Log filename
 * \param dbgLvl - Debug verbosity level
 */
void mddWire_Log( const char *pLog, int dbgLvl );

/* Utilities */

/**
 * \brief Return current time as Unix time (secs / uS since Jan 1, 1970).
 *
 * \return Current time as Unix time (secs / uS since Jan 1, 1970).
 */
double mddWire_TimeNs( void );

/**
 * \brief Return current time as Unix time (secs since Jan 1, 1970).
 *
 * \return Current time as Unix time (secs since Jan 1, 1970).
 */
time_t mddWire_TimeSec( void );

/**
 * \brief Returns current time formatted in YYYY-MM-DD HH:MM:SS.mmm
 *
 * \param outbuf - Output buffer to hold formatted message time
 * \return Current time formatted in YYYY-MM-DD HH:MM:SS.mmm
 */
char *mddWire_pDateTimeMs( char *outbuf );

/**
 * \brief Returns current time formatted in HH:MM:SS.mmm
 *
 * \param outbuf - Output buffer to hold formatted message time
 * \return Current time formatted in HH:MM:SS.mmm
 */
char *mddWire_pTimeMs( char *outbuf );

/**
 * \brief Calculate number of seconds to a point in the future
 *
 * \param hr - Hour
 * \param min - Minute
 * \param sec - Second
 * \return Number of seconds to a point in the future
 */
int mddWire_Time2Mark( int hr, int min, int sec );

/**
 * \brief Sleep for a period
 *
 * \param tSlp - Sleep time in seconds / microseconds
 */
void mddWire_Sleep( double tSlp );

/**
 * \brief Hex dump a message into an output buffer
 *
 * \param msg - Message to dump
 * \param len - Message length
 * \param outbuf - Output buffer to hold hex dump
 * \return Length of hex dump in outbuf
 */
int mddWire_hexMsg( char *msg, int len, char *outbuf );

/**
 * \brief Number of elements in vector, based on buffer size
 *
 * \param buf - mddBuf holding vector
 * \return Number of elements in the vector
 */
int mddWire_vectorSize( mddBuf b );

/* Windows DLL Horse Shit */

/**
 * \brief Allocate an mddFieldList of specific size
 *
 * You must call mddFieldList_Alloc() / mddFieldList_Free() when 
 * using mddFieldList's to ensure integrity of the heap.
 *
 * \param  nAlloc - Allocated size of field list
 * \return Allocated mddFieldList
 */
mddFieldList mddFieldList_Alloc( int nAlloc );

/**
 * \brief Free an mddFieldList allocated by mddFieldList_Alloc()
 *
 * You must call mddFieldList_Alloc() / mddFieldList_Free() when 
 * using mddFieldList's to ensure integrity of the heap. 
 * 
 * \param  fl - mddFieldList from mddFieldList_Alloc()
 */
void mddFieldList_Free( mddFieldList fl );


/**
 * \brief Allocate an mddBldBuf of specific size
 *
 * You must call mddBldBuf_Alloc() / mddBldBuf_Free() when using
 * mddBldBuf's to ensure integrity of the heap.
 * 
 * \param  nAlloc - Allocated size of field list
 * \return Allocated mddBldBuf
 */
mddBldBuf mddBldBuf_Alloc( int nAlloc );

/**
 * \brief Free an mddBldBuf allocated by mddBldBuf_Alloc() 
 *
 * You must call mddBldBuf_Alloc() / mddBldBuf_Free() when using 
 * mddBldBuf's to ensure integrity of the heap.
 * 
 * \param  fl - mddFieldList from mddFieldList_Alloc() 
 */
void mddBldBuf_Free( mddBldBuf buf );

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


/* Useful FieldList Macros */

/**
 * \brief Initialize an mddFieldList
 */
#define mddWire_InitFieldList( fl, flds, nAlloc ) \
   do {                                           \
      fl._nFld   = 0;                             \
      fl._flds   = flds;                          \
      fl._nAlloc = nAlloc;                        \
   } while( 0 )

/**
 \brief Add an mddFld_string-type mddField to an mddFieldList
 */
#define mddWire_AddString( fl, fid, str, sz )       \
   do {                                             \
      fl._flds[fl._nFld]._fid      = fid;           \
      fl._flds[fl._nFld]._type     = mddFld_string; \
      fl._flds[fl._nFld]._val._buf._data = str;     \
      fl._flds[fl._nFld]._val._buf._dLen = sz;      \
      fl._nFld += 1;                                \
   } while( 0 )

/**
 * \brief Add a null-terminated mddFld_string-type mddField to an mddFieldList
 */ 
#define mddWire_AddStringZ( fl, fid, str )                \
   mddWire_AddString( fl, fid, str, strlen( str ) )

/**
 * \brief Add an mddFld_float-type mddField to an mddFieldList
 */
#define mddWire_AddFloat( fl, fid, r32 )           \
   do {                                            \
      fl._flds[fl._nFld]._fid      = fid;          \
      fl._flds[fl._nFld]._type     = mddFld_float; \
      fl._flds[fl._nFld]._val._r32 = r32;          \
      fl._nFld += 1;                               \
   } while( 0 )

/**
 * \brief Add an mddFld_double-type mddField to an mddFieldList
 */
#define mddWire_AddDouble( fl, fid, r64 )           \
   do {                                             \
      fl._flds[fl._nFld]._fid      = fid;           \
      fl._flds[fl._nFld]._type     = mddFld_double; \
      fl._flds[fl._nFld]._val._r64 = r64;           \
      fl._nFld += 1;                                \
   } while( 0 )

/**
 * \brief Add an mddFld_int8-type mddField to an mddFieldList
 */
#define mddWire_AddInt8( fl, fid, i8 )           \
   do {                                          \
      fl._flds[fl._nFld]._fid     = fid;         \
      fl._flds[fl._nFld]._type    = mddFld_int8; \
      fl._flds[fl._nFld]._val._i8 = i8;          \
      fl._nFld += 1;                             \
   } while( 0 )

/**
 * \brief Add an mddFld_int16-type mddField to an mddFieldList
 */
#define mddWire_AddInt16( fl, fid, i16 )           \
   do {                                            \
      fl._flds[fl._nFld]._fid      = fid;          \
      fl._flds[fl._nFld]._type     = mddFld_int16; \
      fl._flds[fl._nFld]._val._i16 = i16;          \
      fl._nFld += 1;                               \
   } while( 0 )

/**
 * \brief Add an mddFld_int32-type mddField to an mddFieldList
 */
#define mddWire_AddInt32( fl, fid, i32 )           \
   do {                                            \
      fl._flds[fl._nFld]._fid      = fid;          \
      fl._flds[fl._nFld]._type     = mddFld_int32; \
      fl._flds[fl._nFld]._val._i32 = i32;          \
      fl._nFld += 1;                               \
   } while( 0 )

/**
 * \brief Add an mddFld_int64-type mddField to an mddFieldList
 */
#define mddWire_AddInt64( fl, fid, i64 )           \
   do {                                            \
      fl._flds[fl._nFld]._fid      = fid;          \
      fl._flds[fl._nFld]._type     = mddFld_int64; \
      fl._flds[fl._nFld]._val._i64 = i64;          \
      fl._nFld += 1;                               \
   } while( 0 )

/* Useful mddField Macros */

/**
 * \brief Get the mddValue::_r64 value from the mddField
 */
#define mddField_double( f ) f._val._r64
/**
 * \brief Get the mddValue::_i32 value from the mddField
 */
#define mddField_int32( f )  f._val._i32
/**
 * \brief Get the mddValue::_i32 value from the mddField
 */
#define mddField_int( f )    (int)mddField_int32( f )
/**
 * \brief Get the mddValue::_i64 value from the mddField
 */
#define mddField_int64( f )  f._val._i64
/**
 * \brief Get the mddValue::_i64 value from the mddField
 */
#define mddField_long( f )   (long)mddField_int64( f )
/**
 * \brief Dump mddField contents to ASCII
 */
#define mddWire_dumpField( f, buf )                                 \
   do {                                                             \
      mddValue   v = f._val;                                        \
      mddBuf     b = v._buf;                                        \
      double     r64;                                               \
      int        fSz, ymd, hms;                                     \
      time_t     tUnx;                                              \
      u_int64_t  tNano;                                             \
      struct tm *tm, lt;                                            \
                                                                    \
      switch( f._type ) {                                           \
         case mddFld_undef:                                         \
            strcpy( buf, "???" );                                   \
            break;                                                  \
         case mddFld_string:                                        \
            fSz = gmin( b._dLen, K-1 );                             \
            ::memcpy( buf, b._data, fSz );                          \
            buf[fSz] = '\0';                                        \
            break;                                                  \
         case mddFld_int32:                                         \
            sprintf( buf, "%d", v._i32 );                           \
            break;                                                  \
         case mddFld_double:                                        \
            sprintf( buf, "%.6f", v._r64 );                         \
            break;                                                  \
         case mddFld_date:                                          \
         {                                                          \
            char *dp = buf;                                         \
                                                                    \
            r64 = v._r64 / 1000000.0;                               \
            ymd = (int)r64;                                         \
            r64 = ::fmod( v._r64, 1000000.0 );                      \
            hms = (int)r64;                                         \
            dp += sprintf( dp, "%04d-%02d-%02d",                    \
               ymd/10000, ( ymd/100 ) % 100, ymd%100 );             \
            if ( !hms )                                             \
               break;                                               \
            dp += sprintf( dp, "%02d:%02d:%02d",                    \
               hms/10000, ( hms/100 ) % 100, hms%100 );             \
            break;                                                  \
         }                                                          \
         case mddFld_time:                                          \
            sprintf( buf, "%.6f", v._r64 );                         \
            break;                                                  \
         case mddFld_timeSec:                                       \
            hms = (int)v._r64;                                      \
            r64 = ( v._r64 - hms ) * 1000;                          \
            sprintf( buf, "%02d:%02d:%02d.%03d",                    \
               hms/10000, ( hms/100 ) % 100, hms%100, (int)r64 );   \
            break;                                                  \
         case mddFld_float:                                         \
            sprintf( buf, "%.4f", v._r32 );                         \
            break;                                                  \
         case mddFld_int8:                                          \
            sprintf( buf, "%d", v._i8 );                            \
            break;                                                  \
         case mddFld_int16:                                         \
            sprintf( buf, "%d", v._i16 );                           \
            break;                                                  \
         case mddFld_int64:                                         \
            sprintf( buf, mdd_PRId64, v._i64 );                     \
            break;                                                  \
         case mddFld_real:                                          \
            r64 = mddWire_RealToDouble( v._real );                  \
            sprintf( buf, "%.6f", r64 );                            \
            break;                                                  \
         case mddFld_bytestream:                                    \
            strcpy( buf, "TBD" );                                   \
            break;                                                  \
         case mddFld_vector:                                        \
         {                                                          \
            double *dv;                                             \
            char   *vp, *dp, num, fmt[16];                          \
            int     i, nv;                                          \
                                                                    \
            vp  = buf;                                              \
            dp  = b._data;                                          \
            num = *dp++;                                            \
            sprintf( fmt, "%%.%df,", num );                         \
            dv  = (double *)dp;                                     \
            nv  = mddWire_vectorSize( b );                          \
            vp += sprintf( vp, "[%d] ", nv );                       \
            for ( i=0; i<nv; vp+=sprintf( vp, fmt, dv[i++] ) );     \
            break;                                                  \
         }                                                          \
         case mddFld_unixTime:                                      \
            tUnx  = v._i64 / _NANO;                                 \
            tNano = v._i64 % _NANO;                                 \
            tm    = ::localtime_r( &tUnx, &lt );                    \
            if ( !tm )                                              \
               break;                                               \
            sprintf( buf, "%04d-%02d-%02d %02d:%02d:%02d.%09d",     \
                  lt.tm_year + 1900,                                \
                  lt.tm_mon + 1,                                    \
                  lt.tm_mday,                                       \
                  lt.tm_hour,                                       \
                  lt.tm_min,                                        \
                  lt.tm_sec,                                        \
                  (int)tNano );                                     \
            break;                                                  \
      }                                                             \
   } while( 0 )

/** \brief XML DTD */

#define _mdd_pAttrBcst   (char *)"BROADCAST"
#define _mdd_pAttrBin    (char *)"BINARY"
#define _mdd_pAttrCache  (char *)"Cache"
#define _mdd_pAttrDict   (char *)"DataDict"
#define _mdd_pAttrMF     (char *)"MF"
#define _mdd_pAttrName   (char *)"Name"
#define _mdd_pAttrPing   (char *)"Ping"
#define _mdd_pAttrRdy    (char *)"SrcReady"
#define _mdd_pAttrSvc    (char *)"Service"
#define _mdd_pAttrTag    (char *)"TAG"
#define _mdd_pNo         (char *)"NO"
#define _mdd_pUp         (char *)"UP"
#define _mdd_pYes        (char *)"YES"
#define _mdd_pUser       (char *)"User"
#define _mdd_pType       (char *)"Type"
#define _mdd_pCode       (char *)"Code"
#define _mdd_pError      (char *)"Error"
#define _mdd_pGlobal     (char *)"Global"

#define _mdd_pAttrHop    (char *)"HopCnt"
#define _mdd_pLoginID    (char *)"LoginID"
#define _mdd_pAuth       (char *)"Authenticate"
#define _mdd_pNoAuth     (char *)"No_authentiation"
#define _mdd_pAttrPerm   (char *)"PERMS"
#define _mdd_pPword      (char *)"Password"
#define _mdd_pLoginID    (char *)"LoginID"
#define _mdd_pAttrSnap   (char *)"SNAP"

/** \brief Marketfeed Protocol (ASCII Field List) */

#ifndef FS
#define FS 0x1c
#define GS 0x1d
#define RS 0x1e
#define US 0x1f
#endif // FS

#endif // __LIB_MDD_WIRE_H
