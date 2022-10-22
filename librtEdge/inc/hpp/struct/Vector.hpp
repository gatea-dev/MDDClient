/******************************************************************************
*
*  Vector.hpp
*     librtEdge Vector
*
*  REVISION HISTORY:
*     21 OCT 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Vector_H
#define __RTEDGE_Vector_H
#include <assert.h>
#include <hpp/ByteStream.hpp>

#ifndef DOXYGEN_OMIT
/** 
 * \brief Minimum alloc size 
 */
#define _MIN_VECTOR  16*K

/** 
 * \brief Raw Buffer w/ bytes allocated 
 */
typedef struct {
   char     *_data;
   u_int64_t _dLen;
   u_int64_t _nAlloc;
} VecBuf64;

/** 
 *\brief Empty buffer 
 */
static VecBuf64 _zBuf64 = { (char *)0, 0, 0 };

#pragma pack (push, 1)

/**
 * \brief Vector Update Value
 */
typedef struct {
   /** \brief Location in Vector */
   int       _Index; 
   /** \brief Value : 10 sig-fig */
   u_int64_t _Value;
} VecWireUpdVal;

/**
 * \brief ByteStream Header
 */
typedef struct {
   /** \brief Message Size, including header */
   u_int64_t _MsgSz; 
   /** \brief Precision */
   int       _Precision; 
   /** \brief true for complete Vector Image; false for Updates ... */
   bool      _bImg;
   /** \brief Number of values */
   int       _nVal;
   /*
    * Either-or, depending on _bImg
    *
   u_int64_t     _vals[_nVal];
   VecWireUpdVal _vals[_nVal];
    */
} VecWireHdr;

#pragma pack (pop)

#endif // DOXYGEN_OMIT

namespace RTEDGE
{


///////////////////////////////////////////////
//
//      c l a s s   V e c t o r V a l u e
//
////////////////////////////////////////////////

/**
 * \class VectorValue
 * \brief Single Vector Value including index
 */
class VectorValue
{
public:
   /** \brief Location in Vector */
   int    _Index;
   /** \brief Value */
   double _Value;

}; // class VectorValue

typedef std::vector<double>      VectorImage;
typedef std::vector<VectorValue> VectorUpdate;

#ifndef DOXYGEN_OMIT
class IVector
{
public:
	virtual ~IVector() { ; }
	virtual void IOnSubscribeData( int nByte, int totByte ) = 0;
	virtual void IOnError( const char *err ) = 0;
	virtual void IOnSubscribeComplete( VecBuf64 buf ) = 0;
	virtual void IOnPublishData( int nByte, int totByte ) = 0;
	virtual void IOnPublishComplete( int nByte ) = 0;
}; // class IVector
#endif // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//        c l a s s   V e c t o r
//
////////////////////////////////////////////////

/**
 * \class Vector
 * \brief This class encapsulates a one-dimensional Vector : Pub or Sub
 *
 * When consuming you receive asynchronous notifications as follows:
 * + OnData() - When a "chunk" of the stream is received (i.e. 1 message)
 * + OnError() - Stream error
 * + OnSubscribeComplete() - Stream completely consumed
 *
 * When publishing you receive asynchronous notifications as follows:
 * + OnPublishData() - When a "chunk" of the stream has been published
 * + OnError() - Stream error
 * + OnPublishComplete() - Stream completely published
 */
class Vector : public IVector
{
#ifndef DOXYGEN_OMIT
	////////////////////////////////////////////////
	//
	//      c l a s s   V e c t o r S t r e a m
	//
	////////////////////////////////////////////////
	/**
	 * \class VectorStream
	 * \brief 'Has-a' ByteStream class for marshalling / demarshalling
	 */
public:
	class VectorStream : public ByteStream
	{
	   ////////////////////////////////////
	   // Constructor / Destructor
	   ////////////////////////////////////
	public:
	   VectorStream( IVector &vec, const char *svc, const char *tkr ) :
	      ByteStream( svc, tkr ),
	      _vec( vec ),
	      _buf( _zBuf64 )
	   { ; }

	   ~VectorStream()
	   {
	      if ( _buf._data )
	         delete[] _buf._data;
	   }

	   ////////////////////////////////////
	   // Access
	   ////////////////////////////////////
	public:
	   VecBuf64 data()
	   {
	      return _buf;
	   }

	   ////////////////////////////////////
	   // Asynchronous Callbacks - ByteStream
	   ////////////////////////////////////
	protected:
	   virtual void OnData( rtBUF bs )
	   {
	      _CopyIn( bs );
	      _vec.IOnSubscribeData( bs._dLen, _buf._dLen );
	   }

	   virtual void OnError( const char *err )
	   {
	      _vec.IOnError( err );
	   }

	   virtual void OnSubscribeComplete()
	   {
	      _vec.IOnSubscribeComplete( _buf );
	   }

	   virtual void OnPublishData( int nByte, int totByte )
	   {
	      _vec.IOnPublishData( nByte, totByte );
	   }

	   virtual void OnPublishComplete( int nByte )
	   {
	      _vec.IOnPublishComplete( nByte );
	   }

	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   void _CopyIn( rtBUF bs )
	   {
	      VecBuf64 &rc = _buf;
	      char     *cp;
	      u_int64_t off, sz, aSz;

	      // Double as needed

	      off = rc._dLen;
	      sz  = bs._dLen;
	      if ( (off+sz) >= rc._nAlloc ) {
	         aSz      = rc._nAlloc ? _MIN_VECTOR : ( rc._nAlloc >> 1 );
	         cp       = rc._data;
	         rc._data = new char[aSz];
	         if ( cp ) {
	            ::memcpy( rc._data, cp, off );
	            delete[] cp;
	         }
	      }

	      // Copy 'er in 

	      ::memcpy( rc._data+off, bs._data, sz );
	      rc._dLen += sz;
	   }

