/******************************************************************************
*
*  IntList.hpp
*     libyamr Integer List IntList Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_IntList_H
#define __YAMR_IntList_H
#include <hpp/data/Data.hpp>


namespace YAMR
{

namespace Data
{

////////////////////////////////////////////////
//
//       c l a s s    I n t L i s t
//
////////////////////////////////////////////////

/**
 * \class IntList
 * \brief yamRecorder Integer List
 */
class IntList : public Codec
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
	      _idb()
	   {
	   }

	   ~Decoder()
	   {
	   }


	   ////////////////////////////////////
	   // Access / Operations
	   ////////////////////////////////////
	public:
	   Ints &idb()
	   {
	      return _idb;
	   }

	   bool Decode8( yamrBuf yb )
	   {
	      u_int32_t *num, i;
	      u_int8_t  *i8;
	      char      *cp;

	      /*
	       * typedef struct {
	       *    u_int32_t _Num;
	       *    u_int8_t  _List[_Num];
	       * } IntList8;
	       */

	      _idb.clear();
	      cp  = yb._data;
	      num = (u_int32_t *)cp;
	      cp += sizeof( u_int32_t );
	      i8  = (u_int8_t *)cp;
	      for ( i=0; i<*num; _idb.push_back( i8[i++] ) );
	      return true;
	   }

	   bool Decode16( yamrBuf yb )
	   {
	      u_int32_t *num, i;
	      u_int16_t *i16;
	      char      *cp;

	      /*
	       * typedef struct {
	       *    u_int32_t _Num;
	       *    u_int16_t _List[_Num];
	       * } IntList16;
	       */

	      _idb.clear();
	      cp  = yb._data;
	      num = (u_int32_t *)cp;
	      cp += sizeof( u_int32_t );
	      i16 = (u_int16_t *)cp;
	      for ( i=0; i<*num; _idb.push_back( i16[i++] ) );
	      return true;
	   }

	   bool Decode32( yamrBuf yb )
	   {
	      u_int32_t *num, i;
	      u_int32_t *i32;
	      char      *cp;

	      /*
	       * typedef struct {
	       *    u_int32_t _Num;
	       *    u_int32_t _List[_Num];
	       * } IntList32;
	       */

	      _idb.clear();
	      cp  = yb._data;
	      num = (u_int32_t *)cp;
	      cp += sizeof( u_int32_t );
	      i32 = (u_int32_t *)cp;
	      for ( i=0; i<*num; _idb.push_back( i32[i++] ) );
	      return true;
	   }



	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Ints _idb;

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
	      _idb(),
	      _maxIdx( 0 ),
	      _nAlloc( 0 )
	   {
	      _yb._data = (char *)0;
	      _yb._dLen = 0;
	   }

	   ~Encoder()
	   {
	      _idb.clear();
	      if ( _yb._data )
	         delete[] _yb._data;
	   }


	   ////////////////////////////////////
	   // Encoder Operations
	   ////////////////////////////////////
	public:
	   size_t Size()
	   {
	      return _idb.size();
	   }

	   void Add( u_int32_t idx )
	   {
	      _maxIdx = gmax( _maxIdx, idx );
	      _idb.push_back( idx );
	   }

	   bool Send( Codec &codec, u_int16_t mPro )
	   {
	      yamrBuf   yb;
	      u_int16_t wPro;

	      // 1) Encode, packing if possible

	      if ( _maxIdx <= _MAX_PACK8 ) {
	         yb   = _Encode8();
	         wPro = _PROTO_INTLIST8;
	      }
	      else if ( _maxIdx <= _MAX_PACK16 ) {
	         yb  = _Encode16();
	         wPro = _PROTO_INTLIST16;
	      }
	      else {
	         yb   = _Encode32();
	         wPro = _PROTO_INTLIST32;
	      }

	      // 2) Ship it; Free buffer allocated in _EncodeXX()

	      _idb.clear();
	      return codec.writer().Send( yb, wPro, mPro );
	   }


	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   yamrBuf _Encode8()
	   {
	      yamrBuf    yb;
	      char      *cp;  
	      u_int32_t *i32;  
	      u_int8_t   *idb;
	      size_t     i, mSz;

	      /*
	       * 1) Header : List Size as 32-bit int
	       * 2) Alloc reusable yamrBuf if  : List Size as 32-bit int
	       */
	      mSz  = sizeof( u_int32_t ); 
	      mSz += ( Size() * sizeof( u_int8_t ) );
	      yb   = _GetBuf( mSz );
	      cp   = yb._data;
	      i32  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      idb  = (u_int8_t *)cp;

	      // 3) Payload and return

	      *i32 = _idb.size();
	      for ( i=0; i<*i32; idb[i]=(u_int8_t)_idb[i], i++ );
	      return yb;
	   }

	   yamrBuf _Encode16()
	   {
	      yamrBuf    yb;
	      char      *cp;  
	      u_int32_t *i32;  
	      u_int16_t *idb;
	      size_t     i, mSz;

	      /*
	       * 1) Header : List Size as 32-bit int
	       * 2) Alloc reusable yamrBuf if  : List Size as 32-bit int
	       */
	      mSz  = sizeof( u_int32_t ); 
	      mSz += ( Size() * sizeof( u_int16_t ) );
	      yb   = _GetBuf( mSz );
	      cp   = yb._data;
	      i32  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      idb  = (u_int16_t *)cp;

	      // 3) Payload and return

	      *i32 = _idb.size();
	      for ( i=0; i<*i32; idb[i]=(u_int16_t)_idb[i], i++ );
	      return yb;
	   }

	   yamrBuf _Encode32()
	   {
	      yamrBuf    yb;
	      char      *cp;  
	      u_int32_t *i32;  
	      u_int32_t *idb;
	      size_t     i, mSz;

	      /*
	       * 1) Header : List Size as 32-bit int
	       * 2) Alloc reusable yamrBuf if  : List Size as 32-bit int
	       */
	      mSz  = sizeof( u_int32_t ); 
	      mSz += ( Size() * sizeof( u_int32_t ) );
	      yb   = _GetBuf( mSz );
	      cp   = yb._data;
	      i32  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      idb  = (u_int32_t *)cp;

	      // 3) Payload and return

	      *i32 = _idb.size();
	      for ( i=0; i<*i32; idb[i]=(u_int32_t)_idb[i], i++ );
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
	   Ints      _idb;
	   u_int32_t _maxIdx;
	   yamrBuf   _yb;
	   size_t    _nAlloc;

	};  // class Encoder

