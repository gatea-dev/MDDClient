/******************************************************************************
*
*  SubChannel.hpp
*     librtEdge SubChannel connection to rtEdgeCache3
*
*  REVISION HISTORY:
*      3 SEP 2014 jcs  Created.
*     11 DEC 2014 jcs  rtEdge.hpp
*      6 JAN 2015 jcs  Build 29: Chain; SetRandomize()
*      3 JUL 2015 jcs  Build 31: RTEDGE::Channel; OnDead()
*     14 NOV 2015 jcs  Build 32: IsSnapshot(); Channel.OnIdle()
*     11 SEP 2016 jcs  Build 33: SetUserStreamID()
*     12 OCT 2017 jcs  Build 36: StartTape()
*     10 DEC 2018 jcs  Build 41: _bStrMtx
*     12 FEB 2020 jcs  Build 42: Channel.SetHeartbeat()
*     10 SEP 2020 jcs  Build 44: SetTapeDirection(); Query()
*     17 SEP 2020 jcs  Build 45: Parse()
*      3 DEC 2020 jcs  Build 47: PumpTapeSliceSlice(); PumpFullTape()
*      6 OCT 2021 jcs  Build 50: doxygen de-lint
*     22 SEP 2022 jcs  Build 56: Rename StartTape() to PumpTape()
*     22 OCT 2022 jcs  Build 58: ByteStream.Service(); CxtMap
*     20 JUL 2023 jcs  Build 64: dox : OnData() msg is volatile
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_SubChannel_H
#define __RTEDGE_SubChannel_H
#include <hpp/rtEdge.hpp>
#include <map>

namespace RTEDGE
{

// Forward declarations

class Message;
class Schema;
class SubChannel;

#ifndef DOXYGEN_OMIT
static CxtMap<SubChannel *> _subChans;

static const char *_tape_all = "*";
static const char *_dflt_fld = "6";

typedef hash_map<int, ByteStream *> ByteStreams;
typedef hash_map<int, Chain *>      Chains;
typedef hash_set<int>               SymLists;

#endif // DOXYGEN_OMIT

////////////////////////////////////////////////
//
//        c l a s s   S u b C h a n n e l
//
////////////////////////////////////////////////

/**
 * \class SubChannel
 * \brief Subscription channel from data source - rtEdgeCache3 or Tape File
 *
 * The 1st argument to Start() defines your data source:
 * 1st Arg | Data Source | Data Type
 * --- | --- | ---
 * host:port | rtEdgeCache3 | Streaming real-time data
 * filename | Tape File (64-bit only) | Recorded market data from tape
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
 */
