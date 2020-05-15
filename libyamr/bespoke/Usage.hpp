/******************************************************************************
*
*  Usage.hpp
*     libyamr Usage Recording Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*     29 JUL 2019 jcs  Build  2: Sharable
*      9 DEC 2019 jcs  Build  3: No using namespace
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_Usage_Usage_H
#define __YAMR_Usage_Usage_H
#include <hpp/data/String/StringDict.hpp>

#ifndef DOXYGEN_OMIT
#define _PROTO_USAGE  0x0001 // YAMR::bespoke::Usage 
#endif // DOXYGEN_OMIT

namespace YAMR
{

namespace bespoke
{

////////////////////////////////////////////////
//
//        c l a s s    U s a g e
//
////////////////////////////////////////////////

/**
 * \class Usage
 * \brief Usage Data
 */
class Usage : public YAMR::Data::Codec
{
	////////////////////////////////////////////////
	//
	//        c l a s s    R e c o r d
	//
	////////////////////////////////////////////////

	/**
	 * \struct Record
	 *
	 * \brief String-based generic log message (Usage-like).
	 *
	 * All strings in this structure are "free-form"; There are no 
	 * restrictions on any string data.
	 *
	 * \see RecordBin
	 */
	typedef struct
	{
	   /** \brief UsageType : OPEN, CLOSE, PERMIT, DENY, etc. */ 
	   const char  *_UsageType;
	   /** \brief Username on ads */
	   const char  *_Username;
	   /** \brief TREP Service : IDN_RDF, ERT, etc. */
	   const char  *_Service;
	   /** \brief TREP Ticker */
	   const char  *_Ticker;
	   /** \brief TREP Quality of Service : REAL-TIME, DELAYED, etc. */
	   const char  *_QoS;
	   /** \brief Number of Extra Columns in Record::_xtraCols */
	   u_int32_t    _NumCols;
	   /** \brief Number of Extra Values in Record::_xtraVals */
	   u_int32_t    _NumVals;
	   /** \brief Array of Extra Column Names */
	   const char **_xtraCols;
	   /** \brief Array of Extra Column Values */
	   const char **_xtraVals;
	} Record;


	/**
	 * \struct RecordBin
	 *
	 * \brief String Index-based generic log message (Usage-like).
	 *
	 * All must have valid indices in the associated YAMR::String::Reader
	 *
	 * \see YAMR::String::Reader
	 */
	typedef struct
	{
	   /** \brief Usage Type string index */
	   u_int32_t  _UsageType;
	   /** \brief Username string index */
	   u_int32_t  _Username;
	   /** \brief Service string index */
	   u_int32_t  _Service;
	   /** \brief Ticker string index */
	   u_int32_t  _Ticker;
	   /** \brief Quality of Service string index */
	   u_int32_t  _QoS;
	   /** \brief Number of Extra Columns in RecordBin::_xtraCols */
	   u_int32_t  _NumCols;
	   /** \brief Number of Extra Values in RecordBin::_xtraVals */
	   u_int32_t  _NumVals;
	   /** \brief Array of Extra Column Name string indices */
	   u_int32_t *_xtraCols;
	   /** \brief Array of Extra Column Value string indices */
	   u_int32_t *_xtraVals;
	} RecordBin;


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
	   Decoder()
	   { ; }


	   ////////////////////////////////////
	   // Access / Operations
	   ////////////////////////////////////
	public:
	   Record &lg()
	   {
	      return _lg;
	   }

	   void Clear()
	   {
	      ::memset( &_lg, 0, sizeof( _lg ) );
	      _lg._xtraCols = _cols;
	      _lg._xtraVals = _vals;
	   }

