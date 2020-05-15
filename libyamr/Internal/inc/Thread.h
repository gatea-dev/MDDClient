/******************************************************************************
*
*  Thread.h
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_THREAD_H
#define __YAMR_THREAD_H
#include <Internal.h>

namespace YAMR_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class Socket;

/////////////////////////////////////////
// Thread
/////////////////////////////////////////
class Thread
{
protected:
	yamrThreadFcn _fcn;
	void         *_arg;
	bool          _bRun;
	pthread_t     _tid;
	HANDLE        _hThr;
	Pump          _pump;
	volatile bool _ready;

	// Constructor / Destructor
public:
	Thread();
	Thread( yamrThreadFcn, void * );
	~Thread();

	// Access / Operations

	Pump     &pump();
	pthread_t tid();
	bool      IsOurThread();
	bool      IsRunning();
	int       SetThreadProcessor( int );
	int       GetThreadProcessor();

	// Thread Shit

	void  Start();
	void  Stop();
protected:
	void *Run();

	// Class-wide

	static THR_ARG _thrProc(void * );
public:
	static pthread_t CurrentThreadID();

}; // class Thread

} // namespace YAMR_PRIVATE

#endif // __YAMR_THREAD_H