class SubChannel : public Channel
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  Call Start() to connect to rtEdgeCache3.
	 * 
	 * The constructor initializes internal variables, including reusable 
	 * data objects passed to the OnData().  You call Start() to connect 
	 * to the rtEdgeCache3 server.
	 */
	SubChannel() :
	   Channel( false ),
	   _hosts(),
	   _user(),
	   _schema(),
	   _bBinary( false ),
	   _bRandom( false ),
	   _bUsrStreamID( false ),
	   _bTapeDir( false ),
	   _msg( (Message *)0 ),
	   _msgQ( (Message *)0 ),
	   _msgP( (Message *)0 ),
	   _flParse( ::mddFieldList_Alloc( K ) ),
	   _msgSchema( ::mddFieldList_Alloc( K ) ),
	   _bStrDb(),
	   _bStrMtx(),
	   _chainDb(),
	   _symListDb()
	{
	   SetCache( false );
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   ::memset( &_qryAll, 0, sizeof( _qryAll ) );
	   ::memset( &_dParse, 0, sizeof( _dParse ) );
	}

	virtual ~SubChannel()
	{
	   FreeResult();
	   Stop();
	   ::mddFieldList_Free( _flParse );
	   ::mddFieldList_Free( _msgSchema );
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/**
	 * \brief Returns comma-separated list of rtEdgeCache3 hosts to
	 * connect this subscription channel to.
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
	 * \brief Returns subscription channel username
	 *
	 * You define the username when you call Start().
	 *
	 * \return Subscription channel username
	 */
	const char *pUsername()
	{
	   return _user.data();
	}

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
	bool IsSnapshot()
	{
	   bool bSnap;

	   bSnap = false;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_isSnapChan, (void *)&bSnap );
	   return bSnap;
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
	// SubChannel Operations
	////////////////////////////////////
public:
	/**
	 * \brief Initialize the connection to rtEdgeCache3 server.
	 *
	 * Your application is notified via OnConnect() when you have 
	 * successfully connnected and established a session.
	 *
	 * \param hosts - Comma-separated list of rtEdgeCache3 \<host\>:\<port\>
	 * to connect to.
	 * \param user - rtEdgeCache3 username
	 * \return Textual description of the connection state
	 */
	const char *Start( const char *hosts, const char *user )
	{
	   // Pre-condition(s)

	   if ( _cxt )
	      return "Already connected";
	   if ( !hosts )
	      return "No hostname specified";
	   if ( !user )
	      return "No username specified";

	   // Tape??

	   if ( _IsTape( hosts ) )
	      return _StartTape( hosts );

	   // Initialize our channel

	   _hosts = hosts; 
	   _user  = user; 
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   _attr._pSvrHosts = pSvrHosts();
	   _attr._pUsername = pUsername();
	   _attr._connCbk   = _connCbk;
	   _attr._svcCbk    = _svcCbk;
	   _attr._dataCbk   = _dataCbk;
	   _attr._schemaCbk = _schemaCbk;
	   _cxt             = ::rtEdge_Initialize( _attr );
	   _msg             = new Message( _cxt );
	   _subChans.Add( _cxt, this );
	   if ( IsCache() ) {
	      ::rtEdge_ioctl( _cxt, ioctl_enableCache, (void *)_bCache );
	      _msgQ = new Message( _cxt );
	   }
	   ::rtEdge_ioctl( _cxt, ioctl_binary, (void *)_bBinary );
	   ::rtEdge_ioctl( _cxt, ioctl_randomize, (void *)_bRandom );
	   SetUserStreamID( _bUsrStreamID );
	   SetHeartbeat( _tHbeat );
	   SetIdleCallback( _bIdleCbk );
	   return ::rtEdge_Start( _cxt );
	}

	/**
	 * \brief Return true if this channel pumps from tape; false if rtEdgeCache3
	 *
	 * \return true if this channel pumps from tape; false if rtEdgeCache3
	 */
	bool IsTape()
	{
	   return _attr._bTape ? true : false;
	}

#ifndef DOXYGEN_OMIT
private:
	bool _IsTape( const char *svr )
	{
	   rtBuf64 b;
	   bool    rtn;

	   b   = MapFile( svr, false );
	   rtn = ( b._data && b._dLen );
	   UnmapFile( b );
	   return rtn;
	}

	const char *_StartTape( const char *tape )
	{
	   // Pre-condition(s)

	   if ( _cxt )
	      return "Already connected";
	   if ( !tape )
	      return "No tape filename specified";
	   if ( !Is64Bit() )
	      return "Tape data source not supported on 32-bit platform";

	   // Initialize our channel

	   _hosts = tape; 
	   _user  = tape; 
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   _attr._bTape     = 1;
	   _attr._pSvrHosts = pSvrHosts();
	   _attr._pUsername = pUsername();
	   _attr._connCbk   = _connCbk;
	   _attr._svcCbk    = _svcCbk;
	   _attr._dataCbk   = _dataCbk;
	   _attr._schemaCbk = _schemaCbk;
	   _cxt             = ::rtEdge_Initialize( _attr );
	   _msg             = new Message( _cxt );
	   _subChans.Add( _cxt, this );
	   SetIdleCallback( _bIdleCbk );
	   SetTapeDirection( _bTapeDir );
	   return ::rtEdge_Start( _cxt );
	}
#endif // DOXYGEN_OMIT

	/**
	 * \brief Pump data from the tape
	 *
	 * Messages from the tape are pumped as follows: 
	 * + All messages are delivered to your app via OnData() or OnDead() 
	 * + All messages are delivered in the library thread for this channel
	 * + If you Subscribe()'ed to any tickers, only those are pumped
	 * + If you did not Subscribe(), then ALL tickers are pumped
	 */
public:
	void PumpTape()
	{
	   if ( _attr._bTape )
	      Subscribe( pSvrHosts(), _tape_all, (void *)0 );
	}

	/**
	 * \brief Pump data from the tape between the given start and end times.
	 *
	 * Messages from the tape are pumped as follows: 
	 * + All messages are delivered to your app via OnData() or OnDead() 
	 * + All messages are delivered in the library thread for this channel
	 * + If you Subscribe()'ed to any tickers, only those are pumped
	 * + If you did not Subscribe(), then ALL tickers are pumped
	 *
	 * tStart and tEnd strings are parsed as follows:
	 * + __DateTime__ or __Time__ are supported
	 * + __DateTime__ = Space separated __Date__ __Time__
	 * + __Date__ = YYYYMMDD or YYYY-MM-DD
	 * + __Time__ =  HH:MM:SS, HH:MM or HHMMSS
	 *
	 * \param tStart - Start time; Default is SOD (Start Of Day)
	 * \param tEnd - End time; Default is EOD (End Of Day)
	 */
	void PumpTapeSlice( const char *tStart = "00:00:00",
	                    const char *tEnd   = "23:59:59" )
	{
	   std::string tkr( tStart );

	   tkr += "|";
	   tkr += tEnd;
	   if ( _attr._bTape )
	      Subscribe( pSvrHosts(), tkr.data(), (void *)0 );
	}

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
	 * String-ified time format is either "YYYYMMDD HH:MM:SS" or "HH:MM:SS"
	 *
	 * \param tStart - Start time
	 * \param tEnd - End time
	 * \param tInterval - Interval in seconds
	 * \param pFlds - CSV list of Field IDs or Names of interest
	 */
	void PumpTapeSliceSample( const char *tStart,
	                          const char *tEnd,
	                          int         tInterval,
	                          const char *pFlds )
	{
	   std::string tkr;
	   char        buf[K];

	   // Pre-condition

	   if ( !tInterval || !pFlds )
	      return PumpTapeSlice( tStart, tEnd );

	   // OK : Tape Slice Sample

	   sprintf( buf, "%d", tInterval ? tInterval : 60 );
	   tkr  = tStart ? tStart : "00:00:00";
	   tkr += "|";
	   tkr += tEnd ? tEnd : "23:59:59.999999";
	   tkr += "|";
	   tkr += buf;
	   tkr += "|";
	   tkr += pFlds ? pFlds : _dflt_fld;
	   if ( _attr._bTape )
	      Subscribe( pSvrHosts(), tkr.data(), (void *)0 );
	}

	/** \brief Stop pumping data from tape */
	void StopTape()
	{
	   if ( _attr._bTape )
	      Unsubscribe( pSvrHosts(), _tape_all );
	}

	/**
	 * \brief Requests channel be set in binary
	 *
	 * The channel protocol is set once in Start().  Therefore, this
	 * must be called BEFORE calling Start().
	 *
	 * \param bBin - true to set to binary; Else ASCII (default)
	 */
	void SetBinary( bool bBin )
	{
	   _bBinary = bBin;
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
	 * Setting to true allows you to have all subscribers share a
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
	 * \brief Sets the channel so that the subscription ID passed to 
	 * the rtEdgeCache3 server is the 3rd argument passed to Subscribe().
	 *
	 * The subscription ID (Stream ID) is included in the data stream by  
	 * the rtEdgeCache3 server.  Setting to true allows you to define what 
	 * that value is (e.g., record / playback server).
	 *
	 * \param bUsrStreamID - true to set to randomize; false to round robin
	 */
	void SetUserStreamID( bool bUsrStreamID )
	{
	   _bUsrStreamID = bUsrStreamID;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_userDefStreamID, (void *)_bUsrStreamID );
	}

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
	void SetTapeDirection( bool bTapeDir ) 
	{
	   _bTapeDir = bTapeDir;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_tapeDirection, (void *)_bTapeDir );
	}

	/**
	 * \brief Return Tape start time in Unix Time; 0 if not tape
	 *
	 * \return Tape start time in Unix Time; 0 if not tape
	 */
	time_t GetTapeStartTime()
	{
	   u_int64_t tm;

	   tm = 0;
	   if ( _cxt && _attr._bTape )
	      ::rtEdge_ioctl( _cxt, ioctl_tapeStartTime, (void *)&tm );
	      return tm;
	   }

	/**
	 * \brief Return Tape end (last insert) time in Unix Time; 0 if not tape
	 *
	 * \return Tape end (last insert) time in Unix Time; 0 if not tape
	 */
	time_t GetTapeEndTime()
	{
	   u_int64_t tm;

	   tm = 0;
	   if ( _cxt && _attr._bTape )
	      ::rtEdge_ioctl( _cxt, ioctl_tapeEndTime, (void *)&tm );
	   return tm;
	}


	////////////////////////////////////
	// Database Directory Query
	////////////////////////////////////
	/**
	 * \brief Query ChartDB or Tape for directory of all tickers
	 *
	 * \return ::MDDResult struct with list of all tickers
	 */
	::MDDResult Query()
	{
	   FreeResult();
	   _qryAll = ::MDD_Query( _cxt );
	   return _qryAll;
	}

	/**
	 * \brief Release resources associated with the last call to Query().
	 */
	void FreeResult()
	{
	   ::MDD_FreeResult( &_qryAll );
	   ::memset( &_qryAll, 0, sizeof( _qryAll ) );
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
	   StopTape();
	   _bStrDb.clear();
	   _chainDb.clear();
	   _subChans.Remove( _cxt );
	   Channel::Stop();
	   if ( _msg )
	      delete _msg;
	   if ( _msgQ )
	      delete _msgQ;
	   if ( _msgP )
	      delete _msgP;
	   _msg  = (Message *)0;
	   _msgQ = (Message *)0;
	   _msgP = (Message *)0;
	}


	////////////////////////////////////
	// Subscribe / Unsubscribe
	////////////////////////////////////
