/******************************************************************************
*
*  Mutex.hpp
*
*  REVISION HISTORY:
*     24 JAN 2015 jcs  Created.
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Mutex_H
#define __RTEDGE_Mutex_H

#ifdef WIN32
#include <windows.h>
#define pthread_t       u_long 
#define pthread_cond_t  HANDLE
#define pthread_mutex_t CRITICAL_SECTION 
#else
#include <pthread.h>
#if !defined(DWORD)
#define DWORD           u_long
#endif // !defined(DWORD)
#endif // WIN32

namespace RTEDGE
{

////////////////////////////////////////////////
//
//        c l a s s   M u t e x
//
////////////////////////////////////////////////

/**
 * \class Mutex
 * \brief Wrapper class around OS-specifc mutex for synchronizing 
 * access to critical parts of your code.
 *
 * Typically used with the Locker class.
 */

class Mutex
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
	/** \brief Constructor */
public:
	Mutex() :
		_tid( 0 ),
		_cnt( 0 )
	{
#ifdef WIN32
		::InitializeCriticalSection( &_mtx );
#else
		::pthread_mutex_init( &_mtx, 0 );
#endif // WIN32
	}

	/** \brief Destructor */
	~Mutex()
	{
#ifdef WIN32
		::DeleteCriticalSection( &_mtx );
#else
		::pthread_mutex_destroy( &_mtx );
#endif // WIN32
	}

	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
	/**
	 * \brief Return reference to native OS mutex  
	 *
	 * \return Reference to native OS mutex  
	 */
	pthread_mutex_t *mtx()
	{
		return &_mtx;
	}

	/**
	 * \brief Return thread ID of locking thread.
	 *
	 * \return Thread ID of locking thread.
	 */
	pthread_t tid()
	{
		return _tid;
	}

	/** \brief Lock the Mutex */
	void Lock()
	{
#ifdef WIN32
		::EnterCriticalSection( &_mtx );
#else
		::pthread_mutex_lock( &_mtx );
#endif // WIN32
		_cnt += 1;
		_tid   = CurrentThreadID();
	}

	/** \brief Unlock the locked Mutex */ 
	void Unlock()
	{
		_cnt--;
		_tid = 0;
#ifdef WIN32
		::LeaveCriticalSection( &_mtx );
#else
		::pthread_mutex_unlock( &_mtx );
#endif // WIN32
	}


	////////////////////////////////////
	// Class-wide
	////////////////////////////////////
	/**
	 * \brief Returns thread ID of calling thread
	 *
	 * \return Thread ID of calling thread
	 */
	pthread_t CurrentThreadID()
	{
#ifdef WIN32
		return (pthread_t)::GetCurrentThreadId();
#else
		return ::pthread_self();
#endif // WIN32
	}

	////////////////////////
	// Private Members
	////////////////////////
private:
	pthread_mutex_t _mtx;
	pthread_t       _tid;
	int             _cnt;

}; // class Mutex



////////////////////////////////////////////////
//
//       c l a s s   L o c k e r
//
////////////////////////////////////////////////

/**
 * \class Locker
 * \brief Class that Locks the referenced Mutex in the constructor and 
 * Unlocks in the destructor.
 *
 * The Locker class has logic to allow you to lock the Mutex multiple 
 * times in the same calling thread (call stack) but only locking the 
 * the Mutex once.  When the stack unwinds, Locker unlocks the Mutex 
 * once.  This is required on platforms that will block if you lock the 
 * OS Mutex a 2nd time in the same calling thread.
 *
 * The Locker is normally used as an automatic variable on the stack to 
 * synchronize critical sections of code. 
 */
class Locker
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
	/*
	 * \brief Constructor.  Locks the associated Mutex
	 *
	 * \param mtx - Mutex to Lock in constructor and Unlock in destructor 
	 */
public:
	Locker( Mutex &mtx ) :
		_mtx( mtx ),
		_bLock( mtx.tid() != mtx.CurrentThreadID() )
	{
		if ( _bLock )
			_mtx.Lock();
	}

	/*
	 * \brief Destructor.  Locks the associated Mutex
	 *
	 * \param mtx - Mutex to Lock in constructor and Unlock in destructor 
	 */
	~Locker()
	{
		if ( _bLock )
			_mtx.Unlock();
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	Mutex &_mtx;
	bool   _bLock;

}; // class Locker

}  // namespace RTEDGE

#endif // __RTEDGE_Mutex_H
