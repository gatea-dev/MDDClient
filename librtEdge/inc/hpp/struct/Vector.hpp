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

using namespace std;

#ifndef DOXYGEN_OMIT
/**
 * \brief Put ByteStream payload into  this field
 */
static int _fidPayload = 10001;

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

typedef vector<double>      VectorImage;
typedef vector<VectorValue> VectorUpdate;

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
 * \brief A one-dimensional Vector of doubles : Pub or Sub
 *
 * When consuming you receive asynchronous notifications as follows:
 * + OnData( VectorImage & ) - Complete Vector Update
 * + OnData( VectorUpdate & ) - Partiel Vector Update
 * + OnError() - Error
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
	   {
	      // We are updating stream

	      SetSnapshot( false );
	   }

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
	      _buf._dLen = 0;
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
	      while ( (off+sz) >= rc._nAlloc ) {
	         aSz        = !rc._nAlloc ? _MIN_VECTOR : ( rc._nAlloc >> 1 );
	         cp         = rc._data;
	         rc._data   = new char[aSz];
	         rc._nAlloc = aSz;
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
	 * \param precision - Sig Fig; 0 to 'learn' from Subscription Stream
	 */
	Vector( const char *svc, const char *tkr, int precision=0 ) :
	   _str( *this, svc, tkr ),
	   _upds(),
	   _vals(),
	   _precision( 0 ),
	   _precIn( 0.0 ),
	   _precOut( 0.0 ),
	   _bImg( true )
	{
	   if ( precision )
	      SetPrecision( precision );
	}

	virtual ~Vector()
	{
	   _vals.clear();
	}

	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Service Name of this Vector
	 *
	 * \return Service Name of this Vector
	 */
	const char *Service()
	{
	   return _str.Service();
	}

	/**
	 * \brief Returns Ticker Name of this Vector
	 *
	 * \return Ticker Name of this Vector
	 */
	const char *Ticker()
	{
	   return _str.Ticker();
	}

	/**
	 * \brief Return Vector Values
	 *
	 * \param img - Resultant Vector to populate
	 * \return VectorImage of all values
	 */
	VectorImage Get( VectorImage &img )
	{
	   img.clear();
	   img = _vals;
	   return img;
	}


	////////////////////////////////////
	// Mutator
	////////////////////////////////////
	/**
	 * \brief Set Entire Vector of Values
	 *
	 * \param img - vector of values to load
	 * \return Number loaded
	 */
	size_t Update( VectorImage &img )
	{
	   size_t i, n;

	   _vals.clear();
	   _upds.clear();
	   n = img.size();
	   for ( i=0; i<n; _vals.push_back( img[i++] ) );
	   _bImg = true;
	   return n;
	}

	/**
	 * \brief Set Single Value
	 *
	 * \param idx - Value Index
	 * \param val - Value
	 * \return 1 if set; 0 if outside boundary
	 */
	int UpdateAt( size_t idx, double val )
	{
	   VectorValue u = { (int)idx, val };
	   size_t      nAdd;

	   // Grow as necessary

	   nAdd = (idx+1) - _vals.size();
	   for ( size_t i=0; i<nAdd; _vals.push_back( 0.0 ), i++ );

	   // Safe to set

	   _vals[idx] = val;
	   _upds.push_back( u );
	   return 1;
	}

	/**
	 * \brief Shift values left (down), optionally rolling to end
	 *
	 * \param num - Number to shift
	 * \param bRollToEnd - true to roll values to end; false for 0.0 
	 * \return Number shifted
	 */
	size_t ShiftLeft( size_t num, bool bRollToEnd=true )
	{
	   VectorImage tmp;
	   size_t      i, n;
	   double      dz;

	   // Up to _vals.size()

	   num = gmin( _vals.size(), num );
	   for ( i=0; i<num; i++ ) {
	      tmp.push_back( _vals.front() );
	      _vals.erase( _vals.begin() );
	   }
	   n = tmp.size();
	   for ( i=0; i<n; i++ ) {
	      dz = bRollToEnd ? tmp[i] : 0.0;
	      _vals.push_back( dz );
	   }
	   return num;
	}

	/**
	 * \brief Shift values right (up), optionally rolling to beginning
	 *
	 * \param num - Number to shift
	 * \param bRollToFront - true to roll values to front; false for 0.0 
	 * \return Number shifted
	 */
	size_t ShiftRight( size_t num, bool bRollToFront=true )
	{
	   VectorImage tmp;
	   size_t      i, n;
	   double      dz;

	   // Up to _vals.size()

	   num = gmin( _vals.size(), num );
	   for ( i=0; i<num; i++ ) {
	      tmp.push_back( _vals.back() );
	      _vals.pop_back();
	   }
	   n = tmp.size();
	   for ( i=0; i<n; i++ ) {
	      dz = bRollToFront ? tmp[(n-1)-i] : 0.0;
	      _vals.insert( _vals.begin(), dz );
	   }
	   return num;
	}

	////////////////////////////////////
	// Operations
	////////////////////////////////////
	/**
	 * \brief Subscribe to published Vector
	 *
	 * \param sub - SubChannel to subscribe to
	 * \return Unique Subscription ID
	 */
	int Subscribe( SubChannel &sub )
	{
	   return sub.Subscribe( _str );
	}

	/**
	 * \brief Unsubscribe to published Vector
	 *
	 * \param sub - SubChannel to subscribe to
	 */
	void Unsubscribe( SubChannel &sub )
	{
	   sub.Unsubscribe( _str );
	}

	/**
	 * \brief Publish Image or Update
	 *
	 * For best performance, we keep track internally of whether the entire 
	 * vector has been published or not.  Based on this, your consumer 
	 * Vector will see the following:
	 *
	 * Vector State | Publish | Consumer Callback
	 * --- | --- | ---
	 * Unpublished | VectorImage | OnData( VectorImage & )
	 * Published | VectorUpdate | OnData( VectorUpdate & )
	 * Comletely Updated | VectorImage | OnData( VectorImage & )
	 * Not Updated | None | None
	 *
	 * \param u - Update publishing object
	 * \param StreamID - Unique MDDirect Stream ID
	 * \param bImg - true to force Image
	 * \return Number of data points published
	 */
	size_t Publish( RTEDGE::Update &u, int StreamID, bool bImg=false )
	{
	   VecWireHdr    *h;
	   u_int64_t     *img;
	   VecWireUpdVal *upd;
	   double         r64;
	   char          *bp, *cp;
	   size_t         sz, n;
	   rtBUF          pb;
	   int            ix;

	   // Pre-condition

	   bImg |= _bImg;
	   if (! bImg && !_upds.size() )
	      return 0;
	   /*
	    * 1) Allocate Buffer
	    */
	   n   = bImg ? _vals.size() : _upds.size();
	   sz  = bImg ? sizeof( u_int64_t ) : sizeof( VecWireUpdVal ); 
	   sz *= n;
	   sz += sizeof( VecWireHdr );
	   bp  = new char[sz+10];
	   cp  = bp;
	   h   = (VecWireHdr *)cp;
	   /*
	    * 2) Fill in header
	    */
	   h->_MsgSz     = sz;
	   h->_Precision = _precision;
	   h->_bImg      = bImg;
	   h->_nVal      = (int)n;
	   cp           += sizeof( *h );
	   img           = (u_int64_t *)cp;
	   upd           = (VecWireUpdVal *)cp;
	   /*
	    * 3) Build payload
	    */
	   SetPrecision( _precision );
	   for ( int i=0; i<(int)n; i++ ) {
	      ix   = bImg ? i        : _upds[i]._Index;
	      r64  = bImg ? _vals[i] : _upds[i]._Value;
	      r64 *= _precOut; 
	      if ( bImg ) {
	         img[i] = (u_int64_t)r64;
	      }
	      else {
	         upd[i]._Index = ix;
	         upd[i]._Value = (u_int64_t)r64;
	      }
	   }
	   /*
	    * 4) Publish
	    */
	   pb._data = bp;
	   pb._dLen = (int)sz;
	   _str.SetPublishData( pb );
	   u.Init( _str.Ticker(), StreamID, true );
	   u.Publish( _str, _fidPayload );

	   // Clean up

	   delete[] bp;
	   _bImg = false;
	   _upds.clear();
	   return n;
	}


	////////////////////////////////////
	// Debugging
	////////////////////////////////////
	/**
	 * \brief Dump Vector contents in formatted string
	 *
	 * \param bPage : true for < 80 char per row; false for 1 row
	 * \return Vector contents as formatted string
	 */
	string Dump( bool bPage=true )
	{
	   char  *cp, bp[K], fmt[K];
	   size_t i, n;
	   string s;

	   sprintf( fmt, "%%.%df,", _precision );
	   n   = _vals.size();
	   cp  = bp;
	   cp += sprintf( cp, "[%04ld values] ", n );
	   for ( i=0; i<n; i++ ) {
	      cp += sprintf( cp, fmt, _vals[i] );
	      if ( cp-bp >= 76 ) {
	         cp += sprintf( cp, "\n" );
	         s  += bp;
	         cp  = bp;
	         *cp = '\0';
	      }
	   }
	   if ( cp-bp ) {
	      cp += sprintf( cp, "\n" );
	      s  += bp;
	   }
	   return string( s );
	}

	/**
	 * \brief Dump Vector Update contents in formatted string
	 *
	 * \param bPage : true for < 80 char per row; false for 1 row
	 * \return Vector Update contents as formatted string
	 */
	string Dump( VectorUpdate &upd, bool bPage=true )
	{
	   char  *cp, bp[K], fmt[K];
	   size_t i, n;
	   string s;

	   sprintf( fmt, "%%.%df,", _precision );
	   n   = upd.size();
	   cp  = bp;
	   cp += sprintf( cp, "[%04ld values] ", n );
	   for ( i=0; i<n; i++ ) {
	      cp += sprintf( cp, "%d=", upd[i]._Index );
	      cp += sprintf( cp, fmt, upd[i]._Value );
	      if ( ( cp-bp ) >= 76 ) {
	         cp += sprintf( cp, "\n" );
	         s  += bp;
	         cp  = bp;
	         *cp = '\0';
	      }
	   }
	   if ( cp-bp ) {
	      cp += sprintf( cp, "\n" );
	      s  += bp;
	   }
	   return string( s );
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
	   double         dv;
	   int            i, ix, nv;

	   // Header

	   bp  = buf._data;
	   sz  = buf._dLen;
	   cp  = bp;
	   h   = (VecWireHdr *)cp;
	   nv  = h->_nVal;
assert( h->_MsgSz == sz );
	   cp += sizeof( VecWireHdr );
	   SetPrecision( h->_Precision );

	   // Pull 'em out : Grow as necessary

	   vdb = (u_int64_t *)cp;
	   udb = (VecWireUpdVal *)cp;
	   _vals.clear();
	   _vals.resize( nv );
	   img.resize( nv );
	   for ( i=0; i<nv; i++ ) {
	      ix = i;
	      if ( h->_bImg ) {
	         dv        = _precIn * vdb[i];
	         _vals[ix] = dv;
	         cp       += sizeof( u_int64_t );
	         img[ix]   = dv;
	      } 
	      else {
	         ix        = udb[i]._Index;
	         dv        = _precIn * udb[i]._Value;
	         _vals[ix] = dv;
	         cp       += sizeof( VecWireUpdVal );
	         v._Index  = ix;
	         v._Value  = dv;
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

	////////////////////////////////////
	// Helpers Interface
	////////////////////////////////////
private:
	void SetPrecision( int prec )
	{
	   // Once, if not defined by user

	   if ( !_precision && !_precOut ) {
	      _precision = prec;
	      _precOut   = ::pow10( _precision );
	      _precIn    = 1.0 / _precOut;
	   }
	}
#endif // DOXYGEN_OMIT

	////////////////////////
	// private Members
	////////////////////////
private:
	VectorStream _str;
	VectorUpdate _upds;
	VectorImage  _vals;
	int          _precision;
	double       _precIn;
	double       _precOut;
	bool         _bImg;

}; // class Vector

} // namespace RTEDGE

#endif // __LIBRTEDGE_Vector_H 
