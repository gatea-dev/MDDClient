/******************************************************************************
*
*  Pump.cpp
*     Platform-indepdnent event pump
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     17 AUG 2009 jcs  Build  2: IdleFcns
*      5 OCT 2009 jcs  Build  5: TimerTable
*     26 SEP 2010 jcs  Build  8: Class-wide _sockMsg
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*     11 SEP 2023 jcs  Build 32: _className
*      5 OCT 2023 jcs  Build 65: poll() only
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
LRESULT CALLBACK u_wndProc( HWND, UINT, WPARAM, LPARAM );
void    CALLBACK u_tmrProc( HWND, UINT, UINT, DWORD );
static UINT _sockMsg = 0;
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
   _pollMap( (PollMap *)0 ),
   _maxFd( 0 ),
#ifdef WIN32
   _hWnd( 0 ),
   _tmrID( 0 ),
#endif // WIN32
   _bRun( true ),
   _t0( dNow() )
{
   _pollMap = new PollMap( *this );
}

Pump::~Pump()
{
   Locker            lck( _mtx );
   Sockets::iterator it;

   for ( it=_sox.begin(); it!=_sox.end(); delete (*it).second, it++ );
   _sox.clear();
   delete _pollMap;
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
   TimerTable::iterator it;
   EdgIdleCbk          cbk;
   size_t               i;
   double               d1;

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

   for ( it=_tmrs.begin(); it!=_tmrs.end(); it++ )
      (*it)->On1SecTimer();
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
#ifdef WIN32
   MSG msg;
   int tTmr;

   tTmr   = (int)( dPoll * 1000.0 );
   _tmrID = ::SetTimer( _hWnd, NT_TIMER_ID, tTmr, (TIMERPROC)&u_tmrProc );
   while( _bRun && ::GetMessage( &msg, _hWnd, 0, 0 ) ) {
      ::TranslateMessage( &msg );
      ::DispatchMessage( &msg );
      buildFDs();
#else
   int            maxFd, err, i, fd, rc, tmMs;
   Socket        *sock;
   struct timeval tv, *tm, tt;
   double         dt, age, tExp;
   PollMap       &pm = *_pollMap;
   struct pollfd *pl = pm.pollList();

   // Polling Interval

   for ( dt=dNow(); _bRun; ) {
      maxFd = buildFDs();
      age   = dNow() - dt;
      tExp  = gmax( 0.0, dPoll - age );
      tv    = Logger::dbl2time( tExp );
      tt    = tv;
      tm    = ( tv.tv_sec || tv.tv_usec ) ? &tv : NULL;
      tmMs  = tv.tv_sec*1000 + tv.tv_usec/1000;
      rc    = ::poll( pl, pm.nPoll(), tmMs );
      switch( rc ) {
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
            dt = dNow();
            _OnTimer();
            break;
         default:
         {
            Locker lck( _mtx );

            for ( i=0; i<rc; i++ ) {
               fd = pl[i].fd;
               if ( !(sock=_sox[fd]) )
                  continue; // for-i
               if ( pl[i].revents & pm._rdEvts )
                  sock->OnRead();
               if ( pl[i].revents & pm._wrEvts )
                  sock->OnWrite();
               if ( pl[i].revents & pm._exEvts )
                  sock->OnException();
            }
            age = dNow() - dt;
            if ( age > dPoll ) {
               dt = dNow();
               _OnTimer();
            }
            break;
         }
      }
#endif // WIN32
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

   // 1) Register (hidden) window class

   ::memset( (void *)&wndClass, 0, sizeof( wndClass ) );
   wndClass.lpszClassName = "EventPump Window";
   wndClass.hInstance     = GetWindowInstance( NULL );
   wndClass.lpfnWndProc   = &u_wndProc;
   wndClass.cbWndExtra    = sizeof(Pump *);
   if ( !_sockMsg ) {
      if ( !::RegisterClass( &wndClass ) ) {
         ::MessageBox( NULL, "Error", "RegisterClass()", MB_OK );
         return;
      }
      _sockMsg = ::RegisterWindowMessage( "EventPump" );
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
   Socket           *sock;
   Sockets           tmp;
   Sockets::iterator it;

   // Disconnect all
   {
      Locker lck( _mtx );

      tmp = _sox;
   }
   for ( it=tmp.begin(); it!=tmp.end(); it++ ) {
      if ( (sock=(*it).second) )
         sock->Disconnect( "Pump.Destroy()" );
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
   int               rtn;

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
   _pollMap->buildFDs( _sox );
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
LRESULT CALLBACK u_wndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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

void CALLBACK u_tmrProc( HWND hWnd, UINT msg, UINT idEvt, DWORD dwTime )
{
   Pump *p;

   if ( hWnd && (p=(Pump *)::_GetWindowLong( hWnd, 0 )) )
      p->tmrProc( dwTime );
}

#endif // WIN32


////////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      G L p o l l M a p
//
////////////////////////////////////////////////////////////////////////////////

short PollMap::_rdEvts = POLLIN  | POLLPRI | POLLRDNORM | POLLRDBAND;
short PollMap::_wrEvts = POLLOUT | POLLWRBAND;
short PollMap::_exEvts = POLLERR | POLLHUP |POLLNVAL;

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
PollMap::PollMap( Pump &pmp ) :
   _pmp( pmp ),
   _maxx( _MAX_POLLFD ),
   _nPoll( 0 )
{
   u_char *bp;
   int     sz;

   // Object ID

   sz        = _maxx * sizeof( struct pollfd );
   bp        = new u_char[sz];
   _pollList = (struct pollfd *)bp;
}

PollMap::~PollMap()
{
   u_char *bp;
   
   bp = (u_char *)_pollList;
   delete[] bp;
}  


////////////////////////////////////////////
// Access
////////////////////////////////////////////
struct pollfd *PollMap::pollList()
{
   return _pollList;
}

int PollMap::nPoll()
{
   return _nPoll;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
static struct pollfd _zp = { 0, 0, 0 };

void PollMap::buildFDs( Sockets &sdb )
{
   Sockets::iterator it;
   Socket           *sox;
   short             evts;
   int               fd, maxfd;

   maxfd  = 0;
   _nPoll = 0;
   for ( it=sdb.begin(); it!=sdb.end(); it++ ) {
      if ( !(sox=(*it).second) )
         continue; // for-it
      fd    = sox->fd();
      evts  = _rdEvts;
      evts |= sox->IsWritable() ? _wrEvts : 0;
      evts |= _exEvts;
      _pollList[_nPoll].fd     = fd;
      _pollList[_nPoll].events = evts;
      _nPoll++;
      maxfd = gmax( maxfd, fd );
   }
   _pollList[_nPoll] = _zp;
}

void PollMap::Reset()
{
   int i;

   for ( i=0; i<_nPoll; i++ )
      _pollList[i].revents = 0;
}
