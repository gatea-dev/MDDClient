/******************************************************************************
*
*  PubChannel.h
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*      7 JAN 2015 jcs  Build 29: ByteStreamFld
*     28 FEB 2015 jcs  Build 30: SetHeartbeat()
*      6 JUL 2015 jcs  Build 31: GetSocket(); OnOverflow()
*     16 APR 2016 jcs  Build 32: SetUserPubMsgTy(); OnIdle()
*     14 JUL 2017 jcs  Build 34: class Channel
*     29 APR 2020 jcs  Build 43: BDS
*     30 MAR 2022 jcs  Build 52: SetUnPacked()
*     26 APR 2022 jcs  Build 53: 1 constructor
*     23 MAY 2022 jcs  Build 54: OnError()
*      1 SEP 2022 jcs  Build 56: pSvrHosts()
*     23 OCT 2022 jcs  Build 58: cli::array<>
*     26 OCT 2023 jcs  Build 65: BDSSymbolList
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#include <ByteStream.h>
#include <Schema.h>
#endif // DOXYGEN_OMIT

#ifndef DOXYGEN_OMIT

namespace librtEdgePRIVATE
{

////////////////////////////////////////////////
//
// c l a s s   I r t E d g e P u b l i s h e r
//
////////////////////////////////////////////////
/**
 * \class IrtEdgePublisher
 * \brief Abstract class to handle publication channel events 
 * such as OnConnect() and OnPubOpen().
 */
public interface class IrtEdgePublisher
{
	// IrtEdgePublisher Publish Interface

	virtual int             GetFid( String ^ ) abstract;
	virtual RTEDGE::Update &upd() abstract;

	// IrtEdgePublisher Asynch Interface
public:
	virtual void OnConnect( String ^, librtEdge::rtEdgeState ) abstract;
	virtual void OnOpen( String ^, IntPtr ) abstract;
	virtual void OnClose( String ^ ) abstract;
	virtual void OnOpenBDS( String ^, IntPtr ) abstract;
	virtual void OnCloseBDS( String ^ ) abstract;
	virtual void OnSchema( librtEdge::rtEdgeSchema ^ ) abstract;
	virtual void OnOverflow() abstract;
	virtual void OnIdle() abstract;
	virtual void OnError( String ^ ) abstract;
	virtual void OnSymListQuery( int ) abstract;
	virtual void OnRefreshImage( String ^, IntPtr ) abstract;
};


////////////////////////////////////////////////
//
//     c l a s s   P u b C h a n n e l
//
////////////////////////////////////////////////

/**
 * \class PubChannel
 * \brief RTEDGE::PubChannel sub-class to hook 3 virtual methods
 * from native librtEdge library and dispatch to .NET consumer. 
 */ 
class PubChannel : public RTEDGE::PubChannel
{
private:
	gcroot < IrtEdgePublisher^ >         _cli;
	gcroot < librtEdge::rtEdgeSchema ^ > _schema;

	// Constructor
public:
	/**
	 * \brief Constructor for class to hook native events from 
	 * native librtEdge library and pass to .NET consumer via
	 * the IrtEdgePublisher interface.
	 *
	 * \param cli - Event receiver - IrtEdgePublisher::OnPubOpen(), etc.
	 * \param pubName - Publication name
	 */
	PubChannel( IrtEdgePublisher ^cli, const char *pubName );
	~PubChannel();

	// Schema Stuff

	librtEdge::rtEdgeSchema ^schema();

	// RTEDGE::PubChannel Interface

	virtual RTEDGE::Update *CreateUpdate();

	// Asynchronous Callbacks
protected:
	virtual void OnConnect( const char *msg, bool bUP );
	virtual void OnPubOpen( const char *tkr, void *arg );
	virtual void OnPubClose( const char *tkr );
	virtual void OnOpenBDS( const char *tkr, void *arg );
	virtual void OnCloseBDS( const char *tkr );
	virtual void OnSchema( RTEDGE::Schema &sch );
	virtual void OnOverflow();
	virtual void OnIdle();
	virtual void OnError( const char *err );
	virtual void OnSymListQuery( int nSym );
	virtual void OnRefreshImage( const char *tkr, void *StreamID );
};
} // namespace librtEdgePRIVATE

#endif // DOXYGEN_OMIT



