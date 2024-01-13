/******************************************************************************
*
*  TapeHeader.cpp
*     Platform-agnostic tape header
*
*  REVISION HISTORY:
*     16 JAN 2023 jcs  Created
*     25 JAN 2023 jcs  Working on WIN64
*      2 MAR 2023 jcs  hdr()
*     12 JAN 2024 jcs  librtEdge
*
*  (c) 1994-2024, Gatea Ltd.
*******************************************************************************/
#include <TapeHeader.h>
#include <assert.h>
#include <string.h>

using namespace RTEDGE_PRIVATE;

/////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      T a p e H e a d e r
//
/////////////////////////////////////////////////////////////////////////////

static u_int64_t   _dSz   = sizeof( GLrecDictEntry );
static u_int64_t   _iSz   = sizeof( u_int64_t );

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
TapeHeader::TapeHeader( GLmmap &vw ) :
   string( "undefined" ),
   _vw( vw ),
   _type( tape_native ),
   _hdr( (GLrecTapeHdr *)0 ),
   _hdr4( (Win64Hdr::GLrecTapeHdr *)0 ),
   _hdr8( (Linux64Hdr::GLrecTapeHdr *)0 ),
   _sizeofLong( sizeof( long ) )
{ ; }


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
const char *TapeHeader::pType()
{
   return data();
}

char *TapeHeader::hdr()
{
   return _vw.data();
}

TapeType TapeHeader::type()
{
   return _type;
}

Bool TapeHeader::Map()
{
   GLrecTapeHdr *h;
   u_int64_t     hSz;
   const char   *pt, *pm, *pp;
   char         *b1, *b2, buf[K];
   int           mSz, tSz;

   // Pre-condition

   if ( !_vw.isValid() )
      return false;

   // Expected size; Map

   b1  = _vw.data();
   h   = (GLrecTapeHdr *)b1;
   hSz = h->_hdrSiz;
   mSz = _sizeofLong;
   tSz = h->_sizeofLong;
   b2  = ( hSz > _vw.siz() ) ?  _vw.map( 0, hSz ) : b1;
   if ( !_vw.isValid() )
      return false;

   // OK to rock  on

   if ( mSz > tSz ) {
      _hdr4 = (Win64Hdr::GLrecTapeHdr *)b2;
      _type = tape_win64;
   }
   else if ( mSz < tSz ) {
      _hdr8 = (Linux64Hdr::GLrecTapeHdr *)b2;
      _type = tape_linux64;
   }
   else
      _hdr = (GLrecTapeHdr *)b2;
   pp = _bMDDirect() ? "MDDirect" : "UPA";
   pt = ( tSz == 8 ) ? "Linux64"  : "Win64"; 
   pm = ( mSz == 8 ) ? "Linux64"  : "Win64"; 
   sprintf( buf, "%s tape recorded on %s; Replaying on %s", pp, pt, pm );
   string::assign( buf );
   return IsValid();
}


////////////////////////////////////////////
// Tape Sizing
////////////////////////////////////////////
u_int64_t TapeHeader::_DbHdrSize( int numDictEntry,
                                  int numSecIdx,
                                  int numRec )
{       
   u_int64_t rtn;
        
   rtn  = _hSz();
   rtn += ( _dSz * numDictEntry );
   rtn += ( _iSz * numSecIdx );
   rtn += ( _RecSiz() * numRec );
   return rtn;         
}

u_int64_t TapeHeader::_hSz()
{
   switch( _type ) {
      case tape_native:  return sizeof( GLrecTapeHdr );
      case tape_win64:   return sizeof( Win64Hdr::GLrecTapeHdr );
      case tape_linux64: return sizeof( Linux64Hdr::GLrecTapeHdr );
   }
assert( 0 );
   return 0;
}

u_int64_t TapeHeader::_sSz()
{
// TODO
   return sizeof( Sentinel );
}

u_int64_t TapeHeader::_RecSiz()
{
   u_int64_t rSz;

   rSz  = TapeRecHdr::_RecSiz( *this );
   rSz += ( _numSecIdxR() * sizeof( u_int64_t ) );
   return rSz;
}

u_int64_t TapeHeader::_DailyIdxSize()
{       
   u_int64_t daySz;

   daySz  = _DbHdrSize( 0, _numSecIdxT(), _maxRec() );
   daySz -= _hSz();   // No TapeHeader
   daySz -= _sSz();   // But Sentinel
   return daySz;  
}       



////////////////////////////////////////////
// Platform-agnostic Accessor
////////////////////////////////////////////
Bool TapeHeader::IsValid()
{
   return( _hdr || _hdr4 || _hdr8 );
}

