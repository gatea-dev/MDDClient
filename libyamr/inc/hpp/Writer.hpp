/******************************************************************************
*
*  Writer.hpp
*     libyamr Writer Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*     29 JUL 2019 jcs  Build  2: Sharable; DumpCSV()
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_Writer_H
#define __YAMR_Writer_H
#include <hpp/yamr.hpp>

#define _MAX_CHAN 64*K // Cycle-through 64*K connections??

namespace YAMR
{

// Forwards

class Writer;  // Forward declaration for bInit_ / schans_

static bool     bInit_ = false;
static Writer *schans_[_MAX_CHAN];

typedef std::vector<IWriteListener *> IWriteListeners;


////////////////////////////////////////////////
//
//        c l a s s    C h a n n e l
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
class Writer : public Sharable,
               public IWriteListener
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor. */
	Writer() :
	   _hosts(),
	   _cxt( (yamr_Context)0 ),
	   _bRandom( false ),
	   _bIdleCbk( false ),
	   _lsn()
	{
	   int i;

	  // Initialize channel lookup (ONCE)

	   for ( i=0; !bInit_ && i<_MAX_CHAN; schans_[i++]=(Writer *)0 );
	   bInit_ = true;
	   ::memset( &_attr, 0, sizeof( _attr ) );

	   // We are our own listener

	   AddListener( *this );
	}

	/** \brief Destructor. */
	virtual ~Writer()
	{
	   Stop();
	}


	////////////////////////////////////
	// Writer Operations
	////////////////////////////////////
public:
	/**
	 * \brief Initialize the connection to yamRecorder
	 *
	 * Your application is notified via OnConnect() when you have 
	 * successfully connnected and established a session.
	 *
	 * \param hosts - Comma-separated list of yamRecorder \<host\>:\<port\>
	 * to connect to.
	 * \param SessionID - Unique Session ID for this box
	 * \param bConnect - true to Connect()
	 * \return Textual description of the connection state
	 * \see yamr_Start()
	 * \see Connect()
	 */
	const char *Start( const char *hosts, 
	                   int         SessionID,
	                   bool        bConnect = false )
	{
	   // Pre-condition(s)

	   if ( _cxt )
	      return "Already connected";
	   if ( !hosts )
	      return "No hostname specified";

	   // Initialize our channel

	   _hosts = hosts; 
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   _attr._pSvrHosts = pSvrHosts();
	   _attr._SessionID = SessionID;
	   _attr._connCbk   = _connCbk;
	   _attr._stsCbk    = _stsCbk;
	   _attr._idleCbk   = _idleCbk;
	   _cxt             = ::yamr_Initialize( _attr );
	   schans_[_cxt]    = this;
	   Randomize( _bRandom );
	   SetIdleCallback( _bIdleCbk );
	   return bConnect ? Connect() : "yamr start pending ...";
	}

	/**
	 * \brief Connect to yamRecorder
	 *
	 * Your application is notified via OnConnect() when you have
	 * successfully connnected and established a session.
	 *
	 * \return Textual description of the connection state
	 * \see Start()
	 */
	const char *Connect()
	{
	   return ::yamr_Start( _cxt );
	}

	/**
	 * \brief Destroy connection to the yamrRecorder
	 *
	 * Calls ::yamr_Destroy() to disconnect from the yamrRecorder
	 */
	void Stop()
	{
	   if ( _cxt )
	      ::yamr_Destroy( _cxt );
	   _cxt  = (yamr_Context)0;
	}

	/**
	 * \brief Send unstructured data to yamRecorder
	 *
	 * \param data - Unstructured data as std::string
	 * \param WireProto - Wire Protocol : < YAMR_PROTO_MAX
	 * \param MsgProto - Msg Protocol : 0 means = WireProto
	 * \return true if consumed
	 */
	bool Send( std::string &data, u_int16_t WireProto, u_int16_t MsgProto=0 )
	{
	   return Send( data.data(), data.length(), WireProto, MsgProto );
	}

	/**
	 * \brief Send unstructured data to yamRecorder
	 *
	 * \param data - Unstructured data to send
	 * \param dLen - Unstructured data length
	 * \param WireProto - Wire Protocol : < YAMR_PROTO_MAX
	 * \param MsgProto - Msg Protocol : 0 means = WireProto
	 * \return true if consumed
	 */
	bool Send( const char *data, 
	           size_t      dLen, 
	           u_int16_t   WireProto, 
	           u_int16_t   MsgProto=0 )
	{
	   return Send( (char *)data, dLen, WireProto, MsgProto );
	}

	/**
	 * \brief Send unstructured data to yamRecorder
	 *
	 * \param data - Unstructured data to send
	 * \param dLen - Unstructured data length
	 * \param WireProto - Wire Protocol : < YAMR_PROTO_MAX
	 * \param MsgProto - Msg Protocol : 0 means = WireProto
	 * \return true if consumed
	 */
	bool Send( char     *data, 
	           size_t    dLen, 
	           u_int16_t WireProto, 
	           u_int16_t MsgProto=0 )
	{
	   yamrBuf yb;

	   yb._data = data;
	   yb._dLen = dLen;
	   return Send( yb, WireProto, MsgProto );
	}

	/**
	 * \brief Send unstructured data to yamRecorder
	 *
	 * \param yb - Unstructured data to send
	 * \param WireProto - Wire Protocol : < YAMR_PROTO_MAX
	 * \param MsgProto - Msg Protocol : 0 means = WireProto
	 * \return true if consumed
	 */
	bool Send( yamrBuf yb, u_int16_t WireProto, u_int16_t MsgProto=0 )
	{
	   if ( !MsgProto )
	      MsgProto = WireProto;
	   return ::yamr_Send( _cxt, yb, WireProto, MsgProto ) ? true : false;
	}


	////////////////////////////////////
	// Access / Mutator
	////////////////////////////////////