namespace librtEdge
{

////////////////////////////////////////////////
//
//  c l a s s   r t E d g e P u b l i s h e r
//
////////////////////////////////////////////////
/**
 * \class rtEdgePublisher
 * \brief Publication channel to rtEdgeCache3.
 *
 * This is the main foundational class in your C# consumer 
 * application.  You derive from this class and override
 * the following with your business logic:
 * + OnConnect()
 * + OnOpen()
 * + OnClose()
 * + OnSchema()
 * + OnOverflow()
 * + OnIdle()
 *
 * \include rtEdgePublisher_override.h
 */
public ref class rtEdgePublisher : public librtEdge::Channel,
                                   public librtEdgePRIVATE::IrtEdgePublisher 
{
private: 
	String                       ^_hosts;
	librtEdgePRIVATE::PubChannel *_pub;
protected: 
	bool                          _bInteractive;
	bool                          _bSchema;
	bool                          _bBinary;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.  Calls PubStart() to connect to rtEdgeCache3.
	 *
	 * The constructor initializes internal variables.  You call PubStart() 
	 * to connect to the rtEdgeCache3 server.
	 *
	 * The following are equivalent:
	 * \include rtEdgePublisher_constructor1.h
	 * \include rtEdgePublisher_constructor2.h
	 *
	 * \param hosts - Comma-separated list of rtEdgeCache3 \<host\>:\<port\> 
	 * to connect to. 
	 * \param pubName - Publication service name
	 * \param bBinary - true for binary; false for ASCII (MarketFeed) protocol
	 * \param bStart - true to call PubStart()
	 */
	rtEdgePublisher( String ^hosts, String ^pubName, bool bBinary, bool bStart );

	/**
	 * \brief Destructor.  Calls Stop() to disconnect; Cleans up internally.
	 */
	~rtEdgePublisher();


	/////////////////////////////////
	// Operations
	/////////////////////////////////
public:
	/**
	 * \brief Initialize the connection to rtEdgeCache3 server.
	 *
	 * Your application is notified via OnConnect() when you have 
	 * successfully connnected and established a session.
	 *
	 * \return Textual description of the connection state
	 */
	String ^PubStart();

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
	 * \return Textual description of the connection state
	 */
	String ^PubStartConnectionless();

	/**
	 * \brief Destroy connection to the rtEdgeCache3
	 */
	void Stop();

	/**
	 * \brief Requests channel be set in binary
	 *
	 * The channel protocol is set once in PubStart().  Therefore, this
	 * must be called BEFORE calling PubStart().
	 *
	 * \param bBin - true to set to binary; Else ASCII (default)
	 */
	void SetBinary( bool bBin );

	/**
	 * \brief Enables / Disables packing of field lists in Binary
	 *
	 * Only valid for binary publication channels
	 *
	 * \param bUnPacked - true to publish unpacked; Else packed (default)
	 */
	void SetUnPacked( bool bUnPacked );

	/**
	 * \brief Requests channel be set to supply perms
	 *
	 * The channel perms is set once in Start().  Therefore, this
	 * must be called BEFORE calling Start().
	 *
	 * \param bPerms - true to set to supply perms; Else false (default)
	 */
	void SetPerms( bool bPerms );

	/**
	 * \brief Allow / Disallow user-supplied message type in Publish().
	 *
	 * \param bUserMsgTy - true to use user-supplied message type; false
	 *  to have library determine edg_image or edg_update.  Default is
	 * false.
	 */
	void SetUserPubMsgTy( bool bUserMsgTy );

	/**
	 * \brief Requests channel be set in randomize connect mode
	 *
	 * At connect time, the comma-separated list of Edge2 servers you
	 * specified in the 1st argument to PubStart() is accessed to try to
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
	void SetRandomize( bool bRandom );

	/**
	 * \brief Sets channel heartbeat / ping interval in seconds.
	 *
	 * At connect time, this value is passed to the rtEdgeCache3 server.
	 * The server sends a Ping request at this interval, which the library
	 * returns.  If the server does not get a message from the library
	 * in twice this interval, the connection is terminated by the
	 * server.
	 *
	 * This must be called BEFORE calling PubStart() for it to take effect.
	 * If not called, the library default of 3600 (1 hour) is used.
	 *
	 * \param tHbeat - Heartbeat interval in seconds.
	 */
	void SetHeartbeat( int tHbeat );

	/**
	 * \brief Allow / Disallow the library timer to call out to OnIdle()
	 * every second or so.
	 *
	 * This is useful for performing tasks in the librtEdge thread
	 * periodically, for example if you need to handle OnOverflow() 
	 * events ONLY from the publication channel thread when it is 
	 * dormant (not dispatching events).
	 *
	 * \param bIdleCbk - true to receive OnIdle(); false to disable
	 */
	void SetIdleCallback( bool bIdleCbk );


	/////////////////////////////////
	// Access
	/////////////////////////////////
public:
#ifndef DOXYGEN_OMIT
	RTEDGE::PubChannel &cpp() { return *_pub; }
#endif // DOXYGEN_OMIT

	/**
	 * \brief Return socket file descriptor for this Channel
	 *
	 * \return Socket file descriptor for this Channel
	 */
	int GetSocket();

	/**
	 * \brief Set SO_RCVBUF for this QuoddFeed channel
	 *
	 * \param bufSiz - SO_RCVBUF size
	 * \return  GetRxBufSize()
	 */
	int SetRxBufSize( int bufSiz );

	/**
	 * \brief Get SO_RCVBUF for this QuoddFeed channel
	 *
	 * \return SO_RCVBUF size
	 */
	int GetRxBufSize();

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
	 * \brief Returns comma-separated list of rtEdgeCache3 hosts to
	 * connect this publication channel to.
	 *
	 * This list is defined when you call Start() and is specified as
	 * \<host1\>:\<port1\>,\<host2\>:\<port2\>,...  The library tries to connect
	 * to \<host1\>; If failure, then \<host2\>, etc.  This
	 *
	 * \return Comma-separated list of rtEdgeCache3 hosts.
	 */
	String ^pSvrHosts();

	/**
	 * \brief Returns publication name of this service
	 *
	 * \return Publication name of this service
	 */
	String ^pPubName();

	/**
	 * \brief Returns true if channel is Binary
	 *
	 * \return true if channel is Binary
	 */
	bool IsBinary();

	/**
	 * \brief Return true if field lists published unpacked; false if packed
	 *
	 * Only valid for binary publication channels
	 *
	 * \return true if field lists published unpacked; false if packed
	 */
	bool IsUnPacked();

	/**
	 * \brief Sets the minimum hop count for the publication channel
	 * created by rtEdge_PubInit().  Default is 0
	 *
	 * \param hopCnt - Minimum hop count
	 */
	void PubSetHopCount( int hopCnt );

	/**
	 * \brief Gets the minimum hop count set by PubSetHopCount().
	 *
	 * \return Minimum hop count set via PubSetHopCount()
	 */
	int PubGetHopCount();

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
	cli::array<Byte> ^PubGetData();


	/////////////////////////////////
	// IrtEdgePublisher Interface - Publish
	/////////////////////////////////
public:
	/**
	 * \brief Return FieldID from Schema based on Field Name
	 *
	 * \param fldName - Field Name
	 * \return Field ID
	 */
	virtual int GetFid( String ^fldName );

	/**
	 * \brief Return underlying RTEDGE::Update object
	 * \return Underlying RTEDGE::Update object
	 */
	virtual RTEDGE::Update &upd() { return _pub->upd(); }


	/////////////////////////////////
	// BDS Publication
	/////////////////////////////////
	/**
	 * \brief Publish a Symbol List (BDS)
	 *
	 * \param bds - BDS name
	 * \param StreamID - Unique Stream ID from OnOpenBDS()
	 * \param symbols - Array of symbols to publish
	 * \return  Number of bytes published; 0 if overflow
	 */
	int PublishBDS( String ^bds, int StreamID, cli::array<String ^> ^symbols );


	/////////////////////////////////
	// IrtEdgePublisher Asynch interface
	/////////////////////////////////
public:
	/**
	 * \brief Called asynchronously when we connect or disconnect from
	 * rtEdgeCache3.
	 *
	 * Override this method in your C# application to take action when
	 * your connect or disconnect from the rtEdgeCache3.
	 *
	 * \param msg - Textual description of connection state
	 * \param state - Connection state : Up or Down
	 */
	virtual void OnConnect( String ^msg, rtEdgeState state )
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
	virtual void OnOpen( String ^tkr, IntPtr tag )
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
	virtual void OnClose( String ^tkr )
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
	virtual void OnOpenBDS( String ^bds, IntPtr  tag )
	{ ; }

	/**
	 * \brief Called asynchronously whenever rtEdgeCache3 closes an existing
	 * BDS stream from this service.
	 *
	 * Override this method in your application to stop publishing
	 * updates to this BDS stream.
	 *
	 * \param bds - Symbol List Name to close
	 */
	virtual void OnCloseBDS( String ^bds )
	{ ; }

	/**
	 * \brief Called asynchronously when data dictionary arrives
	 * on this publication channel from rtEdgeCache3
	 *
	 * Override this method in your C# application to process the schema.
	 *
	 * \param schema - rtEdgeSchema
	 */
	virtual void OnSchema( librtEdge::rtEdgeSchema ^schema )
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
	 * \brief Called asynchronously - roughly once per second - when the
	 *
	 * \param err - Error Message
	 */
	virtual void OnError( String ^err )
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
	 * \param StreamID - User-defined tag to be returned in rtEdge::_arg
	 * field on every call to Publish() for this stream
	 */
	virtual void OnRefreshImage( String ^tkr, IntPtr StreamID )
	{ ; }


	/////////////////////////////////
	// librtEdgeDotNet Compatibility
	/////////////////////////////////
public:
	/** \brief Not used : librtEdgeDotNet Compatibility */
	void SetBestPerformance() { ; } 
	/** \brief Not used : librtEdgeDotNet Compatibility */
	void SetBuildInDotNet( bool ) { ; } 
	/** \brief Not used : librtEdgeDotNet Compatibility */
	void SetNativeFields( bool ) { ; } 

};  // class rtEdgePublisher

#ifndef DOXYGEN_OMIT

public ref class BDSSymbolList : public StringDoor
{
private:
	vector<char *> _tkrs;

	//////////////////////////////////
	// Constructor / Destructor
	//////////////////////////////////
public:
	BDSSymbolList() :
	   StringDoor(),
	   _tkrs()
	{ ; }

	//////////////////////////////////
	// Access / Operations
	//////////////////////////////////
public:
	char **tkrs() 
	{
	   return _tkrs.data();
	}

	void Add( String ^tkr )
	{
	   _tkrs.push_back( _pStr( tkr ) );
	} 

	void EOF()
	{
	   _tkrs.push_back( (char *)0 );
	} 

}; // class BDSSymbolList

#endif // DOXYGEN_OMIT

} // namespace librtEdge
