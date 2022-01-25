/******************************************************************************
*
*  Mutex.h
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     28 JUL 2009 jcs  Build  4: WIN32
*     20 MAR 2012 jcs  Build 18: WaitEvent( double )
*     12 NOV 2014 jcs  Build 28: Semaphore; RTEDGE_PRIVATE
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*     25 JAN 2022 jcs  Build 51: pthread_t as 64-bit on WINxx
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_MUTEX_H
#define __EDGLIB_MUTEX_H
#include <EDG_Internal.h>

#ifdef WIN32
#define pthread_t       u_int64_t 
#define pthread_cond_t  HANDLE
#define pthread_mutex_t CRITICAL_SECTION 
#else
#include <semaphore.h>
#define DWORD           u_long
#endif // WIN32

namespace RTEDGE_PRIVATE
{

/////////////////////////////////////////
// Mutex 
/////////////////////////////////////////
class Mutex
{
protected:
	pthread_mutex_t _mtx;
	pthread_t       _tid;
	int             _cnt;

	// Constructor / Destructor
public:
	Mutex();
	~Mutex();

	// Access / Operations

	pthread_mutex_t *mtx();
	pthread_t        tid();
	void             Lock();
	void             Unlock();

	// Class-wide
public:
	static pthread_t CurrentThreadID();
};

/////////////////////////////////////////
// Event
/////////////////////////////////////////
class Event
{
protected:
	Mutex          _mtx;
	pthread_cond_t _cv;

	// Constructor / Destructor
public:
	Event();
	~Event();

	// Operations

	void WaitEvent( double dWait=0.0 );
	void SetEvent();
};


/////////////////////////////////////////
// Semaphore
/////////////////////////////////////////
class Semaphore
{
protected:
#ifdef WIN32
	HANDLE _sem;
#else
	sem_t  _SEM;
	sem_t *_sem;
#endif // WIN32
	int    _maxCount;
	int    _curCount;
	char  *_name;

	// Constructor / Destructor
public:
	Semaphore( int   iInitialCount = 1,
	           int   iMaxCount     = 1,
	           char *pName         = NULL );
	Semaphore( char * );
	~Semaphore();

	// Access / Operations

	char *name();
	bool  Lock( DWORD waitMillis=INFINITE );
	void  Unlock( int iCount=1 );
	void  LockAll();
	void  UnlockAll();

	// Helpers
protected:
	void _SetName( char * );
};

/////////////////////////////////////////
// Locker 
/////////////////////////////////////////
class Locker
{
protected:
	Mutex &_mtx;
	bool   _bLock;

	// Constructor / Destructor
public:
	Locker( Mutex & );
	~Locker();
};

/////////////////////////////////////////
// Handshake
/////////////////////////////////////////
class Handshake
{
protected:
	Event _evt1;
	Event _evt2;

	// Constructor / Destructor
public:
	Handshake();
	~Handshake();

	// Operations

	void Connect();
	void Accept();
};

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_MUTEX_H
