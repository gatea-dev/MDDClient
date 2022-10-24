/******************************************************************************
*
*  ByteStream.h
*
*  REVISION HISTORY:
*     13 DEC 2014 jcs  Created.
*     23 OCT 2022 jcs  Build 58: Formatting.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#endif // DOXYGEN_OMIT

#ifndef DOXYGEN_OMIT

namespace librtEdgePRIVATE
{
////////////////////////////////////////////////
//
//    c l a s s   I B y t e S t r e a m
//
////////////////////////////////////////////////
/**
 * \class IByteStream
 * \brief Abstract class to handle subscriber channel events
 * such as OnConnect() and OnData().
 */
public interface class IByteStream
{
	// IByteStream Interface
public:
	virtual void OnData( array<Byte> ^ ) abstract;
	virtual void OnError( String ^ ) abstract;
	virtual void OnSubscribeComplete() abstract;
	virtual void OnPublishComplete( int ) abstract;
};


////////////////////////////////////////////////
//
//      c l a s s   B y t e S t r e a m C
// 
////////////////////////////////////////////////

/**
 * \class ByteStreamC
 * \brief RTEDGE::ByteStream sub-class to hook 4 virtual methods
 * from native librtEdge library and dispatch to .NET consumer. 
 */ 
class ByteStreamC : public RTEDGE::ByteStream
{
private:
	gcroot < IByteStream^ > _cli;
	::rtBUF                 _pubDataC;

	// Constructor
public:
	/**
	 * \brief Constructor for class to hook native events from 
	 * native librtEdge library and pass to .NET consumer via
	 * the IByteStream interface.
	 *
	 * \param cli - Event receiver - IByteStream::OnData(), etc.
	 * \param svc - Service supplying this ByteStream if Subscribe()
	 * \param tkr - Published name of this ByteStream
	 */
	ByteStreamC( IByteStream ^cli, const char *svc, const char *tkr );
	ByteStreamC( IByteStream ^cli, const char *svc, const char *tkr,
	   int, int, int, int );
	~ByteStreamC();

	// Access / Mutator

	::rtBUF pubDataC();
	::rtBUF SetPubDataC( char *, int );
	void    ClearPubDataC();

	// Asynchronous Callbacks
protected:
	virtual void OnData( ::rtBUF );
	virtual void OnError( const char * );
	virtual void OnSubscribeComplete();
	virtual void OnPublishComplete( int );
};

} // namespace librtEdgePRIVATE

#endif // DOXYGEN_OMIT



