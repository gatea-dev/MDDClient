/******************************************************************************
*
*  Field.hpp
*     librtEdge subscription Field
*
*  REVISION HISTORY:
*     15 SEP 2014 jcs  Created.
*     16 JAN 2015 jcs  Build 29: ByteStream.hpp; _str2dbl()
*      5 FEB 2016 jcs  Build 32: Dump()
*     14 OCT 2017 jcs  Build 36: int Dump( char * )
*     23 JAN 2018 jcs  Build 39: GetAsDateTime(); Set( mddField )
*     21 JUL 2018 jcs  Build 40: GetAsString() - All types
*     15 DEC 2018 jcs  Build 41: Dump() : Include Name()
*      8 FEB 2020 jcs  Build 42: public Set( Schema &, ... )
*     16 MAR 2020 jcs  Build 43: _StripTrailing0()
*     12 AUG 2020 jcs  Build 44: Date/Time : _r64, not GetAsDouble()
*     27 NOV 2020 jcs  Build 47: GetAsString() : Deep copy string to _s
*     30 MAR 2022 jcs  Build 52: GetAsDateTime()
*     23 MAY 2022 jcs  Build 54: rtFld_unixTime
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Field_H
#define __RTEDGE_Field_H
#include <hpp/rtEdge.hpp>
#include <math.h>

#ifndef DOXYGEN_OMIT
static double f_MIL     =  1000000.0;
static double _MIN_DATE = 20000101.0;
static double _MIN_DTTM = ( f_MIL * _MIN_DATE );
static double _MAX_TIME =   235959.999999;

#if !defined(WithinRange)
#define gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )
#define gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define InRange( a,b,c )     ( ((a)<=(b)) && ((b)<=(c)) )
#define WithinRange( a,b,c ) ( gmin( gmax((a),(b)), (c)) )
#endif // !defined(WithinRange)
#endif // DOXYGEN_OMIT

namespace RTEDGE
{

// Forwards

class Message;
class Schema;


////////////////////////////////////////////////
//
//     c l a s s   B y t e S t r e a m F l d
//
////////////////////////////////////////////////

/**
 * \class ByteStreamFld
 * \brief This class encapsulates the ByteStream value of a Field.
 *
 * The contents of this object are VOLATILE and valid only during the 
 * life of the update.
 */
class ByteStreamFld
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	ByteStreamFld()
	{
	   ::memset( &_buf, 0, sizeof( _buf ) );
	}

	virtual ~ByteStreamFld() { ; }


	////////////////////////////////////
	// Mutator
	////////////////////////////////////
	/**
	 * \brief Store (volatile) ByteStrem data from a field
	 *
	 * \param buf - (volatile) buffer containing ByteStream data
	 * \return this
	 */
public:
	ByteStreamFld &Set( rtBUF buf )
	{
	   _buf = buf;
	   return *this;
	}

	/**
	 * \brief Store (volatile) ByteStrem data from a field
	 *
	 * \param data - (volatile) ByteStream data
	 * \param dLen - data length
	 * \return this
	 */
public:
	ByteStreamFld &Set( char *data, int dLen )
	{
	   _buf._data = data;
	   _buf._dLen = dLen;
	   return *this;
	}

	/** \brief Clears ByteStreamFld contents */
	void Clear()
	{
	   _buf._data = NULL;
	   _buf._dLen = 0;
	}

	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns raw buffer
	 *
	 * \return Raw buffer
	 */
	rtBUF buf()
	{
	   return _buf;
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	rtBUF _buf;

};  // class ByteStreamFld



////////////////////////////////////////////////
//
//         c l a s s   F i e l d
//
////////////////////////////////////////////////

/**
 * \class Field
 * \brief This class encapsulates the rtFIELD message structure.
 *
 * This is a single field of a real-time market data Message received 
 * on the SubChannel subscription channel from rtEdgeCache3.
 */
class Field
{
friend class Message;
friend class Schema;
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	Field() :
	   _msg( (Message *)0 ),
	   _bLVC( false ),
	   _cxt( (rtEdge_Context)0 ),
	   _bStr(),
	   _schemaType( rtFld_undef ),
	   _dump(),
	   _s()
	{
	   ::memset( &_fld, 0, sizeof( _fld ) );
	}

	virtual ~Field() { ; }

