/******************************************************************************
*
*  SubChannel.h
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*     13 NOV 2014 jcs  QueryCache()
*      7 JAN 2015 jcs  Build 29: Chain
*      6 JUL 2015 jcs  Build 31: GetSocket()
*     15 APR 2016 jcs  Build 32: IsSnapshot(); OnDead(); OnIdle()
*     14 JUL 2017 jcs  Build 34: class Channel
*     13 OCT 2017 jcs  Build 36: Tape
*     29 APR 2020 jcs  Build 43: BDS
*     10 SEP 2020 jcs  Build 44: SetTapeDirection(); Query()
*     30 SEP 2020 jcs  Build 45: Parse() / ParseView()
*      3 DEC 2020 jcs  Build 47: XxxxPumpFullTape()
*     23 MAY 2022 jcs  Build 54: OnError()
*     22 SEP 2022 jcs  Build 56: Rename StartTape() to PumpTape()
*     24 OCT 2022 jcs  Build 58: Opaque cpp()
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <librtEdge.h>
#include <rtEdge.h>
#include <ByteStream.h>
#include <Chain.h>
#include <ChartDB.h>
#include <Data.h>
#include <Schema.h>
#endif // DOXYGEN_OMIT

#ifndef DOXYGEN_OMIT

namespace librtEdgePRIVATE
{
////////////////////////////////////////////////
//
// c l a s s   I r t E d g e S u b s c r i b e r
//
////////////////////////////////////////////////
/**
 * \class IrtEdgeSubscriber
 * \brief Abstract class to handle subscriber channel events
 * such as OnConnect() and OnData().
 */
public interface class IrtEdgeSubscriber
{
	// IrtEdgeSubscriber Interface
public:
	virtual void OnConnect( String ^, librtEdge::rtEdgeState ) abstract;
	virtual void OnService( String ^, librtEdge::rtEdgeState ) abstract;
	virtual void OnData( librtEdge::rtEdgeData ^ ) abstract;
	virtual void OnRecovering( librtEdge::rtEdgeData ^ ) abstract;
	virtual void OnStale( librtEdge::rtEdgeData ^ ) abstract;
	virtual void OnDead( librtEdge::rtEdgeData ^, String ^ ) abstract;
	virtual void OnStreamDone( librtEdge::rtEdgeData ^ ) abstract;
	virtual void OnSymbol( librtEdge::rtEdgeData ^, String ^ ) abstract;
	virtual void OnSchema( librtEdge::rtEdgeSchema ^ ) abstract;
	virtual void OnIdle() abstract;
	virtual void OnError( String ^ ) abstract;
};


////////////////////////////////////////////////
//
//     c l a s s   S u b C h a n n e l
//
////////////////////////////////////////////////

/**
 * \class SubChannel
 * \brief RTEDGE::SubChannel sub-class to hook 4 virtual methods
 * from native librtEdge library and dispatch to .NET consumer. 
 */ 
class SubChannel : public RTEDGE::SubChannel
{
private:
	gcroot < IrtEdgeSubscriber^ >       _cli;
	gcroot < librtEdge::rtEdgeSchema^ > _schema;
	gcroot < librtEdge::rtEdgeData^ >   _upd;

	// Constructor
public:
	/**
	 * \brief Constructor for class to hook native events from 
	 * native librtEdge library and pass to .NET consumer via
	 * the IrtEdgeSubscriber interface.
	 *
	 * \param cli - Event receiver - IrtEdgeSubscriber::OnData(), etc.
	 */
	SubChannel( IrtEdgeSubscriber ^cli );
	~SubChannel();

	// Access

	librtEdge::rtEdgeData   ^data();
	librtEdge::rtEdgeSchema ^schema();

	// Asynchronous Callbacks
protected:
	virtual void OnConnect( const char *msg, bool );
	virtual void OnService( const char *msg, bool );
	virtual void OnData( RTEDGE::Message &msg );
	virtual void OnRecovering( RTEDGE::Message &msg );
	virtual void OnStale( RTEDGE::Message &msg );
	virtual void OnDead( RTEDGE::Message &msg, const char *pErr );
	virtual void OnStreamDone( RTEDGE::Message &msg );
	virtual void OnSymbol( RTEDGE::Message &msg, const char *sym );
	virtual void OnSchema( RTEDGE::Schema &sch );
	virtual void OnIdle();
	virtual void OnError( const char *err );
};
} // namespace librtEdgePRIVATE

