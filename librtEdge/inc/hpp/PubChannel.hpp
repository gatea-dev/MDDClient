/******************************************************************************
*
*  PubChannel.hpp
*     librtEdge PubChannel connection to rtEdgeCache3
*
*  REVISION HISTORY:
*      3 SEP 2014 jcs  Created.
*     11 DEC 2014 jcs  rtEdge.hpp
*      8 JAN 2015 jcs  Build 29: SetRandomize()
*     28 FEB 2015 jcs  Build 30: SetHeartbeat()
*     20 JUN 2015 jcs  Build 31: RTEDGE::Channel; PubGetData()
*      5 MAR 2016 jcs  Build 32: OnIdle(); _authReq; OnPermQuery()
*      7 JUL 2016 jcs  Build 33: Start( pubName )
*     26 MAY 2017 jcs  Build 34: StartConnectionless()
*      6 DEC 2018 jcs  Build 41: VOID_PTR
*     12 FEB 2020 jcs  Build 42: Channel.SetHeartbeat()
*     29 APR 2020 jcs  Build 43: OnOpenBDS()
*     26 JUN 2020 jcs  Build 44: De-lint
*     29 MAR 2022 jcs  Build 52: _bUnPacked
*     22 OCT 2022 jcs  Build 58: ByteStream.Service(); CxtMap
*     24 OCT 2023 jcs  Build 65: _EmptyBDS
*      5 JAN 2024 jcs  Build 67: SetCircularBuffer()
*     21 FEB 2024 jcs  Build 68: PublishRaw()
*     26 JUN 2024 jcs  Build 72: Default : binary / unpacked / circQ
*      7 NOV 2024 jcs  Build 74: ioctl_setRawLog
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_PubChannel_H
#define __RTEDGE_PubChannel_H
#include <hpp/rtEdge.hpp>

namespace RTEDGE
{

class PubChannel;
class Update;

#ifndef DOXYGEN_OMIT
static CxtMap<PubChannel *> _pubChans;

static char *_EmptyBDS = (char *)"Empty BDS";
 
#endif // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//        c l a s s   P u b C h a n n e l
//
////////////////////////////////////////////////

/**
 * \class PubChannel
 * \brief Publication channel to rtEdgeCache3
 *
 * Use the Update class to build and publish an update on this channel. 
 *
 * From Build 72 (June 2024) and above, the following are defaults:
 * - SetBinary( true ) 
 * - SetCircularBuffer( true ) 
 * - SetUnPacked( true ) 
 */

