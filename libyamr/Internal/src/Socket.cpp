/******************************************************************************
*
*  Socket.cpp
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <Internal.h>

using namespace YAMR_PRIVATE;

#define _Q_BAND 20.0  // Default Hi/Lo band : ( 20, 80 )

static socklen_t _dSz = sizeof( struct sockaddr_in );

/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      B u f f e r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Buffer::Buffer( int nAlloc, int maxSz ) :
   _bp( (char *)0 ),
   _cp( (char *)0 ),
   _nAlloc( 0 ),
   _maxSiz( maxSz ),
   _bHiMark( false ),
   _dLoBand( _Q_BAND ),
   _dHiBand( 100 - _Q_BAND )
{
   Grow( nAlloc );
   Reset();
}

Buffer::~Buffer()
{
   delete[] _bp;
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
yamrBuf Buffer::buf()
{
   yamrBuf b;

   b._data = _bp;
   b._dLen = bufSz();
   return b;
}

int Buffer::bufSz()
{
   return( _cp - _bp );
}

int Buffer::nLeft()
{
   int rtn;

   rtn = _nAlloc - bufSz() - 1;
   return rtn;
}

double Buffer::pctFull()
{
   double df;

   df = ( 100.0 * bufSz() ) / _nAlloc;
   return df;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
bool Buffer::Grow( int nReqGrow )
{
   char *lwc;
   int   bSz, nGrow;

   // Pre-condition

   nGrow = gmin( nReqGrow, ( _maxSiz-_nAlloc ) );
   if ( !nGrow )
      return false;

   // Save

   lwc = _bp;
   bSz = _cp - _bp;

   // Grow

   _nAlloc += nGrow;
   _bp      = new char[_nAlloc];
   _cp      = _bp;

   // Restore

   if ( lwc ) {
      if ( bSz ) {
         ::memcpy( _bp, lwc, bSz );
         _cp += bSz;
      }
      delete[] lwc;
   }
   return true;
}

void Buffer::Reset()
{
   _cp      = _bp;
   _bHiMark = false;
}

void Buffer::Move( int off, int len )
{
   char *cp;

   cp  = _bp;
   cp += off;
   ::memmove( _bp, cp, len );
   _cp  = _bp;
   _cp += len;
   _bHiMark &= ( pctFull() >= _dLoBand );
}

bool Buffer::Append( char *cp, int len )
{
   // Grow by up to _maxSiz, if needed

   if ( ( nLeft() < len ) && !Grow( _nAlloc ) )
      return false;

   // OK to copy

   ::memcpy( _cp, cp, len );
   _cp += len;
   _bHiMark |= ( pctFull() >= _dHiBand );
   return true;
}



/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      S o c k e t 
//
/////////////////////////////////////////////////////////////////////////////

Logger *Socket::_log = (Logger *)0;

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Socket::Socket( const char *pHosts, bool bUDP ) :
   _thr( new Thread() ),
   _bUDP( bUDP ),
   _mtx(),
   _flags( -1 ),
   _nNonBlk( 0 ),
   _hosts(),
   _ports(),
   _dstConn( "Undefined:0" ),
   _fd( 0 ),
   _bStart( false ),
   _in( K*K ),
   _out( bUDP ? YAMR_MAX_MTU : K*K, 
         bUDP ? YAMR_MAX_MTU : _DFLT_BUF_SIZ ),
   _st( (yamrChanStats *)0 ),
   _bRandomize( false ),
   _bIdleCbk( false ),
   _SO_RCVBUF( 0 )
{
   char       *cp, *hp, *pp, *rp;
   bool        tcp;
   const char *_sep0 = ",";
   const char *_sep1 = ":";
   Hosts       tmp;
   size_t      i, sz;

   // host:port,host:port, ...

   ::memset( &_dst, 0, _dSz );
   tcp = ( ::strstr( pHosts, "tcp:" ) == pHosts );
   cp  = (char *)pHosts;
   cp += ( tcp || _bUDP ) ? 4 : 0;
   for ( cp=::strtok_r( cp,_sep0,&rp ); cp; cp=::strtok_r( NULL,_sep0,&rp ) )
      tmp.push_back( new string( cp ) );
   for ( i=0; i<tmp.size(); i++ ) {
      cp = (char *)tmp[i]->data();
      hp = ::strtok_r( cp, _sep1, &rp );
      pp = ::strtok_r( NULL, _sep1, &rp );
      if ( hp && pp && atoi( pp ) ) {
         _hosts.push_back( new string( hp ) );
         _ports.push_back( atoi( pp ) );
      }
   }
   for ( i=0,sz=tmp.size(); i<sz; delete tmp[i++] );

   // Stats

   SetStats( &_dfltStats );
   ::memset( _st, 0, sizeof( yamrChanStats ) );

   // Timer / Idle

   thr().Start();
   pump().AddTimer( *this );
   if ( _bUDP )
      pump().AddIdle( Socket::_OnIdle, this );
}

Socket::~Socket()
{
   int i, sz;

   // Timer

   thr().Stop();
   pump().RemoveTimer( *this );

   // Guts

   ::memset( _st, 0, sizeof( yamrChanStats ) );
   for ( i=0,sz=_hosts.size(); i<sz; delete _hosts[i++] );
   Disconnect( "Socket.~Socket" );
   delete _thr;
   if ( _log )
      _log->logT( 3, "~Socket( %s )\n", dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
Thread &Socket::thr()
{
   return *_thr;
}

Pump &Socket::pump()
{
   return thr().pump();
}

Mutex &Socket::mtx()
{
   return _mtx;
}

const char *Socket::dstConn()
{
   return _dstConn.data();
}

int Socket::fd()
{
   return (int)_fd;
}

Buffer &Socket::out()
{
   return _out;
}

bool Socket::IsWritable()
{
   return( _out.bufSz() > 0 );
}

int Socket::SetSockBuf( int bufSiz, bool bRX )
{
   int       rtn, val, gerrno, opt;
   socklen_t siz;

   opt = bRX ? SO_RCVBUF : SO_SNDBUF;
   if ( fd() && bufSiz ) {
      val = bufSiz;
      siz = sizeof( val );
      rtn = ::setsockopt( fd(), SOL_SOCKET, opt, (char *)&val, siz );
      if ( rtn || (gerrno=_GetError()) )
         ::yamr_breakpoint();
   }
   return GetSockBuf( bRX );
}

int Socket::GetSockBuf( bool bRX )
{
   int       rtn, val, gerrno, opt;
   socklen_t siz;

   // Pre-condition

   if ( !fd() )
      return 0;

   opt = bRX ? SO_RCVBUF : SO_SNDBUF;
   val = 0;
   siz = sizeof( val );
   rtn = ::getsockopt( fd(), SOL_SOCKET, opt, (char *)&val, &siz );
   if ( rtn || (gerrno=_GetError()) )
      ::yamr_breakpoint();
   return ( rtn == 0 ) ? val : 0;
}


////////////////////////////////////////////
// Run-time Stats
////////////////////////////////////////////
yamrChanStats &Socket::stats()
{
   return *_st;
}

void Socket::SetStats( yamrChanStats *st )
{
   if ( _st )
      ::memmove( st, _st, sizeof( yamrChanStats ) );
   _st = st;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
const char *Socket::Connect()
{
   yamrChanStats     &st = stats();
   struct sockaddr_in a;
   struct hostent    *h;
   struct in_addr    *ip;
   const char        *pHost, *pRtn;
   int                rc, dom, ty;
   size_t             i, n, idx;
   char               buf[K];

   // Create socket

   _bStart = true;
   ATOMIC_EXCH( &_nNonBlk, 0 );
   Disconnect( "Socket.Connect()" );
   pRtn = (const char *)0;
   dom  = AF_INET;
   ty   = _bUDP ? SOCK_DGRAM : SOCK_STREAM;
   if ( (_fd=::socket( dom, ty, 0 )) <= 0 ) {
      Disconnect( "socket() system call failed" );
      return pRtn;
   }
   pump().Add( this );

   // Connect, et al

   n = _hosts.size();
   for ( i=0; !pRtn && i<n; i++ ) {
      ::memset( &_dst, 0, sizeof( _dst ) );
      idx             = _bRandomize ? ( ::rand() % n ) : i;
      _dst.sin_family = AF_INET;
      _dst.sin_port   = htons( _ports[idx] );
      pHost           = _hosts[idx]->data();
      if ( (h=::gethostbyname( pHost )) ) {
         ip                  = (struct in_addr *)*(h->h_addr_list);
         _dst.sin_addr.s_addr = ip->s_addr;
      }
      else
         _dst.sin_addr.s_addr = inet_addr( pHost );
      if ( _bUDP ) {
         ::memset( &a, 0, sizeof( a ) );
         a.sin_family      = AF_INET;
         a.sin_port        = 0; // Ephemeral
         a.sin_addr.s_addr = INADDR_ANY;
         rc = ::bind( fd(), (struct sockaddr *)&a, sizeof( a ) );
      }
      else
         rc = ::connect( fd(), (struct sockaddr *)&_dst, _dSz );
      if ( !rc ) {
         sprintf( buf, "%s:%d", pHost, _ports[i] );
         _dstConn = buf;
         pRtn = dstConn();
         NagleOff();
      }
   }

   // Stats / Notify

   if ( pRtn ) { 
      safe_strcpy( st._dstConn, dstConn() );
      st._lastConn = (int)::yamr_TimeSec();
      st._nConn   += 1;
      st._bUp      = true;
      OnConnect( pRtn );
   }
   else
      OnException();
   return pRtn;
}

bool Socket::Disconnect( const char *reason )
{
   Locker         lck( _mtx );
   yamrChanStats &st = stats();

   // Stats / Close

   st._lastDisco = fd() ? (int)::yamr_TimeSec() : st._lastDisco;
   st._bUp       = false;
   st._qSiz      = 0;
   st._qSizMax   = 0;
   safe_strcpy( st._dstConn, "Not connected" );
   pump().Remove( this );
   if ( fd() > 0 )
      CLOSE( _fd );
   _fd = 0;
   _in.Reset();
   _out.Reset();
   return true;
}

bool Socket::Write( const char *pData, int dLen, bool bFlush )
{
   Locker         lck( _mtx );
   yamrChanStats &st = stats();
   bool           bOK, bHiMark;
   int            bSz;

   // 1) UDP packing

   if ( _bUDP ) {
      bSz = _out.bufSz();
      if ( ( bSz+dLen ) > YAMR_MAX_MTU )
         OnWrite();
   }

   // 2) Do it

   bHiMark = _out._bHiMark;
   if ( (bOK=_out.Append( (char *)pData, dLen )) && bFlush && !_bUDP )
      OnWrite();
   st._qSiz    =  _out.bufSz();
   st._qSizMax = gmax( st._qSiz, st._qSizMax );
   if ( !_bUDP )
      _CheckQueueMark( bHiMark );
   return bOK;
}


////////////////////////////////////////////
// Socket Interface 
////////////////////////////////////////////
void Socket::Ioctl( yamrIoctl ctl, void *arg )
{
   Locker          lck( _mtx );
   yamrChanStats **rtn;
   bool            bArg;
   int            *i32;
   u_int64_t      *i64;

   bArg  = ( arg != (void *)0 );
   i32   = arg ? (int *)arg       : (int *)0;
   i64   = arg ? (u_int64_t *)arg : (u_int64_t *)0;
   switch( ctl ) {
      case ioctl_getStats:
         rtn  = (yamrChanStats **)arg;
         *rtn = _st;
         break;
      case ioctl_randomize:
         _bRandomize = bArg;
         break;
      case ioctl_getFd:
         *i32 = fd();
         break;
      case ioctl_setRxBufSize:
         _SO_RCVBUF = *i32;
         SetSockBuf( _SO_RCVBUF, true );
         break;
      case ioctl_getRxBufSize:
         *i32 = GetSockBuf( true );
         break;
      case ioctl_setTxBufSize:
         if ( _bUDP )
            SetSockBuf( *i32, false );
         else
            _out._maxSiz = *i32;
         break;
      case ioctl_getTxBufSize:
         *i32 = _bUDP ? GetSockBuf( false ) : _out._maxSiz;
         break;
      case ioctl_getTxQueueSize:
         *i32 = _out.bufSz();
         break;
      case ioctl_setThreadProcessor:
         thr().SetThreadProcessor( *i32 );
         break;
      case ioctl_getThreadProcessor:
         *i32 = thr().GetThreadProcessor();
         break;
      case ioctl_getThreadId:
         *i64 = thr().tid();
         break;
      case ioctl_setIdleTimer:
         _bIdleCbk = bArg;
         break;
      case ioctl_QlimitHiLoBand:
         _out._dLoBand = WithinRange( 5, *i32, 45 );
         _out._dHiBand = 100.0 - _out._dLoBand;
         break;
      case ioctl_getSocketType:
         *i32 = _bUDP ? 1 : 0;
         break;
   }
}


////////////////////////////////////////////
// Thread Notifications
////////////////////////////////////////////
void Socket::OnRead()
{
   yamrChanStats &st = stats();
   int              tot, nb, nL;

   // Drain ; Connectionless is 1 read ...

   setNonBlocking();
   if ( !(nL=_in.nLeft()) )
      _in.Grow( _in._nAlloc );
   nL = _in.nLeft();
   for ( tot=0; (nb=READ( fd(), _in._cp, nL )) > 0; ) {
      nb         = gmax( nb,0 );
      st._nByte += nb;
      if ( nb && _log && _log->CanLog( 3 ) ) {
         Locker lck( _log->mtx() );

         _log->logT( 3, "[RAW-RD %06d bytes]\n", nb );
         _log->logT( 4, "   " );
         _log->HexLog( 4, _in._cp, nb );
      }
      _in._cp += nb;
      nL      -= nb;
      tot     += nb;
   }
   setBlocking();
#if !defined(WIN32)
   if ( !tot )
      OnException();
#endif // !defined(WIN32)
}

void Socket::OnWrite()
{
   Locker           lck( _mtx );
   yamrChanStats &st = stats();
   const char      *cp;
   int              wSz, nWr, nL, tb, nb;
   bool             bHiMark;

   // Pre-condition

   if ( !fd() )
      return;

   // 1) Drain ...

   bHiMark = _out._bHiMark;
   setNonBlocking();
   cp  = _out._bp;
   wSz = _out.bufSz();
   nWr = wSz;
   for ( tb=0; nWr && ( (nb=_Write( cp, nWr )) > 0 );  ) {
      cp += nb;
      tb += nb;
      nWr = gmax( nWr-nb, 0 );
   }
   setBlocking();

   // 2) Re-set

   if ( _bUDP )
      _out.Reset();
   else if ( tb ) {
      nL = wSz - tb;
      if ( nL )
         _out.Move( tb, nL );
      else
         _out.Reset();
      st._qSiz    =  _out.bufSz();
      st._qSizMax = gmax( st._qSiz, st._qSizMax );
      _CheckQueueMark( bHiMark );
   }
}

void Socket::OnException()
{
   const char *err = "EBADF"; // "OnException()

   OnDisconnect( err );
   Disconnect( err );
   if ( _log )
      _log->logT( 2, "Socket::OnRead() : Connection to %s lost\n", dstConn() );
}


////////////////////////////////////////////
// TimerEvent Notifications
////////////////////////////////////////////
void Socket::On1SecTimer()
{
   if ( _bStart && !fd() )
      Connect();
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
int Socket::_Write( const char *bp, int nWr )
{
   struct sockaddr *pd;
   int              rc;

   if ( _bUDP ) {
      pd = (struct sockaddr *)&_dst;
      rc = ::sendto( fd(), bp, nWr, 0, pd, sizeof( _dst ) );
   }
   else
      rc = WRITE( fd(), bp, nWr );
   if ( rc != nWr )
      ::yamr_breakpoint();
   return rc;
}

int Socket::setNonBlocking()
{
   long nb;

   if ( (nb=ATOMIC_INC( &_nNonBlk )) == 1 ) {
#ifdef WIN32
/*
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ms738573(v=vs.85).aspx
 *
 * "The WSAAsyncSelect and WSAEventSelect functions automatically set a
 * socket to nonblocking mode."
 *
      u_long iMode = 1;

      ::ioctlsocket( fd(), FIONBIO, &iMode );
 */
