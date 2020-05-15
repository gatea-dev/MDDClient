/******************************************************************************
*
*  DoubleList.h
*     libyamr Double List
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
//      c l a s s    D o u b l e L i s t
//
////////////////////////////////////////////////

/**
 * \class DoubleList
 * \brief List of Doubles
 */
public ref class DoubleList
{
private:
	YAMR::Data::DoubleList *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	DoubleList( Reader ^reader ) :
	   _cpp( new YAMR::Data::DoubleList( reader->cpp(), true ) )
	{ ; }

	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 * \param bRegister - true to register the protocol
	 */
	DoubleList( Reader ^reader, bool bRegister ) :
	   _cpp( new YAMR::Data::DoubleList( reader->cpp(), bRegister ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	DoubleList( Writer ^writer ) :
	   _cpp( new YAMR::Data::DoubleList( writer->cpp() ) )
	{ ; }

	/** 
	 * \brief Destructor
	 */
	~DoubleList()
	{
	   delete _cpp;
	}



	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded double List
	 *
	 * \return Decoded double List
	 */
	array<double> ^doubleList()
	{
	   YAMR::Data::Doubles &ddb = _cpp->doubleList();
	   array<double>       ^rc;
	   size_t               i, n;

	   rc = nullptr;
	   if ( (n=ddb.size()) ) {
	      rc = gcnew array<double>( n );
	      for ( i=0; i<n; rc[i]=ddb[i], i++ );
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
	 * \brief Add double to list
	 *
	 * \param idx - Double to add to list
	 */
	void Add( double idx )
	{
	   _cpp->Add( idx );
	}

	/**
	 * \brief Add array of doubles to list
	 *
	 * \param arr - Array of doubles to add
	 */
	void Add( array<double> ^arr )
	{
	   int i, n;

	   n = arr->Length;
	   for ( i=0; i<n; _cpp->Add( arr[i++] ) );
	}

	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \return true if message consumed by channel; false otherwise
	 */
	bool Send()
	{
	   return Send( 0 );
	}

	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \param MsgProto - Message Protocol
	 * \return true if message consumed by channel; false otherwise
	 */
	bool Send( short MsgProto )
	{
	   return _cpp->Send( MsgProto );
	}

}; // class DoubleList

} // namespace Data

} // namespace libyamr