	   bool Decode( yamrBuf yb, YAMR::Data::StringDict &dict )
	   {
	      const char *str;
	      char       *cp;
	      u_int32_t   i;
	      size_t      mSz;

	      Clear();
	      cp  = yb._data;
	      cp += _GetString( cp, dict, &_lg._UsageType );
	      cp += _GetString( cp, dict, &_lg._Username );
	      cp += _GetString( cp, dict, &_lg._Service );
	      cp += _GetString( cp, dict, &_lg._Ticker );
	      cp += _GetString( cp, dict, &_lg._QoS );
	      cp += _GetInt( cp, _lg._NumCols );
	      cp += _GetInt( cp, _lg._NumVals );
	      for ( i=0; i<_lg._NumCols; i++ ) {
	         cp      += _GetString( cp, dict, &str );
	         _cols[i] = str;
	      }
	      for ( i=0; i<_lg._NumVals; i++ ) {
	         cp      += _GetString( cp, dict, &str );
	         _vals[i] = str;
	      }
	      mSz  = cp - yb._data;
assert( mSz == yb._dLen );
	      return true;
	   }

	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   size_t _GetInt( char *cp, u_int32_t &val )
	   {
	      u_int32_t *i32;

	      i32 = (u_int32_t *)cp;
	      val = *i32;
	      return sizeof( u_int32_t );
	   }

	   size_t _GetString( char *cp, YAMR::Data::StringDict &dict, const char **val )
	   {
	      size_t    rc;
	      u_int32_t idx;

	      rc   = _GetInt( cp, idx );
	      *val = dict.GetString( idx );
	      return sizeof( idx );
	   }


	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Record  _lg;
	   const char *_vals[K];
	   const char *_cols[K];

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
	      _nAlloc( 0 )
	   {
	      _yb._data = (char *)0;
	      _yb._dLen = 0;
	      Clear();
	   }

	   ~Encoder()
	   {
	      if ( _yb._data )
	         delete[] _yb._data;
	   }


	   ////////////////////////////////////
	   // Access /  Operations
	   ////////////////////////////////////
	public:
	   RecordBin &bin()
	   {
	      return _bin;
	   }

	   void Clear()
	   {
	      ::memset( &_bin, 0, sizeof( _bin ) );
	   }

	   void Add( RecordBin &b )
	   {
	      _bin = b;
	   }

	   void Add( YAMR::Data::Codec &codec, Record &lg )
	   {
	      YAMR::Data::StringDict  &dict = codec.writer().strDict();
	      const char             **cdb  = lg._xtraCols;
	      const char             **vdb  = lg._xtraVals;
	      u_int32_t                i, nc, nv;

	      Clear();
	      nc = lg._NumCols;
	      nv = lg._NumVals;
	      _bin._UsageType = dict.GetStrIndex( lg._UsageType );
	      _bin._Username  = dict.GetStrIndex( lg._Username );
	      _bin._Service   = dict.GetStrIndex( lg._Service );
	      _bin._Ticker    = dict.GetStrIndex( lg._Ticker );
	      _bin._QoS       = dict.GetStrIndex( lg._QoS );
	      _bin._NumCols   = nc;
	      _bin._NumVals   = nv;
	      _bin._xtraCols  = _cols;
	      _bin._xtraVals  = _vals;
	      for ( i=0; i<nc; _cols[i]=dict.GetStrIndex( cdb[i] ), i++ );
	      for ( i=0; i<nv; _vals[i]=dict.GetStrIndex( vdb[i] ), i++ );
	   }

