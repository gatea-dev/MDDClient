/******************************************************************************
*
*  Vector.h
*
*  REVISION HISTORY:
*     21 OCT 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#include <PubChannel.h>
#include <SubChannel.h>
#endif // DOXYGEN_OMIT

namespace librtEdge
{

////////////////////////////////////////////////
//
//       c l a s s   V e c t o r V a l u e
//
////////////////////////////////////////////////

/**
 * \class VectorValue
 * \brief Single Vector Value including index
 */
public ref class VectorValue
{
private:
	int    _pos;
	double _val;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
	/**  
	 * \brief Constructor
	 *    
	 * \param pos : Position in Vector
	 * \param value : Value
	 */   
public:
	VectorValue( int pos, double value ) :
	   _pos( pos ),
	   _val( value )
	{ ; }

	~VectorValue() { ; }

	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns the position */
	property int _position
	{
	   int get() { return _pos; }
	}

	/** \brief Returns the value */
	property double _value
	{
	   double get() { return _val; }
	}

}; // class VectorValue

} // namespace librtEdge


#ifndef DOXYGEN_OMIT

namespace librtEdgePRIVATE
{
////////////////////////////////////////////////
//
//    c l a s s   I V e c t o r
//
////////////////////////////////////////////////
/**
 * \class IVector
 * \brief Abstract class to handle Vector notification events
 */
public interface class IVector
{
	// IVector Interface
public:
	virtual void OnData( cli::array<double> ^ ) abstract;
	virtual void OnData( cli::array<librtEdge::VectorValue ^> ^ ) abstract;
	virtual void OnError( String ^ ) abstract;
	virtual void OnPublishComplete( int ) abstract;

}; // class IVector


////////////////////////////////////////////////
//
//       c l a s s   V e c t o r C
// 
////////////////////////////////////////////////

/**
 * \class VectorC
 * \brief RTEDGE::Vector sub-class to hook 4 virtual methods
 * from native librtEdge library and dispatch to .NET consumer. 
 */ 
class VectorC : public RTEDGE::Vector
{
private:
	gcroot < librtEdgePRIVATE::IVector^ > _cli;

	// Constructor
public:
	VectorC( librtEdgePRIVATE::IVector ^cli, const char *svc, const char *tkr, int );

	// Asynchronous Callbacks
protected:
	virtual void OnData( RTEDGE::VectorImage & );
	virtual void OnData( RTEDGE::VectorUpdate & );
	virtual void OnError( const char * );
	virtual void OnPublishComplete( int );

}; // class VectorC

} // namespace librtEdgePRIVATE

#endif // DOXYGEN_OMIT


