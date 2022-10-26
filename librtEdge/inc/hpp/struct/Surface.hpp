/******************************************************************************
*
*  Surface.hpp
*     librtEdge Surface
*
*  REVISION HISTORY:
*     21 OCT 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Surface_H
#define __RTEDGE_Surface_H
#include <assert.h>
#include <hpp/struct/Vector.hpp>

using namespace std;

namespace RTEDGE
{

///////////////////////////////////////////////
//
//     c l a s s   S u r f a c e V a l u e
//
////////////////////////////////////////////////

/**
 * \class SurfaceValue
 * \brief Single Surface Value including ( row, col ) position
 */
class SurfaceValue
{
public:
	/** \brief Surface Row */
	int    _row;
	/** \brief Surface Col */
	int    _col;
	/** \brief Value */
	double _value;

}; // class SurfaceValue


/** \brief One row of a surface */
typedef vector<double>           SurfaceRow;
/** \brief Surface = List or SurfaceRow's */
typedef vector< vector<double> > SurfaceRows;
/** \brief Surface Update */
typedef vector<SurfaceValue>     SurfaceUpdate;

#ifndef DOXYGEN_OMIT

class ISurface
{
public:
	virtual ~ISurface() { ; }
	virtual void OnSurface( SurfaceRows & ) = 0;
	virtual void OnSurface( SurfaceUpdate & ) = 0;
	virtual void OnError( const char *err ) = 0;
	virtual void OnPublishComplete( int nByte ) = 0;

}; // class ISurface
#endif // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//        c l a s s   S u r f a c e
//
////////////////////////////////////////////////

/**
 * \class Surface
 * \brief A 2-D Surface of doubles : Pub or Sub
 *
 * When consuming you receive asynchronous notifications as follows:
 * + OnSurface( SurfaceRows & ) - Complete Surface Update
 * + OnSurface( SurfaceUpdate & ) - Partial Surface Update
 * + OnError() - Error
 *
 * When publishing you receive asynchronous notifications as follows:
 * + OnPublishData() - When a "chunk" of the stream has been published
 * + OnError() - Stream error
 * + OnPublishComplete() - Stream completely published
 */
class Surface : public ISurface
{
#ifndef DOXYGEN_OMIT
	////////////////////////////////////////////////
	//
	//      c l a s s   _ S u r f V e c t o r
	//
	////////////////////////////////////////////////
	/**
	 * \class _SurfVector
	 * \brief 'Has-a' 1-D Vector which we cut up into 
	 */
public:
	class _SurfVector : public Vector
	{
	   ////////////////////////////////////
	   // Constructor / Destructor
	   ////////////////////////////////////
	public:
	   _SurfVector( ISurface   &iSurf,
	                const char *svc,
	                const char *tkr,
	                int         Width,
	                int         precision ) :
	      Vector( svc, tkr, precision ),
	      _iSurf( iSurf ),
	      _Width( Width ),
	      _surface()
	   { ; }

	   ////////////////////////////////////
	   // Access
	   ////////////////////////////////////
	public:
	   SurfaceRows &surface()
	   {
	      return _surface;
	   }

	   int Width()
	   {
	      return _Width;
	   }

	   ////////////////////////////////////
	   // Vector Interface
	   ////////////////////////////////////
	public:
	   virtual void OnData( VectorImage &img )
	   {
	      SurfaceRows &sdb = _surface;
	      SurfaceRow   row;

	      // Chop up img into _Width rows

	      sdb.clear();
	      for ( size_t i=0; i<img.size(); i++ ) {
	         if ( i && !(i%_Width) ) {
	            sdb.push_back( SurfaceRow( row ) );
	            row.clear();
	         }
	         row.push_back( img[i] );
	      }
	      if ( row.size() )
	         sdb.push_back( SurfaceRow( row ) );
	      _iSurf.OnSurface( sdb );
	   }

	   virtual void OnData( VectorUpdate &upd )
	   {
	      SurfaceUpdate sdb;
	      SurfaceValue  vs;
	      VectorValue   vv;

	      for ( size_t i=0; i<upd.size(); i++ ) {
	         vv        = upd[i];
	         vs._row   = vv._position / _Width;
	         vs._col   = vv._position % _Width;
	         vs._value = vv._value;
	         sdb.push_back( vs );
	      }
	      _iSurf.OnSurface( sdb );
	   }

	   virtual void OnError( const char *err )
	   {
	      _iSurf.OnError( err );
	   }

	   virtual void OnPublishData( int nByte, int totByte )
	   { ; }

	   virtual void OnPublishComplete( int nByte )
	   { ; }

	   ////////////////////////
	   // private Members
	   ////////////////////////
	private:
	   ISurface   &_iSurf;
	   int         _Width;
	   SurfaceRows _surface;

	}; // class _SurfVector

#endif // DOXYGEN_OMIT

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor for both Publish() and Subscribe()
	 *
	 * \param svc - Service supplying this Surface if Subscribe()
	 * \param tkr - Published name of this Surface
	 * \param Width - Surface Width (columns)
	 * \param precision - Sig Fig; 0 to 'learn' from Subscription Stream
	 */
	Surface( const char *svc, 
	         const char *tkr, 
	         int         Width,
	         int         precision=0 ) :
	   _vec( *this, svc, tkr, Width, precision ),
	   _bImg( true )
	{ ; }

	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Service Name of this Surface
	 *
	 * \return Service Name of this Surface
	 */
	const char *Service()
	{
	   return _vec.Service();
	}

