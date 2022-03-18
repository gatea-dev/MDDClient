/******************************************************************************
*
*  rtEdgeField.h
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*     13 DEC 2014 jcs  Build 29: ByteStreamFld
*      5 FEB 2016 jcs  Build 32: Dump()
*     14 JAN 2018 jcs  Build 39: _name
*      9 MAR 2020 jcs  Build 42: Copy constructor; _bStrCpy
*     11 AUG 2020 jcs  Build 44: GetAsDateTime() filled in
*     18 MAR 2022 jcs  Build 52: long long GetAsInt64()
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#endif // DOXYGEN_OMIT

namespace librtEdge
{

////////////////////////////////////////////////
//
//    c l a s s   B y t e S t r e a m F l d
//
////////////////////////////////////////////////

/**
 * \class ByteStreamFld
 * \brief A view on a ByteStreamFld from a single Field.
 */
public ref class ByteStreamFld
{
private: 
	RTEDGE::ByteStreamFld *_bStr;
	array<Byte>           ^_bRaw;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/** \brief Constructor */
public:
	ByteStreamFld();
	~ByteStreamFld();


	/////////////////////////////////
	//  Access
	/////////////////////////////////
	/**
	 * \brief Return RTEDGE::ByteStreamFld populating this object
	 *
	 * \return RTEDGE::ByteStreamFld populating this object
	 */
	RTEDGE::ByteStreamFld *bStr();


	/////////////////////////////////
	//  Operations
	/////////////////////////////////
	/**
	 * \brief Set the internal state of this ByteStreamFld
	 *
	 * \param bStr - Current ByteStreamFld contents
	 */
	ByteStreamFld ^Set( RTEDGE::ByteStreamFld &bStr );

	/**
	 * \brief Clear out internal state so this message may be reused.
	 */
	void Clear();


	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** 
	 * \brief Returns ByteStreamFld data contents 
	 *
	 * \return ByteStreamFld data contents
	 */
	property array<Byte> ^_data
	{
	   array<Byte> ^get() {
	      if ( _bRaw == nullptr )
	         _bRaw = rtEdge::_memcpy( _bStr->buf() );
	      return _bRaw;
	   }
	}

	/** \brief Returns length of the ByteStreamFld */
	property u_int _len
	{
	   u_int get() { return _data->Length; }
	}
};  // class ByteStreamFld



////////////////////////////////////////////////
//
//       c l a s s   r t E d g e F i e l d
//
////////////////////////////////////////////////

/**
 * \class rtEdgeField
 * \brief A single Field from a rtEdgeData out of the rtEdgeSubscriber 
 * channel.
 *
 * When pulling an rtEdgeField from an incoming rtEdgeData, this object
 * is reused.  The contents are volatile and only valid until the next
 * call to rtEdgeData::forth().
 *
 */
public ref class rtEdgeField
{
private: 
	RTEDGE::Field *_fld;
	bool           _bCopy;
	bool           _bStrCpy;
	ByteStreamFld ^_bStr;
	String        ^_val;
	String        ^_name;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Reusable field object constructor
	 *
	 * Normally created by rtEdgeData as the single (reusable) field 
	 * when iterating through an message in the 
	 * rtEdgeSubscriber::OnData() as follows:
	 *
	 * \include rtEdgeData_OnData.h
	 */
	rtEdgeField();

	/**
	 * \brief Copy constructor
	 *
	 * Makes deep copy of the field; Useful for crossing thread boundaries
	 * \param src - Source rtEdgeField to copy
	 */
public:
	rtEdgeField( rtEdgeField ^src );

	/** \brief Destructor */
	~rtEdgeField();


	/////////////////////////////////
	// Access
	/////////////////////////////////
	/** 
	 * \brief Returns unmanaged Field
	 *   
	 * \return Unmanaged Field
	 */  
	RTEDGE::Field *cpp();

	/** 
	 * \brief Returns field ID
	 *   
	 * \return Field ID
	 */  
	int Fid();

	/** 
	 * \brief Returns field Name
	 *   
	 * \return Field Name
	 */  
	String ^Name();

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
	rtFldType Type();

	/**
	 * \brief Returns field value as 8-bit int
	 *
	 * This method has significantly better performance than GetAsString() 
	 * or _string_value and is preferred when you know the field is a 
	 * 8-bit int.
	 *
	 * \return Field Value as 8-bit int
	 */
	u_char GetAsInt8();

	/**
	 * \brief Returns field value as 16-bit int
	 *
	 * This method has significantly better performance than GetAsString() 
	 * or _string_value and is preferred when you know the field is a 
	 * 16-bit int.
	 *
	 * \return Field Value as 16-bit int
	 */
	u_short GetAsInt16();

	/**
	 * \brief Returns field value as 32-bit int
	 *
	 * This method has significantly better performance than GetAsString() 
	 * or _string_value and is preferred when you know the field is a 
	 * 32-bit int.
	 *
	 * \return Field Value as 32-bit int
	 */
	int GetAsInt32();

	/**
	 * \brief Returns field value as 64-bit long
	 *
	 * This method has significantly better performance than GetAsString() 
	 * or _string_value and is preferred when you know the field is a 
	 * 64-bit long.
	 *
	 * \return Field Value as 64-bit long
	 */
	long long GetAsInt64();

	/**
	 * \brief Returns field value as float
	 *
	 * This method has significantly better performance than GetAsString() 
	 * or _string_value and is preferred when you know the field is a 
	 * float.
	 *
	 * \return Field Value as float
	 */
	float GetAsFloat();

	/**
	 * \brief Returns field value as double
	 *
	 * This method has significantly better performance than GetAsString() 
	 * or _string_value and is preferred when you know the field is a 
	 * double.
	 *
	 * \return Field Value as double
	 */
	double GetAsDouble();

	/**
	 * \brief Returns field value as String
	 *
	 * This method is expensive.  It is preferred that you use the 
	 * following logic:
	 * \include FieldCLI_example.h
	 *
	 * \param bShowType - true to include type prefix - (str), (int), etc.
	 * \return Field Value as String
	 */
	String ^GetAsString( bool bShowType );

	/** \brief Returns field value as ByteStreamFld
	 *
	 * \return Field value as ByteStreamFld
	 */
	ByteStreamFld ^GetAsByteStream();

	/**
	 * \brief Dumps field contents as string
	 *
	 * \return Field contents as string
	 */
	String ^Dump();

	/** \brief Returns field value as System::DateTime
	 *
	 * \return Field value as System::DateTime
	 */
	DateTime ^GetAsDateTime();


	/////////////////////////////////
	// Operations
	/////////////////////////////////
public:
	/**
	 * \brief Dumps field contents to Console as [fid (type)] value
	 */
	void DumpToConsole();


	/////////////////////////////////
	//  Mutator
	/////////////////////////////////
	/**
	 * \brief Called by rtEdgeData::field() to set the internal state
	 * of this reusable field.
	 *
	 * \param fld - Current field value from rtEdgeSubscriber channel.
	 * \return this
	 */
	rtEdgeField ^Set( RTEDGE::Field *fld );

	/**
	 * \brief Copy contents of sourc field into this field
	 *
	 * \param src - Source field to copy in
	 */
	void Copy( RTEDGE::Field *src );

	/**
	 * \brief Clear out internal state so this field may be reused.
	 */
	void Clear();

	/** \brief Backwards compatibility */
	String ^Data() { return GetAsString( false ); }


	/////////////////////////////////
	// Helpers
	/////////////////////////////////
private:
	int _WithinRange( int, int, int );

}; // class rtEdgeField

} // namespace librtEdge
