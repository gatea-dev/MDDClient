/******************************************************************************
*
*  Pump.h
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     17 AUG 2009 jcs  Build  2: IdleFcns
*      5 OCT 2009 jcs  Build  5: TimerTable
*     26 SEP 2010 jcs  Build  8: Class-wide _sockMsg
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*     11 SEP 2023 jcs  Build 32: _className
*      5 OCT 2023 jcs  Build 65: poll() only
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef _EDGLIB_PUMP_H
#define _EDGLIB_PUMP_H
#include <EDG_Internal.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/poll.h>
#endif // !defined(_WIN32) && !defined(_WIN64)

#define _MAX_POLLFD   64  // < 64 channels / thread

#if defined(WIN32)
typedef struct pollfd {
   int   fd;            /* file desc to poll */
   short events;        /* events of interest on fd */
   short revents;       /* events that occurred on fd */
} pollfd_t;
int poll(struct pollfd *, unsigned long, int);
#define POLLIN      0x0001      /* fd is readable */
#define POLLPRI     0x0002      /* high priority info at fd */
#define POLLOUT     0x0004      /* fd is writeable (won't block) */
#define POLLERR     0x0008      /* fd has error condition */
#define POLLHUP     0x0010      /* fd has been hung up on */
#define POLLNVAL    0x0020      /* invalid pollfd entry */
#define POLLRDNORM  0x0040      /* normal data is readable */
#define POLLWRNORM  POLLOUT
#define POLLRDBAND  0x0080      /* out-of-band data is readable */
#define POLLWRBAND  0x0100      /* out-of-band data is writeable */
#elif defined(linux)
#endif

// Idle Processing

typedef void (*EdgIdleFcn)( void * );
class EdgIdleCbk
{
public:
   EdgIdleFcn _fcn;
   void      *_arg;
};


namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class PollMap;
class Socket;
class Thread;
class TimerEvent;

// Collections

typedef hash_map<int, Socket *> Sockets;
typedef vector<int>             SoxFD;
typedef vector<EdgIdleCbk>      IdleFcns;
typedef vector<TimerEvent *>    TimerTable;


/////////////////////////////////////////
// Event Pump
/////////////////////////////////////////
class Pump
{
protected:
	Sockets    _sox;
	TimerTable _tmrs;
	SoxFD      _dels;
	IdleFcns   _idle;
	Mutex      _mtx;
	PollMap   *_pollMap;
	int        _maxFd;
#ifdef WIN32
public:
	HWND      _hWnd;
	UINT      _tmrID;
#endif // WIN32
protected:
	bool      _bRun;
	double    _t0;

	// Constructor / Destructor
public:
	Pump();
	~Pump();

	// Access

	Mutex &mtx() { return _mtx; }
	bool   IsRunning() { return _bRun; }

	// Socket Operations

	void Add( Socket * );
	void Remove( Socket * );

	// Timer Operations

	void AddTimer( TimerEvent & );
	void RemoveTimer( TimerEvent & );
private:
	void _OnTimer();

	// Thread Operations
public:
	void Start();
	void Run( double dPoll );
	void Stop();
	void AddIdle( EdgIdleFcn, void * ); 
	void RemoveIdle( EdgIdleFcn );

	// Helpers
private:
	void _Create();
	void _Destroy();
	int  buildFDs();

	// WIN32 Handlers
#ifdef WIN32
public:
	LRESULT wndProc( WPARAM, LPARAM );
	void    tmrProc( DWORD );
#endif // WIN32
};

/////////////////////////////////////////
// ::poll()-based map
/////////////////////////////////////////
class PollMap
{
public:
	static short _rdEvts;
	static short _wrEvts;
	static short _exEvts;
protected:
	Pump          &_pmp;
	struct pollfd *_pollList;
	int            _maxx;
	int            _nPoll;

	// Constructor / Destructor
public:
	PollMap( Pump & );
	~PollMap();

	// Access

	struct pollfd *pollList();
	int            nPoll();

	// Operations

	void buildFDs( Sockets & );
	void Reset();
};


} // namespace RTEDGE_PRIVATE

#endif // _EDGLIB_PUMP_H