	/**
	 * \brief Called to deep copy a Field from a Field List
	 *
	 * \param src - Source Field to copy
	 * \return this
	 */
	Field &Copy( Field &src )
	{
	   _msg        = (Message *)0;  // Message is volatile
	   _bLVC       = src.IsLVC();
	   _cxt        = src.cxt();
	   _fld        = src.field();
	   _schemaType = src.TypeFromSchema();
	   return *this;
	}

	/**
	 * \brief Called to deep copy an mddField from a Field List
	 *
	 * \param fld - mddField
	 * \return this
	 */
	Field &Set( mddField fld )
	{
	   ::memcpy( &_fld, &fld, sizeof( _fld ) );
	   return *this;
	}

	/**
	 * \brief Called by anyone to reuse this message
	 *
	 * \param msg - Message supplying this Field
	 * \param bLVC - true if from LVC; False if from rtEdgeCache3
	 * \param fld - New rtFIELD struct
	 * \return this
	 */
public:
	Field &Set( Message &msg, 
	            bool     bLVC,
	            rtFIELD  fld )
	{
	   _msg        = &msg;
	   _bLVC       = bLVC;
	   _cxt        = 0;
	   _fld        = fld;
	   _schemaType = rtFld_undef;
	   return *this;
	}

	/**
	 * \brief Called by Message to reuse this message
	 *
	 * \param msg - Message supplying this Field
	 * \param cxt - Channel context supplying Message
	 * \param bLVC - true if from LVC; False if from rtEdgeCache3
	 * \param fld - New rtFIELD struct
	 * \param schemaType - Field type from channel schema
	 * \return this
	 */
protected:
	Field &Set( Message       &msg, 
	            bool           bLVC,
	            rtEdge_Context cxt,
	            rtFIELD        fld, 
	            rtFldType      schemaType=rtFld_undef )
	{
	   _msg        = &msg;
	   _bLVC       = bLVC;
	   _cxt        = cxt;
	   _fld        = fld;
	   _schemaType = schemaType;
	   return *this;
	}

	/**
	 * \brief Called by Schema to reuse this message
	 *
	 * \param sch - Schema supplying this Field
	 * \param fld - New rtFIELD struct
	 */
public:
	Field &Set( Schema &sch, rtFIELD fld )
	{
	   _msg        = (Message *)0;
	   _bLVC       = false;
	   _cxt        = 0;
	   _fld        = fld;
	   _schemaType = _fld._type;
	   return *this;
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Message supplying this field
	 *
	 * \return Message supplying this field
	 */
	Message &msg()
	{
	   return *_msg;
	}

	/**
	 * \brief Returns true if Message sourced from LVC
	 *
	 * \return true if Message sourced from LVC
	 */
	bool IsLVC()
	{
	   return _bLVC;
	}

	/**
	 * \brief Returns context from Message supplying this field
	 *
	 * \return Context from Message supplying this field
	 */
	rtEdge_Context cxt()
	{
	   return _cxt;
	}

	/**
	 * \brief Returns field ID
	 *
	 * \return Field ID
	 */
	int Fid()
	{
	   return _fld._fid;
	}

	/**
	 * \brief Returns field Name
	 *
	 * \return Field Name
	 */
	const char *Name()
	{
	   const char *pn;

	   return (pn=_fld._name) ? pn : "";
	}

	/**
	 * \brief Returns underlying rtFIELD struct
	 *
	 * \return Underlying rtFIELD struct
	 */
	rtFIELD &field()
	{
	   return _fld;
	}

	/**
	 * \brief Returns native field type.
	 *
	 * This method attempts to determine the native field type by
	 * interrogating TypeFromMsg() first.  If undefined or string, 
	 * it then calls TypeFromSchema().  In this manner, Type() 
	 * handles ASCII Field List protocols which always set TypeFromMsg()
	 * to string (or undef).
	 * 
	 * \return Native field type
	 */
	rtFldType Type()
	{
	   rtFldType mTy, sTy, rtn;

	   // Handle ASCII Field List

	   mTy = TypeFromMsg();
	   sTy = TypeFromSchema();
	   rtn = mTy;
	   switch( mTy ) {
	      case rtFld_undef:
	      case rtFld_string:
	         rtn = sTy;
	         break;  
	      case rtFld_int:
	      case rtFld_double:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_float:
	      case rtFld_int8:
	      case rtFld_int16:
	      case rtFld_int64:
	      case rtFld_real:
	      case rtFld_bytestream:
	      case rtFld_unixTime:
	         break;
	   }
	   return rtn;
	}

	/**
	 * \brief Returns field type from Message
	 *
	 * \return Field Type from Message
	 */
	rtFldType TypeFromMsg()
	{
/*
 * 16-02-06 jcs  Build 32: Binary LVC
	   return _bLVC ? rtFld_string : _fld._type;
 */
	   return _fld._type;
	}

	/**
	 * \brief Returns field type from Schema
	 *
	 * \return Field Type from Schema
	 */
	rtFldType TypeFromSchema()
	{
	   return _schemaType;
	}

	/**
	 * \brief Returns field value as 8-bit int
	 *
	 * \return Field Value as 8-bit int
	 */
	u_char GetAsInt8()
	{
	   rtVALUE  &v = _fld._val;
	   rtFldType ty;
	   u_char    i8;

	   i8  = 0;
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_int8:   i8 = v._i8;          break; 
	      case rtFld_int16:  i8 = v._i16;         break; 
	      case rtFld_int:    i8 = (u_char)v._i32; break; 
	      case rtFld_unixTime:
	      case rtFld_int64:  i8 = (u_char)v._i64; break; 
	      case rtFld_double: i8 = (u_char)v._r64; break; 
	      case rtFld_float:  i8 = (u_char)v._r32; break; 
	      case rtFld_string:
	      case rtFld_bytestream:
	         i8 = (u_char)atoi( GetAsString() );
	         break;
	      case rtFld_undef:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_real:
	         break;
	   }
	   return i8;
	}

