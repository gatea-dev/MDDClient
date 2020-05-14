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
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_PUMP_H
#define __EDGLIB_PUMP_H
#include <EDG_Internal.h>

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
class Socket;
class TimerEvent;

// Collections

typedef map<int, Socket *>   Sockets;
typedef vector<int>          SoxFD;
typedef vector<EdgIdleCbk>  IdleFcns;
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

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_PUMP_H