public:
	/**
	 * \brief Open a subscription stream for ( svc, tkr ) data stream
	 *
	 * Calls ::rtEdge_Subscribe() to open a subscription stream for this
	 * ( Service, Ticker ) tuple.  Market data updates are returned in 
	 * the following asynchronous calls:
	 * + OnData()
	 * + OnStale()
	 * + OnError()
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \param arg - User-supplied argument returned in OnData()
	 * \return Unique Stream ID
	 */
	int Subscribe( const char *svc, const char *tkr, void *arg )
	{
	   return ::rtEdge_Subscribe( _cxt, svc, tkr, arg );
	}

	/**
	 * \brief Closes a subscription stream for ( svc, tkr ) data stream
	 *
	 * Calls ::rtEdge_Unsubscribe() to close a subscription stream for this
	 * ( Service, Ticker ) tuple.  Market data updates are stopped
	 *
	 * \param svc - Service name (e.g. BLOOMBERG) passed into Subscribe()
	 * \param tkr - Ticker name (e.g. EUR CURNCY) passed into Subscribe()
	 * \return Unique Stream ID
	 */
	int Unsubscribe( const char *svc, const char *tkr )
	{
	   return ::rtEdge_Unsubscribe( _cxt, svc, tkr );
	}


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
	int Subscribe( ByteStream &bStr )
	{
	   Locker       lck( _bStrMtx );
	   ByteStreams &bdb = _bStrDb;
	   int          idx;

	   // Pre-condition

	   if ( !IsBinary() ) {
	      bStr.OnError( "Subscription Channel must be binary" );
	      return 0;
	   }

	   idx      = Subscribe( bStr.Service(), bStr.Ticker(), (void *)0 );
	   bdb[idx] = &bStr;
	   bStr.SetStreamID( idx );
	   return idx;
	}

	/**
	 * \brief Closes a subscription stream for this Byte Stream
	 *
	 * \param bStr - ByteStream we subscribed to
	 * \return Unique Stream ID
	 */
	int Unsubscribe( ByteStream &bStr )
	{
	   Locker                lck( _bStrMtx );
	   ByteStreams::iterator it;
	   int                   sid, rtn;

	   rtn = Unsubscribe( bStr.Service(), bStr.Ticker() );
	   sid = bStr.StreamID();
	   if ( (it=_bStrDb.find( sid )) != _bStrDb.end() )
	      _bStrDb.erase( it );
	   bStr.SetStreamID( 0 );
	   return rtn;
	}


	////////////////////////////////////
	// Chain
	////////////////////////////////////
