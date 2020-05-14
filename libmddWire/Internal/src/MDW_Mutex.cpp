/******************************************************************************
*
*  MDW_Mutex.cpp
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge)
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>

using namespace MDDWIRE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s       M u t e x
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Mutex::Mutex() :
   _tid( 0 ),
   _cnt( 0 )
{
#ifdef WIN32
   ::InitializeCriticalSection( &_mtx );
#else
   ::pthread_mutex_init( &_mtx, 0 );
#endif // WIN32
}

Mutex::~Mutex()
{
#ifdef WIN32
   ::DeleteCriticalSection( &_mtx );
#else
   ::pthread_mutex_destroy( &_mtx );
#endif // WIN32
}


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
pthread_mutex_t *Mutex::mtx()
{
   return &_mtx;
}

void Mutex::Lock()
{
   // Allow locking by same thread

   if ( _cnt && ( _tid == CurrentThreadID() ) ) {
      _cnt += 1;
      return;
   }
#ifdef WIN32
   ::EnterCriticalSection( &_mtx );
#else
   ::pthread_mutex_lock( &_mtx );
#endif // WIN32
    _cnt += 1;
   _tid   = CurrentThreadID();
}

void Mutex::Unlock()
{
   _cnt--;
   if ( _cnt > 0 )
      return;
   _tid = 0;
#ifdef WIN32
   ::LeaveCriticalSection( &_mtx );
#else
   ::pthread_mutex_unlock( &_mtx );
#endif // WIN32
}


////////////////////////////////////////////
// Class-wide
////////////////////////////////////////////
pthread_t Mutex::CurrentThreadID()
{
#ifdef WIN32
   return (pthread_t)::GetCurrentThreadId();
#else
   return ::pthread_self();
#endif // WIN32

}



/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s       E v e n t
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Event::Event() :
   _mtx()
{
#ifdef WIN32
   _cv = ::CreateEvent( NULL, false, false, NULL );
#else
   ::pthread_cond_init( &_cv, (const pthread_condattr_t *)0 );
#endif // WIN32
}

Event::~Event()
{
#ifdef WIN32
   ::CloseHandle( _cv );
#else
   ::pthread_cond_destroy( &_cv );
#endif // WIN32
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
void Event::WaitEvent( double dWait )
{
#ifdef WIN32
   DWORD dw, dMs;

   dMs = dWait ? (DWORD)( dWait * 1000.0 ) : INFINITE;
   dw  = ::WaitForSingleObject( _cv, dMs );
#else
   Locker          lck( _mtx );
   struct timespec ts;

   ::pthread_mutex_lock( _mtx.mtx() );
   if ( dWait ) {
      ts.tv_sec  = (time_t)dWait;
      dWait     -= ts.tv_sec;
      ts.tv_nsec = (long)( dWait * 1000000000.0 );
      ::pthread_cond_timedwait( &_cv, _mtx.mtx(), &ts );
   }
   else
      ::pthread_cond_wait( &_cv, _mtx.mtx() );
#endif // WIN32
}

void Event::SetEvent()
{
   Locker lck( _mtx );

#ifdef WIN32
   ::SetEvent( _cv );
#else
   ::pthread_cond_signal( &_cv );
#endif // WIN32
}



/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      L o c k e r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Locker::Locker( Mutex &mtx ) :
   _mtx( mtx )
{
   _mtx.Lock();
}

Locker::~Locker()
{
   _mtx.Unlock();
}




/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      H a n d s h a k e
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Handshake::Handshake() :
   _evt1(),
   _evt2()
{
}

Handshake::~Handshake()
{
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
void Handshake::Connect()
{
   _evt1.SetEvent();
   _evt2.WaitEvent();
}

void Handshake::Accept()
{
   _evt1.WaitEvent();
   _evt2.SetEvent();
}