public:
	/**
	 * \brief Returns yamr_Context associated with this channel
	 *
	 * \return yamr_Context associated with this channel
	 */
	yamr_Context cxt()
	{
	   return _cxt;
	}

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
	const char *pSvrHosts()
	{
	   return _hosts.data();
	}

	/**
	 * \brief Returns subscription channel username
	 *
	 * You define the username when you call Start().
	 *
	 * \return Subscription channel username
	 */
	int SessionID()
	{
	   return _attr._SessionID;
	}

	/**
	 * \brief Return true if this Writer has been initialized and not Stop()'ed
	 *
	 * \return true if this Writer has been initialized and not Stop()'ed
	 */
	bool IsValid()
	{
	   return( _cxt != (yamr_Context)0 );
	}

	/**
	 * \brief Configure pub / sub channel
	 *
	 * \param cmd - Command from yamrIoctl
	 * \param val - Command value
	 * \see ::yamr_ioctl()
	 */
	void Ioctl( yamrIoctl cmd, void *val )
	{
	   if ( IsValid() )
	      ::yamr_ioctl( _cxt, cmd, val );
	}

#ifdef TODO_GETSTATS
	/**
	 * \brief Retrieve channel run-time stats
	 *
	 * \return Writer run-time stats
	 */
	yamrChanStats *GetStats()
	{
	   yamrChanStats *rtn;

	   rtn = (yamrChanStats *)0;
	   Ioctl( ioctl_getStats &rtn );
	   return rtn;
	}
#endif // TODO_GETSTATS

	/**
	 * \brief Randomize yamrAttr._pSvrHosts when connecting
	 *
	 * \param bRand : true to randomize; false to round-robin in order
	 */
	void Randomize( bool bRand )
	{
	   _bRandom = bRand;
	   Ioctl( ioctl_randomize, (void *)_bRandom );
	}

	/**
	 * \brief Return socket file descriptor for this Writer
	 *
	 * \return Socket file descriptor for this Writer
	 */
	int GetSocket()
	{
	   int fd;

	   fd = 0;
	   Ioctl( ioctl_getFd, &fd );
	   return fd;
	}

	/**
	 * \brief Set SO_RCVBUF for this yamrCache3 channel
	 *
	 * \param bufSiz - SO_RCVBUF size
	 * \return  GetRxBufSize()
	 */
	int SetRxBufSize( int bufSiz )
	{
	   Ioctl( ioctl_setRxBufSize, &bufSiz );
	   return GetRxBufSize();
	}

	/**
	 * \brief Get SO_RCVBUF for this yamrCache3 channel
	 *
	 * \return SO_RCVBUF size
	 */
	int GetRxBufSize()
	{
	   int sz;

	   sz = 0;
	   Ioctl( ioctl_getRxBufSize, &sz );
	   return sz;
	}

	/**
	 * \brief Sets the max queue size on outbound channel to yamRecorder
	 *
	 * \param bufSiz - Max queue size
	 * \return  GetTxBufSize()
	 */
	int SetTxBufSize( int bufSiz )
	{
	   Ioctl( ioctl_setTxBufSize, &bufSiz );
	   return GetTxBufSize();
	}

	/**
	 * \brief Gets current outbound channel queue size.   Max queue
	 * size had been set via SetTxBufSize()
	 *
	 * \return Current queue size on outbound channel to yamRecorder
	 */
	int GetTxBufSize()
	{
	   int sz;

	   sz = 0;
	   Ioctl( ioctl_getTxBufSize, &sz );
	   return sz;
	}

	/**
	 * \brief Gets current outbound channel queue size.
	 *
	 * \return Current outbound channel queue size.
	 */
	int GetTxQueueSize()
	{
	   int sz;

	   sz = 0;
	   Ioctl( ioctl_getTxQueueSize, &sz );
	   return sz;
	}

	/**
	 * \brief Tie this channel thread to a specific CPU
	 *
	 * \param cpu - CPU core to attach this channel thread to
	 * \return  GetThreadProcessor()
	 */
	int SetThreadProcessor( int cpu )
	{
	   Ioctl( ioctl_setThreadProcessor, &cpu );
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
	   Ioctl( ioctl_getThreadProcessor, &cpu );
	   return cpu;
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
	   ::yamr_ioctl( _cxt, ioctl_getThreadId, &tid );
	   return tid;
	}

	/**
	 * \brief Allow / Disallow the library timer to call out to OnIdle()
	 * every second or so.
	 *
	 * This is useful for performing tasks in the libyamr thread periodically.
	 *
	 * \param bIdleCbk - true to receive OnIdle(); false to disable
	 */
	void SetIdleCallback( bool bIdleCbk )
	{
	   _bIdleCbk = bIdleCbk;
	   Ioctl( ioctl_setIdleTimer, (void *)_bIdleCbk );
	}

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
	void SetHiLoBand( int hiLoBand )
	{
	   Ioctl( ioctl_QlimitHiLoBand, (void *)&hiLoBand );
	}

	/**
	 * \brief Returns true if UDP Channel; false if TCP
	 *
	 * \return true if UDP Channel; false if TCP
	 */
	bool IsUDP()
	{
	   size_t udp;

	   Ioctl( ioctl_getSocketType, (void *)udp );
	   return udp ? true : false;
	}


	////////////////////////////////////
	// IWriteListener
	////////////////////////////////////
