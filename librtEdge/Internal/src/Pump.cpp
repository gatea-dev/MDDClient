/******************************************************************************
*
*  Pump.cpp
*     Platform-indepdnent event pump

*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     17 AUG 2009 jcs  Build  2: IdleFcns
*      5 OCT 2009 jcs  Build  4: TimerTable
*     26 SEP 2010 jcs  Build  8: Class-wide _sockMsg
*     19 SEP 2012 jcs  Build 20: Check for !sox in buildFDs()
*     12 NOV 2014 jcs  Build 28: -Wall
*      5 FEB 2015 jcs  Build 30: Dispatch : Non-zero Socket from _sox
*      2 JUL 2015 jcs  Build 31: Socket._log
*     20 MAR 2016 jcs  Build 32: EDG_Internal.h; _SetWindowLong
*     29 AUG 2016 jcs  Build 33: _Destroy() : if ( s )
*     11 SEP 2023 jcs  Build 64: _sockMsgName / _className
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

#ifdef _WIN64
#define _SetWindowLong SetWindowLongPtr
#define _GetWindowLong GetWindowLongPtr
#define _LONG          LONG_PTR
#else
#define _SetWindowLong SetWindowLong
#define _GetWindowLong GetWindowLong
#define _LONG          LONG
#endif  // _WIN64


using namespace RTEDGE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s       P u m p
//
/////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
#define NT_TIMER_ID       7
#define NT_READ_EVENTS    ( FD_READ  | FD_ACCEPT | FD_OOB )
#define NT_WRITE_EVENTS   ( FD_WRITE | FD_CONNECT )
#define NT_EXCEPT_EVENTS  ( FD_CLOSE )
LRESULT CALLBACK _wndProc( HWND, UINT, WPARAM, LPARAM );
void    CALLBACK _tmrProc( HWND, UINT, UINT, DWORD );
static UINT   _sockMsg = 0;
static string _sockMsgName( "EventPump" );
static string _dfltClassName( "RTEDGE_PRIVATE::EventPump" );
#endif // WIN32

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Pump::Pump() :
   _sox(),
   _tmrs(),
   _dels(),
   _idle(),
   _mtx(),
   _maxFd( 0 ),
#ifdef WIN32
   _hWnd( 0 ),
   _tmrID( 0 ),
   _className( _dfltClassName ),
#endif // WIN32
   _bRun( true ),
   _t0( dNow() )
{
   FD_ZERO( &_rds );
   FD_ZERO( &_wrs );
   FD_ZERO( &_exs );
}

Pump::~Pump()
{
   Locker            lck( _mtx );
   Sockets::iterator it;

   for ( it=_sox.begin(); it!=_sox.end(); it++ )
      delete (*it).second;
   _sox.clear();
}

 
////////////////////////////////////////////
// Socket Operations
////////////////////////////////////////////
void Pump::Add( Socket *s )
{
   Locker lck( _mtx );
   int    fd;

   if ( (fd=s->fd()) ) {
      Remove( s );
      _sox[fd] = s;
   }
}

void Pump::Remove( Socket *s )
{
   Locker            lck( _mtx );
   Sockets::iterator it;
   int               fd;

   if ( (it=_sox.find( (fd=s->fd()) )) != _sox.end() )
      _sox.erase( it );
   _dels.push_back( fd );
}


////////////////////////////////////////////
// Timer Operations
////////////////////////////////////////////
void Pump::AddTimer( TimerEvent &t )
{
   Locker lck( _mtx );

   _tmrs.push_back( &t );
}

void Pump::RemoveTimer( TimerEvent &t )
{
   Locker               lck( _mtx );
   TimerEvent          *tt;
   TimerTable::iterator it;

   for ( it=_tmrs.begin(); it!=_tmrs.end(); it++ ) {
      tt = (*it);
      if ( tt == &t ) {
         _tmrs.erase( it );
         return;
      }
   }
}

void Pump::_OnTimer()
{
   EdgIdleCbk cbk;
   size_t     i, n;
   double     d1;

   // Registered Idle functions

   for ( i=0; i<_idle.size(); i++ ) {
      cbk = _idle[i];
      (*cbk._fcn)( cbk._arg );
   }

   // 1-sec timers

   d1 = dNow();
   if ( ( d1-_t0 ) < 1.0 )
      return;
   _t0 = d1;

   // TimerEvent.On1SecTimer()

   TimerTable tmp( _tmrs );

   n = tmp.size();
   for ( i=0; i<n; tmp[i++]->On1SecTimer() );
}



////////////////////////////////////////////
// Thread Operations
////////////////////////////////////////////
void Pump::Start()
{
   _Create();
}

void Pump::Run( double dPoll )
{
   Logger *lf;

#ifdef WIN32
   MSG msg;
   int tTmr;

   tTmr   = (int)( dPoll * 1000.0 );
   _tmrID = ::SetTimer( _hWnd, NT_TIMER_ID, tTmr, (TIMERPROC)&_tmrProc );
   while( _bRun && ::GetMessage( &msg, _hWnd, 0, 0 ) ) {
      ::TranslateMessage( &msg );
      ::DispatchMessage( &msg );
      buildFDs();
#else
   int            i, res, maxFd, err;
   int            iSec, uSec;
   fd_set         rds, wrs, exs;
   struct timeval tv, tPoll;
Socket *sock;

   // Polling Interval

   iSec          = (int)dPoll;
   uSec          = (int)( ( dPoll - iSec ) * 1000000.0 );
   tPoll.tv_sec  = iSec;
   tPoll.tv_usec = uSec;
   for ( ; _bRun; ) {
      maxFd = buildFDs();
      tv    = tPoll;
      rds   = _rds;
      wrs   = _wrs;
      exs   = _exs;
      res   = ::select( maxFd, &rds, &wrs, &exs, &tv );
      switch( res ) {
         case -1:      // Error
         {
            err = errno;
            switch( err ) {
               case EBADF:      // Find bad descriptor and drop ...
                  break;
            }
            break;
         }
         case 0:      // Timeout
            _OnTimer();
            break;
         default:
         {
            Locker  lck( _mtx );

            for ( i=2; i<maxFd; i++ ) {  // Skip stdin, stdout, stderr
               if ( FD_ISSET( i, &rds ) && (sock=_sox[i]) )
                  sock->OnRead();
               if ( FD_ISSET( i, &wrs ) && (sock=_sox[i]) )
                  sock->OnWrite();
               if ( FD_ISSET( i, &exs ) && (sock=_sox[i]) )
                  sock->OnException();
            }
            break;
         }
      }
#endif // WIN32
      if ( (lf=Socket::_log) )
         lf->log( 5, (char *)"." );
   }
   _Destroy();
}

void Pump::Stop()
{
   _bRun = false;
}

void Pump::AddIdle( EdgIdleFcn fcn, void *arg )
{
   EdgIdleCbk c;
   Locker      lck( _mtx );

   c._fcn = fcn;
   c._arg = arg;
   _idle.push_back( c );
}

void Pump::RemoveIdle( EdgIdleFcn fcn )
{
   Locker             lck( _mtx ); 
   EdgIdleCbk         c;
   IdleFcns::iterator it;

   for ( it=_idle.begin(); it!=_idle.end(); it++ ) {
      c = (*it);
      if ( c._fcn == fcn ) {
         _idle.erase( it );
         return;
      }
   }
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
void Pump::_Create()
{
   // WIN32-specific
#ifdef WIN32
   WNDCLASS wndClass;
   char     obuf[K];

   // 1) Register (hidden) window class

   ::memset( (void *)&wndClass, 0, sizeof( wndClass ) );
   _className            += ::rtEdge_pTimeMs( obuf, ::rtEdge_TimeNs() );
   wndClass.lpszClassName = _className.data();
   wndClass.hInstance     = GetWindowInstance( NULL );
   wndClass.lpfnWndProc   = &_wndProc;
   wndClass.cbWndExtra    = sizeof(Pump *);
   if ( !::RegisterClass( &wndClass ) ) {
      ::MessageBox( NULL, "Error", "RegisterClass()", MB_OK );
      return;
   }
   if ( !_sockMsg ) {
      _sockMsgName += obuf;
      _sockMsg      = ::RegisterWindowMessage( _sockMsgName.data() );
   }

   // 3) Create (hidden) window; Start timer

   _hWnd = ::CreateWindow( wndClass.lpszClassName,
                           wndClass.lpszClassName,
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           NULL,
                           ::CreateMenu(),
                           wndClass.hInstance,
                           NULL );
   ::_SetWindowLong( _hWnd, 0, (_LONG)this );
#endif // WIN32
}

void Pump::_Destroy()
{
   Sockets           tmp;
   Socket           *s;
   Sockets::iterator it;

   // Disconnect all
   {
      Locker lck( _mtx );

      tmp = _sox;
   }
   for ( it=tmp.begin(); it!=tmp.end(); it++ ) {
      s = (*it).second;
      if ( s )
         s->Disconnect( "Pump.Destroy()" );
   }

   // WIN32-specific
#ifdef WIN32
   ::KillTimer( _hWnd, _tmrID );
   ::DestroyWindow( _hWnd );
#endif // WIN32
}

int Pump::buildFDs()
{
   Locker            lck( _mtx );
   Sockets::iterator it;
   Socket           *sox;
   int               fd, rtn;

   /*
    * WIN32 : _dels; Then walk all _sox
    * Linux : Re-build always
    */

   rtn = 0;
