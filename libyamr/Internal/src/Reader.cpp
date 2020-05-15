/******************************************************************************
*
*  Reader.cpp
*     yamrCache tape Reader
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
//                   c l a s s      R e a d e r
//
/////////////////////////////////////////////////////////////////////////////

static u_int64_t _maxFileSz = 0x0000ffffffffffff; // 255 TB
static u_int64_t _winSz     = 64 * 1024 * 1024;
static size_t    _hSz       = sizeof( GLyamrTapeHdr );
static size_t    _iSz       = sizeof( u_int64_t );
static size_t    _thSz      = sizeof( yamrTapeMsg );

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Reader::Reader( const char     *file,
                yamrTape_Context cxt ) :
   GLmmap( (char *)file, (char *)0, (OFF_T)0, (OFF_T)_maxFileSz ),
   _cxt( cxt ),
   _tape( file ),
   _pos( 0 ),
   _bOK( true )
{
   // 'Snap' Header : Which fixes end of file at load time

   ::memset( &_hdr, 0, _hSz );
   if ( (_bOK=isValid()) )
      ::memcpy( &_hdr, data(), _hSz );
   Rewind();
}

Reader::~Reader()
{
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
GLyamrTapeHdr &Reader::hdr()
{
   return _hdr;
}

yamrTape_Context Reader::cxt()
{
   return _cxt;
}

const char *Reader::tape()
{
   return _tape.data();
}

u_int64_t *Reader::idb()
{
   char *cp;

   cp  = data();
   cp += _hSz;
   return (u_int64_t *)cp;
}

u_int64_t Reader::HdrSz()
{
   return( _hSz + ( _hdr._numSecIdxT * _iSz ) );
}

u_int64_t Reader::NumLeftMap()
{
   u_int64_t nL;

   nL = siz() - ( _pos - offset() );
   return nL;
}

u_int64_t Reader::NumLeftTape()
{
   return( _hdr._curLoc - _pos );
}

bool Reader::ReadWindow( u_int64_t winSz )
{
   if ( !winSz )
      winSz = gmin( NumLeftTape(), _winSz );
   if ( !winSz )
      return false;
   map( _pos, winSz );
   return isValid();
}

u_int64_t Reader::Rewind( u_int64_t initPos )
{
   yamrMsg ym;

   // Header / Validate

   _pos = initPos;
   if ( !(_bOK=ReadWindow( _hSz )) )
      return 0;
   ::memcpy( &_hdr, data(), _pos );
   _bOK  = isValid();
   _bOK &= !::strcmp( _hdr._signature, YAMR_SIG_001 );
   _bOK &= ( _hdr._curLoc <= _hdr._fileSiz );
   _pos  = HdrSz();
   _bOK  = ReadWindow();

   // View-only of 1st message

   Read( ym, true );
   return ym._Timestamp; 
}

u_int64_t Reader::RewindTo( u_int64_t tmPos )
{
   yamrMsg    ym;
   u_int64_t  tUnix, tNs;
   time_t     t;
   int        idx;
   struct tm *tm, lt;

   /*
    * 1) 1-day tape only
    * 2) 'Rough' Hop to '_secPerIdxT' mark
    */
   tUnix = tmPos / _NANO;
   tNs   = tmPos % _NANO;
   t     = (time_t)tUnix;
   tm    = ::localtime_r( &t, &lt );
   idx   = ( lt.tm_hour * 3600 ) + ( lt.tm_min * 60 ) + lt.tm_sec;
   idx   = WithinRange( 0, idx / _hdr._secPerIdxT, _hdr._numSecIdxT-1 );
   _pos  = idb()[idx];
   if ( tNs || tm )
      ::yamr_breakpoint();
   /*
    * 3) Walk until we get to tmPos; View, then read
    */
   ::memset( &ym, 0, sizeof( ym ) );
   while( ym._Timestamp < tmPos ) {
      if ( !Read( ym, true ) )
         break; // for-while
      if( ym._Timestamp >= tmPos )
         break; // for-while 
      Read( ym, false );
   }
   return ym._Timestamp;
}

bool Reader::Read( yamrMsg &y, bool bViewOnly )
{
   yamrBuf     &yb = y._Data;
   yamrTapeMsg *yt;
   char        *cp;
   u_int64_t    nL, off;
   size_t       mSz;

   // Any room at the inn??

   ::memset( &y, 0, sizeof( y ) );
   if ( !_bOK )
      return false;
   mSz = _thSz;
   if ( (nL=NumLeftMap()) < mSz ) {
      if ( !ReadWindow() )
         return false;
   }
   off = _pos - offset();
   cp  = data();
   cp += off;
   yt  = (yamrTapeMsg *)cp;
   mSz = yt->_MsgLen;
   if ( ( (nL=NumLeftMap()) < mSz ) ) {
      if ( !ReadWindow() )
         return false;
      off = _pos - offset();
      cp  = data();
      cp += off;
      yt  = (yamrTapeMsg *)cp;
   }
   cp += _thSz;

   // Fill in yamrMsg

   y._Timestamp    = yt->_Timestamp;
   y._Host         = yt->_Host;
   y._SessionID    = yt->_SessionID;
   y._SeqNum       = yt->_SeqNum;
   y._MsgProtocol  = yt->_MsgProtocol;
   y._WireProtocol = yt->_WireProtocol;
   yb._data        = cp;
   yb._dLen        = yt->_MsgLen - _thSz;
   yb._opaque      = (void *)0;

   // Bump read offset and return

   _pos += bViewOnly ? 0 : mSz;
   return true;
}
