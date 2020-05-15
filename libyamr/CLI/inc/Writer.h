/******************************************************************************
*
*  Writer.h
*
*  REVISION HISTORY:
*     19 JUN 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#pragma once
#include <Data.h>

#ifndef DOXYGEN_OMIT

namespace libyamrPRIVATE
{
////////////////////////////////////////////////
//
//      c l a s s   I W r i t e r
//
////////////////////////////////////////////////
/**
 * \class IWriter
 * \brief Abstract class to handle subscriber channel events
 * such as OnConnect() and OnData().
 */
public interface class IWriter
{
	// IWriter Interface
public:
	virtual void OnConnect( String ^, bool ) abstract;
	virtual void OnStatus( libyamr::yamrStatus ^ ) abstract;
	virtual void OnIdle() abstract;

}; // class IWriter


////////////////////////////////////////////////
//
//     c l a s s   _ W r i t e r
//
////////////////////////////////////////////////

/**
 * \class Writer
 * \brief YAMR::Writer sub-class to hook 3 virtual methods
 * from native libyamr library and dispatch to .NET consumer. 
 */ 
class _Writer : public YAMR::Writer
{
private:
	gcroot < IWriter^ >          _cli;
	gcroot < libyamr::yamrMsg^ > _msg;

	// Constructor
public:
	/**
	 * \brief Constructor for class to hook native events from 
	 * native libyamr library and pass to .NET consumer via
	 * the IWriter interface.
	 *
	 * \param cli - Event receiver - IWriter::OnData(), etc.
	 */
	_Writer( IWriter ^cli );
	~_Writer();

	// Access

	libyamr::yamrMsg ^msg();

	// Asynchronous Callbacks
protected:
	virtual void OnConnect( const char *msg, bool );
	virtual void OnStatus( ::yamrStatus );
	virtual void OnIdle();

}; // cass _Writer

}  // namespace libyamrPRIVATE

#endif // DOXYGEN_OMIT


namespace libyamr
{

////////////////////////////////////////////////
//
//          c l a s s   W r i t e r
//
////////////////////////////////////////////////
/**
 * \class Writer
 * \brief Unstructured data writer to yamRecorder class.
 *
 * All structured data transfer derive from and utilize the services
 * of the unstructured writer and reader classes.
 * \see Reader
 */
public ref class Writer : public libyamr::yamr,
	                       public libyamrPRIVATE::IWriter 
{
private: 
	libyamrPRIVATE::_Writer *_sub;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.  Call Start() to connect to yamrRecorder.
	 *
	 * The constructor initializes internal variables, including reusable 
	 * data objects passed to the OnData().  You call Start() to connect 
	 * to the yamrRecorder server.
	 */
	Writer();

	/**
	 * \brief Destructor.  Calls Stop() to disconnect; Cleans up internally.
	 */
	~Writer();


	/////////////////////////////////
	// Operations
	/////////////////////////////////
public:
	/**
	 * \brief Initialize the connection to yamrRecorder server.
	 *
	 * Your application is notified via OnConnect() when you have 
	 * successfully connnected and established a session.
	 *
	 * \param hosts - Comma-separated list of yamRecorder \<host\>:\<port\>
	 * to connect to.
	 * \param SessionID - Unique Session ID for this box
	 * \return Textual description of the connection state
	 */
	String ^Start( String ^hosts, int SessionID );

	/**
	 * \brief Destroy connection to the yamrRecorder
	 */
	void Stop();

	/**
	 * \brief Send unstructured data to yamRecorder
	 *
	 * \param data - Unstructured data as std::string
	 * \param WireProto - Wire Protocol : < YAMR_PROTO_MAX
	 * \param MsgProto - Msg Protocol : 0 means = WireProto
	 * \return true if consumed
	 */
	bool Send( array<Byte> ^data, short WireProto, short MsgProto );

	/**
	 * \brief Send unstructured data to yamRecorder
	 *
	 * \param data - Unstructured data as std::string
	 * \param WireProto - Wire Protocol : < YAMR_PROTO_MAX
	 * \return true if consumed
	 */
	bool Send( array<Byte> ^data, short WireProto );


	////////////////////////////////////
	// Access / Mutator
	////////////////////////////////////
public:
	/**
	 * \brief Returns comma-separated list of yamRecorder hosts to
	 * connect this subscription channel to.
	 *
	 * This list is defined when you call Start() and is specified as
	 * \<host1\>:\<port1\>,\<host2\>:\<port2\>,...  The library tries to connect
	 * to \<host1\>; If failure, then \<host2\>, etc.  This
	 *
	 * \return Comma-separated list of yamRecorder hosts.
	 */
	String ^pSvrHosts();

	/**
	 * \brief Returns subscription channel username
	 *
	 * You define the username when you call Start().
	 *
	 * \return Subscription channel username
	 */
	int SessionID();

	/**
	 * \brief Randomize yamrAttr._pSvrHosts when connecting
	 *
	 * \param bRand : true to randomize; false to round-robin in order
	 */
	void Randomize( bool bRand );

	/**
	 * \brief Set SO_RCVBUF for this yamrCache3 channel
	 *
	 * \param bufSiz - SO_RCVBUF size
	 * \return  GetRxBufSize()
	 */
	int SetRxBufSize( int bufSiz );

	/**
	 * \brief Get SO_RCVBUF for this yamrCache3 channel
	 *
	 * \return SO_RCVBUF size
	 */
	int GetRxBufSize();

	/**
	 * \brief Sets the max queue size on outbound channel to yamRecorder
	 *
	 * \param bufSiz - Max queue size
	 * \return  GetTxBufSize()
	 */
	int SetTxBufSize( int bufSiz );

	/**
	 * \brief Gets current outbound channel queue size.   Max queue
	 * size had been set via SetTxBufSize()
	 *
	 * \return Current queue size on outbound channel to yamRecorder
	 */
	int GetTxBufSize();

	/**
	 * \brief Gets current outbound channel queue size.
	 *
	 * \return Current outbound channel queue size.
	 */
	int GetTxQueueSize();

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
	 * \brief Set Hi / Lo Queue Notification Bands (in Percent)
	 *
	 * You get notified in your OnStatus() when outbound queue
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
	 * \see OnStatus()
	 * \param hiLoBand - Percentage Bands in range ( 5 to 45 )
	 */
	void SetHiLoBand( int hiLoBand );

	/**
	 * \brief Returns true if UDP Channel; false if TCP
	 *
	 * \return true if UDP Channel; false if TCP
	 */
	bool IsUDP();

#ifndef DOXYGEN_OMIT
	/**
	 * \brief Returns reference to unmanaged YAMR::Writer object
	 *
	 * \return Reference to unmanaged YAMR::Writer object
	 */
	YAMR::Writer &cpp()
	{
	   return *_sub;
	}
#endif // DOXYGEN_OMIT


	/////////////////////////////////
	// IWriter interface
	/////////////////////////////////
public:
	/**
	 * \brief Called asynchronously when we connect or disconnect from
	 * yamrRecorder.
	 *
	 * Override this method in your C# application to take action when
	 * your connect or disconnect from the yamrRecorder.
	 *
	 * \param msg - Textual description of connection state
	 * \param bUP - True if UP; False if DOWN
	 */
	virtual void OnConnect( String ^msg, bool bUP )
	{ ; }

	/**
	 * \brief Called asynchronously when Writer status updates
	 *
	 * \param sts - Writer status
	 */
	virtual void OnStatus( libyamr::yamrStatus ^sts )
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

};  // class Writer

} // namespace libyamr