#ifdef WIN32
   int  i;
   long evts;

   for ( i=0; i<_dels.size(); i++ )
      ::WSAAsyncSelect( (SOCKET)_dels[i], _hWnd, _sockMsg, 0 );
   for ( it=_sox.begin(); it!=_sox.end(); it++ ) {
      if ( !(sox=(*it).second) )
         continue; // for-it
      fd    = sox->fd();
      evts  = NT_READ_EVENTS | NT_EXCEPT_EVENTS;
      evts |= sox->IsWritable() ? NT_WRITE_EVENTS : 0;
      ::WSAAsyncSelect( (SOCKET)fd, _hWnd, _sockMsg, evts );
   }
#else
   FD_ZERO( &_rds );
   FD_ZERO( &_wrs );
   FD_ZERO( &_exs );
   for ( it=_sox.begin(); it!=_sox.end(); it++ ) {
      if ( !(sox=(*it).second) )
         continue; // for-it
      fd  = sox->fd();
      FD_SET( fd, &_rds );
      if ( sox->IsWritable() )
         FD_SET( fd, &_wrs );
      FD_SET( fd, &_exs );
      rtn = gmax( rtn, fd+1 );
   }
#endif // WIN32

   // Clear 'em out ...

   _dels.clear();
   return rtn;
}

