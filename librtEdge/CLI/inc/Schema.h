/******************************************************************************
*
*  Schema.h
*
*  REVISION HISTORY:
*     14 NOV 2014 jcs  Created.
*      9 FEB 2020 jcs  Build 42: GetDef()
*     23 OCT 2022 jcs  Build 58: Formatting
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#include <Field.h>
#endif // DOXYGEN_OMIT

namespace librtEdge 
{

////////////////////////////////////////////////
//
//       c l a s s   r t E d g e S c h e m a
//
////////////////////////////////////////////////

/**
 * \class rtEdgeSchema
 * \brief A single market data update from the rtEdgeSubscriber 
 * channel.
 *
 * This class is reused by rtEdgeSubscriber.  When you receive it
 * in rtEdgeSubscriber::OnData(), it is volatile and only valid 
 * for the life of the callback.
 */
public ref class rtEdgeSchema
{
private:
	RTEDGE::Schema *_schema;
	RTEDGE::Field  *_fldC;
	rtEdgeField    ^_fld;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/**
	 * \brief Reusable message object constructor
	 *
	 * This (reusable) object is created once by the rtEdgeSubscriber
	 * and reused on every market data update sent to you via  
	 * rtEdgeSubscriber::OnData() as follows:
	 *
	 * \include rtEdgeData_OnData.h
	 */
public:
	rtEdgeSchema();
	~rtEdgeSchema();


	////////////////////////////////////
	// Access - Iterate All Fields
	////////////////////////////////////
public:
	/**
	 * \brief Advances the iterator to the next field.
	 *
	 * \include rtEdgeData_iterate.h
	 *
	 * \return true if not end of message; false if EOM.
	 */
	void reset();

	/** 
	 * \brief Advances the iterator to the next field.
	 *   
	 * \include rtEdgeData_iterate.h
	 *   
	 * \return true if not end of message; false if EOM.
	 */  
	bool forth();

	/** 
	 * \brief Return Field at the current iterator position
	 *   
	 * \include rtEdgeData_iterate.h
	 *   
	 * \return Field at the current iterator position
	 */  
	rtEdgeField ^field();


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Schema Size
	 *
	 * \return Schema Size
	 */
	int Size();

	/** 
	 * \brief Return Field Definition by Name
	 *   
	 * \param name - Field Name
	 * \return Field Definition by Name
	 */  
	rtEdgeField ^GetDef( String ^name );

	/** 
	 * \brief Return Field Definition by Field ID
	 *   
	 * \param fid - Field ID
	 * \return Field Definition by Field ID
	 */  
	rtEdgeField ^GetDef( int fid );

	/**
	 * \brief Returns field ID from field name
	 *
	 * \param fld - Field name
	 * \return Field ID for name
	 */
	int Fid( String ^fld );

	/**
	 * \brief Returns field name from field ID
	 *
	 * \param fid - Field ID
	 * \return Field name for ID
	 */
	String ^Name( int fid );

	/**
	 * \brief Returns field type from field ID
	 *
	 * \param fid - Field ID
	 * \return Field type for ID
	 */
	rtFldType Type( int fid );

	/**
	 * \brief Returns field type from field name
	 *
	 * \param fld - Field name
	 * \return Field type for name
	 */
	rtFldType Type( String ^fld );


	////////////////////////////////////
	// Mutator
	////////////////////////////////////
public:
	/**
	 * \brief Set guts from channel Schema
	 *
	 * \param sch - RTEDGE::Schema from channel
	 */
	void Set( RTEDGE::Schema &sch );

};  // class rtEdgeSchema

} // namespace librtEdge
