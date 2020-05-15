/******************************************************************************
*
*  Mutex.h
*
*  REVISION HISTORY:
*     14 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_MUTEX_H
#define __YAMR_MUTEX_H
#include <Internal.h>

#ifdef WIN32
#define pthread_t       u_long 
#else
#define DWORD           u_long
#endif // WIN32

namespace YAMR_PRIVATE
{

/////////////////////////////////////////
// Mutex 
/////////////////////////////////////////
class Mutex
{
protected:
	pthread_t       _tid;
	volatile long   _cnt;
	volatile long   _atomicLock;

	// Constructor / Destructor
public:
	Mutex();
	~Mutex();

	// Access / Operations

	pthread_t tid();
	void      Lock();
	void      Unlock();

	// Class-wide
public:
	static pthread_t CurrentThreadID();

}; // class Mutex


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

}; // class Locker

} // namespace YAMR_PRIVATE

#endif // __YAMR_MUTEX_H