#endif // DOXYGEN_OMIT


	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor
	 * 
	 * \param reader - Reader channel driving us
	 * \param bRegister - true to register the protocol
	 */
	IntList( Reader &reader, bool bRegister=true ) :
	   Codec( reader ),
	   _decode(),
	   _encode()

	{
	   if ( bRegister ) {
	      reader.RegisterProtocol( *this, _PROTO_INTLIST8, "IntList8" );
	      reader.RegisterProtocol( *this, _PROTO_INTLIST16, "IntList16" );
	      reader.RegisterProtocol( *this, _PROTO_INTLIST32, "IntList32" );
	   }
	}

	/**
	 * \brief Constructor. 
	 *
	 * \param writer - Writer we are encoding on
	 */
	IntList( Writer &writer ) :
	   Codec( writer ),
	   _decode(),
	   _encode()
	{ ; }


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return decoded Integer List
	 *
	 * \return decoded Integer List
	 */
	Ints &intList()
	{
	   return _decode.idb();
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
	      case _PROTO_INTLIST8:  rc = _decode.Decode8( b );  break; 
	      case _PROTO_INTLIST16: rc = _decode.Decode16( b ); break; 
	      case _PROTO_INTLIST32: rc = _decode.Decode32( b ); break; 
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
	   Ints       &idb = _decode.idb();
	   std::string s;
	   char        buf[K], *cp;
	   size_t      i;

	   for ( i=0; i<idb.size(); i++ ) {
	      cp  = buf;
	      cp += i ? sprintf( cp, "," ) : 0;
	      cp += sprintf( cp, "%d", idb[i] );
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
	 * \brief Add integer to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param idx - Integer to add to list
	 * \see Send()
	 */
	void Add( u_int32_t idx )
	{
	   _encode.Add( idx );
	}

	/**
	 * \brief Add list of integers to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param lst - List of Integers to send
	 * \see Send()
	 */
	void Add( Ints &lst )
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

};  // class IntList

} // namespace Data

} // namespace YAMR

#endif // __YAMR_IntList_H
