/******************************************************************************
*
*  Update.h
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*     23 JAN 2015 jcs  Build 29: ByteStreamFld; PubChainLink() 
*      7 JUL 2015 jcs  Build 31: Publish( array<Byte> ^ )
*     29 APR 2020 jcs  Build 43: Moved IrtEdgePubUpdate in here
*     30 MAR 2022 jcs  Build 52: doxygen; AddFieldAsDate() / Time()
*     23 MAY 2022 jcs  Build 54: AddFieldAsUnixTime()
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <PubChannel.h>
#include <Field.h>
#include <Data.h>
#endif // DOXYGEN_OMIT

#ifndef DOXYGEN_OMIT

using namespace librtEdgePRIVATE;

namespace librtEdge
{
////////////////////////////////////////////////
//
// c l a s s   I r t E d g e P u b U p d a t e
//
////////////////////////////////////////////////
/**
 * \class IrtEdgePubUpdate
 * \brief Abstract class to handle subscriber channel events
 * such as OnConnect() and OnPubOpen().
 */
public interface class IrtEdgePubUpdate
{
	// IrtEdgePublisher Interface - Publish
public:
	virtual int Publish() abstract;
	virtual int Publish( librtEdge::ByteStream ^, int ) abstract;
	virtual int Publish( librtEdge::ByteStream ^, int, int, int, int ) abstract;
	virtual int PubError( String ^ ) abstract;

	// IrtEdgePublisher Interface - Field
public:
	virtual void AddFieldAsString( int, String ^ ) abstract;
	virtual void AddFieldAsInt32( int, int ) abstract;
	virtual void AddFieldAsInt64( int, long long ) abstract;
	virtual void AddFieldAsDouble( int, double ) abstract;
	virtual void AddFieldAsByteStream( int, librtEdge::ByteStreamFld ^ ) abstract;
	virtual void AddFieldAsDateTime( int, DateTime ^ ) abstract;
	virtual void AddFieldAsDate( int, DateTime ^ ) abstract;
	virtual void AddFieldAsTime( int, DateTime ^ ) abstract;
	virtual void AddFieldAsString( String ^, String ^ ) abstract;
	virtual void AddFieldAsInt32( String ^, int ) abstract;
	virtual void AddFieldAsInt64( String ^, long long ) abstract;
	virtual void AddFieldAsDouble( String ^, double ) abstract;
	virtual void AddFieldAsByteStream( String ^, librtEdge::ByteStreamFld ^ ) abstract;
	virtual void AddFieldAsDateTime( String ^, DateTime ^ ) abstract;
	virtual void AddFieldAsUnixTime( String ^, DateTime ^ ) abstract;
	virtual void AddFieldAsDate( String ^, DateTime ^ ) abstract;
	virtual void AddFieldAsTime( String ^, DateTime ^ ) abstract;

	// IrtEdgePublisher Interface - Field
public:
	virtual int PubChainLink( String ^, IntPtr, int, bool, array<String ^> ^ ) abstract;
	virtual int PubChainLink( String ^, IntPtr, int, bool, array<String ^> ^, int ) abstract;

};  // class IrtEdgePubUpdate

#endif // DOXYGEN_OMIT

////////////////////////////////////////////////
//
//   c l a s s   r t E d g e P u b U p d a t e
//
////////////////////////////////////////////////

/**
 * \class rtEdgePubUpdate
 * \brief A single update to be published.  This class simplifies the 
 * publication of market data.  You may:
 * + Add Fields 
 * + Copy in fields from rtEdgeData
 * + Copy in subscriber data via rtEdgeData (i.e. "pipe"-like application)
 * + Publish your results
 *
 * The internal state of this object is reset after each call to Publish().
 * For best performance, it is highly recommended that you reuse this
 * object in your application by calling Init() each update.
 */