public:
	/**
	 * \brief Subscribe to any and all unsubscribed ChainLinks and
	 * ChainRecords in a chain
	 *
	 * \param chn - Chain to subscribe to
	 * \return Number of streams closed
	 */
	int Subscribe( Chain &chn )
	{
	   Locker       lck( _bStrMtx );
	   Chains       &cdb = _chainDb;
	   ChainLinks   &ldb = chn.links();
	   ChainRecords &rdb = chn.records();
	   ChainLink    *lnk;
	   ChainRecord  *rec;
	   size_t        i;
	   int           rtn, sid;

	   // 1) ChainLinks

	   for ( i=0,rtn=0; i<ldb.size(); i++ ) {
	      lnk = ldb[i];
	      sid = lnk->StreamID();
	      if ( sid != 0 )
	         continue; // for-i
	      sid  = Subscribe( chn.svc(), lnk->name(), (void *)0 );
	      rtn += 1;
	      chn._SetStreamID( lnk, sid );
	      cdb[sid] = &chn;
	   }

	   // 2) ChainRecords

	   for ( i=0; i<rdb.size(); i++ ) {
	      rec = rdb[i];
	      sid = rec->StreamID();
	      if ( sid != 0 )
	         continue; // for-i
	      sid  = Subscribe( chn.svc(), rec->name(), (void *)0 );
	      rtn += 1;
	      chn._SetStreamID( rec, sid );
	      cdb[sid] = &chn;
	   }
	   return rtn;
	}

	/**
	 * \brief Closes ALL subscription streams for this Chain
	 *
	 * \param chn - Chain subscribed to
	 * \return Number of streams closed
	 */
	int Unsubscribe( Chain &chn )
	{
	   Locker       lck( _bStrMtx );
	   Chains          &cdb = _chainDb;
	   ChainLinks      &ldb = chn.links();
	   ChainRecords    &rdb = chn.records();
	   Chains::iterator ct;
	   ChainLink       *lnk;
	   ChainRecord     *rec;
	   size_t           i;
	   int              rtn, sid;

	   // 1) ChainLinks

	   for ( i=0,rtn=0; i<ldb.size(); i++ ) {
	      lnk = ldb[i];
	      sid = lnk->StreamID();
	      if ( sid == 0 )
	         continue; // for-i
	      Unsubscribe( chn.svc(), lnk->name() );
	      rtn += 1;
	      chn._SetStreamID( lnk, 0 );
	      if ( (ct=cdb.find( sid )) != cdb.end() )
	         cdb.erase( ct );
	   }

	   // 2) ChainRecords

	   for ( i=0; i<rdb.size(); i++ ) {
	      rec = rdb[i];
	      sid = rec->StreamID();
	      if ( sid == 0 )
	         continue; // for-i
	      Unsubscribe( chn.svc(), rec->name() );
	      rtn += 1;
	      chn._SetStreamID( rec, 0 );
	      if ( (ct=cdb.find( sid )) != cdb.end() )
	         cdb.erase( ct );
	   }
	   return rtn;
	}


	////////////////////////////////////
	// BDS / Symbol List
	////////////////////////////////////
