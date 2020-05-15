/******************************************************************************
*
*  StringList.h
*     libyamr String List
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
//      c l a s s    S t r i n g L i s t
//
////////////////////////////////////////////////

/**
 * \class StringList
 * \brief List of Strings
 */
public ref class StringList : public libyamr::yamr
{
private:
	YAMR::Data::StringList *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	StringList( Reader ^reader ) :
	   _cpp( new YAMR::Data::StringList( reader->cpp() ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	StringList( Writer ^writer ) :
	   _cpp( new YAMR::Data::StringList( writer->cpp() ) )
	{ ; }

	/** \brief Destructor */
	~StringList()
	{
	   delete _cpp;
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded String List
	 *
	 * \return Decoded String List
	 */
	array<String ^> ^stringList()
	{
	   YAMR::Data::Strings &ddb = _cpp->strList();
	   array<String ^>     ^rc;
	   size_t               i, n;

	   rc = nullptr;
	   if ( (n=ddb.size()) ) {
	      rc = gcnew array<String ^>( n );
	      for ( i=0; i<n; i++ )
	         rc[i] = gcnew String( ddb[i].data() );
	   }
	   return rc;
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
	 * \brief Add string to list
	 *
	 * \param str - String to add to list
	 * \see Send()
	 */
	void AddString( String ^str )
	{
	   _cpp->AddString( _pStr( str ) );
	}

	/**
	 * \brief Add String index to List
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param idx - String Index
	 * \see Send()
	 */
	void AddIndex( int idx )
	{
	   _cpp->AddIndex( idx );
	}

	/**
	 * \brief Add array of strings to String List
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param arr - Array of Strings to add to StringList
	 * \see Send()
	 */
	void AddList( array<String ^> ^arr )
	{
	   size_t i;

	   for ( i=0; i<arr->Length; AddString( arr[i++] ) );
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

}; // class StringList

} // namespace Data

} // namespace libyamr
