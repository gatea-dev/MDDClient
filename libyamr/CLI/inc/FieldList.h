/******************************************************************************
*
*  FieldList.h
*     libyamr Field List
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
//      e n u m e r a t e d   t y p e s
//
////////////////////////////////////////////////
/**
 * \enum FieldType
 * \brief Field Type : 32-bit int, double, string, etc.
 */
public enum class FieldType
{
	ty_int8     = YAMR::Data::Field::Type::ty_int8,
	ty_int16    = YAMR::Data::Field::Type::ty_int16,
	ty_int32    = YAMR::Data::Field::Type::ty_int32,
	ty_int64    = YAMR::Data::Field::Type::ty_int64,
	ty_float    = YAMR::Data::Field::Type::ty_float,
	ty_double   = YAMR::Data::Field::Type::ty_double,
	ty_string   = YAMR::Data::Field::Type::ty_string,
	ty_date     = YAMR::Data::Field::Type::ty_date,
	ty_time     = YAMR::Data::Field::Type::ty_time,
	ty_dateTime = YAMR::Data::Field::Type::ty_dateTime

}; // class FieldType


////////////////////////////////////////////////
//
//        c l a s s   F i e l d
//
////////////////////////////////////////////////

/**
 * \class Field
 * \brief A single ( FID, Value ) Field
 */
public ref class Field : public libyamr::yamr
{
private:
	YAMR::Data::Field &_fld;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/** \brief Constructor */
	Field( YAMR::Data::Field &fld ) :
	   _fld( fld )
	{ ; }


	/////////////////////////////////
	// Properties
	/////////////////////////////////
	/** \brief Returns field ID */
	property int _Fid
	{
	   int get() { return _fld._Fid; }
	}

	/** \brief Returns field type */
	property FieldType _Type
	{
	   FieldType get() {
	      FieldType ft;

	      ft = FieldType::ty_int8;
	      switch( _fld._Type ) {
	         case YAMR::Data::Field::Type::ty_int8:
	            ft = FieldType::ty_int8;
	            break;
	         case YAMR::Data::Field::Type::ty_int16:
	            ft = FieldType::ty_int16;
	            break;
	         case YAMR::Data::Field::Type::ty_int32:
	            ft = FieldType::ty_int32;
	            break;
	         case YAMR::Data::Field::Type::ty_int64:
	            ft = FieldType::ty_int64;
	            break;
	         case YAMR::Data::Field::Type::ty_float:
	            ft = FieldType::ty_float;
	            break;
	         case YAMR::Data::Field::Type::ty_double:
	            ft = FieldType::ty_double;
	            break;
	         case YAMR::Data::Field::Type::ty_string:
	            ft = FieldType::ty_string;
	            break;
	         case YAMR::Data::Field::Type::ty_date:
	            ft = FieldType::ty_date;
	            break;
	         case YAMR::Data::Field::Type::ty_time:
	            ft = FieldType::ty_time;
	            break;
	         case YAMR::Data::Field::Type::ty_dateTime:
	            ft = FieldType::ty_dateTime;
	            break;
	      }
	      return ft;
	   }
	}

	/**
	 * \brief Returns field value as 8-bit int
	 *
	 * \return Field Value as 8-bit int
	 */
	byte GetAsInt8()
	{
	   return (byte)_fld._Value.v_int8;
	}

	/**
	 * \brief Returns field value as 16-bit short
	 *
	 * \return Field Value as 16-bit short
	 */
	short GetAsInt16()
	{
	   return (short)_fld._Value.v_int16;
	}

	/**
	 * \brief Returns field value as 32-bit int
	 *
	 * \return Field Value as 32-bit int
	 */
	int GetAsInt32()
	{
	   return (int)_fld._Value.v_int32;
	}

	/**
	 * \brief Returns field value as 64-bit long
	 *
	 * \return Field Value as 64-bit long
	 */
	long GetAsInt64()
	{
	   return (long)_fld._Value.v_int64;
	}

	/**
	 * \brief Returns field value as float
	 *
	 * \return Field Value as float
	 */
	float GetAsFloat()
	{
	   return _fld._Value.v_float;
	}

	/**
	 * \brief Returns field value as double
	 *
	 * \return Field Value as double
	 */
	double GetAsDouble()
	{
	   return _fld._Value.v_double;
	}

	/**
	 * \brief Returns field value as String
	 *
	 * \return Field Value as String
	 */
	String ^GetAsString()
	{
	   bool bStr;

	   bStr = ( _Type == FieldType::ty_string );
	   return bStr ?  gcnew String( _fld._Value.v_string ) : nullptr;
	}

#ifdef FIELD_TODO_TYPES
	/**
	 * \brief Returns field value as DateTime
	 *
	 * \return Field Value as DateTime
	 */
	DateTime ^GetAsDateTime()
	{
	}

#endif // FIELD_TODO_TYPES
}; // class Field


