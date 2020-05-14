/******************************************************************************
*
*  Mutex.cpp
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     28 JUL 2009 jcs  Build  4: WIN32
*     20 MAR 2012 jcs  Build 18: WaitEvent( double )
*     12 NOV 2014 jcs  Build 28: Semaphore; RTEDGE_PRIVATE
*     20 MAR 2016 jcs  Build 32: EDG_Internal.h
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;

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

pthread_t Mutex::tid()
{
   return _tid;
}

void Mutex::Lock()
{
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
   _tid = 0;
#ifdef WIN32
   ::LeaveCriticalSection( &_mtx );
#else
   ::pthread_mutex_unlock( &_mtx );
#endif // WIN32
}

#ifdef FUCKED
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
#endif // FUCKED


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
//                  c l a s s      S e m a p h o r e
//
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
// Constructor / Destructor
///////////////////////////////////////////
Semaphore::Semaphore( int   iInitialCount, 
                      int   iMaxCount, 
                      char *pName ) :
   _maxCount( iMaxCount ),
   _curCount( iInitialCount ),
   _name( (char *)0 )
{
   _SetName( pName );
#ifdef WIN32
   _sem = ::CreateSemaphore( NULL, iInitialCount, iMaxCount, name() );
#else
   mode_t mode;

   mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;  // chmod( 755 )
   if ( name() )
      _sem = ::sem_open( name(), O_CREAT, mode, _maxCount );
   else {
      ::sem_init( &_SEM, 1, _maxCount );
      _sem = &_SEM;
   }
#endif // WIN32
}

Semaphore::Semaphore( char *pName ) :
   _maxCount( 0 ),
   _curCount( 0 ),
   _name( (char *)0 )
{
   _SetName( pName );
#ifdef WIN32
   _sem = ::OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE, name() );
#else
   _sem = ::sem_open( name(), O_RDONLY );
#endif // WIN32
}

Semaphore::~Semaphore()
{
#ifdef WIN32
   ::CloseHandle( _sem );
#else
   ::sem_destroy( _sem );
#endif // WIN32
   if ( _name )
      delete[] _name;
}


///////////////////////////////////////////
// Access / Operations
///////////////////////////////////////////
char *Semaphore::name()
{
   return _name;
}

bool Semaphore::Lock( DWORD waitMillis )
{
   DWORD rtn;
   bool  lck;

   lck = false;
#ifdef WIN32
   rtn = ::WaitForSingleObject( _sem, waitMillis );
   switch( rtn ) {
      case WAIT_OBJECT_0:
         lck = true;
         break;
      default:
         lck = _sem ? false : true;
         break;
   }
#else
   rtn = ::sem_wait( _sem );
   lck = ( rtn == 0 );
#endif // WIN32
   _curCount++;
   return lck;
}

void Semaphore::Unlock( int iCount )
{
#ifdef WIN32
   long lPrev;
   BOOL rtn;
   DWORD err;

   rtn = TRUE;
   if ( _sem )
      rtn = ::ReleaseSemaphore( _sem, iCount, &lPrev );
   err = rtn ? 0 : ::GetLastError();
   _curCount -= iCount;
#else
   while( iCount-- > 0 ) {
      ::sem_post( _sem );
      _curCount--;
   }
#endif // WIN32
}

void Semaphore::LockAll()
{
   int  i;

   for ( i=0; i<_maxCount; Lock(),i++ );
}

void Semaphore::UnlockAll()
{
   Unlock( _maxCount );
}

void Semaphore::_SetName( char *pn )
{
   /*
    * WIN32 : http:msdn.microsoft.com/en-us/library/windows/desktop/ms682438(v=vs.85).aspx
    * Linux : man sem_overview
    */
#ifdef WIN32
   static const char *_pfx = "Global\\";
#else
   static const char *_pfx = "/";
#endif // WIN32
   char *cp, ch;
   int   i, sz;

   if ( !pn )
      return;

   // Replace \\ w/ /; / with _

   sz    = strlen( _pfx ) + strlen( pn );
   _name = new char[sz+4];
   ::memset( _name, 0, sz+4 );
   cp  = _name;
   cp += sprintf( cp, _pfx );
   sprintf( cp, pn );
   sz = strlen( pn );
   for ( i=0; i<sz; i++ ) {
      ch = cp[i];
      switch( ch ) {
         case 0x2f:  // Forward slash
         case 0x5c:  // Backward slash
            cp[i] = '_';
            break;
      }
   }
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

