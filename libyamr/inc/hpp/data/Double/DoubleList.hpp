/******************************************************************************
*
*  DoubleList.hpp
*     libyamr Double List DoubleList Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_DoubleList_H
#define __YAMR_DoubleList_H
#include <hpp/data/Data.hpp>


namespace YAMR
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
 * \brief yamRecorder Double List
 */
class DoubleList : public Codec
{
#ifndef DOXYGEN_OMIT
	////////////////////////////////////////////////
	//
	//       c l a s s    D e c o d e r
	//
	////////////////////////////////////////////////

	class Decoder
	{
	   ////////////////////////////////////
	   // Constructor / Destructor
	   ////////////////////////////////////
	public:
	   Decoder() : 
	      _ddb()
	   { ; }

	   ~Decoder()
	   { ; }


	   ////////////////////////////////////
	   // Access / Operations
	   ////////////////////////////////////
	public:
	   Doubles &ddb()
	   {
	      return _ddb;
	   }

	   bool Decode( yamrBuf yb )
	   {
	      u_int32_t *num, i;
	      double     dv, mul;
	      u_int64_t *i64;
	      char      *cp;

	      /*
	       * typedef struct {
	       *    u_int32_t _Num;
	       *    u_int64_t _List[_Num];
	       * } DoubleList;
	       */
	      _ddb.clear();
	      cp  = yb._data;
	      num = (u_int32_t *)cp;
	      cp += sizeof( u_int32_t );
	      /*
	       * Doubles off the wire at 1.0 / YAMR_DBL_PRECISION
	       */
	      i64 = (u_int64_t *)cp;
	      mul = 1.0 / YAMR_DBL_PRECISION;
	      for ( i=0; i<*num; i++ ) {
	         dv = mul * i64[i];
	         _ddb.push_back( dv );
	      }
	      return true;
	   }


	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Doubles _ddb;

	};  // class Decoder


	////////////////////////////////////////////////
	//
	//       c l a s s    E n c o d e r
	//
	////////////////////////////////////////////////

	class Encoder
	{
	   ////////////////////////////////////
	   // Constructor / Destructor
	   ////////////////////////////////////
	public:
	   Encoder() :
	      _ddb(),
	      _nAlloc( 0 )
	   {
	      _yb._data = (char *)0;
	      _yb._dLen = 0;
	   }

	   ~Encoder()
	   {
	      _ddb.clear();
	      if ( _yb._data )
	         delete[] _yb._data;
	   }


	   ////////////////////////////////////
	   // Encoder Operations
	   ////////////////////////////////////
	public:
	   size_t Size()
	   {
	      return _ddb.size();
	   }

	   void Add( double idx )
	   {
	      _ddb.push_back( idx );
	   }

	   bool Send( Codec &codec, u_int16_t mPro )
	   {
	      yamrBuf   yb;
	      u_int16_t wPro;

	      /*
	       * 1) Encode
	       * 2) Clear List
	       * 3) Ship it
	       */
	      yb   = _Encode();
	      wPro = _PROTO_DOUBLELIST;
	      _ddb.clear();
	      return codec.writer().Send( yb, wPro, mPro );
	   }


	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   yamrBuf _Encode()
	   {
	      yamrBuf    yb;
	      char      *cp;  
	      u_int32_t *i32;  
	      u_int64_t *ddb;
	      double     dv, mul;
	      size_t     i, mSz;

	      /*
	       * typedef struct {
	       *    u_int32_t _Num;
	       *    u_int64_t _List[_Num];
	       * } DoubleList;
	       *
	       * Alloc reusable yamrBuf if  : List Size as 32-bit int
	       */
	      mSz  = sizeof( u_int32_t ); 
	      mSz += ( Size() * sizeof( u_int64_t ) );
	      yb   = _GetBuf( mSz );
	      cp   = yb._data;
	      i32  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      /*
	       * Doubles on the wire at YAMR_DBL_PRECISION
	       */
	      *i32 = _ddb.size();
	      ddb  = (u_int64_t *)cp;
	      mul  = YAMR_DBL_PRECISION;
	      for ( i=0; i<*i32; i++ ) {
	         dv     = mul * _ddb[i];
	         ddb[i] = (u_int64_t)dv;
	      }
	      return yb;
	   }

	private:
	   yamrBuf _GetBuf( size_t mSz )
	   {
	      if ( mSz > _nAlloc ) {
	         if ( _yb._data )
	            delete[] _yb._data;
	         _yb._data = new char[mSz+4];
	         _yb._dLen = 0;
	         _nAlloc   = mSz;
	      }
	      _yb._dLen = mSz;
	      return _yb;
	   }

	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Doubles _ddb;
	   yamrBuf _yb;
	   size_t  _nAlloc;

	};  // class Encoder

#endif // DOXYGEN_OMIT


	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - DeMarshall
	 * 
	 * \param reader - Reader channel driving us
	 * \param bRegister - true to register the protocol
	 */
	DoubleList( Reader &reader, bool bRegister=true ) :
	   Codec( reader ),
	   _decode(),
	   _encode()
	{
	   if ( bRegister )
	      reader.RegisterProtocol( *this, _PROTO_DOUBLELIST, "DoubleList" );
	}

	/**
	 * \brief Constructor - Marshall
	 *
	 * \param writer - Writer we are encoding on
	 */
	DoubleList( Writer &writer ) :
	   Codec( writer ),
	   _decode(),
	   _encode()
	{ ; }


	////////////////////////////////////
	// DeMarshall Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded Double List
	 *
	 * \return decoded Integer List
	 */
	Doubles &doubleList()
	{
	   return _decode.ddb();
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

	   rc = false;
	   switch( msg._WireProtocol ) {
	      case _PROTO_DOUBLELIST:
	         rc = _decode.Decode( b );
	         break;
	   }
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
	   Doubles    &ddb = _decode.ddb();
	   std::string s;
	   char        buf[K], *cp;
	   size_t      i;

	   for ( i=0; i<ddb.size(); i++ ) {
	      cp  = buf;
	      cp += i ? sprintf( cp, "," ) : 0;
	      cp += sprintf( cp, "%.4g", ddb[i] );
	   } 
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
	 * \brief Add double to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param idx - Double to add to list
	 * \see Send()
	 */
	void Add( double idx )
	{
	   _encode.Add( idx );
	}

	/**
	 * \brief Add list of integers to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param lst - List of Double to send
	 * \see Send()
	 */
	void Add( Doubles &lst )
	{
	   size_t i;

	   for ( i=0; i<lst.size(); Add( lst[i++] ) );
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
	   return _encode.Send( *this, MsgProto );
	}


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
private:
	Decoder _decode;
	Encoder _encode;

}; // class DoubleList

} // namespace Data

} // namespace YAMR

#endif // __YAMR_DoubleList_H 