namespace librtEdge
{

////////////////////////////////////////////////
//
//     c l a s s    B y t e S t r e a m
//
////////////////////////////////////////////////
/**
 * \class ByteStream
 * \brief This class encapsulates a single- or multi-message Byte Stream.
 *
 * A ByteStream may be stuffed into a single message or multiple messages.
 * If in one message, then rtEdgeCache2 may cache the entire ByteStream;
 * If multiple messages, then only the last message of the ByteStream will
 * be cached by rtEdgeCache2.
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
 * + OnError() - Stream error
 * + OnPublishComplete() - Stream completely published
 *
 *
 * \include ByteStream_example.h
 */
public ref class ByteStream : public librtEdgePRIVATE::IByteStream 
{
private: 
	librtEdgePRIVATE::ByteStreamC *_bStr;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor with default field header arguments.
	 *
	 * Default field arguments are:
	 * + fidOff = 6
	 * + fidLen = 7
	 * + fidNumFld = 8
	 * + fidPayload = 9
	 *
	 * \param svc - Service supplying this ByteStream if Subscribe()
	 * \param tkr - Published name of this stream
	 */
	ByteStream( String ^tkr, String ^svc );

	/**
	 * \brief Constructor with user-defined field header arguments.
	 *
	 * \param svc - Service supplying this ByteStream if Subscribe()
	 * \param tkr - Published name of this stream
	 * \param fidOff - Field containing current offset in Byte Stream
	 * \param fidLen - Field containing total length of Byte Stream
	 * \param fidNumFld - Field containing number of payload fields in
	 * the message
	 * \param fidPayload - Field(s) containing the payload.  The payload
	 */
	ByteStream( String ^tkr, 
	            String ^svc, 
	            int     fidOff,
	            int     fidLen,
	            int     fidNumFld,
	            int     fidPayload );

	/** \brief Destructor.  Cleans up internally.  */
	~ByteStream();


	/////////////////////////////////
	// Access
	/////////////////////////////////
public:
	/**
	 * \brief Returns underlying RTEDGE::ByteStream object 
	 *
	 * \return Underlying RTEDGE::ByteStream object 
	 */
	 RTEDGE::ByteStream &bStr();

	/**
	 * \brief Returns Service Name of this Byte Stream
	 *
	 * \return Service Name of this Byte Stream
	 */
	String ^svc();

	/**
	 * \brief Returns Ticker Name of this Byte Stream
	 *
	 * \return Ticker Name of this Byte Stream
	 */
	String ^tkr();

	/**
	 * \brief Returns field ID containing ByteStream offset
	 *
	 * \return Field ID containing ByteStream offset
	 */
	int fidOff();

	/**
	 * \brief Returns field ID containing total ByteStream length
	 *
	 * \return Field ID containing total ByteStream length
	 */
	int fidLen();

	/**
	 * \brief Returns field ID containing number of fields in msg
	 *
	 * \return Field ID containing number of fields in msg
	 */
	int fidNumFld();

	/**
	 * \brief Returns field ID of 1st payload field
	 *
	 * \return field ID of 1st payload field
	 */
	int fidPayload();

	/**
	 * \brief Returns Subscription Stream ID
	 *
	 * \return Subscription Stream ID
	 */
	int StreamID();

	/**
	 * \brief Returns a byte[] array containing the raw data being consumed.
	 *
	 * The data length in the byte[] array is the amount currently received.
	 * The total expected length of the ByteStream is available in
	 * subBufLen().
	 *
	 * \return Buffer containing the raw data that has been received so far.
	 */
	array<Byte> ^subBuf();

	/**
	 * \brief Returns total expected length of ByteStream being consumed.
	 *
	 * The current data is available in subBuf().
	 *
	 * \return Total expected length of ByteStream
	 */
	int subBufLen();

	/**
	 * \brief Returns a byte[] array containing the raw data to publish
	 *
	 * \return byte[] array containing the raw data to publish
	 */
	array<Byte> ^pubBuf();


	/////////////////////////////////
	// Mutator
	/////////////////////////////////
public:
	/**
	 * \brief Sets data to publish for this ByteStream
	 *
	 * \param buf - Raw buffer of data to publish
	 */
	void SetPublishData( array<Byte> ^buf );


	/////////////////////////////////
	// IByteStream interface
	/////////////////////////////////
public:
	/**
	 * \brief Called asynchronously as we consume chunks of the single-
	 * or multi-message Byte Stream.
	 *
	 * This will be the last message you receive on this ByteStream.
	 *
	 * Override this method in your application to take action when
	 * Byte Stream chunks arrive from rtEdgeCache2.  Current data is
	 * available in subBuf().
	 *
	 * \param buf - Buffer pointing to new chunk
	 */
	virtual void OnData( array<Byte> ^buf )
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
	virtual void OnError( String ^err )
	{ ; }

	/**
	 * \brief Called asynchronously once the complete ByteStream has arrived.
	 *
	 * Override this method in your application to take action when
	 * the complete Byte Stream has arrived from rtEdgeCache2.  Current
	 * data is available in subBuf().
	 */
	virtual void OnSubscribeComplete()
	{ ; }

	/**
	 * \brief Called asynchronously once the complete ByteStream has been
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

};  // class rtEdgeSubscriber

} // namespace librtEdge