public:
	/**
	 * \brief Subscribe to a Broadcast Data Stream (BDS) from a service.
	 *
	 * You receive asynchronous market data updates via OnSymbol()
	 *
	 * \param svc - Name of service supplying the BDS
	 * \param bds - BDS name
	 * \param arg - User-supplied argument returned in OnData()
	 * \return Unique Stream ID
	 * \see CloseBDS
	 * \see OnSymbol
	 */
	int OpenBDS( const char *svc, const char *bds, void *arg=(void *)0 )
	{
	   std::string tkr( _BDS_PFX );
	   int         sid;

	   tkr += bds;
	   sid  = Subscribe( svc, tkr.data(), arg );
	   {
	      Locker lck( _bStrMtx );

	      _symListDb.insert( sid );
	   }
	   return sid;
	}

	/**
	 * \brief Close a BDS stream that was opened via OpenBDS()
	 *
	 * \param svc - Name of service supplying the BDS
	 * \param bds - BDS name
	 * \return Unique Stream ID if successful; 0 if not
	 * \see OpenBDS
	 */
	int CloseBDS( const char *svc, const char *bds )
	{
	   SymLists          &sdb = _symListDb;
	   SymLists::iterator it;
	   std::string        tkr( _BDS_PFX );
	   int                sid;

	   tkr += bds;
	   sid = Unsubscribe( svc, tkr.data() );
	   {
	      Locker lck( _bStrMtx );

	      if ( (it=sdb.find( sid )) != sdb.end() )
	         sdb.erase( it );
	   }
	   return sid;
	}


	////////////////////////////////////
	// Cache Query
	////////////////////////////////////