#else
      if ( _flags == -1 )
         _flags = ::fcntl( fd(), F_GETFL,0 );
      ::fcntl( fd(), F_SETFL, _flags | FNDELAY );
#endif // WIN32
   }
   return _flags;
}

void Socket::setBlocking()
{
   long nb;

   if ( (nb=ATOMIC_DEC( &_nNonBlk )) == 0 ) {
#ifdef WIN32
/*
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ms738573(v=vs.85).aspx
 *
 * "The WSAAsyncSelect and WSAEventSelect functions automatically set a
 * socket to nonblocking mode."
 *
      u_long iMode = 0;

      ::ioctlsocket( fd(), FIONBIO, &iMode );
 */
#else
      ::fcntl( fd(), F_SETFL, _flags );
#endif // WIN32
   }
}

int Socket::_GetError()
{
   int rtn;

   // Platform-specific
#ifdef WIN32
   rtn  = ::WSAGetLastError();
   rtn -= WSABASEERR;
#else
   rtn = errno;
#endif // WIN32
   return rtn;
}

bool Socket::NagleOff()
{
   int       bSet, bs, rtn, r1;
   socklen_t siz;

   // Pre-condition

   if ( !fd() )
      return false;
   bSet = 1;
   siz  = sizeof( bSet );
   rtn  = ::setsockopt( fd(), IPPROTO_TCP, TCP_NODELAY, (char *)&bSet, siz );
   r1   = ::getsockopt( fd(), IPPROTO_TCP, TCP_NODELAY, (char *)&bs, &siz );
   return( !rtn &&!r1 );
}

void Socket::_CheckQueueMark( bool bMark )
{
   if ( bMark != _out._bHiMark ) {
      if ( _out._bHiMark )
         OnQHiMark();
      else
         OnQLoMark();
   }
}


/////////////////////////////////////////
// Idle Loop Processing
////////////////////////////////////////////
void Socket::OnIdle()
{
   OnWrite();
}

void Socket::_OnIdle( void *arg )
{
   Socket *us;

   us = (Socket *)arg;
   us->OnIdle();
}

