/******************************************************************************
*
*  Mutex.cpp
*
*  REVISION HISTORY:
*     14 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <Internal.h>

using namespace YAMR_PRIVATE;

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
   _cnt( 0 ),
   _atomicLock( 0 )
{
}

Mutex::~Mutex()
{
}


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
pthread_t Mutex::tid()
{
   return _tid;
}

void Mutex::Lock()
{
   pthread_t ourTid;
   long      i, seq;
   
   // Re-entrant
   
   ourTid = CurrentThreadID();
   if ( _tid == ourTid ) {
      ATOMIC_INC( &_cnt );
      return;
   }

   // Safe to lock
   
   for ( i=0; (seq=ATOMIC_CMP_EXCH( &_atomicLock, 0, 1 )) != 0; i++ );
   ATOMIC_INC( &_cnt );
   ATOMIC_EXCH( &_tid, ourTid );
}

void Mutex::Unlock()
{
   pthread_t ourTid;
   long      i, seq;

   // Re-entrant

   ourTid = CurrentThreadID();
   seq = ATOMIC_DEC( &_cnt );
   if ( ( _tid == ourTid ) && seq )
      return;

   // Unlock

   ATOMIC_EXCH( &_tid, 0 );
   for ( i=0; (seq=ATOMIC_CMP_EXCH( &_atomicLock, 1, 0 )) != 1; i++ );
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
//                 c l a s s      L o c k e r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Locker::Locker( Mutex &mtx ) :
   _mtx( mtx ),
   _bLock( mtx.tid() != mtx.CurrentThreadID() )
{
   if ( _bLock )
      _mtx.Lock();
}

Locker::~Locker()
{
   if ( _bLock )
      _mtx.Unlock();
}