////////////////////////////////////////////////
//
//      c l a s s    F i e l d L i s t
//
////////////////////////////////////////////////

/**
 * \class FieldList
 * \brief List of Fields
 */
public ref class FieldList : public libyamr::yamr
{
private:
	YAMR::Data::FieldList *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	FieldList( Reader ^reader ) :
	   _cpp( new YAMR::Data::FieldList( reader->cpp() ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	FieldList( Writer ^writer ) :
	   _cpp( new YAMR::Data::FieldList( writer->cpp() ) )
	{ ; }

	/** \brief Destructor */
	~FieldList()
	{
	   delete _cpp;
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return array of Fields
	 *
	 * \return array of Fields
	 */
	array<Field ^> ^fldList()
	{
	   YAMR::Data::Fields &ddb = _cpp->fldList();
	   array<Field ^>     ^rc;
	   size_t              i, n;

	   rc = nullptr;
	   if ( (n=ddb.size()) ) {
	      rc = gcnew array<Field ^>( n );
	      for ( i=0; i<n; rc[i]=gcnew Field( ddb[i] ), i++ );
	   }
	   return rc;
	}

	/**
	 * \brief Return Service Name of this FieldList
	 *
	 * \return Service Name of this FieldList
	 */
	String ^svc()
	{
	   return gcnew String( _cpp->svc() );
	}

	/**
	 * \brief Return Ticker Name of this FieldList
	 *
	 * \return Ticker Name of this FieldList
	 */
	String ^tkr()
	{
	   return gcnew String( _cpp->tkr() );
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
	 * \brief Returns current dictionary size
	 *
	 * \return Current dictionary size
	 */
	long Size()
	{
	   return _cpp->Size();
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
	void Init( String ^svc, String ^tkr )
	{
	   _cpp->Init( _pStr( svc ), _pStr( tkr ) );
	}

	/**
	 * \brief Add short field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i16 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, short i16 )
	{
	   _cpp->Add( fid, (u_int16_t)i16 );
	}

	/**
	 * \brief Add int field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i32 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, int i32 )
	{
	   _cpp->Add( fid, (u_int32_t)i32 );
	}

	/**
	 * \brief Add long field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param i64 - Field value to add
	 * \see Send()
	 */
	void Add( int fid, long i64 )
	{
	   _cpp->Add( fid, (u_int64_t)i64 );
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
	   _cpp->Add( fid, r64 );
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
	   _cpp->Add( fid, r32 );
	}

	/**
	 * \brief Add string field to list
	 *
	 * Call Send() to send complete list to yamRecorder
	 *
	 * \param fid - Field ID to add
	 * \param buf - Byte buffer to add
	 * \see Send()
	 */
	void Add( int fid, array<Byte> ^buf )
	{
	   _cpp->Add( fid, _memcpy( buf ) );
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
	void Add( int fid, String ^str )
	{
	   _cpp->Add( fid, _pStr( str ) );
	}

#ifdef TODO_FIELD
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
	   _cpp->Add( fid, dt );
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
	   _cpp->Add( fid, tm );
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
	   _cpp->Add( fid, dtTm );
	}
#endif // TODO_FIELD
	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \return true if message consumed by channel; false otherwise
	 */
	bool Send()
	{
	   return _cpp->Send();
	}

}; // class FieldList

} // namespace Data

} // namespace libyamr
