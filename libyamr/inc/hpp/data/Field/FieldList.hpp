/******************************************************************************
*
*  FieldList.hpp
*     libyamr Field List FieldList Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*     29 JUL 2019 jcs  Build  2: Sharable
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_FieldList_H
#define __YAMR_FieldList_H
#include <hpp/data/String/StringDict.hpp>
#include <assert.h>


namespace YAMR
{

namespace Data
{

////////////////////////////////////////////////
//
//        c l a s s    F i e l d
//
////////////////////////////////////////////////

/**
 * \class Field
 * \brief A single ( FID, Value ) Field
 */
class Field
{
public:
	/**
	 * \enum Type
	 * \brief Field Type : 32-bit int, double, string, etc.
	 */
	typedef enum {
	   /** \brief 8-bit integer */
	   ty_int8   = 0x01,
	   /** \brief 16-bit integer */
	   ty_int16  = 0x02,
	   /** \brief 32-bit integer */
	   ty_int32  = 0x03,
	   /** \brief 64-bit integer */
	   ty_int64  = 0x04,
	   /** 
	    * \brief Single-precision floating point number
	    *
	    * On the wire this is a 32-bit int with 4 implied decimal places
	    */
	   ty_float  = 0x05,
	   /** 
	    * \brief Double-precision floating point number
	    *
	    * On the wire this is a 64-bit int with 10 implied decimal places
	    */
	   ty_double = 0x06,
	   /** \brief string from dictionary*/
	   ty_string = 0x07,
	   /** \brief Date as YYYYMMDD in u_int32_t */
	   ty_date   = 0x08,
	   /** \brief Time as millis since midnight in u_int32_t */
	   ty_time     = 0x09,
	   /** \brief ( Date, Time ) as Unix Time in u_int32_t w/o millis */
	   ty_dateTime = 0x0a
	} Type;

	/**
	 * \struct Date
	 * \brief Date of Year : ( Year, Month, Day )
	 */
	typedef struct {
	   /** \brief Year : 4-digit */
	   int _Year;
	   /** \brief Month : 1 thru 12 */
	   int _Mon;
	   /** \brief Day of Month : 1 thru 31 */
	   int _Day;
	} Date;

	/**
	 * \struct Time
	 * \brief Time of Day : ( Hour, Minute, Second, Millisecond )
	 */
	typedef struct {
	   /** \brief Hour : 0 thru 23 */
	   int _Hour;
	   /** \brief Minute : 0 thru 59 */
	   int _Min;
	   /** \brief Second : 0 thru 59 */
	   int _Sec;
	   /** \brief MilliSecond : 0 thru 999 */
	   int _Milli;
	} Time;

	/**
	 * \struct DateTime
	 * \brief Unix Time (no millis)
	 * \see Date
	 * \see Time
	 */
	typedef struct {
	   /** \brief YAMR Date */
	   Date _Date;
	   /** \brief YAMR Time */
	   Time _Time;
	} DateTime;

	/**
	 * \union Value
	 * \brief Field Value : 32-bit int, double ,string etc.
	 */
	typedef union {
	   /** \brief Field::ty_int8 */
	   u_int8_t    v_int8;
	   /** \brief Field::ty_int16 */
	   u_int16_t   v_int16;
	   /** \brief Field::ty_int32 */
	   u_int32_t   v_int32;
	   /** \brief Field::ty_int64 */
	   u_int64_t   v_int64;
	   /** \brief Field::ty_float */
	   float       v_float;
	   /** \brief Field::ty_double */
	   double      v_double;
	   /** \brief Field::ty_string */
	   const char *v_string;
	   /** \brief Field::ty_date */
	   Date        v_date;
	   /** \brief Field::ty_time */
	   Time        v_time;
	   /** \brief Field::ty_dateTime */
	   DateTime    v_dtTm;
	} Value;

	/** \brief Unique Field ID */
	int     _Fid;
	/** \brief Field Type : 32-bit int, double, string, etc. */
	Type _Type;
	/** \brief Field Value */
	Value  _Value;

};  // class Field

#ifndef DOXYGEN_OMIT

#define _MAX_WIREFLD_SIZE 16

typedef std::vector<Field> Fields;

#endif // DOXYGEN_OMIT



////////////////////////////////////////////////
//
//       c l a s s    F i e l d L i s t
//
////////////////////////////////////////////////

/**
 * \class FieldList
 * \brief Field ( FID, Value ) List
 */