	/**
	 * \brief Returns field value as 16-bit int
	 *
	 * \return Field Value as 16-bit int
	 */
	u_short GetAsInt16()
	{
	   rtVALUE  &v = _fld._val;
	   rtFldType ty;
	   u_short   i16;

	   i16 = 0;
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_int8:   i16 = v._i8;           break; 
	      case rtFld_int16:  i16 = v._i16;          break; 
	      case rtFld_int:    i16 = (u_short)v._i32; break; 
	      case rtFld_unixTime:
	      case rtFld_int64:  i16 = (u_short)v._i64; break; 
	      case rtFld_double: i16 = (u_short)v._r64; break; 
	      case rtFld_float:  i16 = (u_short)v._r32; break; 
	      case rtFld_string:
	      case rtFld_bytestream:
	         i16 = (u_short)atoi( GetAsString() );
	         break;
	      case rtFld_undef:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_real:
	         break;
	   }
	   return i16;
	}

	/**
	 * \brief Returns field value as 32-bit int
	 *
	 * \return Field Value as 32-bit int
	 */
	int GetAsInt32()
	{
	   rtVALUE  &v = _fld._val;
	   rtFldType ty;
	   int       i32;

	   i32 = 0;
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_int8:   i32 = v._i8;       break; 
	      case rtFld_int16:  i32 = v._i16;      break; 
	      case rtFld_int:    i32 = v._i32;      break; 
	      case rtFld_unixTime:
	      case rtFld_int64:  i32 = (int)v._i64; break; 
	      case rtFld_double: i32 = (int)v._r64; break; 
	      case rtFld_float:  i32 = (int)v._r32; break; 
	      case rtFld_string:
	      case rtFld_bytestream:
	         i32 = atoi( GetAsString() );
	         break;
	      case rtFld_undef:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_real:
	         break;
	   }
	   return i32;
	}

	/**
	 * \brief Returns field value as 64-bit long
	 *
	 * \return Field Value as 64-bit long
	 */
	u_int64_t GetAsInt64()
	{
	   rtVALUE  &v = _fld._val;
	   rtFldType ty;
	   u_int64_t i64;

	   i64 = 0;
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_int8:   i64 = v._i8;             break; 
	      case rtFld_int16:  i64 = v._i16;            break; 
	      case rtFld_int:    i64 = v._i32;            break; 
	      case rtFld_unixTime:
	      case rtFld_int64:  i64 = v._i64;            break; 
	      case rtFld_double: i64 = (u_int64_t)v._r64; break; 
	      case rtFld_float:  i64 = (u_int64_t)v._r32; break; 
	      case rtFld_string:
	      case rtFld_bytestream:
	         i64 = atoi( GetAsString() );
	         break;
	      case rtFld_undef:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_real:
	         break;
	   }
	   return i64;
	}

	/**
	 * \brief Returns field value as float
	 *
	 * \return Field Value is float
	 */
	float GetAsFloat()
	{
	   rtVALUE  &v = _fld._val;
	   rtFldType ty;
	   float     r32;

	   r32 = 0.0;
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_int8:   r32 = v._i8;         break; 
	      case rtFld_int16:  r32 = v._i16;        break; 
	      case rtFld_int:    r32 = v._i32;        break; 
	      case rtFld_unixTime:
	      case rtFld_int64:  r32 = (float)v._i64; break; 
	      case rtFld_double: r32 = v._r64;        break; 
	      case rtFld_float:  r32 = v._r32;        break; 
	      case rtFld_string:
	      case rtFld_bytestream:
	         r32 = atof( GetAsString() );
	         break;
	      case rtFld_undef:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_real:
	         break;
	   }
	   return r32;
	}

	/**
	 * \brief Returns field value as double
	 *
	 * \return Field Value is double
	 */
	double GetAsDouble()
	{
	   rtVALUE  &v = _fld._val;
	   rtFldType ty;
	   double    r64;

	   r64 = 0.0;
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_int8:   r64 = v._i8;          break; 
	      case rtFld_int16:  r64 = v._i16;         break; 
	      case rtFld_int:    r64 = v._i32;         break; 
	      case rtFld_unixTime:
	      case rtFld_int64:  r64 = (double)v._i64; break; 
	      case rtFld_double: r64 = v._r64;         break; 
	      case rtFld_float:  r64 = v._r32;         break; 
	      case rtFld_string:
	      case rtFld_bytestream:
	         r64 = _str2dbl( GetAsString() );
	         break;
	      case rtFld_undef:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_real:
	         break;
	   }
	   return r64;
	}

	/**
	 * \brief Return field value interpreted as rtDateTime
	 *
	 * \return Field value interpreted as rtDateTime
	 */
	rtDateTime GetAsDateTime()
	{
	   rtDateTime dtTm;
	   rtDate    &dt = dtTm._date;
	   rtTime    &tm = dtTm._time;
	   rtVALUE   &v  = _fld._val;
	   double     r64 = v._r64;
	   int        ymd;
	   time_t     now;
	   struct tm  lt;

	   // rtFld_unixTime??

	   if ( _IsUnixTime() )
	      return _GetFromUnixTime( v._i64 );

	   /*
	    * 1) > _MIN_DTTM : YYYYMMDDHHMMSS.uuuuuu
	    * 2) > _MIN_DATE : YYYYMMDD
	    * 3) < _MAX_TIME : HHMMSS.uuuuuu
	    */
	   if ( r64 >= _MIN_DTTM ) {
	      dt = GetAsDate();
	      tm = GetAsTime();
	   }
	   else if ( r64 >= _MIN_DATE ) {
	      r64 *= f_MIL;
	      dt   = _GetAsDate( r64 );
	   }
	   else if ( r64 <= _MAX_TIME ) {
	      now = ::rtEdge_TimeSec();
	      ::localtime_r( &now, &lt );
	      ymd  = ( lt.tm_year + 1900 ) * 10000;
	      ymd += ( lt.tm_mon  +    1 ) *   100;
	      ymd += lt.tm_sec;
	      r64  = ymd;
	      r64 *= f_MIL;
	      dt  = _GetAsDate( r64 );
	      tm  = GetAsTime();
	   }
	   return dtTm;
	}

	/**
	 * \brief Returns field value as rtDate
	 *
	 * \return Field Value is rtDate
	 */
	rtDate GetAsDate()
	{
	   rtVALUE &v = _fld._val;

	   if ( _IsUnixTime() )
	      return _GetFromUnixTime( v._i64 )._date;
	   return _GetAsDate( v._r64 );
	}

	/**
	 * \brief Returns field value as rtTime
	 *
	 * \return Field Value is rtTime
	 */
	rtTime GetAsTime()
	{
	   rtVALUE  &v   = _fld._val;
	   double    r64 = v._r64;
	   u_int64_t i64 = (u_int64_t)r64;
	   int       hmd = (int)::fmod( r64, f_MIL );
	   rtTime    t;

	   if ( _IsUnixTime() )
	      return _GetFromUnixTime( v._i64 )._time;

	   // HHMMSS.uuuuuu

	   t._hour   = hmd / 10000;
	   t._minute = ( hmd / 100 ) % 100;
	   t._second = hmd % 100;
	   r64      -= i64;
	   t._micros = (int)( f_MIL * r64 );
	   t._micros = WithinRange( 0, t._micros, 999999 );
	   return t;
	}

	/**
	 * \brief Returns field value as string
	 *
	 * \return Field Value as NULL-terminated string
	 */
	const char *GetAsString()
	{
	   rtVALUE    &v = _fld._val;
	   rtBUF      &b = v._buf;
	   rtFldType   ty;
	   mddField    f;
	   int         sz;
	   char       *cp, buf[K];
	   const char *rtn;

	   rtn = "Wrong field type";
	   ty  = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_string:
	      case rtFld_bytestream:
	         rtn = "";
	         sz  = b._dLen;
	         if ( sz > 0 ) {
	            _s.assign( b._data, b._dLen );
	            rtn = _s.data();
	            cp  = (char *)rtn;
	            cp += sz;
	            *cp = '\0';
	         }
	         break;
	      case rtFld_undef:
	      case rtFld_int:
	      case rtFld_double:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_float:
	      case rtFld_int8:
	      case rtFld_int16:
	      case rtFld_int64:
	      case rtFld_real:
	      case rtFld_unixTime:
	         ::memcpy( &f, &_fld, sizeof( _fld ) );
	         mddWire_dumpField( f, buf );
	         _s  = _StripTrailing0( buf );
	         rtn = _s.data();
	         break;
	   }
	   return rtn;
	}

	/**
	 * \brief Returns field value as ByteStream
	 *
	 * \return Field Value as ByteStream
	 */
	ByteStreamFld &GetAsByteStream()
	{
	   rtVALUE  &v = _fld._val;
	   rtBUF    &b = v._buf;
	   rtFldType ty;

	   _bStr.Clear();
	   ty = TypeFromMsg();
	   switch( ty ) {
	      case rtFld_string:
	      case rtFld_bytestream:
	         _bStr.Set( b );
	         break;
	      case rtFld_undef:
	      case rtFld_int:
	      case rtFld_double:
	      case rtFld_date:
	      case rtFld_time:
	      case rtFld_timeSec:
	      case rtFld_float:
	      case rtFld_int8:
	      case rtFld_int16:
	      case rtFld_int64:
	      case rtFld_real:
	      case rtFld_unixTime:
	         break;
	   }
	   return _bStr;
	}

	/**
	 * \brief Dumps field contents as string
	 *
	 * \param bFid - Include field ID in header
	 * \return Field contents as string
	 */
	const char *Dump( bool bFid=false )
	{
	   char buf[K];

	   Dump( buf, bFid );
	   _dump = buf;
	   return _dump.data();
	}

	/**
	 * \brief Returns true if field available in this Message
	 *
	 * \param fieldName - Schema name of field to find
	 * \return true if fieldName exists in this Message
	 */