class PubChannel : public Channel
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  Call Start() to connect to rtEdgeCache3.
	 *
	 * The constructor initializes internal variables.  You call Start() 
	 * to connect to the rtEdgeCache3 server.
	 *
	 * \param pubName - Name of this publisher 
	 */
	PubChannel( const char *pubName=(const char *)0 ) :
	   Channel( true ),
	   _pubName( pubName ? pubName : "" ),
	   _hosts(),
	   _authReq(),
	   _authRsp(),
	   _rawLog(),
	   _schema(),
	   _bBinary( true ),
	   _bPerms( false ),
	   _bUserMsgTy( false ),
	   _bRandom( false ),
	   _bUnPacked( true ),
	   _bCircularBuffer( true ),
	   _hopCnt( 0 ),
	   _upd( (Update *)0 )
	{
	   SetCache( true );
	   ::memset( &_attr, 0, sizeof( _attr ) );
	}

	virtual ~PubChannel()
	{
	   Stop();
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/**
	 * \brief Returns comma-separated list of rtEdgeCache3 hosts to
	 * connect this publication channel to.
	 *
	 * This list is defined when you call Start() and is specified as
	 * \<host1\>:\<port1\>,\<host2\>:\<port2\>,...  The library tries to connect
	 * to \<host1\>; If failure, then \<host2\>, etc.  This
	 *
	 * \return Comma-separated list of rtEdgeCache3 hosts.
	 */
	const char *pSvrHosts()
	{
	   return _hosts.data();
	}

	/**
	 * \brief Returns publication service name
	 *
	 * You define the service name in the constructor.
	 *
	 * \return Publication service name
	 */
	const char *pPubName()
	{
	   return _pubName.data();
	}

	/**
	 * \brief Returns authentication token set via SetAuthToken()
	 *
	 * \return Authentication token set via SetAuthToken()
	 */
	const char *pAuthToken()
	{
	   return _authReq.data();
	}

	/**
	 * \brief Returns the (reusable) Update object for this channel.
	 *
	 * Calls CreateUpdate() on first call to this method
	 *
	 * \return Reusable Update object for this channel
	 */
	Update &upd()
	{
	   _upd = !_upd ? CreateUpdate() : _upd;
	   return *_upd;
	}

	/**
	 * \brief Returns Schema for this channel
	 *
	 * This is also returned to you in OnSchema().
	 *
	 * \return Schema for this channel
	 */
	Schema &schema()
	{
	   return _schema;
	}


	////////////////////////////////////
	// PubChannel Operations
	////////////////////////////////////
public:
	/**
	 * \brief Initialize the connection to rtEdgeCache3 server.
	 *
	 * Your application is notified via OnConnect() when you have
	 * successfully connnected and established a session.
	 *
	 * \param pubName - Name of this publisher 
	 * \param hosts - Comma-separated list of rtEdgeCache3 \<host\>:\<port\> to
	 * connect to.
	 * \param bInteractive - true if interactive; false otherwise
	 * \return Textual description of the connection state
	 */
	const char *Start( const char *pubName,
	                   const char *hosts,
	                    bool        bInteractive=true )
	{
	   _pubName = pubName;
	   return Start( hosts, bInteractive );
	}

	/**
	 * \brief Initialize the connection to rtEdgeCache3 server.
	 *
	 * Your application is notified via OnConnect() when you have
	 * successfully connnected and established a session.
	 *
	 * \param hosts - Comma-separated list of rtEdgeCache3 \<host\>:\<port\> to
	 * connect to.
	 * \param bInteractive - true if interactive; false otherwise
	 * \return Textual description of the connection state
	 */
	const char *Start( const char *hosts, bool bInteractive=true )
	{

	   // Pre-condition(s)

	   if ( _cxt )
	      return "Already connected";
	   if ( !hosts )
	      return "No hostname specified";

	   // Initialize our channel

	   _hosts = hosts; 
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   _attr._pSvrHosts       = pSvrHosts();
	   _attr._pPubName        = pPubName();
	   _attr._bInteractive    = bInteractive ? 1 : 0;
	   _attr._bConnectionless = 0;
	   _attr._bCircularBuffer = _bCircularBuffer ? 1 : 0;
	   _attr._udpPort         = 0;
	   _attr._connCbk         = _connCbk;
	   _attr._openCbk         = _openCbk;
	   _attr._closeCbk        = _closeCbk;
	   _attr._permQryCbk      = _permQryCbk;
	   _attr._symQryCbk       = _symQryCbk;
	   _attr._imgQryCbk       = _imgQryCbk;
	   _cxt                   = ::rtEdge_PubInit( _attr );
	   _pubChans.Add( _cxt, this );
	   ::rtEdge_PubInitSchema( _cxt, _schemaCbk );
	   ::rtEdge_ioctl( _cxt, ioctl_binary, (void *)_bBinary );
	   ::rtEdge_ioctl( _cxt, ioctl_setUserPubMsgTy, (void *)_bUserMsgTy );
	   ::rtEdge_ioctl( _cxt, ioctl_randomize, (void *)_bRandom );
	   ::rtEdge_ioctl( _cxt, ioctl_setPubHopCount, (VOID_PTR)_hopCnt );
	   ::rtEdge_ioctl( _cxt, ioctl_enableCache, (void *)_bCache );
	   if ( _bPerms )
	      ::rtEdge_ioctl( _cxt, ioctl_enablePerm, (void *)_bPerms );
	   if ( strlen( pAuthToken() ) )
	      ::rtEdge_ioctl( _cxt, ioctl_setPubAuthToken, (void *)pAuthToken() );
	   SetHeartbeat( _tHbeat );
	   SetIdleCallback( _bIdleCbk );
	   SetUnPacked( _bUnPacked );
	   if ( _rawLog.size() )
	      SetRawLog( _rawLog.data() );
	   return ::rtEdge_PubStart( _cxt );
	}

	/**
	 * \brief Initialize the connectionless publication channel to 
	 * the rtEdgeCache3 server.
	 *
	 * Your application is notified via OnConnect() when the connectionless
	 * channel is ready.  This method automatically configures the channel to:
	 * Parameter | Setting |
	 * --- | --- |
	 * Protocol | BINARY |
	 * CacheType | BROADCAST |
	 *
	 * \param destUDP - Destination UDP \<host\>:\<port\> to publish to.
	 * \param localPort - bind() to this local port; 0 = ephemeral
	 * \return Textual description of the connection state
	 */
	const char *StartConnectionless( const char *destUDP, int localPort=0 )
	{

	   // Pre-condition(s)

	   if ( _cxt )
	      return "Already connected";
	   if ( !destUDP )
	      return "No destination UDP <host>:<port> specified";

	   // Initialize our channel

	   _hosts   = destUDP; 
	   _bBinary = true;
	   _bCache  = true;
	   _bRandom = true;
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   _attr._pSvrHosts       = pSvrHosts();
	   _attr._pPubName        = pPubName();
	   _attr._bInteractive    = 0;
	   _attr._bConnectionless = 1;
	   _attr._bCircularBuffer = _bCircularBuffer ? 1 : 0;
	   _attr._udpPort         = localPort;
	   _attr._connCbk         = _connCbk;
	   _attr._openCbk         = _openCbk;
	   _attr._closeCbk        = _closeCbk;
	   _attr._permQryCbk      = _permQryCbk;
	   _attr._symQryCbk       = _symQryCbk;
	   _attr._imgQryCbk       = _imgQryCbk;
	   _cxt                   = ::rtEdge_PubInit( _attr );
	   _pubChans.Add( _cxt, this );
	   ::rtEdge_PubInitSchema( _cxt, _schemaCbk );
	   ::rtEdge_ioctl( _cxt, ioctl_binary, (void *)_bBinary );
	   SetIdleCallback( _bIdleCbk );
	   SetUnPacked( _bUnPacked );
	   return ::rtEdge_PubStart( _cxt );
	}

	/**
	 * \brief Requests channel be set in binary
	 *
	 * \param bBin - true to set to binary; Else ASCII (default)
	 */
	void SetBinary( bool bBin )
	{
	   _bBinary = bBin;
	}

	/**
	 * \brief Return true if field lists published unpacked; false if packed
	 *
	 * Only valid for binary publication channels
	 *
	 * \return true if field lists published unpacked; false if packed
	 */
	bool IsUnPacked()
	{
	   return _bUnPacked;
	}

	/**
	 * \brief Enables / Disables packing of field lists in Binary
	 *
	 * Only valid for binary publication channels
	 *
	 * \param bUnPacked - true to publish unpacked; Else packed (default)
	 */
	void SetUnPacked( bool bUnPacked )
	{
	   _bUnPacked = bUnPacked;
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, ioctl_unpacked, (void *)_bUnPacked );
	}

	/**
	 * \brief Requests channel be set to supply perms
	 *
	 * The channel perms is set once in Start().  Therefore, this
	 * must be called BEFORE calling Start().
	 *
	 * \param bPerms - true to set to supply perms; Else false (default)
	 */
	void SetPerms( bool bPerms )
	{
	   _bPerms = bPerms;
	}

	/**
	 * \brief Sets outbound buffer type - Circular or Regular
	 *
	 * The buffer type is set once in Start().  Therefore, this
	 * must be called BEFORE calling Start().
	 *
	 * \param bCircularBuffer - true for Circular outbound buffer
	 */
	void SetCircularBuffer( bool bCircularBuffer )
	{
	   if ( !IsValid() )
	      _bCircularBuffer = bCircularBuffer;
	}

	/**
	 * \brief Allow / Disallow user-supplied message type in Publish().
	 *
	 * \param bUserMsgTy - true to use user-supplied message type; false
	 *  to have library determine edg_image or edg_update.  Default is 
	 * false.
	 */
	void SetUserPubMsgTy( bool bUserMsgTy )
	{
	   _bUserMsgTy = bUserMsgTy;
	   if ( IsValid() )
	      ::rtEdge_ioctl( _cxt, ioctl_setUserPubMsgTy, (void *)_bUserMsgTy );
	}

	/**
	 * \brief Requests channel be set in randomize connect mode
	 *
	 * At connect time, the comma-separated list of Edge2 servers you 
	 * specified in the 1st argument to Start() is accessed to try to 
	 * find an Edge2 server to connect to.  The list is accessed as
	 * follows:
	 *
	 * + bRandom = true : Randomly (stochastically)
	 * + bRandom = false : Round-robin (in order)
	 *
	 * Setting to true allows you to have all publishers share a  
	 * common config file, yet have the library stochastically 
	 * distribute the load across all available Edge2 servers.
	 *
	 * \param bRandom - true to set to randomize; false to round robin
	 */
	void SetRandomize( bool bRandom )
	{
	   _bRandom = bRandom;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_randomize, (void *)_bRandom );
	}

	/**
	 * \brief Sets publication channel authentication token
	 *
	 * At connect time, the rtEdgeCache3 server will send an authentication
	 * string which is an MD5-encoded token.  If you wish to publish into
	 * an Edge3 that has been configured as a controlled device, then your 
	 * application must do the following:
	 * + Call SetAuthToken() BEFORE calling Start()
	 * + In OnConnect(), call GetSvrAuthToken() to get the authentication 
	 * token passed by Edge3, or call IsAuthenticated() which compares what 
	 * you set in SetAuthToken() to GetSvrAuthToken().
	 *
	 * \param auth - Authentication token to match against the controlled
	 * device.
	 */
	void SetAuthToken( const char *auth )
	{
	   _authReq = auth;
	}

	/**
	 * \brief Sets publication channel raw log
	 *
	 * \param logFile - Raw Log Filename
	 */
	void SetRawLog( const char *logFile )
	{
	   _rawLog = logFile;
	   if ( IsValid() && _rawLog.size() )
	      ::rtEdge_ioctl( _cxt, ioctl_setRawLog, (void *)_rawLog.data() );
	}


	/**
	 * \brief Gets publication channel authentication token sent by the
	 * Edge3 server configured as a controlled device.
	 *
	 * At connect time, the rtEdgeCache3 server will send an authentication
	 * string which is an MD5-encoded token.  If you wish to publish into
	 * an Edge3 that has been configured as a controlled device, then your
	 * application must do the following:
	 * + Call SetAuthToken() BEFORE calling Start()
	 * + In OnConnect(), call GetSvrAuthToken() to get the authentication 
	 * token passed by Edge3, or call IsAuthenticated() which compares what 
	 * you set in SetAuthToken() to GetSvrAuthToken().
	 *
	 * \return Authentication token from the controlled device.
	 */
	const char *GetSvrAuthToken()
	{
	   char buf[K];

	   buf[0] = '\0'; 
	   ::rtEdge_ioctl( _cxt, ioctl_getPubAuthToken, (void *)buf );
	   _authRsp = buf;
	   return _authRsp.data();
	}

	/**
	 * \brief Returns true if our authentication token set via SetAuthToken()
	 * matches that sent by the controlled device.
	 *
	 * \return true if our authentication token set via SetAuthToken()
	 * matches that sent by the controlled device.
	 */
	bool IsAuthenticated()
	{
	   const char *req = pAuthToken();
	   const char *rsp = GetSvrAuthToken();
	   size_t      qSz = strlen( req );
	   size_t      rSz = strlen( rsp );

	   /*
	    * 1) Not set by user : OK (Not required)
	    * 2) Not set by Edge3 : OK (Older than Build 21)
	    */
	   if ( !qSz  || !rSz )
	      return true;
	   return ( qSz == rSz ) ? !::strcmp( req, rsp ) : false;
	}

	/**
	 * \brief Publish a single update from a field list
	 *
	 * \param d - Filled in rtEdgeData struct to publish
	 * \return  Number of bytes published; 0 if overflow
	 */
	int Publish( rtEdgeData d )
	{
	   int rtn;

	   if ( !(rtn=::rtEdge_Publish( _cxt, d )) )
	      OnOverflow();
	   return rtn;
	}

	/**
	 * \brief Publish a Symbol List (BDS)
	 *
	 * \param bds - BDS name
	 * \param StreamID - Unique Stream ID from OnOpenBDS()
	 * \param symbols - NULL-terminated list of symbols
	 * \return  Number of bytes published; 0 if overflow
	 */
	int PublishBDS( const char *bds, int StreamID, char **symbols )
	{
	   std::string tkr( _BDS_PFX );
	   rtEdgeData  d;
	   rtFIELD     flds[K], f;
	   rtBUF      &b = f._val._buf;
	   int         i, nf, nb;

	   // LONGLINK1 -> LONGLINK1+_NUM_LINK

	   tkr += bds;
	   ::memset( &d, 0, sizeof( d ) );
	   d._pSvc = pPubName();
	   d._pTkr = tkr.data();
	   d._arg  = (VOID_PTR)StreamID;
	   d._ty   = edg_image;
	   d._flds = flds;
	   for ( i=0,nf=0,nb=0; symbols[i]; i++ ) {
	      f._fid   = LONGLINK1+nf;
	      f._type  = rtFld_string;
	      b._data  = (char *)symbols[i];
	      b._dLen  = strlen( b._data );
	      flds[nf] = f;
	      nf      += 1;
	      if ( nf == _NUM_LINK ) {
	         d._nFld = nf;
	         nb     += Publish( d );
	         d._ty  = edg_update;
	         nf     = 0;
	      }
	   }
	   /*
	    * 23-10-24 jcs Build 66
	    */
	   if ( !nf ) {
	      f._fid   = DSPLY_NAME;
	      f._type  = rtFld_string;
	      b._data  = _EmptyBDS;
	      b._dLen  = strlen( b._data );
	      flds[nf] = f;
	      nf      += 1;
	   }
	   if ( nf ) {
	      d._nFld = nf;
	      nb     += Publish( d );
	   }
	   return nb;
	}

	/**
	 * \brief Publish update with a pre-built payload requiring MsgHdr
	 *
	 * \param d - Filled in rtEdgeData struct to publish
	 * \param b - Pre-built payload requiring MsgHdr
	 * \param dt - Payload data type
	 * \return  Number of bytes published; 0 if overflow
	 */
	int Publish( rtEdgeData d, rtBUF b, mddDataType dt=mddDt_FieldList )
	{
	   rtPreBuiltBUF pb;
	   int           rtn;

	   pb._payload  = b;
	   pb._dataType = dt;
	   pb._bHasHdr  = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_setPubDataPayload, &pb );
	   if ( !(rtn=::rtEdge_Publish( _cxt, d )) )
	      OnOverflow();
	   return rtn;
	}

	/**
	 * \brief Publish update with a pre-built payload containing MsgHdr 
	 *
	 * \param d - Filled in rtEdgeData struct to publish
	 * \param b - Pre-built payload containing MsgHdr
	 * \return  Number of bytes published; 0 if overflow
	 */
	int PublishRaw( rtEdgeData d, rtBUF b )
	{
	   rtPreBuiltBUF pb;
	   int           rtn;

	   pb._payload  = b;
	   pb._dataType = mddDt_undef;
	   pb._bHasHdr  = 1;
	   ::rtEdge_ioctl( _cxt, ioctl_setPubDataPayload, &pb );
	   if ( !(rtn=::rtEdge_Publish( _cxt, d )) )
	      OnOverflow();
	   return rtn;
	}

	/**
	 * \brief Publish a DEAD status message
	 *
	 * After publishing this message, the data stream is closed.
	 *
	 * \param d - Filled in rtEdgeData struct to publish
	 * \param err - Error message
	 * \return  Number of bytes published; 0 if overflow
	 */
	int PubError( rtEdgeData d, const char *err )
	{
	   int rtn;

	   d._pErr = err;
	   if ( !(rtn=::rtEdge_PubError( _cxt, d )) )
	      OnOverflow();
	   return rtn;
	}

	/**
	 * \brief Publish a response to a perm query you processed via 
	 * OnPermQuery().
	 *
	 * \param tuple - Pointer to ( Service, Ticker, User, Location ) tuple
	 * \param reqID - Unique query / request ID
	 * \param bAck - true if permissioned; false otherwise
	 * \param err - Textual error message if bAck is false
	 * \return  Number of bytes published; 0 if overflow
	 */
	int PubPermResponse( const char **tuple, 
	                     int          reqID, 
	                     bool         bAck, 
	                     const char  *err )
	{
	   rtEdgeData   d;
	   mddFieldList fl;
	   int          i, fid, rtn;

	   /*
	    * Special packing in rtEdgeData; 1st 7 fields:
	    *       svc   (str)
	    *       tkr   (str)
	    *       usr   (str)
	    *       loc   (str)
	    *       err   (str)
	    *       reqID (i17)
	    *       bAck  (i8)
	    */
	   fl  = ::mddFieldList_Alloc( 32 );
	   for ( i=0,fid=6120; i<4; i++,fid++ )
	      mddWire_AddStringZ( fl, fid, (char *)tuple[i] );
	   mddWire_AddStringZ( fl, fid++, (char *)err );
	   mddWire_AddInt32( fl, fid++, reqID );
	   mddWire_AddInt8( fl, fid++, bAck ? 1 : 0 );

	   // 2) Pack up rtEdgeData; Publish

	   ::memset( &d, 0, sizeof( d ) );
	   d._ty   = edg_permQuery;
	   d._flds = (rtFIELD *)fl._flds;
	   d._nFld = fl._nFld;
	   if ( !(rtn=::rtEdge_Publish( _cxt, d )) )
	      OnOverflow();
	   ::mddFieldList_Free( fl );
	   return rtn;
	}

	/**
	 * \brief Sets the minimum hop count for the publication channel
	 * created by rtEdge_PubInit().  Default is 0
	 *
	 * \param hopCnt - Minimum hop count
	 */
	void PubSetHopCount( int hopCnt )
	{
	   _hopCnt = hopCnt;
	}

	/**
	 * \brief Gets the minimum hop count set by PubSetHopCount().
	 *
	 * \return Minimum hop count set via PubSetHopCount()
	 */
	int PubGetHopCount()
	{
	   int rtn;

	   rtn = 0;
	   ::rtEdge_ioctl( _cxt, ioctl_getPubHopCount, (void *)&rtn );
	   return rtn;
	}

	/**
	 * \brief Retrieve last published message on this publication channel.
	 *
	 * There is one publication buffer per channel / context.  Therefore
	 * YOU MUST SYNCHRONIZE ACCESS TO THIS INTERNAL BUFFER if you are 
	 * publishing from 2+ threades.  If your application only calls 
	 * rtEdge_Publish() from one thread, then a call to rtEdge_PubGetData() 
	 * is guaranteed to return the message just published.
	 *
	 * \return rtBUF pointing to last published message.
	 */
	rtBUF PubGetData()
	{
	   return ::rtEdge_PubGetData( _cxt );
	}


	////////////////////////////////////
	// RTEDGE::Channel Interface
	////////////////////////////////////
	/**
	 * \brief Destroy connection to the rtEdgeCache3
	 *
	 * Calls ::rtEdge_Destroy() to disconnect from the rtEdgeCache3 server.
	 * Upon return, MD-Direct Subscribers will see this pPubName() service
	 * as DOWN.
	 */
	virtual void Stop()
	{
	   _pubChans.Remove( _cxt );
	   Channel::Stop();
	}


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously when we connect or disconnect from
	 * the rtEdgeCache3.
	 *
	 * Override this method in your application to take action when
	 * your connect or disconnect from the rtEdgeCache3.
	 *
	 * \param msg - Textual description of connection state
	 * \param bUP - true if connected; false if disconnected
	 */
	virtual void OnConnect( const char *msg, bool bUP )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 opens a new
	 * real-time stream from this service on behalf of a subscriber(s).
	 *
	 * Override this method in your application to open and update a
	 * real-time market data stream to the rtEdgeCache3 server.  The
	 * 2nd argument is the unique Stream ID that must be returned in 
	 * the rtEdge::_arg field on each call to Publish().  If using 
	 * the Update class, you pass the 2nd argument into the Update::Init()
	 * call.
	 *
	 * \param tkr - Ticker
	 * \param tag - User-defined tag to be returned in the rtEdge::_arg
	 * field on every call to Publish() for this stream
	 */
	virtual void OnPubOpen( const char *tkr, void *tag )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 close an 
	 * existing real-time stream from this service.
	 *
	 * Override this method in your application to stop publishing 
	 * updates to this real-time stream.
	 *
	 * \param tkr - Ticker
	 */
	virtual void OnPubClose( const char *tkr )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 opens a new
	 * Broadcast Data Stream (BDS) from this publisher.
	 *
	 * Override this method in your application to publish as Symbol List
	 *
	 * \param bds - BDS Name
	 * \param tag - User-defined tag to be returned in the rtEdge::_arg
	 * field on every call to Publish() for this stream
	 */
	virtual void OnOpenBDS( const char *bds, void *tag )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 closes an existing
	 * BDS stream from this service.
	 *
	 * Override this method in your application to stop publishing
	 * updates to this BDS stream.
	 *
	 * \param tkr - Ticker
	 */
	virtual void OnCloseBDS( const char *tkr )
	{ ; }

	/**
	 * \brief Called asynchronously when data dictionary arrives
	 * on this subscription channel from rtEdgeCache3
	 *
	 * Override this method in your application to process the schema.
	 *
	 * \param sch - New Schema
	 */
	virtual void OnSchema( Schema &sch )
	{ ; }

	/**
	 * \brief Called asynchronously when Publish() overflows outbound
	 * buffer to rtEdgeCache3.
	 *
	 * The outbound queue consumes Publish() and PubError() messages in 
	 * an all-or-nothing manner.  Therefore, a "lazy" application may 
	 * overflow the queue and simply continue on happily calling Publish()
	 * or PubError() which will be dropped - and OnOverflow() called - 
	 * until Edge2 drains the outbound queue.
	 *
	 * Override this method in your application to take action when
	 * your outbound queue overflows.
	 */
	virtual void OnOverflow()
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 requests 
	 * permission info for a specific ( Service, Ticker, User, Location ).
	 * You respond with PubPermResponse()
	 *
	 * Override this method in your application to supply permissions 
	 * info on a highly granular basis.
	 *
	 * \param tuple - Pointer to ( Service, Ticker, User, Location ) tuple
	 * \param reqID - Unique query / request ID
	 */
	virtual void OnPermQuery( const char **tuple, int reqID )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 requests 
	 * the Published Symbol List from your connectionless publisher
	 * 
	 * Override this method in your application to be notified when 
	 * your librtEdge library supplies the Symbol List to the rtEdgeCache3
	 * data distributor.
	 *
	 * \param nSym - Number of Symbols returned by librtEdge
	 */
	virtual void OnSymListQuery( int nSym )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 requests 
	 * a Refresh Image from your connectionless publisher
	 * 
	 * Override this method in your application to publish a Refresh 
	 * Image to the rtEdgeCache3 server in order to keep the data
	 * for this ticker up to date in the Edge3 cache.
	 *
	 * \param tkr - Ticker
	 * \param StreamID - User-defined tag to be returned in the rtEdge::_arg
	 * field on every call to Publish() for this stream
	 */
	virtual void OnRefreshImage( const char *tkr, void *StreamID )
	{ ; }


	////////////////////////////////////
	// Asynchronous Callback (private)
	////////////////////////////////////