u_int64_t &TapeHeader::_fileSiz()
{
   if ( _hdr4 ) return _hdr4->_fileSiz;
   if ( _hdr8 ) return _hdr8->_fileSiz;
   return _hdr->_fileSiz;
}

u_int64_t &TapeHeader::_hdrSiz()
{
   if ( _hdr4 ) return _hdr4->_hdrSiz;
   if ( _hdr8 ) return _hdr8->_hdrSiz;
   return _hdr->_hdrSiz;
}

u_int64_t &TapeHeader::_winSiz()
{
   if ( _hdr4 ) return _hdr4->_winSiz;
   if ( _hdr8 ) return _hdr8->_winSiz;
   return _hdr->_winSiz;
}

Bool TapeHeader::_bMDDirect()
{
   if ( _hdr4 ) return _hdr4->_bMDDirect;
   if ( _hdr8 ) return _hdr8->_bMDDirect;
   return _hdr->_bMDDirect;
}

Bool TapeHeader::_bWrite()
{
   if ( _hdr4 ) return _hdr4->_sentinel._bWrite;
   if ( _hdr8 ) return _hdr8->_sentinel._bWrite;
   return _hdr->_sentinel._bWrite;
}

time_t TapeHeader::_tCreate()
{
   if ( _hdr4 ) return _hdr4->_tCreate;
   if ( _hdr8 ) return _hdr8->_tCreate;
   return _hdr->_tCreate;
}

time_t TapeHeader::_tEOD()
{
   if ( _hdr4 ) return _hdr4->_tEOD;
   if ( _hdr8 ) return _hdr8->_tEOD;
   return _hdr->_tEOD;
}

u_int64_t &TapeHeader::_curLoc()
{
   if ( _hdr4 ) return _hdr4->_curLoc;
   if ( _hdr8 ) return _hdr8->_curLoc;
   return _hdr->_curLoc;
}

struct timeval TapeHeader::_curTime()
{
   struct timeval      rc;
   Win64Hdr::Timeval   wt;
   Linux64Hdr::Timeval lt;

   ::memset( &rc, 0, sizeof( rc ) );
   if ( _hdr4 ) {
      wt         = _hdr4->_curTime;
      rc.tv_sec  = wt.tv_sec;
      rc.tv_usec = wt.tv_usec;
   }
   else if ( _hdr8 ) {
      lt         = _hdr8->_curTime;
      rc.tv_sec  = lt.tv_sec;
      rc.tv_usec = lt.tv_usec;
   }
   else if ( _hdr )
      rc = _hdr->_curTime;
   return rc;
}

int &TapeHeader::_numRec()
{
   if ( _hdr4 ) return _hdr4->_numRec;
   if ( _hdr8 ) return _hdr8->_numRec;
   return _hdr->_numRec;
}

double TapeHeader::_dCpu()
{
   if ( _hdr4 ) return _hdr4->_dCpu;
   if ( _hdr8 ) return _hdr8->_dCpu;
   return _hdr->_dCpu;
}

int TapeHeader::_numDictEntry()
{
   if ( _hdr4 ) return _hdr4->_numDictEntry;
   if ( _hdr8 ) return _hdr8->_numDictEntry;
   return _hdr->_numDictEntry;
}

struct timeval TapeHeader::_curIdxTm()
{
   struct timeval      rc;
   Win64Hdr::Timeval   wt;
   Linux64Hdr::Timeval lt;

   ::memset( &rc, 0, sizeof( rc ) );
   if ( _hdr4 ) {
      wt         = _hdr4->_curIdxTm;
      rc.tv_sec  = wt.tv_sec;
      rc.tv_usec = wt.tv_usec;
   }
   else if ( _hdr8 ) {
      lt         = _hdr8->_curIdxTm;
      rc.tv_sec  = lt.tv_sec;
      rc.tv_usec = lt.tv_usec;
   }
   else if ( _hdr )
      rc = _hdr->_curIdxTm;
   return rc;
}


int TapeHeader::_curIdx()
{
   if ( _hdr4 ) return _hdr4->_curIdx;
   if ( _hdr8 ) return _hdr8->_curIdx;
   return _hdr->_curIdx;
}

int TapeHeader::_secPerIdxT()
{
   if ( _hdr4 ) return _hdr4->_secPerIdxT;
   if ( _hdr8 ) return _hdr8->_secPerIdxT;
   return _hdr->_secPerIdxT;
}

int TapeHeader::_numSecIdxT()
{
   if ( _hdr4 ) return _hdr4->_numSecIdxT;
   if ( _hdr8 ) return _hdr8->_numSecIdxT;
   return _hdr->_numSecIdxT;
}

