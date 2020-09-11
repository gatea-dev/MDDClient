/******************************************************************************
*
*  Thread.h
*
*  REVISION HISTORY:
*     28 JUL 2009 jcs  Created
*     17 AUG 2009 jcs  Build  2: pump()
*     12 NOV 2014 jcs  Build 29: RTEDGE_PRIVATE
*      5 FEB 2015 jcs  Build 30: No mo Add() / Remove()
*     24 APR 2015 jcs  Build 31: SetThreadProcessor()
*     15 APR 2016 jcs  Build 32: EDG_Internal.h; tid()
*      6 MAR 2018 jcs  Build 40: _fcn / _arg
*      7 SEP 2020 jcs  Build 44: SetName()
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_THREAD_H
#define __EDGLIB_THREAD_H
#include <EDG_Internal.h>

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class Socket;

/////////////////////////////////////////
// Thread
/////////////////////////////////////////
class Thread : public string
{
protected:
	rtEdgeThreadFcn _fcn;
	void           *_arg;
	bool            _bRun;
	pthread_t       _tid;
	HANDLE          _hThr;
	Pump            _pump;
	Handshake       _ready;

	// Constructor / Destructor
public:
	Thread();
	Thread( rtEdgeThreadFcn, void * );
	~Thread();

	// Access / Operations

	Pump       &pump();
	pthread_t   tid();
	bool        IsOurThread();
	bool        IsRunning();
	int         SetThreadProcessor( int );
	int         GetThreadProcessor();
	int         SetName( const char * );
	const char *GetName();

	// Thread Shit

	void  Start();
	void  Stop();
protected:
	void *Run();

	// Class-wide

	static THR_ARG _thrProc(void * );
public:
	static pthread_t CurrentThreadID();
};
}
 // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_THREAD_H