#ifdef WIN32
LRESULT Pump::wndProc( WPARAM wParam, LPARAM lParam )
{
   Locker  lck( _mtx );
   Socket *sox;
   int     fd, evt, err;

   // Safe to Dispatch

   fd  = wParam;
   sox = _sox[fd];
   evt = WSAGETSELECTEVENT( lParam );
   err = WSAGETSELECTERROR( lParam );
   if ( sox ) {
      if ( evt & NT_READ_EVENTS )
         sox->OnRead();
      if ( evt & NT_WRITE_EVENTS )
         sox->OnWrite();
      if ( evt & NT_EXCEPT_EVENTS )
         sox->OnException();
   }
   return TRUE;
}

void Pump::tmrProc( DWORD dwTime )
{
   _OnTimer();
}


////////////////////////////////////////////
// C-callbacks from NT library
////////////////////////////////////////////
LRESULT CALLBACK _wndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
   Pump *p;

   // Pre-condition(s)

   if ( !hWnd )
      return (LRESULT)0;
   if ( !(p=(Pump *)::_GetWindowLong( hWnd, 0 )) )
      return ::DefWindowProc( hWnd, msg, wParam, lParam );
   if ( msg != _sockMsg )
      return ::DefWindowProc( hWnd, msg, wParam, lParam );
   return p->wndProc( wParam, lParam );
}

void CALLBACK _tmrProc( HWND hWnd, UINT msg, UINT idEvt, DWORD dwTime )
{
   Pump *p;

   if ( hWnd && (p=(Pump *)::_GetWindowLong( hWnd, 0 )) )
      p->tmrProc( dwTime );
}

#endif // WIN32
