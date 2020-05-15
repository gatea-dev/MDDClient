/******************************************************************************
*
*  Thread.cpp
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <Internal.h>

using namespace YAMR_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      T h r e a d
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Thread::Thread() :
   _fcn( (yamrThreadFcn)0 ),
   _arg( (void *)0 ),
   _bRun( true ),
   _tid( (pthread_t)0 ),
   _hThr( (HANDLE)0 ),
   _pump(),
   _ready( false )
{
   Start();
}

Thread::Thread( yamrThreadFcn fcn, void *arg ) :
   _fcn( fcn ),
   _arg( arg ),
   _bRun( true ),
   _tid( (pthread_t)0 ),
   _hThr( (HANDLE)0 ),
   _pump(),
   _ready( false )
{
   Start();
}

Thread::~Thread()
{
   Logger *lf;

   if ( (lf=Socket::_log) )
      lf->logT( 3, "~Thread()\n" );
}

 
////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
Pump &Thread::pump()
{
   return _pump;
}

pthread_t Thread::tid()
{  
   return _tid;
}

bool Thread::IsOurThread()
{
   pthread_t ourTid = CurrentThreadID();

   return( ourTid == _tid );
}

bool Thread::IsRunning()
{
   return _bRun;
}

int Thread::SetThreadProcessor( int cpu )
{
#ifdef WIN32
   DWORD  tMsk, rMsk;

   rMsk  = 0;
   tMsk  = ( 1 << cpu );
   rMsk  = ::SetThreadAffinityMask( _hThr, tMsk );
#elif !defined(old_linux) && !defined(__svr4__)
#include <sched.h>
   u_int     cSz;
   int       rtn, fcn;
   cpu_set_t set;

   cSz = sizeof( set );
   CPU_ZERO( &set );
   CPU_SET( cpu, &set );
   rtn = 0;
   if ( !(fcn=::pthread_setaffinity_np( _tid, cSz, &set )) ) {
      if ( !(fcn=::pthread_getaffinity_np( _tid, cSz, &set )) )
         rtn = CPU_ISSET( cpu, &set ) ? cpu : 0;
   }
   if ( rtn )
      ::yamr_breakpoint();
#endif // WIN32
   return GetThreadProcessor();
}

int Thread::GetThreadProcessor()
{
   int    rtn;
#ifdef WIN32
   DWORD  msk;

   msk  = ::SetThreadAffinityMask( _hThr, 0 );
   ::SetThreadAffinityMask( _hThr, msk );
   rtn = (int)msk;
#elif !defined(old_linux) && !defined(__svr4__)
#include <sched.h>
   u_int     cSz;
   int       i, fcn;
   cpu_set_t set;

   cSz = sizeof( set );
   CPU_ZERO( &set );
   fcn = ::pthread_getaffinity_np( _tid, cSz, &set );
   for ( i=0,rtn=0; !rtn && !fcn && i<K; i++ )
      rtn = CPU_ISSET( i, &set ) ? i : 0;
#endif // WIN32
   return rtn;
}


////////////////////////////////////////////
// Thread Shit ...
////////////////////////////////////////////
void Thread::Start()
{
   // Pre-condition

   if ( _tid )
      return;

   /*
    * Fire one up; On WIN32, 2-Event Handshake to ensure created
    * thread is ready-to-run
    */

#ifdef WIN32
   _hThr = (HANDLE)::_beginthread( Thread::_thrProc, 0, (void *)this );
#else
   int            rtn;
   pthread_attr_t attr;

   ::pthread_attr_init( &attr );
   ::pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM );
   rtn = ::pthread_create( (pthread_t *)&_tid, // Thread ID
                           &attr,              // Attributes
                           Thread::_thrProc,   // Start routine
                           this );             // argument
   ::pthread_attr_destroy( &attr );
   if ( !rtn )
      ::yamr_breakpoint();
#endif // WIN32
   for ( ; !_ready; ) ;
}

void Thread::Stop()
{
   // Pre-condition

   if ( !_tid )
      return;

   // Safe to stop ...

   _bRun = false;
   if ( !_fcn )
      _pump.Stop();
#ifdef WIN32
   ::WaitForSingleObject( _hThr, INFINITE );
#else
   ::pthread_join( _tid, 0 );
#endif // WIN32
   _hThr = 0;
   _tid  = 0;
}

void *Thread::Run()
{
   /*
    * 1) Start pump
    * 2) Free calling thread : 2-Event Handshake
    */

   _tid = CurrentThreadID();
   if ( !_fcn )
      _pump.Start();
   _ready = true;

   // Check every 0.1 sec

   if ( _fcn )
      for ( ; _bRun; (*_fcn)( _arg ) );
   else
      _pump.Run( 0.1 );
   return (void *)0;
}


////////////////////////////////////////////
// Class wide
////////////////////////////////////////////
THR_ARG Thread::_thrProc( void *arg )
{
   Thread *us;

   us = (Thread *)arg;
   us->Run();
#ifndef WIN32
   return (void *)0;
#endif // WIN32
}

pthread_t Thread::CurrentThreadID()
{
   pthread_t rtn;

#ifdef WIN32
   rtn = ::GetCurrentThreadId();
#else
   rtn = ::pthread_self();
#endif // WIN32
   return rtn;
}
