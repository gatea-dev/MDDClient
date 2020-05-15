/******************************************************************************
*
*  Usage.h
*     libyamr Usage Recording
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

namespace bespoke
{

////////////////////////////////////////////////
//
//        c l a s s   R e c o r d
//
////////////////////////////////////////////////

/**
 * \class Record
 *
 * \brief String-based generic log message (Usage-like).
 */
public ref class Record : public libyamr::yamr
{
private:
	String          ^_UsageType;
	String          ^_Username;
	String          ^_Service;
	String          ^_Ticker;
	String          ^_QoS;
	array<String ^> ^_xtraCols;
	array<String ^> ^_xtraVals;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/** \brief Constructor */
	Record() :
	   _UsageType( nullptr ),
	   _Username( nullptr ),
	   _Service( nullptr ),
	   _Ticker( nullptr ),
	   _QoS( nullptr ),
	   _xtraCols( nullptr ),
	   _xtraVals( nullptr )
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
	property RecordType _Type
	{
	   RecordType get() {
	      RecordType ft;

	      ft = RecordType::ty_int8;
	      switch( _fld._Type ) {
	         case YAMR::Data::Record::Type::ty_int8:
	            ft = RecordType::ty_int8;
	            break;
	         case YAMR::Data::Record::Type::ty_int16:
	            ft = RecordType::ty_int16;
	            break;
	         case YAMR::Data::Record::Type::ty_int32:
	            ft = RecordType::ty_int32;
	            break;
	         case YAMR::Data::Record::Type::ty_int64:
	            ft = RecordType::ty_int64;
	            break;
	         case YAMR::Data::Record::Type::ty_float:
	            ft = RecordType::ty_float;
	            break;
	         case YAMR::Data::Record::Type::ty_double:
	            ft = RecordType::ty_double;
	            break;
	         case YAMR::Data::Record::Type::ty_string:
	            ft = RecordType::ty_string;
	            break;
	         case YAMR::Data::Record::Type::ty_date:
	            ft = RecordType::ty_date;
	            break;
	         case YAMR::Data::Record::Type::ty_time:
	            ft = RecordType::ty_time;
	            break;
	         case YAMR::Data::Record::Type::ty_dateTime:
	            ft = RecordType::ty_dateTime;
	            break;
	      }
	      return ft;
	   }
	}

	/**
	 * \brief Returns field value as 8-bit int
	 *
	 * \return Record Value as 8-bit int
	 */
	byte GetAsInt8()
	{
	   return (byte)_fld._Value.v_int8;
	}

	/**
	 * \brief Returns field value as 16-bit short
	 *
	 * \return Record Value as 16-bit short
	 */
	short GetAsInt16()
	{
	   return (short)_fld._Value.v_int16;
	}

	/**
	 * \brief Returns field value as 32-bit int
	 *
	 * \return Record Value as 32-bit int
	 */
	int GetAsInt32()
	{
	   return (int)_fld._Value.v_int32;
	}

	/**
	 * \brief Returns field value as 64-bit long
	 *
	 * \return Record Value as 64-bit long
	 */
	long GetAsInt64()
	{
	   return (long)_fld._Value.v_int64;
	}

	/**
	 * \brief Returns field value as float
	 *
	 * \return Record Value as float
	 */
	float GetAsFloat()
	{
	   return _fld._Value.v_float;
	}

	/**
	 * \brief Returns field value as double
	 *
	 * \return Record Value as double
	 */
	double GetAsDouble()
	{
	   return _fld._Value.v_double;
	}

	/**
	 * \brief Returns field value as String
	 *
	 * \return Record Value as String
	 */
	String ^GetAsString()
	{
	   bool bStr;

	   bStr = ( _Type == RecordType::ty_string );
	   return bStr ?  gcnew String( _fld._Value.v_string ) : nullptr;
	}

#ifdef FIELD_TODO_TYPES
	/**
	 * \brief Returns field value as DateTime
	 *
	 * \return Record Value as DateTime
	 */
	DateTime ^GetAsDateTime()
	{
	}

#endif // FIELD_TODO_TYPES
}; // class Record


////////////////////////////////////////////////
//
//      c l a s s    F i e l d L i s t
//
////////////////////////////////////////////////

/**
 * \class Usage
 * \brief List of Records
 */
public ref class Usage : public libyamr::yamr
{
private:
	YAMR::Data::Usage *_cpp;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** 
	 * \brief Constructor - Reading from tape
	 * 
	 * \param reader - Reader channel driving us
	 */
	Usage( Reader ^reader ) :
	   _cpp( new YAMR::Data::Usage( reader->cpp() ) )
	{ ; }

	/** 
	 * \brief Constructor - Writing to yamRecorder
	 * 
	 * \param writer - Writer channel driving us
	 */
	Usage( Writer ^writer ) :
	   _cpp( new YAMR::Data::Usage( writer->cpp() ) )
	{ ; }

	/** \brief Destructor */
	~Usage()
	{
	   delete _cpp;
	}


	////////////////////////////////////
	// DeMarshall : Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return array of Records
	 *
	 * \return array of Records
	 */
	array<Record ^> ^fldList()
	{
	   YAMR::Data::Records &ddb = _cpp->fldList();
	   array<Record ^>     ^rc;
	   size_t              i, n;

	   rc = nullptr;
	   if ( (n=ddb.size()) ) {
	      rc = gcnew array<Record ^>( n );
	      for ( i=0; i<n; rc[i]=gcnew Record( ddb[i] ), i++ );
	   }
	   return rc;
	}

	/**
	 * \brief Return Service Name of this Usage
	 *
	 * \return Service Name of this Usage
	 */
	String ^svc()
	{
	   return gcnew String( _cpp->svc() );
	}

	/**
	 * \brief Return Ticker Name of this Usage
	 *
	 * \return Ticker Name of this Usage
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
	 * \brief Initialize Usage for ( svc, tkr )
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
	 * \param fid - Record ID to add
	 * \param i16 - Record value to add
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
	 * \param fid - Record ID to add
	 * \param i32 - Record value to add
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
	 * \param fid - Record ID to add
	 * \param i64 - Record value to add
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
	 * \param fid - Record ID to add
	 * \param r64 - Record value to add
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
	 * \param fid - Record ID to add
	 * \param r32 - Record value to add
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
	 * \param fid - Record ID to add
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
	 * \param fid - Record ID to add
	 * \param str - String Record value to add
	 * \see Send()
	 */
	void Add( int fid, String ^str )
	{
	   _cpp->Add( fid, _pStr( str ) );
	}

	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \return true if message consumed by channel; false otherwise
	 */
	bool Send()
	{
	   return _cpp->Send();
	}

}; // class Usage

} // namespace bespoke

} // namespace libyamr