private:
	void _OnSchema( rtEdgeData d )
	{
	   Message msg( 0 );

	   _schema.Initialize( msg.Set( &d, (mddFieldList *)0 ) );
	   OnSchema( _schema );
	}


	////////////////////////////////////
	// PubChannel Interface
	////////////////////////////////////
protected:
	/**
	 * \brief Called by upd() to create an application-specific Update
	 * object.
	 *
	 * Your application MUST implement this method for your class to compile.
	 * You may use the standard Update class if you like.  For application-
	 * specific methods, derive a sub-class of Update and instantiate in 
	 * your CreateUpdate() implementation.
	 *
	 * \return Reusable Update object for this channel
	 */
	virtual Update *CreateUpdate() = 0;


	////////////////////////
	// Private Members
	////////////////////////
private:
	std::string   _pubName;
	std::string   _hosts;
	std::string   _authReq;
	std::string   _authRsp;
	std::string   _rawLog;
	Schema        _schema;
	rtEdgePubAttr _attr;
	bool          _bBinary;
	bool          _bPerms;
	bool          _bUserMsgTy;
	bool          _bRandom;
	bool          _bUnPacked;
	bool          _bCircularBuffer;
	size_t        _hopCnt;
	Update       *_upd;

	////////////////////////////////////
	// Class-wide (private) callbacks
	////////////////////////////////////
