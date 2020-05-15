/******************************************************************************
*
*  StringList.hpp
*     libyamr String List Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*     29 JUL 2019 jcs  Build  2: Sharable
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_StringList_H
#define __YAMR_StringList_H
#include <hpp/data/Int/IntList.hpp>


namespace YAMR
{

namespace Data
{

////////////////////////////////////////////////
//
//       c l a s s    S t r i n g L i s t
//
////////////////////////////////////////////////

/**
 * \class StringList
 * \brief yamRecorder String List
 */
class StringList : public Codec
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - DeMarshall
	 * 
	 * \param reader - Reader channel driving us
	 */
	StringList( Reader &reader ) :
	   Codec( reader ),
	   _decode( new IntList( reader, false ) ),
	   _encode( (IntList *)0 ),
	   _sdb()
	{
	   if ( !reader.HasStringDict() )
	      reader.RegisterStringDict( new StringDict( reader ) );
	   reader.RegisterProtocol( *this, _PROTO_STRINGLIST, "StringList" );
	}

	/** 
	 * \brief Constructor - Marshall
	 * 
	 * \param writer - Writer we are encoding on
	 */
	StringList( Writer &writer ) :
	   Codec( writer ),
	   _decode( (IntList *)0 ),
	   _encode( new IntList( writer ) ),
	   _sdb()
	{
	   if ( !writer.HasStringDict() )
	      writer.RegisterStringDict( new StringDict( writer ) );
	}

	/** \brief Destructor */
	~StringList()
	{
	   if ( _decode )
	      delete _decode;
	   if ( _encode )
	      delete _encode;
	}


	////////////////////////////////////
	// DeMarshall Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded String List
	 *
	 * \return decoded String List
	 */
	Strings &strList()
	{
	   return _sdb;
	}


	////////////////////////////////////
	// IDecodable Interface
	////////////////////////////////////
public:
	/**
	 * \brief Decode message, if correct protocol
	 *
	 * \param msg - Parsed unstructured YAMR message
	 * \return true if our protocol; false otherwise 
	 */
	virtual bool Decode( yamrMsg &msg )
	{
	   StringDict &dict = reader().strDict();
	   const char *ps;
	   size_t      i;

	   // Pre-condition

	   if ( !_reader )
	      return false;

	   // 1) Dictionary??

	   if ( msg._WireProtocol == _PROTO_STRINGDICT )
	      return dict.Decode( msg );

	   // 2) Us; IntList

	   _decode->Decode( msg );

	   // 3) OK, pull out strings from dict

	   Ints &idb = _decode->intList();

	   _sdb.clear();
	   for ( i=0; i<idb.size(); i++ ) {
	      ps = dict.GetString( idb[i] );
	      _sdb.push_back( std::string( ps ) );
	   }
	   return true;
	}

	/**
	 * \brief Dump Message based on protocol
	 *
	 * \param msg - Parsed unstructured YAMR message
	 * \param dmpTy - DumpType : Verbose, CSV, etc.
	 * \return Viewable Message Contents
	 */
	virtual std::string Dump( yamrMsg &msg, DumpType dmpTy ) 
	{
	   std::string s; 
	   char        buf[K], *cp;
	   size_t      i;

	   for ( i=0; i<_sdb.size(); i++ ) {
	      cp  = buf;
	      cp += i ? sprintf( cp, "," ) : 0;
	      cp += sprintf( cp, _sdb[i].data() );
	   }
	   return std::string( s );
	} 


	////////////////////////////////////
	// Marshall Operations
	////////////////////////////////////
public:
	/**
	 * \brief Add string to String List
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param pStr - String
	 * \return Unique string index
	 * \see Send()
	 */
	int AddString( const char *pStr )
	{
	   std::string s( pStr );

	   return AddString( s );
	}

	/**
	 * \brief Add string to String List
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param str - String
	 * \return Unique string index
	 * \see Send()
	 */
	int AddString( std::string &str )
	{
	   int rc;

	   rc = 0;
	   if ( _writer ) {
	      rc = writer().strDict().GetStrIndex( str );
	      AddIndex( rc );
	   }
	   return rc;
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
	   if ( _encode )
	      _encode->Add( idx );
	}

	/**
	 * \brief Add list of strings to String List
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param lst - List of Strings to add to String List
	 * \see Send()
	 */
	void AddList( Strings &lst )
	{
	   size_t i;

	   for ( i=0; i<lst.size(); AddString( lst[i++] ) );
	}


	////////////////////////////////////
	// IEncodable Interface
	////////////////////////////////////
public:
	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \param MsgProto - Message Protocol; 0 = same as Wire Protocol
	 * \return true if message consumed by channel; false otherwise
	 */
	virtual bool Send( u_int16_t MsgProto=0 )
	{
	   return _encode ? _encode->Send( MsgProto ) : false;
	}


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
private:
	IntList *_decode; 
	IntList *_encode;
	Strings  _sdb;

}; // class StringList

} // namespace Data

} // namespace YAMR

#endif // __YAMR_StringList_H 