public:
	bool Has( const char *fieldName )
	{
	   return ::rtEdge_HasField( _cxt, fieldName ) ? true : false;
	}

	/**
	 * \brief Returns true if field available in this Message
	 *
	 * \param fid - Field ID to find
	 * \return true if fieldName exists in this Message
	 */
	bool Has( int fid )
	{
	   return ::rtEdge_HasFieldByID( _cxt, fid ) ? true : false;
	}

	/**
	 * \brief Retrieve and return requested field
	 *
	 * \param fieldName - Schema name of field to retrieve
	 * \return this
	 */
	Field &Get( const char *fieldName )
	{
	   _fld = ::rtEdge_GetField( _cxt, fieldName );
	   return *this;
	}

	/**
	 * \brief Retrieve and return requested field
	 *
	 * \param fid - ID of field to retrieve
	 * \return this
	 */
	Field &Get( int fid )
	{
	   _fld = ::rtEdge_GetFieldByID( _cxt, fid );
	   return *this;
	}

	/**
	 * \brief Returns true if field values are equal
	 * 
	 * \param c - Field to compare
	 * \return true if field values are equal
	 */
	bool IsEqual( mddField c )
	{
	   mddField  f;
	   mddValue &v1 = f._val;
	   mddValue &v2 = c._val;
	   mddBuf   &b1 = v1._buf;
	   mddBuf   &b2 = v2._buf;
	   bool      bOK;
	   u_int     bSz;

	   /*
	    * 1) Same FID
	    * 2) Same Type
	    * 3) Same mddValue : If string, compare strings
	    */
	   ::memcpy( &f, &_fld, sizeof( _fld ) );
	   bOK  = ( f._fid == c._fid );
	   bOK &= ( f._type == c._type );
	   if ( bOK ) {
	      switch( f._type ) {
	         case mddFld_string:
	         case mddFld_bytestream:
	            bSz  = gmin( b1._dLen, b2._dLen );
	            bOK &= ( b1._dLen == b2._dLen );
	            bOK &= ( ::memcmp( b1._data, b2._data, bSz ) == 0 );
	            break;
	         default:
	            bOK &= ( ::memcmp( &v1, &v2, sizeof( v1 ) ) == 0 );
	            break;
	      }
	   }
	   return bOK;
	}


