/******************************************************************************
*
*  Update.h
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*     23 JAN 2015 jcs  Build 29: ByteStreamFld; PubChainLink() 
*      7 JUL 2015 jcs  Build 31: Publish( cli::array<Byte> ^ )
*     29 APR 2020 jcs  Build 43: Moved IrtEdgePubUpdate in here
*     30 MAR 2022 jcs  Build 52: doxygen; AddFieldAsDate() / Time()
*     23 MAY 2022 jcs  Build 54: AddFieldAsUnixTime()
*     23 OCT 2022 jcs  Build 58: cli::array<>
*     30 OCT 2022 jcs  Build 60: rtFld_vector
*     10 NOV 2022 jcs  Build 61: AddFieldAsVector( DateTime )
*     30 JUN 2023 jcs  Build 63: StringDoor
*     24 OCT 2023 jcs  Build 66: AddEmptyField()
*     18 MAR 2024 jcs  Build 70: AddFieldAsDouble( ..., int precision )
*     28 JUN 2024 jcs  Build 72: Nullable GetAsXxx()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <PubChannel.h>
#include <Field.h>
#include <Data.h>
#endif // DOXYGEN_OMIT

using namespace librtEdgePRIVATE;

namespace librtEdge
{

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
public ref class rtEdgePubUpdate : public StringDoor
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
	// Access
	/////////////////////////////////
public:
#ifndef DOXYGEN_OMIT
	RTEDGE::Update &cpp() { return _upd; }
#endif // DOXYGEN_OMIT


	/////////////////////////////////
	// Publish
	/////////////////////////////////
public:
	/**
	 * \brief Publish a single field list update with fields added by 
	 * AddField() and friends
	 *
	 * \return  Number of bytes published
	 */
	int Publish();

	/**
	 * \brief Publish pre-formatted data payload
	 *
	 * \param buf - Pre-formatted data payload (field list)
	 * \param bFieldList - true if field list; Else fixed msg
	 * \return  Number of bytes published
	 */
	int Publish( cli::array<Byte> ^buf, bool bFieldList );

	/**
	 * \brief Publish an error
	 *
	 * \param err - Error string
	 * \return  Number of bytes published
	 */
	int PubError( String ^err );


	/////////////////////////////////
	// ByteStream
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
	int Publish( ByteStream ^bStr, int fidData );

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
	int Publish( ByteStream ^bStr, 
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
	// Fields
	/////////////////////////////////
	/**
	 * \brief Add rtEdgeField to update
	 *
	 * \param fld - rtEdgeField to add
	 */
	void AddField( rtEdgeField ^fld );

	/**
	 * \brief Add empty (null) field
	 *
	 * \param fid - Field ID
	 */
	void AddEmptyField( int fid );

	/**
	 * \brief Add string field to update
	 *
	 * The caller is responsible for ensuring the integrity of the
	 * data in str until Publish() is called.
	 *
	 * \param fid - Field ID
	 * \param str - Field value as string
	 */
	void AddFieldAsString( int fid, String ^str );

	/**
	 * \brief Add 8-bit int field to update
	 *
	 * \param fid - Field ID
	 * \param i8 - Field value as 8-bit char
	 */
	void AddFieldAsInt8( int fid, Nullable<u_char> i8 );

	/**
	 * \brief Add 16-bit int field to update
	 *
	 * \param fid - Field ID
	 * \param i16 - Field value as 16-bit short int
	 */
	void AddFieldAsInt16( int fid, Nullable<u_short> i16 );

	/**
	 * \brief Add 32-bit int field to update
	 *
	 * \param fid - Field ID
	 * \param i32 - Field value as 32-bit int
	 */
	void AddFieldAsInt32( int fid, Nullable<int> i32 );

	/**
	 * \brief Add 64-bit long field to update
	 *
	 * \param fid - Field ID
	 * \param i64 - Field value as 64-bit int
	 */
	void AddFieldAsInt64( int fid, Nullable<long long> i64 );

	/**
	 * \brief Add float field to update
	 *
	 * \param fid - Field ID
	 * \param r32 - Field value as double
	 */
	void AddFieldAsFloat( int fid, Nullable<float> r32 );

	/**
	 * \brief Add double field to update
	 *
	 * \param fid - Field ID
	 * \param r64 - Field value as double
	 */
	void AddFieldAsDouble( int fid, Nullable<double> r64 );

	/**
	 * \brief Add double field to update with precision
	 *
	 * Rules: 
	 *  - The precision argument is only used in unpacked mode
	 *  - Else, the double goes on wire with default precision (10 sigFig)
	 *      
	 * \param fid - Field ID
	 * \param r64 - Field value as double 
	 * \param precision - Number of significant digits; Default is 10
	 */     
	void AddFieldAsDouble( int fid, Nullable<double> r64, int precision );

	/**
	 * \brief Add ByteStreamFld field to update
	 *
	 * \param fid - Field ID
	 * \param bStr - Field Value as ByteStreamFld
	 */
	void AddFieldAsByteStream( int fid, ByteStreamFld ^bStr );

	/**
	 * \brief Add vector of doubles field to update w/ 10 digits of precision on the wire
	 *
	 * \param fid - Field ID
	 * \param vector - Field value as vector of doubles
	 */
	void AddFieldAsVector( int fid, cli::array<double> ^vector );

	/**
	 * \brief Add vector of doubles field to update
	 *
	 * \param fid - Field ID
	 * \param vector - Field value as vector of doubles
	 * \param precision - Vector precision : 0 to 20
	 */
	void AddFieldAsVector( int fid, cli::array<double> ^vector, int precision );

	/**
	 * \brief Add vector of DateTimes field to update
	 *
	 * \param fid - Field ID
	 * \param vector - Field value as vector
	 * \return Array of doubles to store until calling Publish()
	 */
	void AddFieldAsVector( int fid, cli::array<DateTime ^> ^vector );

	/**
	 * \brief Add date-time field to update as DateTime
	 *
	 * \param fid - Field ID
	 * \param dt - Field value as DateTime
	 */
	void AddFieldAsDateTime( int fid, DateTime ^dt );

	/**
	 * \brief Add rtDateTime field to update as UnixTime
	 *
	 * __Use of the UnixTime data type requires the PubChannel run in unpacked
	 * mode.__  If running in packed (default), this method will not add the
	 * field and post an error to PubChannel::OnError().
	 *
	 * \param fid - Field ID
	 * \param dtTm - Field value as DateTime
	 * 
	 * \see PubChannel::IsUnPacked()
	 * \see PubChannel::SetUnPacked()
	 * \see PubChannel::OnError()
	 */
	void AddFieldAsUnixTime( int fid, DateTime ^dtTm );

	/**
	 * \brief Add date-time field to update as Date
	 *
	 * \param fid - Field ID
	 * \param dt - Field value as DateTime
	 */
	void AddFieldAsDate( int fid, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as Time
	 *
	 * \param fid - Field ID
	 * \param dt - Field value as DateTime
	 */
	void AddFieldAsTime( int fid, DateTime ^dt );

	/**
	 * \brief Add string field to update
	 *
	 * The caller is responsible for ensuring the integrity of the
	 * data in str until Publish() is called.
	 *
	 * \param pFld - Field Name
	 * \param str - Field value as string
	 */
	void AddFieldAsString( String ^pFld, String ^str );

	/**
	 * \brief Add 8-bit int field to update
	 *
	 * \param pFld - Field Name
	 * \param i8 - Field value as 8-bit char
	 */
	void AddFieldAsInt8( String ^pFld, u_char i8 );

	/**
	 * \brief Add 16-bit int field to update
	 *
	 * \param pFld - Field Name
	 * \param i16 - Field value as 16-bit short int
	 */
	void AddFieldAsInt16( String ^pFld, u_short i16 );

	/**
	 * \brief Add 32-bit int field to update
	 *
	 * \param pFld - Field Name
	 * \param i32 - Field value as 32-bit int
	 */
	void AddFieldAsInt32( String ^pFld, int i32 );

	/**
	 * \brief Add 64-bit long field to update
	 *
	 * \param pFld - Field Name
	 * \param i64 - Field value as 64-bit int
	 */
	void AddFieldAsInt64( String ^pFld, long long i64 );

	/**
	 * \brief Add float field to update
	 *
	 * \param pFld - Field Name
	 * \param r32 - Field value as double
	 */
	void AddFieldAsFloat( String ^pFld, float r32 );

	/**
	 * \brief Add double field to update
	 *
	 * \param pFld - Field Name
	 * \param r64 - Field value as double
	 */
	void AddFieldAsDouble( String ^pFld, double r64 );

	/**
	 * \brief Add double field to update with precision
	 *
	 * Rules: 
	 *  - The precision argument is only used in unpacked mode
	 *  - Else, the double goes on wire with default precision (10 sigFig)
	 *
	 * \param pFld - Field Name
	 * \param r64 - Field value as double 
	 * \param precision - Number of significant digits; Default is 10
	 */     
	void AddFieldAsDouble( String ^pFld, double r64, int precision );

	/**
	 * \brief Add ByteStreamFld field to update
	 *
	 * \param pFld - Field Name
	 * \param bStr - Field Value as ByteStreamFld
	 */
	void AddFieldAsByteStream( String ^pFld, ByteStreamFld ^bStr );

	/**
	 * \brief Add vector of doubles field to update w/ 10 digits of precision on the wire
	 *
	 * \param pFld - Field Name
	 * \param vector - Field value as vector of doubles
	 */
	void AddFieldAsVector( String ^pFld, cli::array<double> ^vector );

	/**
	 * \brief Add vector of doubles field to update
	 *
	 * \param pFld - Field Name
	 * \param vector - Field value as vector of doubles
	 * \param precision - Vector precision : 0 to 20
	 */
	void AddFieldAsVector( String ^pFld, cli::array<double> ^vector, int precision );

	/**
	 * \brief Add vector of DateTimes field to update
	 *
	 * \param pFld - Field Name
	 * \param vector - Field value as vector of DateTimes
	 */
	void AddFieldAsVector( String ^pFld, cli::array<DateTime ^> ^vector );

	/**
	 * \brief Add date-time field to update as MDD DateTime
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTime
	 */
	void AddFieldAsDateTime( String ^pFld, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as MDD Date
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTime
	 */
	void AddFieldAsDate( String ^pFld, DateTime ^dt );

	/**
	 * \brief Add date-time field to update as MDD Time
	 *
	 * \param pFld - Field Name
	 * \param dt - Field value as DateTime
	 */
	void AddFieldAsTime( String ^pFld, DateTime ^dt );


	/////////////////////////////////
	// Chains
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
	int PubChainLink( String          ^chainName,
	                  IntPtr           arg,
	                  int              linkNum,
	                  bool             bFinal,
	                  cli::array<String ^> ^links,
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
	int PubChainLink( String          ^chainName,
	                  IntPtr           arg,
	                  int              linkNum,
	                  bool             bFinal,
	                  cli::array<String ^> ^links );

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