	   bool Send( YAMR::Data::Codec &codec, u_int16_t mPro )
	   {
	      yamrBuf   yb;
	      u_int16_t wPro;

	      /*
	       * 1) Encode
	       * 2) Clear List
	       * 3) Ship it
	       */
	      yb   = _Encode();
	      wPro = _PROTO_USAGE;
/*
 * 19-06-13 jcs  PerfTest : Reuse the packed binary struct
 *
	      Clear();
 */
	      return codec.writer().Send( yb, wPro, mPro );
	   }


	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   yamrBuf _Encode()
	   {
	      yamrBuf    yb;
	      char      *bp, *cp;  
	      u_int32_t *cdb, *vdb, i, nc, nv;
	      size_t     eSz;

	      /*
	       * Alloc reusable yamrBuf
	       */
	      cdb  = _bin._xtraCols;
	      vdb  = _bin._xtraVals;
	      nc   = _bin._NumCols;
	      nv   = _bin._NumVals;
	      eSz  = sizeof( _bin );
	      eSz += ( ( nc+nv ) * sizeof( u_int32_t ) );
	      yb   = _GetBuf( eSz );
	      bp   = yb._data;
	      /*
	       * Fill 'er in
	       */
	      cp   = bp;
	      cp  += _SetInt( cp, _bin._UsageType );
	      cp  += _SetInt( cp, _bin._Username );
	      cp  += _SetInt( cp, _bin._Service );
	      cp  += _SetInt( cp, _bin._Ticker );
	      cp  += _SetInt( cp, _bin._QoS );
	      cp  += _SetInt( cp, _bin._NumCols );
	      cp  += _SetInt( cp, _bin._NumVals );
	      for ( i=0; i<nc; cp += _SetInt( cp, cdb[i++] ) );
	      for ( i=0; i<nv; cp += _SetInt( cp, vdb[i++] ) );
	      yb._dLen = cp - bp;
assert( yb._dLen < eSz );
	      return yb;
	   }

	   size_t _SetInt( char *cp, u_int32_t val )
	   {
	      u_int32_t *i32;

	      i32  = (u_int32_t *)cp;
	      *i32 = val;
	      return sizeof( val );
	   }

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
	   RecordBin _bin;
	   yamrBuf       _yb;
	   size_t        _nAlloc;
	   u_int32_t     _cols[K];
	   u_int32_t     _vals[K];

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
	 */
	Usage( Reader &reader ) :
	   YAMR::Data::Codec( reader ),
	   _decode(),
	   _encode()
	{
	   if ( !reader.HasStringDict() )
	      reader.RegisterStringDict( new YAMR::Data::StringDict( reader ) );
	   reader.RegisterProtocol( *this, _PROTO_USAGE, "Usage" );
	}

	/**
	 * \brief Constructor - Marshall
	 *
	 * \param writer - Writer we are encoding on
	 */
	Usage( Writer &writer ) :
	   YAMR::Data::Codec( writer ),
	   _decode(),
	   _encode()
	{
	   if ( !writer.HasStringDict() )
	      writer.RegisterStringDict( new YAMR::Data::StringDict( writer ) );
	}


