/******************************************************************************
*
*  MDW_Mutex.h
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge)
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __MDW_MUTEX_H
#define __MDW_MUTEX_H
#include <MDW_Internal.h>

#ifdef WIN32
#define pthread_t       u_long 
#define pthread_cond_t  HANDLE
#define pthread_mutex_t CRITICAL_SECTION 
#endif // WIN32


/////////////////////////////////////////
// Mutex 
/////////////////////////////////////////
namespace MDDWIRE_PRIVATE
{

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
// Locker 
/////////////////////////////////////////
class Locker
{
protected:
	Mutex &_mtx;

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

} // namespace MDDWIRE_PRIVATE

#endif // __MDW_MUTEX_H
