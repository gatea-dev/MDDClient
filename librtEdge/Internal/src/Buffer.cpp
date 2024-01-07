/******************************************************************************
*
*  Buffer.cpp
*
*  REVISION HISTORY:
*      5 JAN 2024 jcs  Created (from Socket.cpp)
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <Buffer.h>

using namespace RTEDGE_PRIVATE;

/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      B u f f e r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Buffer::Buffer( int maxSz ) :
   _bp( (char *)0 ),
   _cp( (char *)0 ),
   _qAlloc( 0 ),
   _qMax( maxSz ),
   _bConnectionless( false )
{ ; }

Buffer::~Buffer()
{
   delete[] _bp;
}


////////////////////////////////////////////
// Common Operations
////////////////////////////////////////////
void Buffer::Init( int nAlloc )
{
   Grow( nAlloc );
   Reset();
}

bool Buffer::Grow( int nReqGrow )
{
   char *lwc;
   int   bSz, nGrow;

   // Pre-condition

   nGrow = b_gmin( nReqGrow, ( _qMax-_qAlloc ) );
   nGrow = b_WithinRange( 0, nGrow, _qMax );  // Build 56
   if ( !nGrow )
      return false;

   // Save

   lwc = _bp;
   bSz = _cp - _bp;

   // Grow

   _qAlloc += nGrow;
   _bp      = new char[_qAlloc];
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

rtBUF Buffer::buf()
{
   rtBUF b;

   b._data = _bp;
   b._dLen = bufSz();
   return b;
}

int Buffer::nLeft()
{
   int rtn;

   rtn = _qAlloc - bufSz() - 1;
   return rtn;
}

void Buffer::Set( int off )
{
   _cp = _bp + off;
}

int Buffer::ReadIn( int fd, int nL )
{
   int nb;

   if ( _bConnectionless )
      nb = ::recv( fd, _cp, nL, 0 );
   else
      nb = READ( fd, _cp, nL );
   _cp += b_gmax( nb, 0 );
// assert( bufSz() <= _qAlloc );
   return nb;
}


////////////////////////////////////////////
// Instance-Specific Operations
////////////////////////////////////////////
void Buffer::Reset()
{
   _cp = _bp;
}

int Buffer::bufSz()
{
   return( _cp - _bp );
}

int Buffer::WriteOut( int fd, int off, int wSz )
{
   char *cp;
   int   nb;

   cp  = _bp + off;
   wSz = b_gmin( wSz, bufSz() );
   nb  = wSz ? WRITE( fd, cp, wSz ) : 0;
   return b_gmax( nb, 0 );
}

bool Buffer::Push( char *cp, int len )
{
   // Grow by up to _qMax, if needed

   if ( ( nLeft() < len ) && !Grow( _qAlloc ) )
      return false;

   // OK to copy

   ::memcpy( _cp, cp, len );
   _cp += len;
   return true;
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



/////////////////////////////////////////////////////////////////////////////
//
//           c l a s s      C i r c u l a r B u f f e r
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
CircularBuffer::CircularBuffer( int maxSz ) :
   Buffer( maxSz ),
   _beg( 0 ),
   _end( 0 ),
   _qSz( 0 )
{ ; }


////////////////////////////////////////////
// Instance-Specific Operations
////////////////////////////////////////////
void CircularBuffer::Reset()
{
   Buffer::Reset();
   _beg = 0;
   _end = 0;
   _qSz = 0;
}

int CircularBuffer::bufSz()
{
   return _qSz;
}

int CircularBuffer::WriteOut( int fd, int notUsed, int wSz )
{
   char *rp;
   int   nb, nb2, nL, sz1, sz2, wSz2;

   /*
    * 1) If capacity, write in single step;
    * 2) Else, write in 2 steps
    */
   wSz = b_gmin( wSz, bufSz() );
   nL  = _qAlloc - _beg;
   rp  = _bp + _beg;
   nb  = 0;
   if ( b_InRange( 0, wSz, nL ) ) {
      nb    = WRITE( fd, rp, wSz );
      nb    = b_gmax( nb, 0 );
      _beg += nb;
      if ( _beg == _qAlloc )
         _beg = 0;
   }
   else if ( wSz ) {
      sz1  = b_gmin( wSz, _qAlloc - _beg );
      nb   = WRITE( fd, rp, sz1 );
      nb   = b_gmax( nb, 0 );
      _beg = ( _beg + nb ) % _qAlloc;
      rp   = _bp + _beg;
      wSz2 = wSz - nb;
      /*
       * nb == sz1??
       */
      if ( wSz2 && !_beg ) {
assert( nb == sz1 );
         sz2  = b_gmin( wSz2, _qAlloc - _beg );
         nb2  = b_gmax( WRITE( fd, rp, sz2 ), 0 );
         _beg = ( _beg + nb2 ) % _qAlloc;
         nb  += nb2;
      }
   }
   _qSz -= nb;
   return nb;
}

bool CircularBuffer::Push( char *data, int dLen )
{
   char *rp;
   int   nL, sz1, sz2;

   // Grow by up to _qMax, if needed

   if ( ( nLeft() < dLen ) && !Grow( _qAlloc ) )
      return false;

   /*
    * 1) If capacity, write in single step;
    * 2) Else, write in 2 steps
    */
   nL  = _qAlloc - _end;
   sz1 = b_gmin( nL, dLen ); 
   sz2 = dLen - sz1;
   rp  = data;
   if ( sz1 )
      rp += _memcpy( rp, sz1 );
   if ( sz2 )
      rp += _memcpy( rp, sz2 );
   _qSz += dLen;
   return true;
}



////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
int CircularBuffer::_memcpy( char *rp, int dLen )
{
   char *wp;

   wp = _bp + _end;
   ::memcpy( wp, rp, dLen );
   _end += dLen;
   if ( _end == _qAlloc )
      _end = 0;
   return dLen;
}
