/******************************************************************************
*
*  FloatList.h
*     libyamr Float List
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
//      c l a s s    F l o a t L i s t
//
////////////////////////////////////////////////

/**
 * \class FloatList
 * \brief List of Floats
 */
public ref class FloatList
{
private:
	YAMR::Data::FloatList *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	FloatList( Reader ^reader ) :
	   _cpp( new YAMR::Data::FloatList( reader->cpp(), true ) )
	{ ; }

	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 * \param bRegister - true to register the protocol
	 */
	FloatList( Reader ^reader, bool bRegister ) :
	   _cpp( new YAMR::Data::FloatList( reader->cpp(), bRegister ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	FloatList( Writer ^writer ) :
	   _cpp( new YAMR::Data::FloatList( writer->cpp() ) )
	{ ; }

	/** 
	 * \brief Destructor
	 */
	~FloatList()
	{
	   delete _cpp;
	}



	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded float List
	 *
	 * \return Decoded float List
	 */
	array<float> ^floatList()
	{
	   YAMR::Data::Floats &ddb = _cpp->floatList();
	   array<float>       ^rc;
	   size_t              i, n;

	   rc = nullptr;
	   if ( (n=ddb.size()) ) {
	      rc = gcnew array<float>( n );
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
	 * \brief Add float to list
	 *
	 * \param idx - Float to add to list
	 */
	void Add( float idx )
	{
	   _cpp->Add( idx );
	}

	/**
	 * \brief Add array of floats to list
	 *
	 * \param arr - Array of floats to add
	 */
	void Add( array<float> ^arr )
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

}; // class FloatList

} // namespace Data

} // namespace libyamr
