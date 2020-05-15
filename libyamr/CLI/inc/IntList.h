/******************************************************************************
*
*  IntList.h
*     libyamr int List
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
//      c l a s s    I n t L i s t
//
////////////////////////////////////////////////

/**
 * \class IntList
 * \brief List of ints
 */
public ref class IntList
{
private:
	YAMR::Data::IntList *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	IntList( Reader ^reader ) :
	   _cpp( new YAMR::Data::IntList( reader->cpp(), true ) )
	{ ; }

	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 * \param bRegister - true to register the protocol
	 */
	IntList( Reader ^reader, bool bRegister ) :
	   _cpp( new YAMR::Data::IntList( reader->cpp(), bRegister ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	IntList( Writer ^writer ) :
	   _cpp( new YAMR::Data::IntList( writer->cpp() ) )
	{ ; }

	/** \brief Destructor */
	~IntList()
	{
	   delete _cpp;
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded int List
	 *
	 * \return decoded Integer List
	 */
	array<int> ^intList()
	{
	   YAMR::Data::Ints &ddb = _cpp->intList();
	   array<int>       ^rc;
	   size_t            i, n;

	   rc = nullptr;
	   if ( (n=ddb.size()) ) {
	      rc = gcnew array<int>( n );
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
	 * \brief Add int to list
	 *
	 * \param idx - int to add to list
	 */
	void Add( int idx )
	{
	   _cpp->Add( idx );
	}

	/**
	 * \brief Add array of ints to list
	 *
	 * \param arr - Array of ints to add
	 */
	void Add( array<int> ^arr )
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

}; // class IntList

} // namespace Data

} // namespace libyamr
