/******************************************************************************
*
*  Channel.cpp
*     yamrCache subscription channel
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <Internal.h>

using namespace YAMR_PRIVATE;

static size_t _hSz8  = sizeof( yamrHdr8 );
static size_t _hSz16 = sizeof( yamrHdr16 );
static size_t _hSz32 = sizeof( yamrHdr32 );
static size_t _max8  = 0x000000ff;
static size_t _max16 = 0x0000ffff;

/////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      C h a n n e l
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Channel::Channel( yamrAttr     attr, 
                  yamr_Context cxt ) :
   Socket( attr._pSvrHosts, 
           ::strstr( attr._pSvrHosts, "udp:" ) == attr._pSvrHosts ),
   _cxt( cxt ),
   _con( attr._pSvrHosts ),
   _SeqNum( 0 ),
   _SessID( 0 )
{
   // libyamr

   _attr            = attr;
   _attr._pSvrHosts = _con.data();

   // Stats

   ::memset( &_dfltStats, 0, sizeof( _dfltStats ) );
   SetStats( &_dfltStats );

   // Session ID

   _SessID = attr._SessionID;
   _SessID = ( _SessID << 32 );
   _SessID = ( _SessID << 16 );
}

Channel::~Channel()
{
   if ( _log )
      _log->logT( 3, "~Channel( %s )\n", dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
yamrAttr Channel::attr()
{
   return _attr;
}

yamr_Context Channel::cxt()
{
   return _cxt;
}

int Channel::Send( yamrBuf yb, u_int16_t wPro, u_int16_t mPro )
{
   yamrHdr8     h8;
   yamrHdr16    h16;
   yamrHdr32    h32;
   yamrBaseHdr *h;
   size_t       nWr, hSz, mSz, nL;

   // Header : < 64K??

   mSz = yb._dLen;
   nWr = 0;
   if ( ( mSz+_hSz8 ) <= _max8 ) {
      hSz        = _hSz8;
      mSz        += hSz;
      h8._MsgLen = mSz;
      h          = &h8;
      h->_Flags  = _YAMR_1BYTE_LEN;
   }
   else if ( ( mSz+_hSz16 ) <= _max16 ) {
      hSz         = _hSz16;
      mSz        += hSz;
      h16._MsgLen = mSz;
      h           = &h16;
      h->_Flags   = _YAMR_2BYTE_LEN;
   }
   else {
      hSz         = _hSz32;
      mSz        += hSz;
      h32._MsgLen = mSz;
      h           = &h32;
      h->_Flags   = _YAMR_4BYTE_LEN;
   }
   h->_Magic        = _YAMR_MAGIC;
   h->_Version      = _YAMR_VERSION;
   h->_MsgProtocol  = mPro;
   h->_WireProtocol = wPro;

   // Header, then payload

   Locker lck( _mtx );

   if ( mSz >= (nL=_out.nLeft()) )
      OnWrite();
   if ( mSz < (nL=_out.nLeft()) ) {
      h->_SeqNum  = ( _SeqNum++ & _YAMR_MAX_MASK );
      h->_SeqNum += _SessID;
      nWr += Write( (char * )h, hSz, false ) ? hSz : 0;
      nWr += Write( yb._data, yb._dLen )     ? yb._dLen : 0;
assert( nWr == mSz );
   }
   return nWr;
}


////////////////////////////////////////////
// Socket Interface
////////////////////////////////////////////
void Channel::OnQLoMark()
{
   if ( _attr._stsCbk )
      (*_attr._stsCbk)( _cxt, yamr_QloMark );
}

void Channel::OnQHiMark()
{
   if ( _attr._stsCbk )
      (*_attr._stsCbk)( _cxt, yamr_QhiMark );
}


////////////////////////////////////////////
// Thread Notifications
////////////////////////////////////////////
void Channel::OnConnect( const char *pc )
{
   if ( _attr._connCbk )
      (*_attr._connCbk)( _cxt, pc, yamr_up );
}

void Channel::OnDisconnect( const char *reason )
{
   if ( _attr._connCbk )
      (*_attr._connCbk)( _cxt, reason, yamr_down );
}

void Channel::OnRead()
{
   Locker      lck( _mtx );
   const char *cp;
   int         i, sz, nMsg, nL;

   // 1) Base class drains channel and populates _in / _cp

   Socket::OnRead();

   // 2) OK, now we chop up ...

   cp = _in._bp;
   sz = _in.bufSz();
   for ( i=0,nMsg=0; i<sz; ) {
#ifdef TODO_YAMR_PROTOCOL
      b._data = (char *)cp;
      b._dLen = sz - i;
      b._hdr  = (mddMsgHdr *)0;
      h       = _InitHdr( mddMt_undef );
      nb      = ::mddSub_ParseHdr( _mdd, b, &h );
      b._dLen = nb;
      b._hdr  = &h;
      if ( !nb )
         break; // for-i
      nMsg  += 1;
      if ( _log && _log->CanLog( 2 ) ) {
         Locker lck( _log->mtx() );
         string s( cp, nb );

         _log->logT( 2, "[XML-RX]" );
         _log->Write( 2, s.data(), nb );
      }
      switch( _proto ) {
         case mddProto_Undef:  break;
         case mddProto_Binary: OnBinary( b ); break;
         case mddProto_MF:     OnMF( b );     break;
         case mddProto_XML:    OnXML( b );    break;
      }
      _ClearUpd();
      cp += nb;
      i  += nb;

      // Stats

      tv            = _tvNow();
      st._lastMsg   = tv.tv_sec;
      st._lastMsgUs = tv.tv_usec;
      st._nMsg     += 1; 
#endif // TODO_YAMR_PROTOCOL
   }

   // Message spans 2+ network packets?

   nL = WithinRange( 0, sz-i, INFINITEs );
   if ( _log && _log->CanLog( 4 ) ) {
      Locker lck( _log->mtx() );

      _log->logT( 4, "%d of %d bytes processed\n", i, sz );
      if ( nL )
         _log->HexLog( 4, cp, nL );
   }
   if ( nMsg && nL )
      _in.Move( sz-nL, nL );
   _in.Reset();
   _in._cp += nL;
}


////////////////////////////////////////////
// TimerEvent Notifications
////////////////////////////////////////////
void Channel::On1SecTimer()
{
   // Reconnect 1st

   Socket::On1SecTimer();

   // Idle Callback

   if ( _bIdleCbk && _attr._idleCbk )
      (*_attr._idleCbk)( _cxt );
}
