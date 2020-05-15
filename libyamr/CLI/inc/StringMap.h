/******************************************************************************
*
*  StringMap.h
*     libyamr String Map
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
//      c l a s s    S t r i n g M a p
//
////////////////////////////////////////////////

/**
 * \class StringMap
 * \brief Map of Strings
 */
public ref class StringMap : public libyamr::yamr
{
private:
	YAMR::Data::StringMap *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	StringMap( Reader ^reader ) :
	   _cpp( new YAMR::Data::StringMap( reader->cpp() ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	StringMap( Writer ^writer ) :
	   _cpp( new YAMR::Data::StringMap( writer->cpp() ) )
	{ ; }

	/** \brief Destructor */
	~StringMap()
	{
	   delete _cpp;
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded String Map
	 *
	 * \return Decoded String Map
	 */
#ifdef WTF
	Dictionary<String ^, String ^> ^strMap()
	{
	   Dictionary<String ^, String ^>     ^rc;
	   YAMR::Data::StringHashMap          &ddb = _cpp->strMap();
	   YAMR::Data::StringHashMap::iterator it;
	   const char                         *k, *v;

	   rc = gcnew Dictionary<String ^, String ^>();
	   for ( it=ddb.begin(); it!=ddb.end(); it++ ) {
	      k = (*it).first.data();
	      v = (*it).second.data();
	      rc->Add( gcnew String( k ), gcnew String( v ) );
	   }
	   return rc;
	}
#endif // WTF

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
	 * \brief Add entry to Map
	 *
	 * \param key- String key
	 * \param val - String value
	 * \see Send()
	 */
	void AddMapEntry( String ^key, String ^val )
	{
	   _cpp->AddMapEntry( _pStr( key ), _pStr( val ) );
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

}; // class StringMap

} // namespace Data

} // namespace libyamr