public:
	/**
	 * \brief Query internal cache for current values.
	 *
	 * Only valid if SetCache() has been called BEFORE Start().
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \return Current values in Message
	 */
	Message *QueryCache( const char *svc, const char *tkr )
	{
	   Message *rtn;

	   if ( !IsCache() )
	      return (Message *)0;
	   _qry = ::rtEdge_GetCache( _cxt, svc, tkr );
	   rtn  = &_msgQ->Set( &_qry, (mddFieldList *)0 );
	   return rtn;
	}


	////////////////////////////////////
	// Parse Only
	////////////////////////////////////
public:
	/**
	 * \brief Parse a raw message : NOT THREAD SAFE
	 *
	 * You normally call Parse() on Tape channels where you have stored
	 * the raw buffer from your callback.
	 *
	 * \param data - Raw data to parse
	 * \param dLen - Raw data length
	 * \return Message containing parsed results
	 */
	Message *Parse( const char *data, int dLen )
	{
	   rtEdgeData &d = _dParse;

	   // Pre-condition

	   if ( !IsValid() )
	      return (Message *)0;
	   
	   // 1 field per byte is conservative

	   ::memset( &d, 0, sizeof( d ) );
	   if ( _flParse._nAlloc < dLen ) {
	      ::mddFieldList_Free( _flParse );
	      _flParse = ::mddFieldList_Alloc( dLen );
	   }

	   // Set up struct

	   d._flds    = (rtFIELD *)_flParse._flds;
	   d._nFld    = _flParse._nAlloc;
	   d._rawData = data;
	   d._rawLen  = dLen;

	   // Parse and return

	   if ( ::rtEdge_Parse( _cxt, &d ) ) {
	      if ( !_msgP ) {
	         _msgP = new Message( _cxt );
	         _msgP->SetParseOnly( true );
	      }
	      _msgP->Set( &d, &_msgSchema );
	      return _msgP;
	   }
	   return (Message *)0;
	}


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
	 * To pump a 'slice', you will need to store the Message.TapePos() from 
	 * the last message received in previous call to PumpFullTape(), then use this
	 * as the off0 in next call to PumpFullTape().
	 *
	 * \param off0 - Beginning offset, or 0 for beginning of tape
	 * \param nMsg - Number of msgs to pump; 0 for all
	 * \return Unique Tape Pumping ID; Kill pump via StopPumpFullTape()
	 * \see StopPumpFullTape()
	 */
	int PumpFullTape( u_int64_t off0, int nMsg )
	{
	   if ( IsValid() )
	      return ::rtEdge_StartPumpFullTape( _cxt, off0, nMsg );
	   return 0;
	}

	/**
	 * \brief Stop pumping from tape
	 *
	 * \param pumpID - Pump ID returned from PumpFullTape()
	 * \return 1 if stopped; 0 if invalid Pump ID
	 * \see PumpFullTape()
	 */
	int StopPumpFullTape( int pumpID )
	{
	   if ( IsValid() )
	      return ::rtEdge_StopPumpFullTape( _cxt, pumpID );
	   return 0;
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
	 * \brief Called asynchronously when a real-time publisher changes 
	 * state (goes UP or DOWN) in the rtEdgeCache3.
	 *
	 * Override this method in your application to take action when
	 * a new publisher goes online or offline.  The library transparently 
	 * re-subscribes any and all streams you have Subscribe()'ed to when 
	 * the service comes back UP.
	 *
	 * \param svc - Real-time service : BLOOMBEG, IDN_RDF, etc.
	 * \param bUP - true if connected; false if disconnected
	 */
	virtual void OnService( const char *svc, bool bUP )
	{ ; }

	/**
	 * \brief Called asynchronously when real-time market data arrives
	 * on this subscription channel from rtEdgeCache3
	 *
	 * Override this method in your application to consume market data.
	 *
	 * <b> THE DATA IN THE msg ARGUMENT IS VOLATILE AND ONLY GOOD FOR THE
	 * LIFE OF THIS CALL </b>
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnData( Message &msg )
	{ ; }

	/**
	 * \brief Called asynchronously when the real-time market data stream
	 * is recovering
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnRecovering( Message &msg )
	{ ; }

	/**
	 * \brief Called asynchronously when the real-time market data stream
	 * goes stale.
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnStale( Message &msg )
	{ ; }

	/**
	 * \brief Called asynchronously when real-time market data stream
	 * opened via Subscribe() becomes DEAD.
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 * \param pErr - Error string
	 */
	virtual void OnDead( Message &msg, const char *pErr )
	{ ; }

	/**
	 * \brief Called asynchronously when the real-time market data stream
	 * is complete
	 *
	 * Override this method in your application to consume market data.
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnStreamDone( Message &msg )
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
	virtual void OnSymbol( Message &msg, const char *sym )
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


	////////////////////////////////////
	// Asynchronous Callback (private)
	////////////////////////////////////
private:
	void _OnData( rtEdgeData d )
	{
	   mddFieldList &fl  = _msgSchema;
	   mddField     *fdb = fl._flds;
	   FieldDef     *sd;
	   mddField      fz;
	   int           i, nf, fid;

	   ::memset( &fz, 0, sizeof( fz ) );
	   nf = d._nFld;
	   for ( i=0; i<nf; i++ ) {
	      fid    = d._flds[i]._fid;
	      sd     = _schema.GetDef( fid );
	      fdb[i] = sd ? sd->mdd() : fz;
	   }
	   fl._nFld = nf;

	   // ByteStream??  Chain?? Else regular stream

	   ByteStreams          &bdb = _bStrDb;
	   Chains               &cdb = _chainDb;
	   SymLists             &sdb = _symListDb;
	   ByteStreams::iterator bt;
	   Chains::iterator      ct;
	   SymLists::iterator    st;
	   ByteStream           *bStr;
	   Chain                *chn;
	   bool                  bBDS;
	   Field                *fld;

	   {
	      Locker lck( _bStrMtx );

	      bt   = bdb.find( d._StreamID );
	      ct   = cdb.find( d._StreamID );
	      st   = sdb.find( d._StreamID );
	      bStr = ( bt != bdb.end() ) ? (*bt).second : (ByteStream *)0;
	      chn  = ( ct != cdb.end() ) ? (*ct).second : (Chain *)0;
	      bBDS = ( st != sdb.end() );
	   }
	   if ( bStr ) {
	      if ( bStr->_OnData( *this, _msg->Set( &d, &fl ) ) ) {
	         if ( bStr->IsSnapshot() )
	            Unsubscribe( *bStr );
	      }
	   }
	   else if ( chn ) {
	      if ( chn->_OnData( *this, _msg->Set( &d, &fl ) ) )
	         Subscribe( *chn );
	   }
	   else if ( bBDS ) {
	      size_t      pfxLen = strlen( _BDS_PFX );
	      std::string tkr( d._pTkr+pfxLen );

	      d._pTkr = tkr.data();
	      _msg->Set( &d, &fl );
	      if ( _msg->IsImage() || _msg->IsUpdate() ) {
	         for ( i=0; i<_NUM_LINK; i++ ) {
	            if ( (fld=_msg->GetField( LONGLINK1+i )) )
	               OnSymbol( *_msg, fld->GetAsString() );
	         }
	      }
	      else if ( _msg->IsDead() )
	         OnDead( *_msg, _msg->Error() );
	   }
	   else {
	      _msg->Set( &d, &fl );
	      if ( _msg->IsImage() || _msg->IsUpdate() )
	         OnData( *_msg );
	      else if ( _msg->IsRecovering() )
	         OnRecovering( *_msg );
	      else if ( _msg->IsStale() )
	         OnStale( *_msg );
	      else if ( _msg->IsDead() )
	         OnDead( *_msg, _msg->Error() );
	      else if ( _msg->IsStreamDone() )
	         OnStreamDone( *_msg );
	   }
	}

	void _OnSchema( rtEdgeData d )
	{
	   _schema.Initialize( _msg->Set( &d, (mddFieldList *)0 ) );
	   OnSchema( _schema );
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	std::string  _hosts;
	std::string  _user;
	rtEdgeAttr   _attr;
	Schema       _schema;
	bool         _bBinary;
	bool         _bRandom;
	bool         _bUsrStreamID;
	bool         _bTapeDir;
	Message     *_msg;
	Message     *_msgQ;     // QueryCache()
	Message     *_msgP;     // Parse()
	mddFieldList _flParse;  // Parse()
	rtEdgeData   _qry;
	rtEdgeData   _dParse;
	mddFieldList _msgSchema;
	ByteStreams  _bStrDb;
	Mutex        _bStrMtx;
	Chains       _chainDb;
	SymLists     _symListDb;
	::MDDResult  _qryAll;


	////////////////////////////////////
	// Class-wide (private) callbacks
	////////////////////////////////////
private:
	static void EDGAPI _connCbk( rtEdge_Context cxt, 
	                             const char    *msg, 
	                             rtEdgeState    state )
	{
	   SubChannel *us;

	   if ( (us=_subChans.Get( cxt )) )
	      us->OnConnect( msg, ( state == edg_up ) );
	}

	static void EDGAPI _svcCbk( rtEdge_Context cxt, 
	                            const char    *pSvc, 
	                            rtEdgeState    state )
	{
	   SubChannel *us;

	   if ( (us=_subChans.Get( cxt )) )
	      us->OnService( pSvc, ( state == edg_up ) );
	} 

	static void EDGAPI _dataCbk( rtEdge_Context cxt, rtEdgeData d )
	{
	   SubChannel *us;
	   int         ty;

	   // ( rtEdgeData._ty ==0 ) implies OnIdle()

	   if ( (us=_subChans.Get( cxt )) ) {
	      ty = (int)d._ty;
	      if ( !ty )
	         us->OnIdle();
	      else
	         us->_OnData( d );
	   }
	}

	static void EDGAPI _schemaCbk( rtEdge_Context cxt, rtEdgeData d )
	{
	   SubChannel *us;

	   if ( (us=_subChans.Get( cxt )) )
	      us->_OnSchema( d );
	}

};  // class SubChannel

} // namespace RTEDGE

#endif // __RTEDGE_SubChannel_H 
