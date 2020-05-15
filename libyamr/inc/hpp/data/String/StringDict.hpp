/******************************************************************************
*
*  StringDict.hpp
*     libyamr String Dict StringDict Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_StringDict_H 
#define __YAMR_StringDict_H 
#include <hpp/Reader.hpp>
#include <hpp/data/Data.hpp>


namespace YAMR
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
 * \brief yamRecorder String Dictionary (index-based)
 */
class StringDict : public Codec
{
#ifndef DOXYGEN_OMIT

	typedef hash_map<std::string, u_int32_t> Str2IdxMap;
	typedef hash_map<u_int32_t, std::string> Idx2StrMap;

	////////////////////////////////////////////////
	//
	//       c l a s s    D e c o d e r
	//
	////////////////////////////////////////////////


	class _BinDict
	{
	public:
	   u_int32_t _ID;
	   u_int16_t _len;
	// u_char    _data[_len];
	};

	class Decoder
	{
	   ////////////////////////////////////
	   // Constructor / Destructor
	   ////////////////////////////////////
	public:
	   Decoder() : 
	      _sdb()
	   {
	   }

	   ~Decoder()
	   {
	      _sdb.clear();
	   }


	   ////////////////////////////////////
	   // Decoder Operations
	   ////////////////////////////////////
	public:
	   u_int16_t protocol()
	   {
	      return _PROTO_STRINGDICT;
	   }

	   const char *GetString( u_int32_t idx )
	   {
	      const char          *rc;
	      std::string          s;
	      Idx2StrMap::iterator it;

	      // Add if not there

	      rc = "not found";
	      if ( (it=_sdb.find( idx )) != _sdb.end() ) {
	         s  = (*it).second;
	         rc = s.data();
	      }
	      return rc;
	   }

	   void Decode( yamrBuf yb )
	   {
	      Idx2StrMap &v = _sdb;
	      char       *cp;
	      u_int32_t   ix;
	      _BinDict   *h;

	      cp    = yb._data;
	      h     = (_BinDict *)cp;
	      ix    = h->_ID;
	      cp   += sizeof( _BinDict );
	      v[ix] = std::string( cp, h->_len );
	   }

	   u_int32_t GetIdx( yamrBuf yb )
	   {
	      _BinDict *h;

	      h = (_BinDict *)yb._data;
	      return h->_ID;
	   }


	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Idx2StrMap _sdb;

	};  // class Decoder


	////////////////////////////////////////////////
	//
	//       c l a s s    E n c o d e r
	//
	////////////////////////////////////////////////
	class Encoder : public IWriteListener
	{
	   ////////////////////////////////////
	   // Constructor / Destructor
	   ////////////////////////////////////
	public:
	   Encoder() :
	      _sdb(),
	      _idx( 0 )
	   {
	   }

	   ~Encoder()
	   {
	      _sdb.clear();
	   }


	   ////////////////////////////////////
	   // Encoder Operations
	   ////////////////////////////////////
	public:
	   size_t Size()
	   {
	      return _sdb.size();
	   }

	   u_int32_t GetIdx( Writer &writer, const char *str )
	   {
	      std::string          s( str );
	      u_int32_t            ix;
	      Str2IdxMap::iterator it;

	      // Add if not there

	      if ( (it=_sdb.find( s )) == _sdb.end() ) {
	         ix      = _idx++;
	         _sdb[s] = ix;
	         _Encode( writer, s, ix );
	      }
	      else
	         ix = (*it).second;
	      return ix;
	   }

	   bool Flush( Writer &writer, u_int16_t MsgProto=0 )
	   {
	      Str2IdxMap::iterator it;
	      bool                 rc;

	      // Flush all on connect

	      rc = true;
	      for ( it=_sdb.begin(); rc && it!=_sdb.end(); it++ )
	         rc &= _Encode( writer, (*it).first, (*it).second, MsgProto );
	      return rc;
	   }


	   ////////////////////////////////////
	   // IWriteListener Notifications
	   ////////////////////////////////////
	   virtual void OnConnect( Writer &ch, bool bUP )
	   {
	      // Flush all on connect

	      if ( bUP )
	         Flush( ch );
	      else
	         ch.breakpoint(); // Do anything??
	   }


	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   bool _Encode( Writer     &writer, 
	                 std::string s, 
	                 u_int32_t   id, 
	                 u_int16_t   mPro=0 )
	   {
	      char      buf[4*K], *cp;
	      size_t    mSz;
	      _BinDict *h;

	      cp      = buf;
	      h       = (_BinDict *)cp;
	      cp     += sizeof( _BinDict );
	      h->_ID  = id;
	      h->_len = s.length();
	      ::memcpy( cp, s.data(), h->_len );
	      cp += h->_len;
	      mSz = ( cp - buf );
	      return writer.Send( buf, mSz, _PROTO_STRINGDICT, mPro );
	   }


	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Str2IdxMap _sdb;
	   u_int32_t  _idx;

	};  // class Encoder

#endif // DOXYGEN_OMIT


	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
	/**
	 * \brief Constructor - DeMarshall
	 *
	 * \param reader - Reader channel driving us
	 */
public:
	StringDict( Reader &reader ) :
	   Codec( reader ),
	   _decode(),
	   _encode()
	{
	   reader.RegisterProtocol( *this, _PROTO_STRINGDICT, "StringDict" );
	}

	/** 
	 * \brief Constructor - Marshall
	 * 
	 * \param writer - Writer we are encoding on
	 */
	StringDict( Writer &writer ) :
	   Codec( writer ),
	   _decode(),
	   _encode()
	{
	   writer.AddListener( _encode );
	}


	////////////////////////////////////
	// DeMarshall Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Retreives string name associated w/ index.
	 *
	 * \param idx - Unique string index
	 * \return String
	 */
	const char *GetString( u_int32_t idx )
	{
	   return _decode.GetString( idx );
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
	   yamrBuf &b = msg._Data;
	   bool     rc;

	   rc = ( msg._WireProtocol == _decode.protocol() );
	   if ( rc )
	      _decode.Decode( b );
	   return rc;
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
	   yamrBuf    &b = msg._Data;
	   u_int32_t   ix;
	   std::string s;
	   char        buf[K];

	   ix = _decode.GetIdx( b );
	   sprintf( buf, "%d=", ix );
	   s  = buf;
	   s += _decode.GetString( ix );
	   return std::string( s );
	} 


	////////////////////////////////////
	// Marshall Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Returns current dictionary size
	 *
	 * \return Current dictionary size
	 */
	size_t Size()
	{
	   return _encode.Size();
	}

	/**
	 * \brief Retreives unique string index from encoder.
	 *
	 * Index remains unique for the life of the session.
	 *
	 * \param str - String
	 * \return Unique string index
	 */
	u_int32_t GetStrIndex( const char *str )
	{
	   std::string s( str );

	   return GetStrIndex( s );
	}

	/**
	 * \brief Retreives unique string index from encoder.
	 *
	 * Index remains unique for the life of the session.
	 *
	 * \param str - String
	 * \return Unique string index
	 */
	u_int32_t GetStrIndex( std::string &str )
	{
	   return _writer ? _encode.GetIdx( *_writer, str.data() ) : 0;
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
	   // Flush Encoder (Refresh)

	   return _writer ? _encode.Flush( *_writer, MsgProto ) : false;
	}


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
private:
	Decoder _decode;
	Encoder _encode;

};  // class StringDict

} // namespace Data

} // namespace YAMR

#endif // __YAMR_StringDict_H 