namespace librtEdge
{

////////////////////////////////////////////////
//
//        c l a s s    V e c t o r
//
////////////////////////////////////////////////
/**
 * \class Vector
 * \brief A one-dimensional Vector of doubles : Pub or Sub
 *
 * When consuming you receive asynchronous notifications as follows:
 * + OnData( cli::array<double> ^ ) - Complete Vector Update
 * + OnData( cli::array<VectorValue ^> ^ ) - Partial Vector Update
 * + OnError() - Error
 *
 * When publishing you receive asynchronous notifications as follows:
 * + OnPublishComplete() - Stream completely published
 */
public ref class Vector : public librtEdge::rtEdge,
                          public librtEdgePRIVATE::IVector
{
private: 
	librtEdgePRIVATE::VectorC *_vec;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor for both Publish() and Subscribe()
	 *
	 * \param svc - Service supplying this Vector if Subscribe()
	 * \param tkr - Published name of this Vector
	 * \param precision - Sig Fig; 0 to 'learn' from Subscription Stream
	 */
	Vector( String ^svc, String ^tkr, int precision );

	/** \brief Destructor.  Cleans up internally.  */
	~Vector();


	/////////////////////////////////
	// Access
	/////////////////////////////////
public:
	/**
	 * \brief Returns Service Name of this Vector
	 *
	 * \return Service Name of this Vector
	 */
	String ^Service();

	/**
	 * \brief Returns Ticker Name of this Vector
	 *
	 * \return Ticker Name of this Vector
	 */
	String ^Ticker();

	/**
	 * \brief Return Vector Values
	 *
	 * \return array of all values
	 */
	cli::array<double> ^Get();


	/////////////////////////////////
	// Mutator
	/////////////////////////////////
public:
	/**
	 * \brief Set Entire Vector of Values
	 *
	 * \param img - vector of values to load
	 * \return Number loaded
	 */
	int Update( cli::array<double> ^img );

	/**
	 * \brief Set Single Value
	 *
	 * \param idx - Value Index
	 * \param val - Value
	 * \return 1 if set; 0 if outside boundary
	 */
	int UpdateAt( int idx, double val );

	/**
	 * \brief Shift values left (down), optionally rolling to end
	 *
	 * \param num - Number to shift
	 * \param bRollToEnd - true to roll values to end; false for 0.0
	 * \return Number shifted
	 */
	int ShiftLeft( int num, bool bRollToEnd );

	/**
	 * \brief Shift values right (up), optionally rolling to beginning
	 *
	 * \param num - Number to shift
	 * \param bRollToFront - true to roll values to front; false for 0.0
	 * \return Number shifted
	 */
	int ShiftRight( int num, bool bRollToFront );

	////////////////////////////////////
	// Operations
	////////////////////////////////////
	/**
	 * \brief Subscribe to published Vector
	 *
	 * \param sub - librtEdge::rtEdgeSubscriber feeding us
	 * \return Unique Subscription ID
	 */
	int Subscribe( librtEdge::rtEdgeSubscriber ^sub );

	/**
	 * \brief Unsubscribe to published Vector
	 *
	 * \param sub - librtEdge::rtEdgeSubscriber feeding us
	 */
	void Unsubscribe( librtEdge::rtEdgeSubscriber ^sub );

	/**
	 * \brief Publish Image or Update
	 *
	 * For best performance, we keep track internally of whether the entire
	 * vector has been published or not.  Based on this, your consumer
	 * Vector will see the following:
	 *
	 * Vector State | Publish | Consumer Callback
	 * --- | --- | ---
	 * Unpublished | cli::array<double> | OnData( cli::array<double> ^ )
	 * Published | cli::array<VectorValue ^> | OnData( cli::array<VectorValue ^> ^ )
	 * Completely Updated | cli::array<double> | OnData( cli::array<double> & )
	 * Not Updated | None | None
	 *
	 * \param pub - Publisher channel
	 * \param StreamID - Unique MDDirect Stream ID
	 * \param bImg - true to force Image
	 * \return Number of data points published
	 */
	int Publish( librtEdge::rtEdgePublisher ^pub, int StreamID, bool bImg );

	////////////////////////////////////
	// Debugging
	////////////////////////////////////
	/**
	 * \brief Dump Vector contents in formatted string
	 *
	 * \param bPage : true for < 80 char per row; false for 1 row
	 * \return Vector contents as formatted string
	 */
	String ^Dump( bool bPage );

	/**
	 * \brief Dump Vector Update contents in formatted string
	 *
	 * \param upd : Collection to dump
	 * \param bPage : true for < 80 char per row; false for 1 row
	 * \return Vector Update contents as formatted string
	 */
	String ^Dump( cli::array<VectorValue ^> ^upd, bool bPage );

	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns name of service supplying this update */
	property double _value[int]
	{
	   void   set( int idx, double val ) { _vec->UpdateAt( idx, val ); }
	   double get( int idx ) {
	      RTEDGE::VectorImage img;
	      size_t              sz;

	      sz = _vec->Get( img ).size();
	      if ( InRange( 0, idx, sz-1 ) )
	         return img[idx];
	      return 0.0;
	   }
	}
	 
	/////////////////////////////////
	// librtEdgePRIVATE::IVector interface
	/////////////////////////////////
public:
	/**  
	 * \brief Called asynchronously when Initial Image for the Vector arrives
	 *    
	 * Override this method in your application to take action when the
	 * Initial Image is available.
	 *    
	 * \param img - Initial Image
	 */   
	virtual void OnData( cli::array<double> &img )
	{ ; }

	/**  
	 * \brief Called asynchronously when Vector update
	 *    
	 * Override this method in your application to take action when the
	 * Vector Updates
	 *    
	 * \param upd - Values that have updated
	 */   
	virtual void OnData( cli::array<VectorValue ^> ^upd )
	{ ; }

	/**  
	 * \brief Called asynchronously if there is an error in consuming
	 * the single- or multi-message Byte Stream.
	 *    
	 * This will be the last message you receive on this Vector.
	 *    
	 * Override this method in your application to take action when
	 * Byte Stream arrives in error.  If needed, the current data is
	 * available in subBuf().
	 *    
	 * \param err - Textual description of error
	 */   
	virtual void OnError( String ^err )
	{ ; }

	/**
	 * \brief Called asynchronously once the complete Vector has been
	 * published.
	 *
	 * Override this method in your application to take action when
	 * the complete Byte Stream has been published into rtEdgeCache2.
	 * Published data data is available in pubBuf().
	 *
	 * \param nByte - Number of bytes published
	 */
	virtual void OnPublishComplete( int nByte )
	{ ; }

};  // class Vector

} // namespace librtEdge
