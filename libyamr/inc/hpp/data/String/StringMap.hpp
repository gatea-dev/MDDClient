/******************************************************************************
*
*  StringMap.hpp
*     libyamr String Map StringMap Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*     29 JUL 2019 jcs  Build  2: Sharable
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_StringMap_H
#define __YAMR_StringMap_H
#include <hpp/data/Int/IntList.hpp>
#include <hpp/data/String/StringDict.hpp>


namespace YAMR
{

namespace Data
{

////////////////////////////////////////////////
//
//       c l a s s    S t r i n g M a p
//
////////////////////////////////////////////////

/**
 * \class StringMap
 * \brief yamRecorder String Map
 */
class StringMap : public Codec
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
	StringMap( Reader &reader ) :
	   Codec( reader ),
	   _decode( new IntList( reader, false ) ),
	   _encode( (IntList *)0 ),
	   _sdb()
	{
	   if ( !reader.HasStringDict() )
	      reader.RegisterStringDict( new StringDict( reader ) );
	   reader.RegisterProtocol( *this, _PROTO_STRINGMAP, "StringMap" );
	}

	/**
	 * \brief Constructor - Marshall
	 *
	 * \param writer - Writer we are encoding on
	 */
	StringMap( Writer &writer ) :
	   Codec( writer ),
	   _decode( (IntList *)0 ),
	   _encode( new IntList( writer ) ),
	   _sdb()
	{
	   if ( !writer.HasStringDict() )
	      writer.RegisterStringDict( new StringDict( writer ) );
	}

	/** \brief Destructor */
	~StringMap()
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
	 * \brief Return decoded String Map
	 *
	 * \return decoded String Map
	 */
	StringHashMap &strMap()
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
	   std::string k, v;
	   size_t      i;

	   // Pre-condition

	   if ( !_reader )
	      return false;

	   // 1) Dictionary??

	   StringDict &dict = reader().strDict();

	   if ( msg._WireProtocol == _PROTO_STRINGDICT )
	      return dict.Decode( msg );

	   // 2) Us; IntList

	   _decode->Decode( msg );

	   // 3) OK, pull out strings from dict

	   Ints  &idb = _decode->intList();

	   _sdb.clear();
	   for ( i=0; i<idb.size(); ) {
	      k       = dict.GetString( idb[i++] );
	      v       = dict.GetString( idb[i++] );
	      _sdb[k] = v;
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
	   StringHashMap::iterator it;
	   std::string             s, k, v;
	   char                    buf[K], fmt[20];
	   size_t                  kSz;

	   // 1) Key Size dictates output format

	   for ( kSz=0,it=_sdb.begin(); it!=_sdb.end(); it++ ) {
	      k   = (*it).first;
	      kSz = gmax( kSz, strlen( k.data() ) );
	   }
	   sprintf( fmt, "\n   %%-%ds = %%s,", (int)kSz );

	   // 2) Formatted Output

	   sprintf( buf, "%d entries", (int)_sdb.size() );
	   s += buf;
	   for ( it=_sdb.begin(); it!=_sdb.end(); it++ ) {
	      k = (*it).first;
	      v = (*it).second;
	      sprintf( buf, fmt, k.data(), v.data() );
	      s += buf;
	   }
	   return std::string( s );
	} 


	////////////////////////////////////
	// Marshall Operations
	////////////////////////////////////
public:
	/**
	 * \brief Add entry to String Map
	 *
	 * Call Send() to send complete map to yamRecorder
	 *
	 * \param key - String Key
	 * \param val - String Value
	 * \see Send()
	 */
	void AddMapEntry( const char *key, const char *val )
	{
	   std::string k( key ), v( val );

	   AddMapEntry( k, v );
	}

	/**
	 * \brief Add entry to String Map
	 *
	 * Call Send() to send complete map to yamRecorder
	 *
	 * \param key - String Key
	 * \param val - String Value
	 * \see Send()
	 */
	void AddMapEntry( std::string &key, std::string &val )
	{
	   StringDict &dict = writer().strDict();
	   int         i1, i2;

	   if ( _writer ) {
	      i1 = dict.GetStrIndex( key );
	      i2 = dict.GetStrIndex( val );
	      _encode->Add( i1 );
	      _encode->Add( i2 );
	   }
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
	   // Send list and clear out internal guts

	   return _encode ? _encode->Send( _PROTO_STRINGMAP ) : false;
	}


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
private:
	IntList      *_decode;
	IntList      *_encode;
	StringHashMap _sdb;

};  // class StringMap

} // namespace Data

} // namespace YAMR

#endif // __YAMR_StringMap_H 