public ref class rtEdgePubUpdate : public IrtEdgePubUpdate
{
private:
	IrtEdgePublisher ^_pub;
	RTEDGE::Update   &_upd;
	String           ^_err;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief A single update to be published on the rtEdgePublisher 
	 * channel.
	 *
	 * It is highly recommended that you reuse this object in your 
	 * application by calling Init() each update.
	 *
	 * \param pub - The rtEdgePublisher channel this is published on
	 * \param tkr - Ticker name
	 * \param arg - Unique Stream ID from rtEdgePublisher::OnOpen()
	 * \param bImg - true if initial image; false if update
	 */
	rtEdgePubUpdate( IrtEdgePublisher ^pub,
	                 String           ^tkr,
	                 IntPtr            arg,
	                 bool              bImg );

	/**
	 * \brief A single (DEAD) error update to be published on the 
	 * rtEdgePublisher channel.
	 *
	 * This constructor is for backwards compatibility with 
	 * librtEdgeDotNetXX.dll and is equivalent to calling PubError().
	 *
	 * \param pub - The rtEdgePublisher channel this is published on
	 * \param tkr - Ticker name
	 * \param arg - Unique Stream ID from rtEdgePublisher::OnOpen()
	 * \param err - true if initial image; false if update
	 */
	rtEdgePubUpdate( IrtEdgePublisher ^pub,
	                 String           ^tkr,
	                 IntPtr            arg,
	                 String           ^err );

	/**
	 * \brief A single update to be published on the rtEdgePublisher 
	 * channel.
	 *
	 * This constructor populates the update from fields (i.e., deep copy)
	 * from the rtEdgeData object (e.g., Pipe applications).  You may add
	 * further fields to this object via the Add() method.
	 *
	 * \param pub - The rtEdgePublisher channel this is published on
	 * \param d - Populated rtEdgeData object to deep copy
	 */
	rtEdgePubUpdate( IrtEdgePublisher ^pub,
	                 rtEdgeData       ^d );

	~rtEdgePubUpdate();


	/////////////////////////////////
	// Operations - Reusability
	/////////////////////////////////
	/**
	 * \brief Initialize this update for publication.
	 *
	 * It is highly recommended that you reuse this object in your 
	 * application by calling this method.
	 *
	 * \param tkr - Ticker name
	 * \param arg - Unique Stream ID from rtEdgePublisher::OnOpen()
	 * \param bImg - true if initial image; false if update
	 */
	void Init( String ^tkr, IntPtr arg, bool bImg );


	/////////////////////////////////
	// IrtEdgePublisher Interface - Publish
	/////////////////////////////////
public:
	/**
	 * \brief Publish a single field list update with fields added by 
	 * AddField() and friends
	 *
	 * \return  Number of bytes published
	 */
	virtual int Publish();

	/**
	 * \brief Publish pre-formatted data payload
	 *
	 * \param buf - Pre-formatted data payload (field list)
	 * \param bFieldList - true if field list; Else fixed msg
	 * \return  Number of bytes published
	 */
	virtual int Publish( array<Byte> ^buf, bool bFieldList );

	/**
	 * \brief Publish an error
	 *
	 * \param err - Error string
	 * \return  Number of bytes published
	 */
	virtual int PubError( String ^err );


	/////////////////////////////////
	// IrtEdgePublisher Interface - ByteStream
	/////////////////////////////////
	/**
	 * \brief Publish a ByteStream with default size and rate.
	 * 
	 * This takes default arguments for message size and rate:
	 * + maxFldSiz = 1024
	 * + maxNumFld = 1024 (thus max 1 MB message size)
	 * + bytesPerSec = 1 MB
	 * 
	 * \param bStr - ByteStream to publish
	 * \param fidData - Field ID where payload data will be put in message.
	 * \return  Number of bytes published
	 */
	virtual int Publish( ByteStream ^bStr, 
	                     int         fidData );

	/**
	 * \brief Publish a ByteStream with user-defined size and rate.
	 * 
	 * \param bStr - ByteStream to publish
	 * \param fidData - Field ID where payload data will be put in message.
	 * \param maxFldSiz - Max bytes in each field
	 * \param maxFld - Max fields per message
	 * \param bytesPerSec - Max bytes / sec to publish if multi-message stream
	 * \return  Number of bytes published
	 */
	virtual int Publish( ByteStream ^bStr, 
	                     int         fidData,
	                     int         maxFldSiz,
	                     int         maxFld,
	                     int         bytesPerSec );

	/**
	 * \brief Terminate ByteStream Publishing
	 *
	 * This method allows a different thread to terminate the publication
	 * of a ByteStream.  This may be useful when publishing a large
	 * ByteStream.
	 *
	 * \param bStr - ByteStream to publish
	 */
	void Stop( ByteStream ^bStr );


	/////////////////////////////////
	// IrtEdgePubUpdate Interface - Fields
	/////////////////////////////////
	/**
	 * \brief Add rtEdgeField to update
	 *
	 * \param fld - rtEdgeField to add
	 */
	virtual void AddField( rtEdgeField ^fld );

	/**
	 * \brief Add string field to update
	 *
	 * The caller is responsible for ensuring the integrity of the
	 * data in str until Publish() is called.
	 *
	 * \param fid - Field ID
	 * \param str - Field value as string
	 */
	virtual void AddFieldAsString( int fid, String ^str );

	/**
	 * \brief Add 8-bit int field to update
	 *
	 * \param fid - Field ID
	 * \param i8 - Field value as 8-bit char
	 */
	virtual void AddFieldAsInt8( int fid, u_char i8 );

	/**
	 * \brief Add 16-bit int field to update
	 *
	 * \param fid - Field ID
	 * \param i16 - Field value as 16-bit short int
	 */
	virtual void AddFieldAsInt16( int fid, u_short i16 );

	/**
	 * \brief Add 32-bit int field to update
	 *
	 * \param fid - Field ID
	 * \param i32 - Field value as 32-bit int
	 */
	virtual void AddFieldAsInt32( int fid, int i32 );

	/**
	 * \brief Add 64-bit long field to update
	 *
	 * \param fid - Field ID
	 * \param i64 - Field value as 64-bit int
	 */
	virtual void AddFieldAsInt64( int fid, long long i64 );

	/**
	 * \brief Add float field to update
	 *
	 * \param fid - Field ID
	 * \param r32 - Field value as double
	 */
	virtual void AddFieldAsFloat( int fid, float r32 );

	/**
	 * \brief Add float field to update
	 *
	 * \param fid - Field ID
	 * \param r64 - Field value as double
	 */
	virtual void AddFieldAsDouble( int fid, double r64 );

	/**
	 * \brief Add ByteStreamFld field to update
	 *
	 * \param fid - Field ID
	 * \param bStr - Field Value as ByteStreamFld
	 */
	virtual void AddFieldAsByteStream( int fid, ByteStreamFld ^bStr );

	/**
	 * \brief Add date-time field to update as DateTime
	 *
	 * \param fid - Field ID
	 * \param dt - Field value as DateTime
	 */
	virtual void AddFieldAsDateTime( int fid, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as Date
	 *
	 * \param fid - Field ID
	 * \param dt - Field value as DateTime
	 */
	virtual void AddFieldAsDate( int fid, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as Time
	 *
	 * \param fid - Field ID
	 * \param dt - Field value as DateTime
	 */
	virtual void AddFieldAsTime( int fid, DateTime ^dt );

	/**
	 * \brief Add string field to update
	 *
	 * The caller is responsible for ensuring the integrity of the
	 * data in str until Publish() is called.
	 *
	 * \param pFld - Field Name
	 * \param str - Field value as string
	 */
	virtual void AddFieldAsString( String ^pFld, String ^str );

	/**
	 * \brief Add 8-bit int field to update
	 *
	 * \param pFld - Field Name
	 * \param i8 - Field value as 8-bit char
	 */
	virtual void AddFieldAsInt8( String ^pFld, u_char i8 );

	/**
	 * \brief Add 16-bit int field to update
	 *
	 * \param pFld - Field Name
	 * \param i16 - Field value as 16-bit short int
	 */
	virtual void AddFieldAsInt16( String ^pFld, u_short i16 );

	/**
	 * \brief Add 32-bit int field to update
	 *
	 * \param pFld - Field Name
	 * \param i32 - Field value as 32-bit int
	 */
	virtual void AddFieldAsInt32( String ^pFld, int i32 );

	/**
	 * \brief Add 64-bit long field to update
	 *
	 * \param pFld - Field Name
	 * \param i64 - Field value as 64-bit int
	 */
	virtual void AddFieldAsInt64( String ^pFld, long long i64 );

	/**
	 * \brief Add float field to update
	 *
	 * \param pFld - Field Name
	 * \param r32 - Field value as double
	 */
	virtual void AddFieldAsFloat( String ^pFld, float r32 );

	/**
	 * \brief Add float field to update
	 *
	 * \param pFld - Field Name
	 * \param r64 - Field value as double
	 */
	virtual void AddFieldAsDouble( String ^pFld, double r64 );

	/**
	 * \brief Add ByteStreamFld field to update
	 *
	 * \param pFld - Field Name
	 * \param bStr - Field Value as ByteStreamFld
	 */
	virtual void AddFieldAsByteStream( String ^pFld, ByteStreamFld ^bStr );

	/**
	 * \brief Add date-time field to update as MDD DateTime
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTime
	 */
	virtual void AddFieldAsDateTime( String ^pFld, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as Unix Time on the wire
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTim
	 */
	virtual void AddFieldAsUnixTime( String ^pFld, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as MDD Date
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTime
	 */
	virtual void AddFieldAsDate( String ^pFld, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as MDD Time
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTime
	 */
	virtual void AddFieldAsTime( String ^pFld, DateTime ^dt );


	/////////////////////////////////
	// IrtEdgePubUpdate Interface - Chain
	/////////////////////////////////
	/**
	 * \brief Builds and publishes chain link.
	 *
	 * This method adds the chain prefix - 0#, 1#, etc. - to the name.
	 *
	 * \param chainName - Un-prefixed Chain Name
	 * \param arg - User-defined argument
	 * \param linkNum - Link Number
	 * \param bFinal - true if this is final link in chain
	 * \param links - Link names
	 * \param dpyTpl - Display template number
	 */
	virtual int PubChainLink( String          ^chainName,
	                          IntPtr           arg,
	                          int              linkNum,
	                          bool             bFinal,
	                          array<String ^> ^links,
	                          int              dpyTpl );

	/**
	 * \brief Builds and publishes chain link.
	 *
	 * This method adds the chain prefix - 0#, 1#, etc. - to the name.
	 *
	 * \param chainName - Un-prefixed Chain Name
	 * \param arg - User-defined argument
	 * \param linkNum - Link Number
	 * \param bFinal - true if this is final link in chain
	 * \param links - Link names
	 */
	virtual int PubChainLink( String          ^chainName,
	                          IntPtr           arg,
	                          int              linkNum,
	                          bool             bFinal,
	                          array<String ^> ^links );

#ifndef DOXYGEN_OMIT
	/////////////////////////////////
	// Helpers : Conversion
	/////////////////////////////////
protected:
	RTEDGE::rtDateTime _ConvertDateTime( DateTime ^cli )
	{
	   RTEDGE::rtDateTime cpp;
	   RTEDGE::rtDate    &dt = cpp._date;
	   RTEDGE::rtTime    &tm = cpp._time;

	   dt._year   = cli->Year;
	   dt._month  = cli->Month - 1;
	   dt._mday   = cli->Day;
	   tm._hour   = cli->Hour; 
	   tm._minute = cli->Minute;
	   tm._second = cli->Second;
	   tm._micros = cli->Millisecond * 1000;
	   return cpp;
	}

#endif //  DOXYGEN_OMIT

}; // class rtEdgePubUpdate

} // namespace librtEdge