#ifndef DOXYGEN_OMIT
	////////////////////////
	// Helpers
	////////////////////////
private:
	bool _IsUnixTime()
	{
	   return( TypeFromMsg() == rtFld_unixTime );
	}

	char *_StripTrailing0( char *op )
	{
	   int i, sz;

	   sz = strlen( op );
	   if ( !::memchr( op, '.', sz ) )
	      return op;
	   for ( i=sz-1; i>=0; i-- ) {
	      switch( op[i] ) {
	         case '.': op[i]   = '\0'; return op;
	         case '0':                 break;
	         default:  op[i+1] = '\0'; return op;
	      }
	   }
	   return op;
	}

	/**
	 * \brief Dumps field contents as string
	 *
	 * \param obuf - User supplied buffer
	 * \param bFid - Include field ID in header
	 * \return Length of string
	 */
	int Dump( char *buf, bool bFid=false )
	{
	   char    *cp;
	   mddField f;

	   // librtEdge-to-libmddWire compatibliity

	   f._fid  = _fld._fid;
	   f._val  = _fld._val;
	   f._name = _fld._name;
	   f._type = (mddFldType)_fld._type;

	   // OK to dump

	   cp  = buf;
	   *cp = '\0';
	   if ( bFid )
	      cp += sprintf( cp, "   [%04d] %-12s : ", Fid(), Name() );
	   mddWire_dumpField( f, cp );
	   _StripTrailing0( cp );
	   cp += strlen( cp );
	   return( cp-buf );
	}

	rtDateTime _GetFromUnixTime( u_int64_t tUnixNs )
	{
	   rtDateTime dtTm;
	   rtDate    &dt = dtTm._date;
	   rtTime    &tm = dtTm._time;
	   time_t     now;
	   u_int64_t  nano;
	   struct tm  lt;

	   now  = tUnixNs / _NANO;
	   nano = tUnixNs % _NANO;
	   ::localtime_r( &now, &lt );
	   dt._year   = lt.tm_year + 1900;
	   dt._month  = lt.tm_mon;
	   dt._mday   = lt.tm_mday;
	   tm._hour   = lt.tm_hour;
	   tm._minute = lt.tm_min;
	   tm._second = lt.tm_sec;
	   tm._micros = nano / 1000;
	   return dtTm;
	}


	/**
	 * \brief Returns double as rtDate
	 *
	 * \return double as rtDate
	 */
	rtDate _GetAsDate( double r64 )
	{
	   int    i32;
	   rtDate d;

	   // YYYYMMDD * f_MIL;

	   r64     /= f_MIL;
	   i32      = (int)r64;
	   d._year  = i32 / 10000;
	   d._month = ( i32 / 100 ) % 100;
	   d._mday  = i32 % 100;
	   return d;
	}

	double _str2dbl( const char *str )
	{
	   const char *num, *den;
	   int         iNum, iDen;
	   double      dv, dd;

	   // 1) double : 123.5

	   dv = atof( str );
	   if ( !(num=::strchr( str, 0x20 )) ) // ' '
	      return dv;

	   // 2) fraction : 123 128/256

	   den = ::strchr( num, 0x2f );  // '/'
	   if ( !den )
	      return dv;
	   den += 1;
	   iNum = _atoin( num, den-num );
	   iDen = atoi( den );
	   dd   = iDen ?  ( 1.0 * iNum ) / iDen : 0.0;
	   dv  += dd;
	   return dv;
	}

	int _atoin( const char *num, int len )
	{
	   char buf[K];
	   int  sz;

	   sz = gmin( K-1, len );
	   ::memcpy( buf, num, sz );
	   buf[sz] = '\0';
	   return atoi( buf );
	}
#endif // DOXYGEN_OMIT

	////////////////////////
	// private Members
	////////////////////////
private:
	Message       *_msg;
	bool           _bLVC;
	rtEdge_Context _cxt;
	rtFIELD        _fld;
	ByteStreamFld  _bStr;
	rtFldType      _schemaType;
	std::string    _dump;
	std::string    _s;

};  // class Field

} // namespace RTEDGE

#endif // __LIBRTEDGE_Field_H 