	////////////////////////////////////
	// DeMarshall Access / Operations
	////////////////////////////////////
	Record &lg()
	{
	   return _decode.lg();
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
	   yamrBuf  &b = msg._Data;
	   u_int16_t pro;
	   bool      rc;

	   // Pre-condition

	   if ( !_reader )
	      return false;

	   // 1) Dictionary??

	   YAMR::Data::StringDict &dict = reader().strDict();

	   pro = msg._WireProtocol;
	   if ( pro == _PROTO_STRINGDICT )
	      return dict.Decode( msg );

	   // 2) Us

	   rc = false;
	   switch( pro ) {
	      case _PROTO_USAGE:
	         rc = _decode.Decode( b, dict );
	         break;
	      default:
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
	   Record  &u   = lg();
	   const char **cdb = u._xtraCols;
	   const char **vdb = u._xtraVals;
	   u_int16_t    pro;
	   std::string  s;
	   size_t       i, nx;

	   // Pre-condition

	   if ( !_reader )
	      return s;

	   // 1) Dictionary??

	   YAMR::Data::StringDict &dict = reader().strDict();

	   pro = msg._WireProtocol;
	   if ( pro == _PROTO_STRINGDICT )
	      return dict.Dump( msg, dmpTy );

	   // 2) Else us

	   _DumpField( s, "UsageType", u._UsageType );
	   _DumpField( s, "Username", u._Username );
	   _DumpField( s, "Service", u._Service );
	   _DumpField( s, "Ticker", u._Ticker );
	   _DumpField( s, "QoS", u._QoS );
	   nx = gmin( u._NumCols, u._NumVals );
	   for ( i=0; i<nx; _DumpField( s, cdb[i], vdb[i] ), i++ );
	   return std::string( s );
	}


	////////////////////////////////////
	// Marshall Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Returns most recently filled RecordBin struct
	 *
	 * \return Most recently filled RecordBin struct
	 */
	RecordBin &lgBin()
	{
	   return _encode.bin();
	}

	/**
	 * \brief Log string-ified Generic Message
	 *
	 * \param usageType  - User-defined log message type
	 * \param username - Name of user recording message
	 * \param service - Service name supplying the data stream (e.g., BLOOMBERG)
	 * \param ticker - Ticker name (e.g., CSCO EQUITY, EUR CURNCY)
	 * \param QoS - Quality of Service
	 * \param xtraCols - List of extra user-defined columns to add
	 * \param xtraVals - List of extra user-defined values to add
	 * \return true if consumed; false if not
	 */
	bool LogGeneric( const char  *usageType,
	                 const char  *username,
	                 const char  *service,
	                 const char  *ticker,
	                 const char  *QoS,
	                 const char **xtraCols = (const char **)0,
	                 const char **xtraVals = (const char **)0 )
	{
	   u_int32_t nc, nv;

	   _lg._UsageType = usageType;
	   _lg._Username  = username;
	   _lg._Service   = service;
	   _lg._Ticker    = ticker;
	   _lg._QoS       = QoS;
	   _lg._xtraCols  = _cols;
	   _lg._xtraVals  = _vals;
	   nc             = 0;
	   nv             = 0;
	   if ( xtraCols )
	      for ( nc=0; xtraCols[nc]; _cols[nc] = xtraCols[nc], nc++ );
	   if ( xtraCols )
	      for ( nv=0; xtraVals[nv]; _vals[nv] = xtraVals[nv], nv++ );
	   _lg._NumCols   = nc;
	   _lg._NumVals   = nv;
	   if ( _writer )
	      _encode.Add( *this, _lg );
	   return Send();
	}

	/**
	 * \brief Log string-ified Generic Message
	 *
	 * \param usageType  - User-defined log message type
	 * \param username - Name of user recording message
	 * \param service - Service name supplying the data stream (e.g., BLOOMBERG)
	 * \param ticker - Ticker name (e.g., CSCO EQUITY, EUR CURNCY)
	 * \param QoS - Quality of Service
	 * \param xtraCols - List of extra user-defined columns to add
	 * \param xtraVals - List of extra user-defined values to add
	 * \return true if consumed; false if not
	 */
	bool LogGeneric( std::string         usageType,
	                 std::string         username,
	                 std::string         service,
	                 std::string         ticker,
	                 std::string         QoS,
	                 YAMR::Data::Strings xtraCols,
	                 YAMR::Data::Strings xtraVals )
	{
	   size_t i;

	   _lg._UsageType = usageType.data();
	   _lg._Username  = username.data();
	   _lg._Service   = service.data();
	   _lg._Ticker    = ticker.data();
	   _lg._QoS       = QoS.data();
	   _lg._NumCols   = xtraCols.size();
	   _lg._NumVals   = xtraVals.size();
	   _lg._xtraCols  = _cols;
	   _lg._xtraVals  = _vals;
	   for ( i=0; i<_lg._NumCols; _cols[i] = xtraCols[i].data(), i++ );
	   for ( i=0; i<_lg._NumVals; _vals[i] = xtraVals[i].data(), i++ );
	   if ( _writer )
	      _encode.Add( *this, _lg );
	   return Send();
	}

	/**
	 * \brief Log pre-built Generic Message
	 *
	 * \param lg - Pre-built RecordBin message
	 * \return true if consumed; false if not
	 */
	bool LogGeneric( RecordBin &lg )
	{
	   _encode.Add( lg );
	   return Send();
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

	   return _writer ? _encode.Send( *this, MsgProto ) : false;
	 }


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	void _DumpField( std::string &s, const char *key, const char *val )
	{
	   s += key;
	   s += "=";
	   s += val;
	   s += ",";
	}


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
private:
	Decoder     _decode;
	Encoder     _encode;
	Record      _lg;
	const char *_cols[K];
	const char *_vals[K];

};  // class Usage

} // namespace bespoke

} // namespace YAMR

#endif // __YAMR_Usage_Marshall_H 