int TapeHeader::_secPerIdxR()
{
   if ( _hdr4 ) return _hdr4->_secPerIdxR;
   if ( _hdr8 ) return _hdr8->_secPerIdxR;
   return _hdr->_secPerIdxR;
}

int TapeHeader::_numSecIdxR()
{
   if ( _hdr4 ) return _hdr4->_numSecIdxR;
   if ( _hdr8 ) return _hdr8->_numSecIdxR;
   return _hdr->_numSecIdxR;
}

int TapeHeader::_maxRec()
{
   if ( _hdr4 ) return _hdr4->_maxRec;
   if ( _hdr8 ) return _hdr8->_maxRec;
   return _hdr->_maxRec;
}

long TapeHeader::_tStart()
{
   if ( _hdr4 ) return _hdr4->_sentinel._tStart;
   if ( _hdr8 ) return _hdr8->_sentinel._tStart;
   return _hdr->_sentinel._tStart;
}

const char *TapeHeader::_version()
{
   if ( _hdr4 ) return _hdr4->_sentinel._version;
   if ( _hdr8 ) return _hdr8->_sentinel._version;
   return _hdr->_sentinel._version;
}

const char *TapeHeader::_signature()
{
   if ( _hdr4 ) return _hdr4->_sentinel._signature;
   if ( _hdr8 ) return _hdr8->_sentinel._signature;
   return _hdr->_sentinel._signature;
}

GLrecUpdStats TapeHeader::_tapeStats()
{
   GLrecUpdStats       rc;
   struct timeval     &tv = rc._tMsg;
   Win64Hdr::Timeval   wt;
   Linux64Hdr::Timeval lt;

   ::memset( &rc, 0, sizeof( rc ) );
   if ( _hdr4 ) {
      wt         = _hdr4->_tapeStats._tMsg;
      tv.tv_sec  = wt.tv_sec;
      tv.tv_usec = wt.tv_usec;
      rc._nMsg   = _hdr4->_tapeStats._nMsg;
      rc._nByte  = _hdr4->_tapeStats._nByte;
   } 
   if ( _hdr8 ) {
      lt         = _hdr8->_tapeStats._tMsg;
      tv.tv_sec  = lt.tv_sec;
      tv.tv_usec = lt.tv_usec;
      rc._nMsg   = _hdr8->_tapeStats._nMsg;
      rc._nByte  = _hdr8->_tapeStats._nByte;
   }
   if ( _hdr ) 
      rc = _hdr->_chanStats;
   return rc;
}

GLrecChanStats TapeHeader::_chanStats()
{
   GLrecChanStats      rc;
   struct timeval     &tv = rc._tMsg;
   Win64Hdr::Timeval   wt;
   Linux64Hdr::Timeval lt;

   ::memset( &rc, 0, sizeof( rc ) );
   if ( _hdr4 ) {
      wt         = _hdr4->_chanStats._tMsg;
      tv.tv_sec  = wt.tv_sec;
      tv.tv_usec = wt.tv_usec;
      rc._nMsg   = _hdr4->_chanStats._nMsg;
      rc._nByte  = _hdr4->_chanStats._nByte;
      ::memcpy( rc._dstConn,  _hdr4->_chanStats._dstConn,  sizeof( rc._dstConn ) );
      ::memcpy( rc._dUPA,     _hdr4->_chanStats._dUPA,     sizeof( rc._dUPA ) );
      ::memcpy( rc._dLockIns, _hdr4->_chanStats._dLockIns, sizeof( rc._dLockIns ) );
      ::memcpy( rc._dIns,     _hdr4->_chanStats._dIns,     sizeof( rc._dIns ) );
   } 
   if ( _hdr8 ) {
      lt         = _hdr8->_chanStats._tMsg;
      tv.tv_sec  = lt.tv_sec;
      tv.tv_usec = lt.tv_usec;
      rc._nMsg   = _hdr8->_chanStats._nMsg;
      rc._nByte  = _hdr8->_chanStats._nByte;
      ::memcpy( rc._dstConn,  _hdr8->_chanStats._dstConn,  sizeof( rc._dstConn ) );
      ::memcpy( rc._dUPA,     _hdr8->_chanStats._dUPA,     sizeof( rc._dUPA ) );
      ::memcpy( rc._dLockIns, _hdr8->_chanStats._dLockIns, sizeof( rc._dLockIns ) );
      ::memcpy( rc._dIns,     _hdr8->_chanStats._dIns,     sizeof( rc._dIns ) );
   }
   if ( _hdr ) 
      rc = _hdr->_chanStats;
   return rc;
}