private:
	static void EDGAPI _connCbk( rtEdge_Context cxt, 
	                             const char    *msg, 
	                             rtEdgeState    state )
	{
	   PubChannel *us;

	   if ( (us=_pubChans.Get( cxt )) )
	      us->OnConnect( msg, ( state == edg_up ) );
	}

	static void EDGAPI _openCbk( rtEdge_Context cxt, 
	                             const char    *tkr, 
	                             void          *arg )
	{
	   PubChannel *us;
	   const char *bds;

	   // tkr == NULL implies OnIdle()

	   if ( (us=_pubChans.Get( cxt )) ) {
	      if ( !tkr )
	         us->OnIdle();
	      else if ( ::strstr( tkr, _BDS_PFX ) == tkr ) {
	         bds = tkr + strlen( _BDS_PFX );
	         us->OnOpenBDS( bds, arg );
	      }
	      else
	         us->OnPubOpen( tkr, arg );
	   } 
	} 

	static void EDGAPI _closeCbk( rtEdge_Context cxt, const char *tkr )
	{
	   PubChannel *us;
	   const char *bds;

	   if ( (us=_pubChans.Get( cxt )) ) {
	      if ( ::strstr( tkr, _BDS_PFX ) == tkr ) {
	         bds = tkr + strlen( _BDS_PFX );
	         us->OnCloseBDS( bds );
	      }
	      else
	         us->OnPubClose( tkr );
	   }
	}

	static void EDGAPI _schemaCbk( rtEdge_Context cxt, rtEdgeData d )
	{ 
	   PubChannel *us;

	   if ( (us=_pubChans.Get( cxt )) )
	      us->_OnSchema( d );
	} 

	static void EDGAPI _permQryCbk( rtEdge_Context cxt,
	                                const char   **tuple,
	                                int            reqID )
	{
	   PubChannel *us;

	   if ( (us=_pubChans.Get( cxt )) )
	      us->OnPermQuery( tuple, reqID );
	}

	static void EDGAPI _symQryCbk( rtEdge_Context cxt,
	                               int            nSym )
	{
	   PubChannel *us;

	   if ( (us=_pubChans.Get( cxt )) )
	      us->OnSymListQuery( nSym );
	}

	static void EDGAPI _imgQryCbk( rtEdge_Context cxt,
	                               const char    *tkr,
	                               void          *StreamID )
	{
	   PubChannel *us;

	   if ( (us=_pubChans.Get( cxt )) )
	      us->OnRefreshImage( tkr, StreamID );
	}

};  // class PubChannel

} // namespace RTEDGE

#endif // __RTEDGE_PubChannel_H 
