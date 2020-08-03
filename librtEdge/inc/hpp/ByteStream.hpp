/******************************************************************************
*
*  ByteStream.hpp
*     librtEdge Byte Stream
*
*  REVISION HISTORY:
*     15 SEP 2014 jcs  Created.
*      7 JAN 2015 jcs  Build 29: ByteStream.hpp
*      5 MAR 2016 jcs  Build 32: edg_permQuery
*     28 JUL 2020 jcs  Build 44: edg_streamDone
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_ByteStream_H
#define __RTEDGE_ByteStream_H


namespace RTEDGE
{

// Forwards

class SubChannel;
class Update;

////////////////////////////////////////////////
//
//        c l a s s   B y t e S t r e a m
//
////////////////////////////////////////////////

/**
 * \class ByteStream
 * \brief This class encapsulates a single- or multi-message Byte Stream.
 *
 * A ByteStream may be stuffed into a single message or multiple messages.
 * If in one message, then rtEdgeCache3 may cache the entire ByteStream;
 * If multiple messages, then only the last message of the ByteStream will 
 * be cached by rtEdgeCache3.
 *
 * The maximum amount of bytes that may be published in a single message
 * is defined by the Publish() arguments.
 *
 * Each message contains a 4 field header followed by 1 or more fields 
 * containing the data payload.
 *
 * The header consists of the following 4 fields:
 * + Offset - Current offset of complete ByteStream message
 * + Length - Total length of ByteStream message
 * + NumFld - Number of fields in this message
 * + PayloadFID - Field ID of the 1st payload field in this message.
 *
 * The payload is contiguous set of \<NumFld\> fields starting in field
 * \<PayloadFID\>.
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
class ByteStream
{
friend class SubChannel;
friend class Update;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor for both Publish() and Subscribe()
	 *
	 * Each message contains a 4 field header followed by 1 or more fields  
	 * containing the data payload. 
	 * 
	 * The header consists of the following 4 fields: 
	 * + Offset - Current offset of complete ByteStream message
	 * + Length - Total length of ByteStream message
	 * + NumFld - Number of fields in this message
	 * + PayloadFID - Field ID of the 1st payload field in this message.
	 *
	 * The payload is contiguous set of \<NumFld\> fields starting in field
	 * \<PayloadFID\>.
	 *
	 * \param svc - Service supplying this ByteStream if Subscribe()
	 * \param tkr - Published name of this ByteStream
	 * \param fidOff - Field containing current offset in Byte Stream
	 * \param fidLen - Field containing total length of Byte Stream 
	 * \param fidNumFld - Field containing number of payload fields in
	 * the message 
	 * \param fidPayload - Field(s) containing the payload.  The payload
	 * is in contiguous fields from fidPayload thru fidPayload+\<fidNumFld\>
	 */
	ByteStream( const char *svc,
	            const char *tkr,
	            int         fidOff=6,
	            int         fidLen=7,
	            int         fidNumFld=8,
	            int         fidPayload=9 ) :
	   _svc( svc ),
	   _tkr( tkr ),
	   _fidOff( fidOff ),
	   _fidLen( fidLen ),
	   _fidNumFld( fidNumFld ),
	   _fidPayload( fidPayload ),
	   _StreamID( 0 )
	{
	   ::memset( &_pubBuf, 0, sizeof( _pubBuf ) );
	   ::memset( &_subBuf, 0, sizeof( _subBuf ) );
	}

	virtual ~ByteStream()
	{
	   _Clear();
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Service Name of this Byte Stream
	 *
	 * \return Service Name of this Byte Stream
	 */
	const char *svc()
	{
	   return _svc.c_str();
	}

	/**
	 * \brief Returns Ticker Name of this Byte Stream
	 *
	 * \return Ticker Name of this Byte Stream
	 */
	const char *tkr()
	{
	   return _tkr.c_str();
	}

	/**
	 * \brief Returns field ID containing ByteStream offset
	 *
	 * \return Field ID containing ByteStream offset
	 */
	int fidOff()
	{
	   return _fidOff;
	}

	/**
	 * \brief Returns field ID containing total ByteStream length
	 *
	 * \return Field ID containing total ByteStream length
	 */
	int fidLen()
	{
	   return _fidLen;
	}

	/**
	 * \brief Returns field ID containing number of fields in msg
	 *
	 * \return Field ID containing number of fields in msg
	 */
	int fidNumFld()
	{
	   return _fidNumFld;
	}

	/**
	 * \brief Returns field ID of 1st payload field
	 *
	 * \return field ID of 1st payload field
	 */
	int fidPayload()
	{
	   return _fidPayload;
	}

	/**
	 * \brief Returns Subscription Stream ID
	 *
	 * \return Subscription Stream ID
	 */
	int StreamID()
	{
	   return _StreamID;
	}

	/**
	 * \brief Returns a buffer containing the raw data being consumed.
	 *
	 * The data length in the rtBUF is the amount currently received.
	 * The total expected length of the ByteStream is available in 
	 * subBufLen().
	 *
	 * \return Buffer containing the raw data that has been received so far.
	 */
	rtBUF subBuf()
	{
	   rtBUF rtn;

	   rtn._data = _subBuf._data;
	   rtn._dLen = _subBuf._dLen;
	   return rtn;
	}

	/**
	 * \brief Returns total expected length of ByteStream being consumed.
	 *
	 * The current data is available in subBuf().
	 *
	 * \return Total expected length of ByteStream
	 */
	int subBufLen()
	{
	   return _subBuf._nAlloc;
	}

	/**
	 * \brief Returns a buffer containing the raw data to publish
	 *
	 * \return Buffer containing the raw data to publish
	 */
	rtBUF pubBuf()
	{
	   return _pubBuf;
	}


	////////////////////////////////////
	// Operations
	////////////////////////////////////
public:
	/**
	 * \brief Sets the Stream ID for this subscription ByteStream
	 *
	 * \param StreamID - ByteStream ID
	 */
	void SetStreamID( int StreamID )
	{
	   _StreamID = StreamID;
	   _Clear();
	}

	/**
	 * \brief Sets data to publish for this ByteStream
	 *
	 * \param buf - Raw buffer of data to publish
	 */
	void SetPublishData( rtBUF buf )
	{
	   _pubBuf = buf;
	}


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously as we consume chunks of the single-
	 * or multi-message Byte Stream.
	 *
	 * Override this method in your application to take action when
	 * Byte Stream chunks arrive from rtEdgeCache3.  Current data is
	 * available in subBuf().
	 *
	 * \param buf - Buffer pointing to new chunk
	 */
	virtual void OnData( rtBUF buf )
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
	 * \brief Called asynchronously once the complete ByteStream has arrived.
	 *
	 * This will be the last message you receive on the stream.
	 *
	 * Override this method in your application to take action when
	 * the complete Byte Stream has arrived from rtEdgeCache3.  Current
	 * data is available in subBuf().
	 */
	virtual void OnSubscribeComplete()
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


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	bool _OnData( SubChannel &, Message &msg )
	{
	   Field    *fld;
	   rtFldType mTy;
	   rtBUF    *bdb, b;
	   char     *cp;
	   bool      bDone;
	   int       idx, fid, off0, off, len, nFld, fidData;

	   // By Message Type

	   switch( msg.mt() ) {
	      case edg_image:
	      case edg_update:
	         break; // OK
	      case edg_stale:
	      case edg_recovering:
	      case edg_permQuery:
	      case edg_streamDone:
	         return false;
	      case edg_dead:
	      {
	         std::string err( "Stream DEAD : " );

	         err += msg.Error();
	         OnError( err.data() );
	         return true;
	      }
	   }

	   // Header ...

	   off0 = 0;
	   len  = nFld = fidData = 0;
	   for ( msg.reset(); (msg)(); ) {
	      fld = msg.field();
	      fid = fld->Fid();
	      if ( fid == _fidOff )
	         off0 = fld->GetAsInt32();
	      else if ( fid == _fidLen )
	         len = fld->GetAsInt32();
	      else if ( fid == _fidNumFld )
	         nFld = fld->GetAsInt32();
	      else if ( fid == _fidPayload )
	         fidData = fld->GetAsInt32();
	   }
	   if ( !len )
	      OnError( "Invalid header : Missing length" );
	   if ( !nFld )
	      OnError( "Invalid header : Missing NumFld" );
	   if ( !fidData )
	      OnError( "Invalid header : Missing Payload field ID" );
	   if ( !len || !nFld || !fidData )
	      return true;

	   // First time?

	   if ( _subBuf._nAlloc == 0 )
	      _subBuf = ::mddBldBuf_Alloc( len );

	   // ... then payload

	   bdb = new rtBUF[nFld];
	   ::memset( bdb, 0, sizeof( rtBUF ) * nFld );
	   for ( msg.reset(); (msg)(); ) {
	      fld = msg.field();
	      mTy = fld->TypeFromMsg();
	      switch ( mTy ) {
	         case rtFld_undef:
	         case rtFld_string:
	         case rtFld_int:
	         case rtFld_double:
	         case rtFld_date:
	         case rtFld_time:
	         case rtFld_timeSec:
	         case rtFld_float:
	         case rtFld_int8:
	         case rtFld_int16:
	         case rtFld_int64:
	         case rtFld_real:
	            break;
	         case rtFld_bytestream:
	            fid = fld->Fid();
	            idx = fid - fidData;
	            b   = fld->GetAsByteStream().buf();
	            if ( InRange( 0, idx, nFld-1 ) )
	               bdb[idx] = b;
	            break;
	      }
	   }
	   off = off0;
	   for ( idx=0; idx<nFld; idx++ ) {
	      b   = bdb[idx];
	      cp  = _subBuf._data;
	      cp += off;
	      if ( b._data && b._dLen ) {
	         ::memcpy( cp, b._data, b._dLen );
	         off += b._dLen;
	      }
	   }
	   _subBuf._dLen = off;
	   delete[] bdb;
	   b._data  = _subBuf._data;
	   b._data += off0;
	   b._dLen  = ( off-off0 );
	   OnData( b );
	   bDone = ( _subBuf._dLen >= _subBuf._nAlloc );
	   if ( bDone )
	      OnSubscribeComplete();
	   return bDone;
	}

	void _Clear()
	{
	   if ( _subBuf._data ) 
	      delete[] _subBuf._data;
	   ::memset( &_subBuf, 0, sizeof( _subBuf ) );
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	std::string _svc;
	std::string _tkr;
	int         _fidOff;
	int         _fidLen;
	int         _fidNumFld;
	int         _fidPayload;
	int         _StreamID;
	rtBUF       _pubBuf;
	mddBldBuf   _subBuf;

};  // class ByteStream


} // namespace RTEDGE

#endif // __LIBRTEDGE_ByteStream_H 