public:
	/**
	 * \brief Register interest to be notified of channel connection events
	 *
	 * \param lsn - Listener to be notified of connection events
	 */
	void AddListener( IWriteListener &lsn )
	{
	   _lsn.push_back( &lsn );
	}


	////////////////////////////////////
	// IWriteListener Interface
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously when we connect or disconnect from
	 * the yamRecorder.
	 *
	 * Override this method in your application to take action when
	 * your connect or disconnect from the yamRecorder.
	 *
	 * \param msg - Textual description of connection state
	 * \param bUP - true if connected; false if disconnected
	 */
	virtual void OnConnect( const char *msg, bool bUP )
	{ ; }

	/** 
	 * \brief Called asynchronously when Writer status updates
	 *
	 * \param sts - Writer status
	 */
	virtual void OnStatus( yamrStatus sts )
	{ ; }


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
	/**
	 * \brief Called asynchronously - roughly once per second - when the
	 * library is idle.
	 *
	 * This is only called if you enable via SetIdleCallback()
	 *
	 * Override this method in your application to perform tasks
	 * in the channel channel thread.
	 */
	virtual void OnIdle()
	{ ; }


	////////////////////////////////////
	// Asynchronous Callback (private)
	////////////////////////////////////
private:
#ifndef DOXYGEN_OMIT
	void _OnConnect( const char *msg, bool bUP )
	{
	   IWriteListeners &ldb = _lsn;

	   for ( size_t i=0; i<ldb.size(); ldb[i++]->OnConnect( *this, bUP ) );
	}

	void _OnStatus( yamrStatus sts )
	{
	   IWriteListeners &ldb = _lsn;

	   for ( size_t i=0; i<ldb.size(); ldb[i++]->OnStatus( *this, sts ) );
	}
#endif // DOXYGEN_OMIT


	////////////////////////////////////
	// Protected Members
	////////////////////////////////////
protected:
	std::string     _hosts;
	yamrAttr        _attr;
	yamr_Context    _cxt;
	bool            _bRandom;
	bool            _bIdleCbk;
	IWriteListeners _lsn;


	////////////////////////////////////
	// Class-wide (private) callbacks
	////////////////////////////////////
private:
	static void YAMRAPI _connCbk( yamr_Context cxt,
	                              const char  *msg,
	                              yamrState    state )
	{
	   Writer *us;

	   if ( (us=schans_[cxt]) )
	      us->_OnConnect( msg, ( state == yamr_up ) );
	}

	static void YAMRAPI _stsCbk( yamr_Context cxt,
	                             yamrStatus   sts )
	{
	   Writer *us;

	   if ( (us=schans_[cxt]) )
	      us->_OnStatus( sts );
	}

	static void YAMRAPI _idleCbk( yamr_Context cxt )
	{
	   Writer *us;

	   if ( (us=schans_[cxt]) )
	      us->OnIdle();
	}

};  // class Writer

} // namespace YAMR

#endif // __YAMR_Writer_H 
