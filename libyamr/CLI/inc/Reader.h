/******************************************************************************
*
*  Reader.h
*
*  REVISION HISTORY:
*     19 JUN 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#pragma once
#include <Data.h>

namespace libyamr
{

////////////////////////////////////////////////
//
//          c l a s s   R e a d e r
//
////////////////////////////////////////////////
/**
 * \class Reader
 * \brief Unstructured tape reader class.
 *
 * All structured data transfer derive from and utilize the services
 * of the unstructured writer and reader classes.
 * \see Writer
 */      
public ref class Reader : public libyamr::yamr
{
private: 
	YAMR::Reader *_cpp;
	::yamrMsg    *_ym;
	yamrMsg      ^_msg;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor. */
	Reader();

	/** \brief Destructor. */
	~Reader();


	////////////////////////////////////
	// Debug : Message Dump
	////////////////////////////////////
	/**
	 * \brief Dump message header
	 *
	 * \param msg - Parsed YAMR messae
	 * \return Viewable Message Header
	 */
	String ^DumpHeader( yamrMsg ^msg );

	/**
	 * \brief Dump message based on protocol
	 *
	 * This class calls out to the Data::IDecodable::Dump() based on the 
	 * protocol in the message.
	 *
	 * \param msg - Parsed YAMR messae
	 * \return Viewable Message Contents
	 */
	String ^DumpBody( yamrMsg ^msg );

	/**
	 * \brief Return protocol name associated w/ message
	 * 
	 * \param msg - Parsed YAMR message
	 * \return Protocol name
	 */ 
	String ^ProtoName( yamrMsg ^msg );

	/**
	 * \brief Return protocol name from protocol ID
	 * 
	 * \param proto - Protocol ID
	 * \return Protocol name
	 */ 
	String ^ProtoName( short proto );

	/**
	 * \brief Return protocol number from registered name
	 * 
	 * \param name - Registered Protocol Name
	 * \return Protocol number; 0 if not found
	 */ 
	short ProtoNumber( String ^name );


	////////////////////////////////////
	// Reader Operations
	////////////////////////////////////
public:
	/**
	 * \brief Opens yamRecorder tape for reading
	 *
	 * \param filename - Tape filenme
	 */
	void Open( String ^filename );

	/** \brief Close yamRecorder tape file */
	void Close();

	/**
	 * \brief Rewind to beginning of tape
	 *
	 * \return Unix Time in Nanos of next message; 0 if empty tape
	 */
	long Rewind();

	/**
	 * \brief Rewind tape to specific position
	 *
	 * \param pos - Rewind position in Nanos since epoch 
	 * \return Unix Time in Nanos of next message; 0 if empty tape
	 */
	long RewindTo( long pos );

	/**
	 * \brief Rewind tape to specific position
	 *
	 * \param pTime - String formatted time
	 * \return Unix Time in Nanos of next message; 0 if empty tape
	 */
	long RewindTo( String ^pTime );

	/**
	 * \brief Read unstructured message from tape
	 *
	 * \return Unstructured message if successful read; null otherwise
	 */
	yamrMsg ^Read();

	/**
	 * \brief Decode message based on protocol
	 *
	 * This class calls out to the Data::IDecodable::Decode() based on the 
	 * protocol in the message.
	 *
	 * \param msg - Parsed YAMR message
	 */
	void Decode( yamrMsg ^msg );


	////////////////////////////////////
	// Access
	////////////////////////////////////
#ifndef DOXYGEN_OMIT
	/**
	 * \brief Returns reference to unmanaged YAMR::Reader object 
	 *
	 * \return Reference to unmanaged YAMR::Reader object 
	 */
	YAMR::Reader &cpp()
	{
	   return *_cpp;
	}
#endif // DOXYGEN_OMIT


};  // class Reader

} // namespace libyamr