#endif // DOXYGEN_OMIT



namespace librtEdge
{

////////////////////////////////////////////////
//
//  c l a s s   r t E d g e S u b s c r i b e r
//
////////////////////////////////////////////////
/**
 * \class rtEdgeSubscriber
 * \brief Subscription channel from data source - rtEdgeCache3 or Tape File
 *
 * The 1st argument to Start() defines your data source:
 * 1st Arg | Data Source | Data Type
 * --- | --- | ---
 * host:port | rtEdgeCache3 | Streaming real-time data
 * filename | Tape File | Recorded market data from tape
 *
 * This class ensures that data from both sources - rtEdgeCache3 and Tape
 * File - is streamed into your application in the exact same manner:
 * + Subscribe()
 * + OnData()
 * + OnDead()
 * + Unsubscribe()
 *
 * Lastly, the Tape File data source is specifically driven from this class:
 * API | Action
 * --- | ---
 * PumpTape() | Pump data for Subscribe()'ed tkrs until end of file
 * PumpTapeSlice() | Pump data for Subscribe()'ed tkrs in a time interval
 * StopTape() | Stop Tape Pump
 *
 * \include rtEdgeSubscriber_override.h
 */
public ref class rtEdgeSubscriber : public librtEdge::Channel,
                                    public librtEdgePRIVATE::IrtEdgeSubscriber 
{
private: 
	librtEdgePRIVATE::SubChannel *_sub;
	String                       ^_hosts;
	String                       ^_user;
	bool                          _bBinary;
	rtEdgeData                   ^_qry;
	rtEdgeField                  ^_fldGet;
	::MDDResult                  *_qryAll;
	rtEdgeData                   ^_parse;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.  Call Start() to connect to rtEdgeCache3.
	 *
	 * The constructor initializes internal variables, including reusable 
	 * data objects passed to the OnData().  You call Start() to connect 
	 * to the rtEdgeCache3 server.
	 *
	 * \param hosts - Comma-separated list of rtEdgeCache3 \<host\>:\<port\> 
	 * to connect to. 
	 * \param user - rtEdgeCache3 username
	 */
	rtEdgeSubscriber( String ^hosts, String ^user );

	/**
	 * \brief Constructor.  Call Start() to connect to rtEdgeCache3.
	 *
	 * The constructor initializes internal variables, including reusable 
	 * data objects passed to the OnData().  You call Start() to connect 
	 * to the rtEdgeCache3 server.
	 *
	 * \param hosts - Comma-separated list of rtEdgeCache3 \<host\>:\<port\> 
	 * to connect to. 
	 * \param user - rtEdgeCache3 username
	 * \param bBinary - true to set channel to binary
	 */
	rtEdgeSubscriber( String ^hosts, String ^user, bool bBinary );

	/**
	 * \brief Destructor.  Calls Stop() to disconnect; Cleans up internally.
	 */
	~rtEdgeSubscriber();


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
	String ^Start();

	/**
	 * \brief Destroy connection to the rtEdgeCache3
	 */
	void Stop();

	/**
	 * \brief Return true if this channel pumps from tape; false if rtEdgeCache3
	 *
	 * \return true if this channel pumps from tape; false if rtEdgeCache3
	 */
	bool IsTape();

	/**
	 * \brief Pump data from the tape
	 *
	 * Messages from the tape are pumped as follows:
	 * + All messages are delivered to your app via OnData() or OnDead()
	 * + All messages are delivered in the library thread for this channel
	 * + If you Subscribe()'ed to any tickers, only those are pumped
	 * + If you did not Subscribe(), then ALL tickers are pumped
	 */
	void PumpTape();

	/**
	 * \brief Pump data from the tape between the given start and end times.
	 *
	 * Messages from the tape are pumped as follows:
	 * + All messages are delivered to your app via OnData() or OnDead()
	 * + All messages are delivered in the library thread for this channel
	 * + If you Subscribe()'ed to any tickers, only those are pumped
	 * + If you did not Subscribe(), then ALL tickers are pumped
	 *
	 * \param tStart - Start time
	 * \param tEnd - End time
	 */
	void PumpTapeSlice( DateTime ^tStart, DateTime ^tEnd );

	/**
	 * \brief Pump data from the tape between the given start and end times
	 * at specific interval and specific field(s)
	 *
	 * Messages from the tape are pumped as follows:
	 * + All messages are pumped from tape into an internal Last Value Cache (LVC)
	 * + At the tInterval, a new message is pumped from LVC into OnData()
	 * + All messages are delivered to your app via OnData() or OnDead()
	 * + All messages are delivered in the library thread for this channel
	 * + If you Subscribe()'ed to any tickers, only those are pumped
	 * + If you did not Subscribe(), then ALL tickers are pumped
	 *
	 * \param tStart - Start time
	 * \param tEnd - End time
	 * \param tInterval - Interval in seconds
	 * \param pFlds - CSV list of Field IDs or Names of interest
	 */
	void PumpTapeSliceSample( DateTime ^tStart, 
	                          DateTime ^tEnd,
	                          int       tInterval,
	                          String   ^pFlds );

	/** \brief Stop pumping data from tape */
	void StopTape();

	/**
	 * \brief Requests channel be set in binary
	 *
	 * The channel protocol is set once in Start().  Therefore, this
	 * must be called BEFORE calling Start().
	 *
	 * \param bBin - true to set to binary; Else ASCII (default)
	 */
	void SetBinary( bool bBin );

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
	 * Setting to true allows you to have all subscribers share a
	 * common config file, yet have the library stochastically
	 * distribute the load across all available Edge2 servers.
	 *
	 * \param bRandom - true to set to randomize; false to round robin
	 */
	void SetRandomize( bool bRandom );

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
	void SetIdleCallback( bool bIdleCbk );

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
	void SetHeartbeat( int tHbeat );

	/**
	 * \brief Sets the direction messages - chronological or reverse - when
	 * pumping from tape.
	 *
	 * Messages are stored on the tape in reverse order as each messsage has
	 * a backward-, not forward-pointer.  As such, when Subscribing from tape,
	 * the messages are read from tape in reverse order.
	 *
	 * Call this method to control whether messages are pumped in reverse or
	 * chronological order.
	 *
	 * \param bTapeDir - true to pump in tape (reverse) order; false in 
	 * chronological order
	 */
	void SetTapeDirection( bool bTapeDir );


	/////////////////////////////////
	// Access
	/////////////////////////////////
public:
#ifndef DOXYGEN_OMIT
	RTEDGE::SubChannel &cpp() { return *_sub; }
#endif // DOXYGEN_OMIT

	librtEdgePRIVATE::SubChannel *_sub;
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
	 * \brief Returns rtEdgeSchema for this channel
	 *
	 * \return rtEdgeSchema for this channel
	 */
	rtEdgeSchema ^schema();

	/**
	 * \brief Returns true if channel is Binary
	 *
	 * \return true if channel is Binary
	 */
	bool IsBinary();

	/**
	 * \brief Returns true if this is snapshot channel
	 *
	 * NOTE: You do not specifically define a subscription channel that you
	 * create via Start() to be snapshot or streaming.  Rather,
	 * this is defined on the rtEdgeCache3 server.  There is a streaming
	 * port and (optional) snapshot port.  After connecting, you may query
	 * via IsSnapshot() to determine if the channel is snapshot or
	 * streaming.
	 *
	 * \return true if snapshot channel; false if streaming
	 */
	bool IsSnapshot();


	/////////////////////////////////
	// Get Field
	/////////////////////////////////
public:
	/**
	 *  \brief Returns true if current update has requested field by name
	 *
	 * \param fieldName - Schema name of field to retrieve
	 * \return true if found; false otherwise
	 */
	bool HasField( String ^fieldName );

	/**
	 *  \brief Returns true if current update has requested field by ID
	 *
	 * \param fid - Requested Field ID
	 * \return true if found; false otherwise
	 */
	bool HasField( int fid );

	/**
	 *  \brief Retrieve and return requested field by name
	 *
	 * \param fieldName - Schema name of field to retrieve
	 * \return rtEdgeField if found; null otherwise
	 */
	rtEdgeField ^GetField( String ^fieldName );

	/**
	 *  \brief Retrieve and return requested field by FID
	 *
	 * \param fid - Requested Field ID
	 * \return rtEdgeField if found; null otherwise
	 */
	rtEdgeField ^GetField( int fid );


	/////////////////////////////////
	// Subscribe / Unsubscribe
	/////////////////////////////////
public:
	/**
	 * \brief Open a subscription stream for ( svc, tkr ) data stream
	 *
	 * Market data updates are returned in the OnData() asynchronous call.
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \param arg - User-supplied arg returned in rtEdgeData::_arg
	 * \return Unique Stream ID
	 */
	int Subscribe( String ^svc, String ^tkr, int arg );

	/**
	 * \brief Closes a subscription stream for ( svc, tkr ) data stream
	 *
	 * Market data updates are stopped
	 *
	 * \param svc - Service name (e.g. BLOOMBERG) passed into Subscribe()
	 * \param tkr - Ticker name (e.g. EUR CURNCY) passed into Subscribe()
	 */
	void Unsubscribe( String ^svc, String ^tkr );


	////////////////////////////////////
	// ByteStream
	////////////////////////////////////
public:
	/**
	 * \brief Subscribe to a Byte Stream
	 *
	 * \param bStr - ByteStream to subscribe to
	 * \return Unique Stream ID
	 */
	int Subscribe( ByteStream ^bStr );

	/**
	 * \brief Closes a subscription stream for this Byte Stream
	 *
	 * \param bStr - ByteStream we subscribed to
	 * \return Unique Stream ID
	 */
	int Unsubscribe( ByteStream ^bStr );


	////////////////////////////////////
	// ByteStream
	////////////////////////////////////
public:
	/**
	 * \brief Opens all subscription streams for this Chain
	 *
	 * \param chn - Chain to subscribe to
	 * \return Number of Chain streams opened
	 */
	int Subscribe( Chain ^chn );

	/**
	 * \brief Closes all subscription streams for this Chain
	 *
	 * \param chn - Chain we subscribed to
	 * \return Number of Chain streams closed
	 */
	int Unsubscribe( Chain ^chn );


	////////////////////////////////////
	// BDS / Symbol List
	////////////////////////////////////
public:
	/**
	 * \brief Subscribe to a Broadcast Data Stream (BDS) from a service.
	 *
	 * \param svc - Name of service supplying the BDS
	 * \param bds - BDS name
	 * \param arg - User-supplied argument returned in OnData()
	 * \return Unique Stream ID
	 */
	int OpenBDS( String ^svc, String ^bds, int arg );

	/**
	 * \brief Close a BDS stream that was opened via OpenBDS()
	 *
	 * \param svc - Name of service supplying the BDS
	 * \param bds - BDS name
	 * \return Unique Stream ID if successful; 0 if not
	 */
	int CloseBDS( String ^svc, String ^bds );


	////////////////////////////////////
	// Cache Query
	////////////////////////////////////
public:
	/**
	 * \brief Enables cache-ing in the library.
	 *
	 * This must be called BEFORE calling Start().
	 *
	 * \param bCache - true to enable cache-ing in library
	 */
	void SetCache( bool bCache );

	/** 
	 * \brief Return true if cache-ing
	 *   
	 * \return true if cache-ing
	 */  
	bool IsCache();

	/** 
	 * \brief Query internal cache for current values.
	 *   
	 * Only valid if EnableCache() has been called BEFORE Start().
	 *   
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \return Current values in rtEdgeData
	 */  
	rtEdgeData ^QueryCache( String ^svc, String ^tkr );


	////////////////////////////////////
	// Database Directory Query
	////////////////////////////////////

	/**
	 * \brief Query ChartDB for complete list of tickers
	 *
	 * \return Complete list of tickers in MDDResult object
	 */
	MDDResult ^Query();

	/**
	 * \brief Release resources associated with last call to Query().
	 */
	void FreeResult();


	////////////////////////////////////
	// Tape Only
	////////////////////////////////////
public:
	/**
	 * \brief Pump slice of Messages from Tape starting at specific offset.
	 *
	 * You receive asynchronous market data updates in the OnData() and are
	 * notified of completion in OnStreamDone().
	 *
	 * To pump a 'slice', you will need to store the rtEdgeData.TapePos() from
	 * the last message received in previous call to PumpFullTape(), 
	 * then use this as the off0 in next call to PumpFullTape().
	 *
	 * \param off0 - Beginning offset, or 0 for beginning of tape
	 * \param nMsg - Number of msgs to pump; 0 for all
	 * \return Unique Tape Pumping ID; Kill pump via StopPumpFullTape()
	 * \see StopPumpFullTape()
	 */
	int PumpFullTape( u_int64_t off0, int nMsg );

	/**
	 * \brief Stop pumping from tape
	 *
	 * \param pumpID - Pump ID returned from rtEdge_PumpFullTape()
	 * \return 1 if stopped; 0 if invalid Pump ID
	 * \see PumpFullTape()
	 */
	int StopPumpFullTape( int pumpID );


	////////////////////////////////////
	// Parse Only
	////////////////////////////////////
public:
	/**
	 * \brief Parse a raw message
	 *
	 * You normally call Parse() on Tape channels where you have stored
	 * the raw buffer from your callback.
	 *
	 * \param data - Raw data to parse
	 * \return rtEdgeData containing parsed results
	 */
	rtEdgeData ^Parse( array<byte> ^data );

	/**
	 * \brief Parse a "view" of the raw message
	 *
	 * You normally call ParseView() on Tape channels where you have stored
	 * the raw buffer from your callback.
	 *
	 * \param vw - Pointer to raw message on Tape
	 * \param len - Length of the vw
	 * \return rtEdgeData containing parsed results
	 */
	rtEdgeData ^ParseView( IntPtr vw, int len );


	/////////////////////////////////
	// IrtEdgeSubscriber interface
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
	 * \brief Called asynchronously when a real-time publisher changes 
	 * state (goes UP or DOWN) in the rtEdgeCache3.
	 *
	 * Override this method in your C# application to take action when
	 * a new publisher goes online or offline.  The library transparently 
	 * re-subscribes any and all streams you have Subscribe()'ed to when 
	 * the service comes back UP.
	 *
	 * \param svc - Real-time service : BLOOMBEG, IDN_RDF, etc.
	 * \param state - Service state : Up or Down
	 */
	virtual void OnService( String ^svc, rtEdgeState state )
	{ ; }

	/**
	 * \brief Called asynchronously when real-time market data arrives
	 * on this subscription channel from rtEdgeCache3
	 *
	 * Override this method in your C# application to consume market data.
	 *
	 * \param msg - Market data update in a rtEdgeData object
	 */
	virtual void OnData( rtEdgeData ^msg )
	{ ; }

	/**
	 * \brief Called asynchronously when the real-time market data stream
	 * is recovering
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnRecovering( rtEdgeData ^msg )
	{ ; }

	/**
	 * \brief Called asynchronously when the real-time market data stream
	 * goes stale.
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnStale( rtEdgeData ^msg )
	{ ; }

	/**
	 * \brief Called asynchronously when real-time market data stream
	 * opened via Subscribe() becomes DEAD.
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 * \param err - Error string
	 */
	virtual void OnDead( rtEdgeData ^msg, String ^err )
	{ ; }

	/**
	 * \brief Called asynchronously when the real-time market data stream
	 * is complete
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnStreamDone( rtEdgeData ^msg )
	{ ; }

	/**
	 * \brief Called asynchronously when Symbol List opened via OpenBDS()
	 * updates.
	 *
	 * Override this method in your application to consume Symbol List
	 *
	 * \param msg - Market data update in a Message object
	 * \param sym - New Ticker in Symbol List
	 */
	virtual void OnSymbol( rtEdgeData ^msg, String ^sym )
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
	 * \brief Called when a General Purpose error occurs on the channe
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


};  // class rtEdgeSubscriber

} // namespace librtEdge