Sentinel TapeHeader::_sentinel()
{
   Sentinel rc;

   ::memset( &rc, 0, sizeof( rc ) );
   if ( _hdr4 ) {
      ::memcpy( &rc, &_hdr4->_sentinel, sizeof( rc ) );
      rc._tStart = _hdr4->_sentinel._tStart;
   } 
   if ( _hdr8 ) {
      ::memcpy( &rc, &_hdr8->_sentinel, sizeof( rc ) );
      rc._tStart = _hdr8->_sentinel._tStart;
   }
   if ( _hdr ) 
      rc = _hdr->_sentinel;
   return rc;
}



/////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      T a p e R e c H d r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
TapeRecHdr::TapeRecHdr( TapeHeader &tapeHdr, char *recHdr ) :
   _tapeHdr( tapeHdr ),
   _type( tapeHdr.type() ),
   _hdr( (GLrecTapeRec *)0 ),
   _hdr4( (Win64Hdr::GLrecTapeRec *)0 ),
   _hdr8( (Linux64Hdr::GLrecTapeRec *)0 )
{
   switch( _type ) {
      case tape_native:  _hdr  = (GLrecTapeRec *)recHdr;             break;
      case tape_win64:   _hdr4 = (Win64Hdr::GLrecTapeRec *)recHdr;   break;
      case tape_linux64: _hdr8 = (Linux64Hdr::GLrecTapeRec *)recHdr; break;
   }
}


////////////////////////////////////////////
// Platform-agnostic Accessor
////////////////////////////////////////////
struct timeval TapeRecHdr::_tMsg()
{
   struct timeval rc;

   rc.tv_sec  = 0;
   rc.tv_usec = 0;
   if ( _hdr4 ) {
      rc.tv_sec  = _hdr4->_tMsg.tv_sec;
      rc.tv_usec = _hdr4->_tMsg.tv_usec;
   }
   else if ( _hdr8 ) {
      rc.tv_sec  = _hdr8->_tMsg.tv_sec;
      rc.tv_usec = _hdr8->_tMsg.tv_usec;
   }
   else if ( _hdr )
      rc = _hdr->_tMsg;
   return rc;
}

u_int64_t TapeRecHdr::_nMsg()
{
   if ( _hdr4 ) return _hdr4->_nMsg;
   if ( _hdr8 ) return _hdr8->_nMsg;
   return _hdr->_nMsg;
}

u_int64_t TapeRecHdr::_nByte()
{
   if ( _hdr4 ) return _hdr4->_nByte;
   if ( _hdr8 ) return _hdr8->_nByte;
   return _hdr->_nByte;
}

int TapeRecHdr::_dbIdx()
{
   if ( _hdr4 ) return _hdr4->_dbIdx;
   if ( _hdr8 ) return _hdr8->_dbIdx;
   return _hdr->_dbIdx;
}

int TapeRecHdr::_StreamID()
{
   if ( _hdr4 ) return _hdr4->_StreamID;
   if ( _hdr8 ) return _hdr8->_StreamID;
   return _hdr->_StreamID;
}

char *TapeRecHdr::_svc()
{
   if ( _hdr4 ) return _hdr4->_svc;
   if ( _hdr8 ) return _hdr8->_svc;
   return _hdr->_svc;
}

char *TapeRecHdr::_tkr()
{
   if ( _hdr4 ) return _hdr4->_tkr;
   if ( _hdr8 ) return _hdr8->_tkr;
   return _hdr->_tkr;
}

int TapeRecHdr::_channelID()
{
   if ( _hdr4 ) return _hdr4->_channelID;
   if ( _hdr8 ) return _hdr8->_channelID;
   return _hdr->_channelID;
}

u_int64_t TapeRecHdr::_loc()
{
   if ( _hdr4 ) return _hdr4->_loc;
   if ( _hdr8 ) return _hdr8->_loc;
   return _hdr->_loc;
}

u_int64_t TapeRecHdr::_locImg()
{
   if ( _hdr4 ) return _hdr4->_locImg;
   if ( _hdr8 ) return _hdr8->_locImg;
   return _hdr->_locImg;
}

u_int64_t TapeRecHdr::_rSz()
{
   return _RecSiz( _tapeHdr );
}


////////////////////////////////////////////
// Class-wide
////////////////////////////////////////////
u_int64_t TapeRecHdr::_RecSiz( TapeHeader &h )
{
   switch( h.type() ) {
      case tape_native:  return sizeof( GLrecTapeRec );
      case tape_win64:   return sizeof( Win64Hdr::GLrecTapeRec );
      case tape_linux64: return sizeof( Linux64Hdr::GLrecTapeRec );
   }
assert( 0 );
   return 0;
}
