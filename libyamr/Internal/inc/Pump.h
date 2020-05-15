/******************************************************************************
*
*  Pump.h
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_PUMP_H
#define __YAMR_PUMP_H
#include <Internal.h>

// Idle Processing

typedef void (*_yamrIdleFcn)( void * );

class _yamrIdleCbk
{
public:
   _yamrIdleFcn _fcn;
   void        *_arg;
};

namespace YAMR_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class Socket;
class TimerEvent;

// Collections

typedef map<int, Socket *>   Sockets;
typedef vector<int>          SoxFD;
typedef vector<_yamrIdleCbk> IdleFcns;
typedef vector<TimerEvent *> TimerTable;


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
	fd_set     _rds;
	fd_set     _wrs;
	fd_set     _exs;
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
	void Run( double dPoll=0.1 );
	void Stop();
	void AddIdle( _yamrIdleFcn, void * ); 
	void RemoveIdle( _yamrIdleFcn );

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

} // namespace YAMR_PRIVATE

#endif // __YAMR_PUMP_H