	/**
	 * \brief Returns Ticker Name of this Surface
	 *
	 * \return Ticker Name of this Surface
	 */
	const char *Ticker()
	{
	   return _vec.Ticker();
	}

	/**
	 * \brief Return Surface Values
	 *
	 * \param dst - Resultant Surface to populate
	 * \return SurfaceRows of all values
	 */
	SurfaceRows &Get( SurfaceRows &dst )
	{
	   dst.clear();
	   dst = _vec.surface();
	   return dst;
	}


	////////////////////////////////////
	// Mutator
	////////////////////////////////////
#ifdef TODO
	/**
	 * \brief Set Entire Surface of Values
	 *
	 * \param img - vector of values to load
	 * \return Number loaded
	 */
	size_t UpdateRow( VectorImage &img )
	{
	   size_t i, n;

	   _vals.clear();
	   _upds.clear();
	   n = img.size();
	   for ( i=0; i<n; _vals.push_back( img[i++] ) );
	   _bImg = true;
	   return n;
	}
#endif // TODO

	/**
	 * \brief Set Entire Surface of Values
	 *
	 * \param src - Surface of values to load
	 * \return Number loaded
	 */
	size_t Update( SurfaceRows &src )
	{
	   SurfaceRows &dst = _vec.surface();

	   dst   = src;
	   _bImg = true;
	   return dst.size();
	}

	/**
	 * \brief Set Single Value
	 *
	 * \param row - Row Index
	 * \param col - Col Index
	 * \param val - Value
	 * \return 1 if set; 0 if outside boundary
	 */
	int UpdateAt( size_t row, size_t col, double val )
	{
	   size_t idx = ( row * _vec.Width() ) + col;

	   return _vec.UpdateAt( idx, val );
	}

// TODO : Shift rows Up / down

	////////////////////////////////////
	// Operations
	////////////////////////////////////
	/**
	 * \brief Subscribe to published Surface
	 *
	 * \param sub - SubChannel feeding us
	 * \return Unique Subscription ID
	 */
	int Subscribe( SubChannel &sub )
	{
	   return _vec.Subscribe( sub );
	}

	/**
	 * \brief Unsubscribe to published Surface
	 *
	 * \param sub - SubChannel feeding us
	 */
	void Unsubscribe( SubChannel &sub )
	{
	   _vec.Unsubscribe( sub );
	}

	/**
	 * \brief Publish Image or Update
	 *
	 * For best performance, we keep track internally of whether the entire 
	 * vector has been published or not.  Based on this, your consumer 
	 * Surface will see the following:
	 *
	 * Surface State | Publish | Consumer Callback
	 * --- | --- | ---
	 * Unpublished | SurfaceRows | OnSurface( SurfaceRows & )
	 * Published | SurfaceUpdate | OnSurface( SurfaceUpdate & )
	 * Comletely Updated | SurfaceRows | OnSurface( SurfaceRows & )
	 * Not Updated | None | None
	 *
	 * \param u - Update publishing object
	 * \param StreamID - Unique MDDirect Stream ID
	 * \param bImg - true to force Image
	 * \return Number of data points published
	 */
	size_t Publish( RTEDGE::Update &u, int StreamID, bool bImg=false )
	{
	   return _vec.Publish( u, StreamID, bImg );
	}


	////////////////////////////////////
	// Debugging
	////////////////////////////////////
	/**
	 * \brief Dump Surface contents in formatted string
	 *
	 * \param bPage : true for < 80 char per row; false for 1 row
	 * \return Surface contents as formatted string
	 */
	string Dump( bool bPage=true )
	{
	   return _vec.Dump( bPage );
	}

	/**
	 * \brief Dump Surface Update contents in formatted string
	 *
	 * \param upd : Collection to dump  
	 * \param bPage : true for < 80 char per row; false for 1 row
	 * \return Surface Update contents as formatted string
	 */
	string Dump( SurfaceUpdate &upd, bool bPage=true )
	{
	   VectorUpdate vdb;
	   SurfaceValue vs;
	   VectorValue  vv;

	   for ( size_t i=0; i<upd.size(); i++ ) {
	      vs           = upd[i];
	      vv._position = ( vs._row * _vec.Width() ) + vs._col; 
	      vv._value    = vs._value; 
	      vdb.push_back( vv );
	   }
	   return _vec.Dump( vdb, bPage );
	}

	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
public:
	/**
	 * \brief Called asynchronously when the Entire Surfaces Updaes
	 *
	 * Override this method in your application to take action when the 
	 * entire Surface is available.
	 *
	 * \param img - Initial Image
	 */
	virtual void OnSurface( SurfaceRows &img )
	{ ; }

	/**
	 * \brief Called asynchronously when Surface update
	 *
	 * Override this method in your application to take action when the 
	 * Surface Updates
	 *
	 * \param upd - Values that have updated
	 */
	virtual void OnSurface( SurfaceUpdate &upd )
	{ ; }

	/**
	 * \brief Called asynchronously if there is an error in consuming
	 * the single- or multi-message Byte Stream.
	 *
	 * This will be the last message you receive on this Surface.
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
	 * \brief Called asynchronously once the complete Surface has been
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

	////////////////////////
	// private Members
	////////////////////////
private:
	_SurfVector _vec;
	bool        _bImg;

}; // class Surface

} // namespace RTEDGE

#endif // __LIBRTEDGE_Surface_H 