class FieldList : public Codec
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
	      _flds(),
	      _svc(),
	      _tkr()
	   { ; }


	   ////////////////////////////////////
	   // Access / Operations
	   ////////////////////////////////////
	public:
	   const char *svc()
	   {
	      return _svc.data();
	   }

	   const char *tkr()
	   {
	      return _tkr.data();
	   }

	   Fields &flds()
	   {
	      return _flds;
	   }

	   bool Decode( yamrBuf yb, StringDict &dict )
	   {
	      char      *bp, *cp;
	      u_int32_t *i32, *svc, *tkr;
	      size_t     i, mSz;

	      /*
	       * Max field on wire; Always compacted based on FID and data type
	       *    typedef struct {
	       *       u_int8_t[0:3] _FidType;  // Type
	       *       u_int8_t[4:7] _ValType;  // Type
	       *       <intType>     _FID;
	       *       <intType>     _Value;
	       *    } WireField;
	       * 
	       *  class _MaxWireField
	       *  {
	       *  public:
	       *     u_int16_t _FidValType;
	       *     u_int32_t _FID;
	       *     u_int64_t _Val;
	       *  
	       *  }; // _MaxWireField
	       *
	       * thus, sizeof( _MaxWireField ) = _MAX_WIREFLD_SIZE = 16
	       *
	       * typedef struct {
	       *    u_int32_t     _Num;
	       *    u_int32_t     _Svc;  // From StrDict
	       *    u_int32_t     _Tkr;  // From StrDict
	       *    _MaxWireField _Flds[_Num]; // Really _Encode()'ed
	       * } FieldList;
	       */
	      bp   = yb._data;
	      cp   = bp;
	      i32  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      svc  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      tkr  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      _svc = dict.GetString( *svc );
	      _tkr = dict.GetString( *tkr );
	      /*
	       * Fields on the wire are packed based on FID and Value Size
	       */
	      _flds.clear();
	      for ( i=0; i<*i32; cp += _DecodeField( cp, dict ), i++ );
	      mSz = cp - bp;
assert( mSz == yb._dLen );
	      return true;
	   }


	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   int _DecodeField( char *bp, StringDict &dict )
	   {
	      Field         f;
	      Field::Value &v  = f._Value;
	      Field::Date   dt;
	      Field::Time   tm;
	      Field::Type   ft, vt;
	      struct tm     lt;
	      char         *cp;
	      u_int8_t     *i8;
	      u_int16_t    *i16;
	      u_int32_t    *i32, unx;
	      u_int64_t    *i64;
	      static size_t _sz8  = sizeof( u_int8_t );
	      static size_t _sz16 = sizeof( u_int16_t );
	      static size_t _sz32 = sizeof( u_int32_t );
	      static size_t _sz64 = sizeof( u_int64_t );

	      /****************************************************************
	       *
	       * Field on wire; Always compacted based on FID and data type
	       *    typedef struct {
	       *       u_int8_t[0:3] _FidType;  // Type
	       *       u_int8_t[4:7] _ValType;  // Type
	       *       <intType>     _FID;
	       *       <intType>     _Value;
	       *    } WireField;
	       *
	       ***************************************************************/

	      /*
	       * 1) Field, Value Packing based on Type
	       */
	      cp  = bp;
	      i8  = (u_int8_t *)cp;
	      cp += _sz8;
	      ft  = (Field::Type)( ( *i8 & 0xf0 ) >> 4 );
	      vt  = (Field::Type)( *i8 & 0x0f );
	      /*
	       * 2) FID
	       */
	      switch( ft ) {
	         case Field::ty_int8:
	            i8     = (u_int8_t *)cp;
	            cp    += _sz8;
	            f._Fid = *i8;
	            break;
	         case Field::ty_int16:
	            i16    = (u_int16_t *)cp;
	            cp    += _sz16;
	            f._Fid = *i16;
	            break;
	         case Field::ty_int32:
	            i32    = (u_int32_t *)cp;
	            cp    += _sz32;
	            f._Fid = *i32;
	            break;
	         default:
assert( 0 );
	      }
	      /*
	       * 3) Value; All packed as ints on wire
	       */
	      f._Type = vt;
	      switch( f._Type ) {
	         case Field::ty_int8:
	            i8       = (u_int8_t *)cp;
	            cp      += _sz8;
	            v.v_int8 = *i8;
	            break;
	         case Field::ty_int16:
	            i16       = (u_int16_t *)cp;
	            cp       += _sz16;
	            v.v_int16 = *i16;
	            break;
	         case Field::ty_int32:
	            i32       = (u_int32_t *)cp;
	            cp       += _sz32;
	            v.v_int32 = *i32;
	            break;
	         case Field::ty_int64:
	            i64       = (u_int64_t *)cp;
	            cp       += _sz64;
	            v.v_int64 = *i64;
	            break;
	         case Field::ty_float:
	            i32       = (u_int32_t *)cp;
	            cp       += _sz32;
	            v.v_float = ( 1.0 / YAMR_FLOAT_PRECISION ) * *i32;
	            break;
	         case Field::ty_double:
	            i64        = (u_int64_t *)cp;
	            cp        += _sz64;
	            v.v_double = ( 1.0 / YAMR_DBL_PRECISION ) * *i64;
	            break;
	         case Field::ty_string:
	            i32        = (u_int32_t *)cp;
	            cp        += _sz32;
	            v.v_string = dict.GetString( *i32 );
	            break;
	         case Field::ty_date:
	            i32      = (u_int32_t *)cp;
	            cp      += _sz32;
	            dt._Year = *i32 / 10000;
	            dt._Mon  = ( *i32 / 10000 ) % 100;
	            dt._Day  = *i32 % 100;
	            v.v_date = dt;
	            break;
	         case Field::ty_time:
	            i32       = (u_int32_t *)cp;
	            cp       += _sz32;
	            unx       = *i32;
	            tm._Milli = unx % 1000;
	            unx      /= 1000;
	            tm._Hour  = unx / 3600;
	            tm._Min   = ( unx / 3600 ) % 60;
	            tm._Sec   = unx % 60;
	            v.v_time  = tm;
	            break;
	         case Field::ty_dateTime:
	            i32        = (u_int32_t *)cp;
	            cp        += _sz32;
	            ::localtime_r( (time_t *)i32, &lt );
	            dt._Year       = lt.tm_year + 1900;
	            dt._Mon        = lt.tm_mon + 1;
	            dt._Day        = lt.tm_mday;
	            tm._Hour       = lt.tm_hour;
	            tm._Min        = lt.tm_min;
	            tm._Sec        = lt.tm_sec;
	            tm._Milli      = 0;
	            v.v_dtTm._Date = dt;
	            v.v_dtTm._Time = tm;
	      }
	      /*
	       * 4) Return length of field on wire
	       */
	      _flds.push_back( f );
	      return( cp-bp );
	   }


	   ////////////////////////////////////
	   // Private Members
	   ////////////////////////////////////
	private:
	   Fields      _flds;
	   std::string _svc;
	   std::string _tkr;

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
	      _svc(),
	      _tkr(),
	      _fdb(),
	      _nAlloc( 0 )
	   {
	      _yb._data = (char *)0;
	      _yb._dLen = 0;
	   }

	   ~Encoder()
	   {
	      _fdb.clear();
	      if ( _yb._data )
	         delete[] _yb._data;
	   }


	   ////////////////////////////////////
	   // Encoder Operations
	   ////////////////////////////////////
	public:
	   void Init( const char *svc, const char *tkr )
	   {
	      Clear();
	      _svc = svc;
	      _tkr = tkr;
	   }

	   void Clear()
	   {
	      _fdb.clear();
	   }

	   size_t Size()
	   {
	      return _fdb.size();
	   }

	   void Add( int fid, u_int8_t i8 )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid   = fid;
	      f._Type  = Field::ty_int8;
	      v.v_int8 = i8;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, u_int16_t i16 )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid    = fid;
	      f._Type   = Field::ty_int16;
	      v.v_int16 = i16;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, u_int32_t i32 )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid    = fid;
	      f._Type   = Field::ty_int32;
	      v.v_int32 = i32;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, u_int64_t i64 )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid    = fid;
	      f._Type   = Field::ty_int64;
	      v.v_int64 = i64;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, double val )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid     = fid;
	      f._Type    = Field::ty_double;
	      v.v_double = val;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, float val )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid    = fid;
	      f._Type   = Field::ty_float;
	      v.v_float = val;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, const char *str, StringDict &dict )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid     = fid;
	      f._Type    = Field::ty_string;
	      v.v_int32  = dict.GetStrIndex( str );
	      _fdb.push_back( f );
	   }

	   void Add( int fid, Field::Date dt )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid   = fid;
	      f._Type  = Field::ty_date;
	      v.v_date = dt;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, Field::Time tm )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid   = fid;
	      f._Type  = Field::ty_time;
	      v.v_time = tm;
	      _fdb.push_back( f );
	   }

	   void Add( int fid, Field::DateTime dtTm )
	   {
	      Field         f;
	      Field::Value &v = f._Value;

	      f._Fid   = fid;
	      f._Type  = Field::ty_dateTime;
	      v.v_dtTm = dtTm;
	      _fdb.push_back( f );
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
	      yb   = _Encode( codec );
	      wPro = _PROTO_FIELDLIST;
	      Clear();
	      return codec.writer().Send( yb, wPro, mPro );
	   }


	   ////////////////////////////////////
	   // Helpers
	   ////////////////////////////////////
	private:
	   yamrBuf _Encode( Codec &codec )
	   {
	      StringDict &dict = codec.writer().strDict();
	      yamrBuf     yb;
	      char       *bp, *cp;  
	      u_int32_t  *i32, *svc, *tkr;
	      size_t      i, eSz;

	      /*
	       * Max field on wire; Always compacted based on FID and data type
	       *    typedef struct {
	       *       u_int8_t[0:3] _FidType;  // Type
	       *       u_int8_t[4:7] _ValType;  // Type
	       *       <intType>     _FID;
	       *       <intType>     _Value;
	       *    } WireField;
	       * 
	       *  class _MaxWireField
	       *  {
	       *  public:
	       *     u_int16_t _FidValType;
	       *     u_int32_t _FID;
	       *     u_int64_t _Val;
	       *  
	       *  }; // _MaxWireField
	       *
	       * thus, sizeof( _MaxWireField ) = _MAX_WIREFLD_SIZE = 16
	       *
	       * typedef struct {
	       *    u_int32_t     _Num;
	       *    u_int32_t     _Svc;  // From StrDict
	       *    u_int32_t     _Tkr;  // From StrDict
	       *    _MaxWireField _Flds[_Num]; // Really _Encode()'ed
	       * } FieldList;
	       *
	       * Alloc reusable yamrBuf if  : List Size as 32-bit int
	       */
	      eSz  = ( 3 * sizeof( u_int32_t ) ); 
	      eSz += ( Size() * _MAX_WIREFLD_SIZE );
	      yb   = _GetBuf( eSz );
	      bp   = yb._data;
	      cp   = bp;
	      i32  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      svc  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      tkr  = (u_int32_t *)cp;
	      cp  += sizeof( u_int32_t );
	      /*
	       * Fields on the wire at YAMR_DBL_PRECISION
	       */
	      *i32 = _fdb.size();
	      *svc = dict.GetStrIndex( _svc.data() );
	      *tkr = dict.GetStrIndex( _tkr.data() );
	      for ( i=0; i<*i32; cp += _EncodeField( cp, _fdb[i++] ) );
	      yb._dLen = cp - yb._data;
assert( yb._dLen <= eSz );
	      return yb;
	   }

	   int _EncodeField( char *bp, Field f )
	   {
	      Field::Value &v  = f._Value;
	      Field::Date   dt;
	      Field::Time   tm;
	      Field::Type   ft, vt;
	      struct tm     lt;
	      time_t        now;
	      char         *cp;
	      int           fid;
	      u_int8_t     *i8;
	      u_int16_t    *i16;
	      u_int32_t    *i32;
	      u_int64_t    *i64;
	      float         r32;
	      double        r64;
	      static size_t _sz8  = sizeof( u_int8_t );
	      static size_t _sz16 = sizeof( u_int16_t );
	      static size_t _sz32 = sizeof( u_int32_t );
	      static size_t _sz64 = sizeof( u_int64_t );

	      /****************************************************************
	       *
	       * Field on wire; Always compacted based on FID and data type
	       *    typedef struct {
	       *       u_int8_t[0:3] _FidType;  // Type
	       *       u_int8_t[4:7] _ValType;  // Type
	       *       <intType>     _FID;
	       *       <intType>     _Value;
	       *    } WireField;
	       *
	       ***************************************************************/

	      /*
	       * 1) Field, Value Packing based on Type
	       */
	      ft  = Field::ty_int32;
	      fid = f._Fid;
	      if ( fid < _MAX_PACK8 )
	         ft = Field::ty_int8;
	      else if ( fid < _MAX_PACK16 )
	         ft = Field::ty_int16;
	      vt  = f._Type;
	      cp  = bp;
	      i8  = (u_int8_t *)cp;
	      *i8 = ( ft << 4 ) | vt;
	      cp += _sz8;
	      /*
	       * 2) Pack FID
	       */
	      switch( ft ) {
	         case Field::ty_int8:
	            i8  = (u_int8_t *)cp;
	            *i8 = fid;
	            cp += _sz8;
	            break;
	         case Field::ty_int16:
	            i16  = (u_int16_t *)cp;
	            *i16 = fid;
	            cp  += _sz16;
	            break;
	         case Field::ty_int32:
	            i32  = (u_int32_t *)cp;
	            *i32 = fid;
	            cp  += _sz32;
	            break;
	         default:
assert( 0 );
	            break;
	      }
	      /*
	       * 3) Pack Value : Pack all to ints
	       */
	      switch( vt ) {
	         case Field::ty_int8:
	            i8  = (u_int8_t *)cp;
	            *i8 = v.v_int8;
	            cp += _sz8;
	            break;
	         case Field::ty_int16:
	            i16  = (u_int16_t *)cp;
	            *i16 = v.v_int16;
	            cp  += _sz16;
	            break;
	         case Field::ty_int32:
	            i32  = (u_int32_t *)cp;
	            *i32 = v.v_int32;
	            cp  += _sz32;
	            break;
	         case Field::ty_int64:
	            i64  = (u_int64_t *)cp;
	            *i64 = v.v_int64;
	            cp  += _sz64;
	            break;
	         case Field::ty_float:
	            i32  = (u_int32_t *)cp;
	            r32  = v.v_float * YAMR_FLOAT_PRECISION;
	            *i32 = (u_int32_t)r32;
	            cp  += _sz32;
	            break;
	         case Field::ty_double:
	            i64  = (u_int64_t *)cp;
	            r64  = v.v_double * YAMR_DBL_PRECISION;
	            *i64 = (u_int64_t)r64;
	            cp  += _sz64;
	            break;
	         case Field::ty_string:
	            i32  = (u_int32_t *)cp;
	            *i32 = v.v_int32;
	            cp  += _sz32;
	            break;
	         case Field::ty_date:
	            i32  = (u_int32_t *)cp;
	            dt   = v.v_date;
	            *i32 = ( dt._Year * 10000 ) + ( dt._Mon * 100 ) + dt._Day;
	            cp  += _sz32;
	            break;
	         case Field::ty_time:
	            i32  = (u_int32_t *)cp;
	            tm   = v.v_time;
	            *i32 = ( tm._Hour * 3600 ) + ( tm._Min * 60 ) + tm._Sec;
	            *i32 = ( *i32 * 1000 ) + tm._Milli;
	            cp  += _sz32;
	            break;
	         case Field::ty_dateTime:
	            i32  = (u_int32_t *)cp;
	            dt   = v.v_dtTm._Date;
	            tm   = v.v_dtTm._Time;
	            now  = ::yamr_TimeSec();
	            ::localtime_r( &now, &lt );
	            lt.tm_sec  = tm._Sec;
	            lt.tm_min  = tm._Min;
	            lt.tm_hour = tm._Hour;
	            lt.tm_mday = dt._Day;
	            lt.tm_mon  = dt._Mon - 1;
	            lt.tm_year = dt._Year - 1900;
	            *i32 = ::mktime( &lt );
	            cp  += _sz32;
	            break;
	      }
	      /*
	       * 4) Return length of field on wire
	       */
	      return( cp-bp );
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
	   std::string _svc;
	   std::string _tkr;
	   Fields      _fdb;
	   yamrBuf     _yb;
	   size_t      _nAlloc;

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
	FieldList( Reader &reader ) :
	   Codec( reader ),
	   _decode(),
	   _encode()
	{
	   if ( !reader.HasStringDict() )
	      reader.RegisterStringDict( new StringDict( reader ) );
	   reader.RegisterProtocol( *this, _PROTO_FIELDLIST, "FieldList" );
	}

	/**
	 * \brief Constructor - Marshall
	 *
	 * \param writer - Writer we are encoding on
	 */
	FieldList( Writer &writer ) :
	   Codec( writer ),
	   _decode(),
	   _encode()
	{
	   if ( !writer.HasStringDict() )
	      writer.RegisterStringDict( new StringDict( writer ) );
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return Service Name of this FieldList
	 *
	 * \return Service Name of this FieldList
	 */
	const char *svc()
	{
	   return _decode.svc();
	}

	/**
	 * \brief Return Ticker Name of this FieldList
	 *
	 * \return Ticker Name of this FieldList
	 */
	const char *tkr()
	{
	   return _decode.tkr();
	}

	/**
	 * \brief Return decoded Field List
	 *
	 * \return decoded Integer List
	 */
	Fields &fldList()
	{
	   return _decode.flds();
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
	   yamrBuf    &b = msg._Data;
	   u_int16_t   pro;
	   bool        rc;

	   // 1) Dictionary??

	   pro = msg._WireProtocol;
	   if ( pro == _PROTO_STRINGDICT )
	      return dict.Decode( msg );

	   // 2) Us

	   rc = false;
	   switch( pro ) {
	      case _PROTO_FIELDLIST:
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
	   Field       f;
	   Fields     &fdb = fldList();
	   u_int16_t   pro;
	   std::string s;
	   char        buf[K], fld[K], *cp;
	   size_t      i;

	   // 1) Dictionary??

	   pro = msg._WireProtocol;
	   if ( pro == _PROTO_STRINGDICT )
	      return reader().strDict().Dump( msg, dmpTy );

	   /*
	    * 2a) Header
	    */
	   strcpy( buf, "" );
	   cp   = buf;
	   switch( dmpTy ) {
	      case dump_none:
	      case dump_verbose:
	         sprintf( buf, "( %s, %s )", svc(), tkr() );
	         break;
	      case dump_CSV:
	         sprintf( buf, "%s,%s,", svc(), tkr() );
	         break;
	      case dump_JSON:
	         cp  = buf;
	         cp += sprintf( cp, "{ \"Service\" : \"%s\",\n", svc() );
	         cp += sprintf( cp, "  \"Ticker\"  : \"%s\",\n", tkr() );
	         break;
	      case dump_JSONmin:
	         cp  = buf;
	         cp += sprintf( cp, "{ \"Service\":\"%s\", ", svc() );
	         cp += sprintf( cp, "\"Ticker\":\"%s\", ", tkr() );
	         break;
	   } 
	   s += buf;
	   /*
	    * 2b) Fields
	    */
	   for ( i=0; i<fdb.size(); i++ ) {
	      f   = fdb[i];
	      cp  = buf;
	      _FieldValue( fld, f );
	      switch( dmpTy ) {
	         case dump_none:
	         case dump_verbose:
	            cp += sprintf( cp, "\n   [%04d] %s", f._Fid, fld );
	            break;
	         case dump_CSV:
	            cp += sprintf( cp, "%d=%s,", f._Fid, fld );
	            break;
	         case dump_JSON:
	            cp += sprintf( cp, "  \"%d\"  : \"%s\",\n", f._Fid, fld );
	            break;
	         case dump_JSONmin:
	            cp += sprintf( cp, "\"%d\":\"%s\", ", f._Fid, fld );
	            break;
	      }
	      s  += buf;
	   }
	   /*
	    * 2c) Trailer
	    */
	   switch( dmpTy ) {
	      case dump_none:
	      case dump_verbose:
	      case dump_CSV:
	         break;
	      case dump_JSON:
	         s.erase( s.size()-2 ); // ,\n
	         s += "\n}";
	         break;
	      case dump_JSONmin:
	         s.erase( s.size()-2 ); // ,
	         s += "}";
	         break;
	   } 
	   s += "\n"; 
	   return std::string( s );
	} 

#ifndef DOXYGEN_OMIT
	int _sprintf( char *buf, float r32 )
	{
	   return _sprintf( buf, (double)r32 );
	}

	int _sprintf( char *buf, double r64 )
	{
	   int i, sz;

	   // Up to 6 sigFig

	   sz = sprintf( buf, "%.6f", r64 );
	   for ( i=0; i<4; i++ ) {
	      if ( buf[sz-(i+1)] != '0' )
	         break; // for-i
	   }
	   buf[sz-i] = '\0';
	   return strlen( buf );
	}
	 
	int _FieldValue( char *bp, Field f )
	{
	   Field::Value &v = f._Value;
	   Field::Date   dt;
	   Field::Time   tm;
	   char         *cp;

	   cp = bp;
	   switch( f._Type ) {
	      case Field::ty_int8:
	         cp += sprintf( cp, "%d", v.v_int8 );
	         break;
	      case Field::ty_int16:
	         cp += sprintf( cp, "%d", v.v_int16 );
	         break;
	      case Field::ty_int32:
	         cp += sprintf( cp, "%d", v.v_int32 );
	         break;
	      case Field::ty_int64:
	         cp += sprintf( cp, yamr_PRId64, v.v_int64 );
	         break;
	      case Field::ty_float:
	         cp += sprintf( cp, "%.4g", v.v_float );
	         break;
	      case Field::ty_double:
	         cp += sprintf( cp, "%.4g", v.v_double );
	         break;
	      case Field::ty_string:
	         cp += sprintf( cp, v.v_string );
	         break;
	      case Field::ty_date:
	         dt  = v.v_date;
	         cp += sprintf( cp, "%04d-%02d-%02d", dt._Year, dt._Mon, dt._Day );
	         break;
	      case Field::ty_time:
	         tm  = v.v_time;
	         cp += sprintf( cp, "%02d:%02d:%02d", tm._Hour, tm._Min, tm._Sec );
	         cp += sprintf( cp, ".%03d", tm._Milli );
	         break;
	      case Field::ty_dateTime:
	         dt  = v.v_dtTm._Date;
	         tm  = v.v_dtTm._Time;
	         cp += sprintf( cp, "%04d-%02d-%02d", dt._Year, dt._Mon, dt._Day );
	         cp += sprintf( cp, " " );
	         cp += sprintf( cp, "%02d:%02d:%02d", tm._Hour, tm._Min, tm._Sec );
	         cp += sprintf( cp, ".%03d", tm._Milli );
	         break;
	   }
	   return( cp-bp );
	}
#endif // DOXYGEN_OMIT

	////////////////////////////////////
	// Marshall Operations
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
	 * \brief Initialize FieldList for ( svc, tkr )
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \see Send()
	 */
	void Init( const char *svc, const char *tkr )
	{
	   _encode.Init( svc, tkr );
	}

	/**
	 * \brief Add u_int8_t field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i8 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, u_int8_t i8 )
	{
	   _encode.Add( fid, i8 );
	}

	/**
	 * \brief Add u_int16_t field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i16 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, u_int16_t i16 )
	{
	   _encode.Add( fid, i16 );
	}

	/**
	 * \brief Add u_int32_t field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i32 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, u_int32_t i32 )
	{
	   _encode.Add( fid, i32 );
	}

	/**
	 * \brief Add u_int64_t field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i64 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, u_int64_t i64 )
	{
	   _encode.Add( fid, i64 );
	}

	/**
	 * \brief Add double field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param r64 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, double r64 )
	{
	   _encode.Add( fid, r64 );
	}

	/**
	 * \brief Add float field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param r32 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, float r32 )
	{
	   _encode.Add( fid, r32 );
	}

	/**
	 * \brief Add string field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param yb - String Field in yamrBuf to add
	 * \see Send()
	 */
	void Add( int fid, yamrBuf yb )
	{
	   std::string s( yb._data, yb._dLen );

	   Add( fid, s );
	}

	/**
	 * \brief Add string field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param str - String Field value to add
	 * \see Send()
	 */
	void Add( int fid, const char *str )
	{
	   if ( _writer )
	      _encode.Add( fid, str, writer().strDict() );
	}

	/**
	 * \brief Add string field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param str - String Field value to add
	 * \see Send()
	 */
	void Add( int fid, std::string &str )
	{
	   Add( fid, str.data() );
	}

	/**
	 * \brief Add Date field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param dt - Date Field value to add
	 * \see Send()
	 */
	void Add( int fid, Field::Date dt )
	{
	   _encode.Add( fid, dt );
	}

	/**
	 * \brief Add Time field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param tm - Time Field value to add
	 * \see Send()
	 */
	void Add( int fid, Field::Time tm )
	{
	   _encode.Add( fid, tm );
	}

	/**
	 * \brief Add DateTime field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param dtTm - DateTime Field to add
	 * \see Send()
	 */
	void Add( int fid, Field::DateTime dtTm )
	{
	   _encode.Add( fid, dtTm );
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
	// private Members
	////////////////////////////////////
private:
	Decoder _decode;
	Encoder _encode;

};  // class FieldList

} // namespace Data

} // namespace YAMR

#endif // __YAMR_FieldList_H 
