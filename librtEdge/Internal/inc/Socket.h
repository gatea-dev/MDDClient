/******************************************************************************
*
*  Socket.h
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*      5 OCT 2009 jcs  Build  4: TimerEvent
*     18 SEP 2010 jcs  Build  7: Optional Thread
*     24 JAN 2012 jcs  Build 17: mtx()
*     22 MAR 2012 jcs  Build 18: NagleOff()
*     11 FEB 2013 jcs  Build 23: NULL's : _cp / _inSz instead of _rdData
*     11 JUL 2013 jcs  Build 26a:rtEdgeChanStats
*     31 JUL 2013 jcs  Build 27: _bStart
*     12 NOV 2014 jcs  Build 28: libmddWire; mddBldBuf _out
*      8 JAN 2015 jcs  Build 29: _bRandomize
*     18 JUN 2015 jcs  Build 31: SetRcvBuf() / GetRcvBuf(); Buffer
*     10 SEP 2015 jcs  Build 32: _bIdleCbk
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*     26 MAY 2017 jcs  Build 34: WireMold64
*     12 FEB 2020 jcs  Build 42: _tHbeat
*      5 MAY 2022 jcs  Build 53: _bPub
*      6 SEP 2022 jcs  Build 56: _bOverflow
*
*  (c) 1994-2022 Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_SOCKET_H
#define __EDGLIB_SOCKET_H
#include <EDG_Internal.h>
#include <WireMold64.h>

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class Logger;
class Mutex;
class Pump;
class Thread;

typedef vector<string *>   Hosts;
typedef vector<int>        Ports;

///////////////////
// Buffer
///////////////////
class Buffer
{
public:
	char *_bp;
	char *_cp;
	int   _nAlloc;
	int   _maxSiz;

	// Constructor / Destructor
public:
	Buffer( int, int maxSiz=_MAX_BUF_SIZ );
	~Buffer();

	// Access
public:
	rtBUF buf();
	int   bufSz();
	int   nLeft();

	// Operations
public:
	bool Grow( int );
	void Reset();
	void Move( int, int );
	bool Append( char *, int );

}; // class Buffer


/////////////////////////////////////////
// Socket
/////////////////////////////////////////
class Socket : public TimerEvent
{
friend class Pump; 
public:
	static Logger     *_log;
protected:
	Thread            *_thr;
	bool               _bConnectionless;
	mddWire_Context    _mdd;
	mddProtocol        _proto;
	bool               _bPub;
	Mutex              _mtx;
	Mutex              _blkMtx;
	int                _nBlk;
	int                _flags;
	volatile long      _nNonBlk;
	Hosts              _hosts;
	Ports              _ports;
	int                _udpPort; // Local
	struct sockaddr_in _dst;
	string             _dstConn;
	volatile int       _fd;
	bool               _bStart;
	Buffer             _in;
	Buffer             _out;
	rtEdgeChanStats   *_st;
	rtEdgeChanStats    _dfltStats;
	mddFieldList       _fl;
	mddBldBuf          _bldBuf;
	bool               _bCache;
	bool               _bBinary;
	bool               _bLatency;
	bool               _bRandomize;
	bool               _bIdleCbk;
	Mutex              _ovrFloMtx;
	string             _overflow;
	int                _tHbeat;
	int                _SO_RCVBUF;
	Mold64Pkt          _udp;

	// Constructor / Destructor
public:
	Socket( const char *, bool bConnectionless=false );
	virtual ~Socket();

	// Access / Operations

	Thread     &thr();
	Pump       &pump();
	Mutex      &mtx();
	const char *dstConn();
	int         fd();
	bool        IsCache();
	bool        IsWritable();
	int         SetRcvBuf( int );
	int         GetRcvBuf();

	// Channel Stats

	rtEdgeChanStats &stats();
	void             SetStats( rtEdgeChanStats * );

	// Operations

	const char *Connect();
	bool        Disconnect( const char * );
	bool        Write( const char *, int );

	// Socket Interface

	virtual bool Ioctl( rtEdgeIoctl, void * );

	// Thread Notifications
protected:
	virtual void OnConnect( const char * ) { ; }
	virtual void OnDisconnect( const char * ) { ; }
	virtual void OnRead();
	virtual void OnWrite();
	virtual void OnException();

	// TimerEvent Notifications
protected:
	virtual void On1SecTimer();

	// libmddWire Utilities
protected:
	void        _CheckHeartbeat( u_int );
	void        _SetHeartbeat();
	void        _SendPing();
	mddMsgHdr   _InitHdr( mddMsgType );
	mddBuf      _SetBuf( const char * );
	mddBuf      _SetBuf( const char *, int );
	const char *_GetAttr( mddMsgHdr &, const char * );
	int         _AddAttr( mddMsgHdr &, const char *, char * );

	// Helpers
private:
	int  setNonBlocking();
	void setBlocking();
	int  _GetError();
	bool NagleOff();

}; // class Socket

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_SOCKET_H
