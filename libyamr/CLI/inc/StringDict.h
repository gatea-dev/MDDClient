/******************************************************************************
*
*  StringDict.h
*     libyamr String Dictionary
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#pragma once
#include <Reader.h>
#include <Writer.h>


namespace libyamr
{

namespace Data
{

////////////////////////////////////////////////
//
//      c l a s s    S t r i n g D i c t
//
////////////////////////////////////////////////

/**
 * \class StringDict
 * \brief List of Doubles
 */
public ref class StringDict : public libyamr::yamr
{
private:
	YAMR::Data::StringDict *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	StringDict( Reader ^reader ) :
	   _cpp( new YAMR::Data::StringDict( reader->cpp() ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	StringDict( Writer ^writer ) :
	   _cpp( new YAMR::Data::StringDict( writer->cpp() ) )
	{ ; }

	/** \brief Destructor */
	~StringDict()
	{
	   delete _cpp;
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Retreives string name associated w/ index.
	 *
	 * \param idx - Unique string index
	 * \return String
	 */
	String ^GetString( int idx )
	{
	   return gcnew String( _cpp->GetString( idx ) );
	}

	/**
	 * \brief Decode message, if correct protocol
	 *
	 * \param msg - Parsed unstructured YAMR message
	 * \return true if our protocol; false otherwise 
	 */
	bool Decode( yamrMsg ^msg )
	{
	   return _cpp->Decode( msg->cpp() );
	}

	/** 
	 * \brief Dump Message based on protocol
	 *   
	 * \param msg - Parsed unstructured YAMR message
	 * \return Viewable Message Contents
	 */  
	String ^Dump( yamrMsg ^msg )
	{
	   std::string s;

	   s = _cpp->Dump( msg->cpp() );
	   return gcnew String( s.data() );
	}


	////////////////////////////////////
	// Marshall : Operations
	////////////////////////////////////
public:
	/**
	 * \brief Returns current dictionary size
	 *
	 * \return Current dictionary size
	 */
	int Size()
	{
	   return _cpp->Size();
	}

	/**
	 * \brief Retreives unique string index from encoder.
	 *
	 * Index remains unique for the life of the session.
	 *
	 * \param str - String
	 * \return Unique string index
	 */
	int GetStrIndex( String ^str )
	{
	   return _cpp->GetStrIndex( _pStr( str ) );
	}

	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \return true if message consumed by channel; false otherwise
	 */
	bool Send()
	{
	   return _cpp->Send();
	}

}; // class StringDict

} // namespace Data

} // namespace libyamr
