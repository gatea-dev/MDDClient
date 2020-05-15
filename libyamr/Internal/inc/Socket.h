/******************************************************************************
*
*  Socket.h
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_SOCKET_H
#define __YAMR_SOCKET_H
#include <Internal.h>

namespace YAMR_PRIVATE
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
	char  *_bp;
	char  *_cp;
	int    _nAlloc;
	int    _maxSiz;
	bool   _bHiMark;
	double _dLoBand;
	double _dHiBand;

	// Constructor / Destructor
public:
	Buffer( int, int maxSiz=_DFLT_BUF_SIZ );
	~Buffer();

	// Access
public:
	yamrBuf buf();
	int     bufSz();
	int     nLeft();
	double  pctFull();

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
	bool               _bUDP;
	Mutex              _mtx;
	int                _flags;
	volatile long      _nNonBlk;
	Hosts              _hosts;
	Ports              _ports;
	struct sockaddr_in _dst;
	string             _dstConn;
	volatile int       _fd;
	bool               _bStart;
	Buffer             _in;
	Buffer             _out;
	yamrChanStats     *_st;
	yamrChanStats      _dfltStats;
	bool               _bRandomize;
	bool               _bIdleCbk;
	int                _SO_RCVBUF;

	// Constructor / Destructor
public:
	Socket( const char *, bool );
	virtual ~Socket();

	// Access / Operations

	Thread     &thr();
	Pump       &pump();
	Mutex      &mtx();
	const char *dstConn();
	int         fd();
	Buffer     &out();
	bool        IsWritable();
	int         SetSockBuf( int, bool );
	int         GetSockBuf( bool );

	// Channel Stats

	yamrChanStats &stats();
	void           SetStats( yamrChanStats * );

	// Operations

	const char *Connect();
	bool        Disconnect( const char * );
	bool        Write( const char *, int, bool bFlush=true );

	// Socket Interface
public:
	virtual void Ioctl( yamrIoctl, void * );
protected:
	virtual void OnQLoMark() { ; }
	virtual void OnQHiMark() { ; }

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

	// Helpers
private:
	int  _Write( const char *, int );
	int  setNonBlocking();
	void setBlocking();
	int  _GetError();
	bool NagleOff();
	void _CheckQueueMark( bool );

	// Idle Loop Processing ...
public:
	void        OnIdle();
	static void _OnIdle( void * );

};  // class Socket

} // namespace YAMR_PRIVATE

#endif // __YAMR_SOCKET_H
