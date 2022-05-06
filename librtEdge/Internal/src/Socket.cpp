/******************************************************************************
*
*  Socket.cpp
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     31 AUG 2009 jcs  Build  3: ::strtok_r()
*      5 OCT 2009 jcs  Build  4: TimerEvent
*     18 SEP 2010 jcs  Build  7: Optional Thread
*     24 JAN 2012 jcs  Build 17: mtx(); Don't logT raw data
*     22 MAR 2012 jcs  Build 18: NagleOff()
*     11 FEB 2013 jcs  Build 23: NULL's : _in / _cp not _rdData
*     10 JUL 2013 jcs  Build 26: Null-out _out in Disconnect()
*     11 JUL 2013 jcs  Build 26a:rtEdgeChanStats
*     31 JUL 2013 jcs  Build 27: _bStart
*     12 NOV 2014 jcs  Build 28: libmddWire; mddBldBuf _out
*      7 JAN 2015 jcs  Build 29: ioctl_getProtocol
*      5 FEB 2015 jcs  Build 30: No mo Threaddd() / Remove()
*     18 JUN 2015 jcs  Build 31: rtEdgeChanStats._qSiz
*     15 APR 2016 jcs  Build 32: ioctl_isSnapChan; ioctl_setUserPubMsgTy
*     12 SEP 2016 jcs  Build 33: ioctl_userDefStreamID
*     26 MAY 2017 jcs  Build 34: WireMold64
*      6 DEC 2018 jcs  Build 41: VS2017
*     12 FEB 2020 jcs  Build 42: _tHbeat
*      7 SEP 2020 jcs  Build 44: XxxThreadName()
*     29 MAR 2022 jcs  Build 52: mddWire ioctl's here
*      5 MAY 2022 jcs  Build 53: _bPub
*
*  (c) 1994-2022 Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;

#define _MAX_ATTR K

static socklen_t _dSz = sizeof( struct sockaddr_in );
static int       _pSz = sizeof( Mold64PktHdr );

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
   _maxSiz( maxSz )
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
rtBUF Buffer::buf()
{
   rtBUF b;

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
   _cp = _bp;
}

void Buffer::Move( int off, int len )
{
   char *cp;

   cp  = _bp;
   cp += off;
   ::memmove( _bp, cp, len );
   _cp  = _bp;
   _cp += len;
}

bool Buffer::Append( char *cp, int len )
{
   // Grow by up to _maxSiz, if needed

   if ( ( nLeft() < len ) && !Grow( _nAlloc ) )
      return false;

   // OK to copy

   ::memcpy( _cp, cp, len );
   _cp += len;
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
Socket::Socket( const char *pHosts, bool bConnectionless ) :
   _thr( new Thread() ),
   _bConnectionless( bConnectionless ),
   _mdd( (mddWire_Context)0 ),
   _proto( bConnectionless ? mddProto_Binary : mddProto_XML ),
   _bPub( false ),
   _mtx(),
   _blkMtx(),
   _nBlk( 0 ),
   _flags( -1 ),
   _nNonBlk( 0 ),
   _hosts(),
   _ports(),
   _udpPort( 0 ),
   _dstConn( "Undefined:0" ),
   _fd( 0 ),
   _bStart( false ),
   _in( K*K ),
   _out( K*K ),
   _st( (rtEdgeChanStats *)0 ),
   _bCache( false ),
   _bBinary( false ),
   _bLatency( false ),
   _bRandomize( false ),
   _bIdleCbk( false ),
   _tHbeat( 3600 ),
   _SO_RCVBUF( 0 )
{
   char       *cp, *hp, *pp, *rp;
   const char *_sep0 = ",";
   const char *_sep1 = ":";
   Hosts       tmp;
   size_t      i, sz;

   // host:port,host:port, ...

   ::memset( &_dst, 0, _dSz );
   cp = (char *)pHosts;
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

   // Initialize FieldList

   _fl = ::mddFieldList_Alloc( _MAX_ATTR );
   ::memset( &_bldBuf, 0, sizeof( _bldBuf ) );

   // Stats

   SetStats( &_dfltStats );
   ::memset( _st, 0, sizeof( rtEdgeChanStats ) );

   // UDP

   ::memset( &_udp, 0, sizeof( _udp ) );
#if (_MSC_VER >= 1910)
   sprintf( (char *)_udp._session, "%010lld", ::rtEdge_TimeSec() );
#else
   sprintf( (char *)_udp._session, "%010ld", ::rtEdge_TimeSec() );
#endif // (_MSC_VER >= 1910)
   _udp._wire_seqNum = 1;

   // Timer

   thr().Start();
   pump().AddTimer( *this );
}

Socket::~Socket()
{
   int i, sz;

   // Timer

   thr().Stop();
   pump().RemoveTimer( *this );

   // Guts

   ::memset( _st, 0, sizeof( rtEdgeChanStats ) );
   for ( i=0,sz=_hosts.size(); i<sz; delete _hosts[i++] );
   Disconnect( "Socket.~Socket" );
   ::mddSub_Destroy( _mdd );
   _mdd = 0;
   ::mddFieldList_Free( _fl );
   ::mddBldBuf_Free( _bldBuf );
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

bool Socket::IsCache()
{
   return _bCache;
}

bool Socket::IsWritable()
{
   return( _out.bufSz() > 0 );
}

int Socket::SetRcvBuf( int bufSiz )
{
   int       rtn, val, gerrno;
   socklen_t siz;

   if ( fd() && bufSiz ) {
      val = bufSiz;
      siz = sizeof( val );
      rtn = ::setsockopt( fd(), SOL_SOCKET, SO_RCVBUF, (char *)&val, siz );
      gerrno = _GetError();
   }
   return GetRcvBuf();
}

int Socket::GetRcvBuf()
{
   int       rtn, val, gerrno;
   socklen_t siz;

   // Pre-condition

   if ( !fd() )
      return 0;

   val = 0;
   siz = sizeof( val );
   rtn = ::getsockopt( fd(), SOL_SOCKET, SO_RCVBUF, (char *)&val, &siz );
   gerrno = _GetError();
   return ( rtn == 0 ) ? val : 0;
}


////////////////////////////////////////////
// Run-time Stats
////////////////////////////////////////////
rtEdgeChanStats &Socket::stats()
{
   return *_st;
}

void Socket::SetStats( rtEdgeChanStats *st )
{
   if ( _st )
      ::memmove( st, _st, sizeof( rtEdgeChanStats ) );
   _st = st;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
const char *Socket::Connect()
{
   rtEdgeChanStats   &st = stats();
   struct sockaddr_in a;
   struct hostent    *h;
   struct in_addr    *ip;
   const char        *pHost, *pRtn;
   int                rtn, type;
   size_t             i, n, idx;
   char               buf[K];

   // Create socket

   _bStart = true;
   ATOMIC_EXCH( &_nNonBlk, 0 );
   Disconnect( "Socket.Connect()" );
   pRtn = (const char *)0;
   type = _bConnectionless ? SOCK_DGRAM : SOCK_STREAM;
   if ( (_fd=::socket( AF_INET, type, 0 )) <= 0 ) {
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
      if ( _bConnectionless ) {
         ::memset( &a, 0, sizeof( a ) );
         a.sin_family      = AF_INET;
         a.sin_port        = htons( _udpPort );  // 0 = Ephemeral
         a.sin_addr.s_addr = INADDR_ANY;
         rtn = ::bind( fd(), (struct sockaddr *)&a, _dSz );
      }
      else
         rtn = ::connect( fd(), (struct sockaddr *)&_dst, _dSz );
      if ( rtn == 0 ) {
         sprintf( buf, "%s:%d", pHost, _ports[i] );
         _dstConn = buf;
         if ( _bConnectionless && _udpPort ) {
            sprintf( buf, "; LocalPort=%d", _udpPort );
            _dstConn += buf;
         }
         pRtn = dstConn();
         NagleOff();
      }
   }

   // Stats / Notify

   if ( pRtn ) { 
      safe_strcpy( st._dstConn, dstConn() );
      st._lastConn = (int)::rtEdge_TimeSec();
      st._nConn   += 1;
      st._bUp      = true;
      _SetHeartbeat();
      OnConnect( pRtn );
   }
   else
      OnException();
   return pRtn;
}

bool Socket::Disconnect( const char *reason )
{
   rtEdgeChanStats &st = stats();

   // Stats / Close

   st._lastDisco = fd() ? (int)::rtEdge_TimeSec() : st._lastDisco;
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

bool Socket::Write( const char *pData, int dLen )
{
   rtEdgeChanStats &st = stats();
   Locker           l( _mtx );
   struct sockaddr *sa;
   bool             bOK;
   char            *pkt;
   int              wSz, pSz;

   // Unbuffered if _bConnectionless

   if ( _bConnectionless ) {
      ::memcpy( _udp._data, pData, dLen );
      _udp._wire_numMsg  = 1;
      pkt                = (char *)&_udp;
      pSz                = _pSz + dLen;
      sa                 = (struct sockaddr *)&_dst;
      wSz                = ::sendto( fd(), pkt, pSz, 0, sa, _dSz );
      _udp._wire_seqNum += _udp._wire_numMsg;
      bOK                = ( wSz == pSz );
      return bOK;
   }

   if ( (bOK=_out.Append( (char *)pData, dLen )) )
      OnWrite();
   st._qSiz    =  _out.bufSz();
   st._qSizMax = gmax( st._qSiz, st._qSizMax );
   return bOK;
}


////////////////////////////////////////////
// Socket Interface 
////////////////////////////////////////////
bool Socket::Ioctl( rtEdgeIoctl ctl, void *arg )
{
   rtEdgeChanStats  &st = stats();
   rtEdgeChanStats **rtn;
   mddProtocol      *pro;
   bool              bArg, *pbArg;
   char             *pArg;
   int              *p32, i32;
   u_int64_t        *i64;
   rtBUF            *b;

   bArg  = ( arg != (void *)0 );
   i32   = arg ? (size_t)arg      : 0;
   p32   = arg ? (int *)arg       : (int *)0;
   i64   = arg ? (u_int64_t *)arg : (u_int64_t *)0;
   pbArg = (bool *)arg;
   pArg  = (char *)arg;
   switch( ctl ) {
      case ioctl_unpacked:
      case ioctl_nativeField:
      case ioctl_fixedLibrary:
         ::mddWire_ioctl( _mdd, (mddIoctl)ctl, arg );
         return true;
      case ioctl_getStats:
         rtn  = (rtEdgeChanStats **)arg;
         *rtn = _st;
         return true;
      case ioctl_binary:
         _bBinary = bArg;
         return true;
      case ioctl_measureLatency:
         _bLatency = bArg;
         return true;
      case ioctl_enableCache:
         _bCache  = bArg;
         _bCache &= ( st._nMsg == 0 );  // BEFORE rtEdge_Start()
         return true;
      case ioctl_getProtocol:
         pro  = (mddProtocol *)arg;
         *pro = _proto;
         return true;
      case ioctl_randomize:
         _bRandomize = bArg;
         return true;
      case ioctl_setHeartbeat:
         _tHbeat = WithinRange( 1, i32, 86400 );
         return true;
      case ioctl_getFd:
         *p32 = fd();
         return true;
      case ioctl_setRxBufSize:
         _SO_RCVBUF = *p32;
         SetRcvBuf( _SO_RCVBUF );
         return true;
      case ioctl_getRxBufSize:
         *p32 = GetRcvBuf();
         return true;
      case ioctl_setTxBufSize:
         _out._maxSiz = *p32;
         return true;
      case ioctl_getTxBufSize:
         *p32 = _out.bufSz();
         return true;
      case ioctl_setThreadProcessor:
         thr().SetThreadProcessor( *p32 );
      case ioctl_getThreadProcessor:
         *p32 = thr().GetThreadProcessor();
         return true;
      case ioctl_setThreadName:
         if ( pArg )
            thr().SetName( pArg );
      case ioctl_getThreadName:
         if ( pArg )
            pArg = (char *)thr().GetName();
         return true;
      case ioctl_getInputBuffer:
         b  = (rtBUF *)arg;
         *b = _in.buf();
         return true;
      case ioctl_isSnapChan:
         *pbArg = false;
         return true;
      case ioctl_setIdleTimer:
         _bIdleCbk = bArg;
         return true;
      case ioctl_getPubAuthToken:
         pArg[0] = '\0';
         return true;
      case ioctl_getThreadId:
         *i64 = thr().tid();
         return true;
      default:
         break;
   }
   return false;
}


////////////////////////////////////////////
// Thread Notifications
////////////////////////////////////////////
void Socket::OnRead()
{
   rtEdgeChanStats &st = stats();
   int              tot, nb, nL;
   double           d0, dd;

   // Drain ; Connectionless is 1 read ...

   setNonBlocking();
   if ( !(nL=_in.nLeft()) )
      _in.Grow( _in._nAlloc );
   nL = _in.nLeft();
   d0 = _bLatency ? ::rtEdge_TimeNs() : 0.0;
   if ( _bConnectionless ) {
      _in.Reset();
      nb         = gmax( ::recv( fd(), _in._cp, nL, 0 ), 0 );
      tot        = nb;
      _in._cp   += nb;
      st._nByte += _bPub ? 0 : nb;
   }
   else {
      for ( tot=0; (nb=READ( fd(), _in._cp, nL )) > 0; ) {
         dd = _bLatency ? 1000000.0 * ( ::rtEdge_TimeNs() - d0 ) : 0.0;
         if ( _bLatency ) {
            printf( "[%d] READ( %d ) in %.1fuS\n", fd(), nb, dd ); 
            fflush( stdout );
         }
         nb         = gmax( nb,0 );
         st._nByte += _bPub ? 0 : nb;
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
   }
   setBlocking();
#if !defined(WIN32)
   if ( !tot )
      OnException();
#endif // !defined(WIN32)
}

void Socket::OnWrite()
{
   Locker           l( _mtx );
   rtEdgeChanStats &st = stats();
   const char      *cp;
   int              wSz, nWr, nL, tb, nb;
   double           d0, dd;

   // Pre-condition

   if ( !fd() )
      return;

   // 1) Drain ...

   setNonBlocking();
   cp  = _out._bp;
   wSz = _out.bufSz();
   nWr = wSz;
   d0  = _bLatency ? ::rtEdge_TimeNs() : 0.0;
   for ( tb=0; nWr && ( (nb=WRITE( fd(), cp, nWr )) > 0 );  ) {
      dd = _bLatency ? 1000000.0 * ( ::rtEdge_TimeNs() - d0 ) : 0.0;
      if ( _bLatency ) {
         printf( "[%d] WRITE( %d ) in %.1fuS\n", fd(), nb, dd );
         fflush( stdout );
      }
      if ( nb && _log && _log->CanLog( 3 ) ) {
         Locker lck( _log->mtx() );

         _log->logT( 3, "[RAW-WR %06d bytes]\n", nb );
         _log->logT( 4, "   " );
         _log->HexLog( 4, cp, nb );
      }
      cp += nb;
      tb += nb;
      nWr = gmax( nWr-nb, 0 );
   }
   setBlocking();
   if ( !tb )
      return;

   // 2) Re-set

   nL = wSz - tb;
   if ( nL )
      _out.Move( tb, nL );
   else
      _out.Reset();
   st._qSiz    =  _out.bufSz();
   st._qSizMax = gmax( st._qSiz, st._qSizMax );
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
// libmddWire Utilities
////////////////////////////////////////////
void Socket::_CheckHeartbeat( u_int tLastMsg )
{
   int now, age;

   // Pre-condition(s)

   if ( !fd() || _bConnectionless )
      return;

   // OK to Check

   now = _tvNow().tv_sec; 
   age = now - tLastMsg;
   if ( age > ( _tHbeat*2 ) )
      Disconnect( "Heartbeat timeout" );
}

void Socket::_SetHeartbeat()
{
   mddMsgHdr h;
   mddBuf    b;
   char      buf[K];

   // Connection Only

   if ( !_bConnectionless ) {
      sprintf( buf, "%d", _tHbeat );
      h  = _InitHdr( mddMt_ctl );
      _AddAttr( h, _mdd_pAttrPing, buf );
      b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
      Write( b._data, b._dLen );
   }
}

void Socket::_SendPing()
{
   mddBuf b;

   b = ::mddWire_Ping( _mdd );
   Write( b._data, b._dLen );
}

mddMsgHdr Socket::_InitHdr( mddMsgType mt )
{
   mddMsgHdr     h;
   mddFieldList &fl = h._attrs;

   ::memset( &h, 0, sizeof( h ) );
   h._mt    = mt;
   fl       = _fl;
   fl._nFld = 0;
   return h;
}

mddBuf Socket::_SetBuf( const char *str )
{
   return _SetBuf( str, strlen( str ) );
}

mddBuf Socket::_SetBuf( const char *data, int dLen )
{
   mddBuf r;

   r._data = (char *)data;
   r._dLen = dLen;
   return r;
}

const char *Socket::_GetAttr( mddMsgHdr &h, const char *key )
{  
   mddFieldList &fl = h._attrs;
   mddField      f;
   mddBuf       &s = f._val._buf;
   int           i;

   for ( i=0; i<fl._nFld; i++ ) {
      f = fl._flds[i];
      if ( !::strcmp( f._name, key ) )
         return s._data;
   }
   return "";
}

int Socket::_AddAttr( mddMsgHdr &h, const char *key, char *val )
{
   mddFieldList &fl = h._attrs;
   mddField      f;
   mddBuf       &s = f._val._buf;
   int           nf;
   
   nf           = fl._nFld;
   f._type      = mddFld_string;
   f._name      = key;
   s._data      = val;
   s._dLen      = strlen( val );
   fl._flds[nf] = f;
   fl._nFld    += 1;
   return fl._nFld;
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
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
   return( rtn == 0 );
}