	   ////////////////////////
	   // private Members
	   ////////////////////////
	private:
	   IVector &_vec;
	   VecBuf64 _buf;

	}; // class VectorStream

#endif // DOXYGEN_OMIT

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor for both Publish() and Subscribe()
	 *
	 * \param svc - Service supplying this ByteStream if Subscribe()
	 * \param tkr - Published name of this ByteStream
	 * \param size - Vector Size
	 * \param precision - Sig Fig; 0 to 'learn' from Subscription Stream
	 */
	Vector( const char *svc, const char *tkr, int size, int precision=0.0 ) :
	   _str( *this, svc, tkr ),
	   _vals( new double[size] ),
	   _size( size ),
	   _precision( precision ),
	   _precIn( 0.0 ),
	   _precOut( 0.0 )
	{
	   if ( _precision ) {
	      _precOut = ::pow10( _precision );
	      _precIn  = 1.0 / _precOut;
	   }
	}

	virtual ~Vector()
	{
	   delete[] _vals;
	}

	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
public:
	/**
	 * \brief Called asynchronously when Initial Image for the Vector arrives
	 *
	 * Override this method in your application to take action when the 
	 * Initial Image is available.
	 *
	 * \param img - Initial Image
	 */
	virtual void OnData( VectorImage &img )
	{ ; }

	/**
	 * \brief Called asynchronously when Vector update
	 *
	 * Override this method in your application to take action when the 
	 * Vector Updates
	 *
	 * \param upd - Values that have updated
	 */
	virtual void OnData( VectorUpdate &upd )
	{ ; }

	/**
	 * \brief Called asynchronously if there is an error in consuming
	 * the single- or multi-message Byte Stream.
	 *
	 * This will be the last message you receive on this ByteStream.
	 *
	 * Override this method in your application to take action when
	 * Byte Stream arrives in error.  If needed, the current data is
	 * available in subBuf().
	 *
	 * \param err - Textual description of error
	 */
	virtual void OnError( const char *err )
	{ ; }

	/**
	 * \brief Called asynchronously as we publish chunks of the single-
	 * or multi-message Byte Stream.
	 *
	 * Override this method in your application to take action when
	 * Byte Stream chunks are published to rtEdgeCache3.
	 *
	 * \param nByte - Number of bytes in chunk
	 * \param totByte - Total bytes published so far
	 */
	virtual void OnPublishData( int nByte, int totByte )
	{ ; }

	/**
	 * \brief Called asynchronously once the complete ByteStream has been
	 * published.
	 *
	 * Override this method in your application to take action when
	 * the complete Byte Stream has been published into rtEdgeCache3.
	 * Published data data is available in pubBuf().
	 *
	 * \param nByte - Number of bytes published
	 */
	virtual void OnPublishComplete( int nByte )
	{ ; }

#ifndef DOXYGEN_OMIT
	////////////////////////////////////
	// IVector Interface
	////////////////////////////////////
private:
	virtual void IOnSubscribeData( int nByte, int totByte )
	{
	   // TODO : Notify??
	}

	virtual void IOnSubscribeComplete( VecBuf64 buf )
	{
	   char          *bp, *cp;
	   VecWireHdr    *h;
	   VecWireUpdVal *udb;
	   VectorImage    img;
	   VectorUpdate   upd;
	   VectorValue    v;
	   u_int64_t      sz, *vdb;
	   int            i, ix, nv;

	   // Header

	   bp  = buf._data;
	   sz  = buf._dLen;
	   cp  = bp;
	   h   = (VecWireHdr *)cp;
	   nv  = h->_nVal;
assert( h->_MsgSz == sz );
assert( nv <= _size ); 
	   cp += sizeof( VecWireHdr );

	   // Once, if not defined by user

	   if ( !_precision && !_precOut ) {
	      _precision = h->_Precision;
	      _precOut   = ::pow10( _precision );
	      _precIn    = 1.0 / _precOut;
	   }

	   // Pull 'em out

	   vdb = (u_int64_t *)cp;
	   udb = (VecWireUpdVal *)cp;
	   for ( i=0; i<nv; i++ ) {
	      ix = i;
	      if ( h->_bImg ) {
	         _vals[ix] = _precIn * vdb[i];
	         cp       += sizeof( u_int64_t );
	         img.push_back( _vals[ix] );
	      } 
	      else {
	         ix        = udb[i]._Index;
	         _vals[ix] = _precIn * udb[i]._Value;
assert( ix < _size );
	         cp       += sizeof( VecWireUpdVal );
	         v._Index  = ix;
	         v._Value  = _vals[ix];
	         upd.push_back( v );
	      } 
	   }
	   if ( h->_bImg )
	      OnData( img );
	   else
	      OnData( upd );
	}

	virtual void IOnError( const char *err )
	{
	   OnError( err );
	}

	virtual void IOnPublishData( int nByte, int totByte )
	{
	   OnPublishData( nByte, totByte );
	}

	virtual void IOnPublishComplete( int nByte )
	{
	   OnPublishComplete( nByte );
	}

#endif // DOXYGEN_OMIT

	////////////////////////
	// private Members
	////////////////////////
private:
	VectorStream _str;
	double      *_vals;
	int          _size;
	int          _precision;
	double       _precIn;
	double       _precOut;

}; // class Vector

} // namespace RTEDGE

#endif // __LIBRTEDGE_Vector_H 
