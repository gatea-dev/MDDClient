/******************************************************************************
*
*  LVC.h
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*      5 FEB 2016 jcs  Build 32: IsBinary
*     25 SEP 2017 jcs  Build 35: LVCAdmin.AddTicker()
*     14 JAN 2018 jcs  Build 39: LVCSnap._nullFld
*      7 MAR 2022 jcs  Build 51: LVCAdmin.AddTickers()
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <librtEdge.h>
#include <rtEdge.h>
#include <Data.h>
#include <Schema.h>
#endif // DOXYGEN_OMIT

namespace librtEdge
{

////////////////////////////////////////////////
//
//            c l a s s   L V C
//
////////////////////////////////////////////////
/**
 * \class LVC
 * \brief  Read-only view on Last Value Cache (LVC) file
 */
public ref class LVC : public rtEdge
{
protected: 
	RTEDGE::LVC  *_lvc;
	LVCData      ^_qry;
	LVCDataAll   ^_qryAll;
	rtEdgeSchema ^_schema;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * The constructor will attach to the Last Value Cache and 
	 * initialize internal variables.  You call Snap() or View() to 
	 * query the data.
	 *
	 * \param pFile - LVC filename
	 */
	LVC( String ^pFile );

	/**
	 * \brief Destructor.  Cleans up internally.
	 */
	~LVC();


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/** 
	 * \brief Return rtEdgeSchema for this LVC
	 *   
	 * \return rtEdgeSchema for this LVC
	 */  
	rtEdgeSchema ^schema();


	////////////////////////////////////
	// Cache Query
	////////////////////////////////////
public:
	/** 
	 * \brief Query LVC for current schema
	 *   
	 * \return Current schema in LVCData
	 */  
	rtEdgeSchema ^GetSchema();

	/** 
	 * \brief Query LVC for current values of a single market data record
	 *   
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \return Current values in LVCData
	 */  
	LVCData ^Snap( String ^svc, String ^tkr );

	/** 
	 * \brief Query LVC for current values of a single market data record
	 *   
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \return Current values in LVCData
	 */  
	LVCData ^View( String ^svc, String ^tkr );

	/** 
	 * \brief Release resources associated with last call to View() 
	 * or Snap().
	 */  
	void Free();

	/** 
	 * \brief Query LVC for current values of a ALL market data records
	 *   
	 * \return Current values in LVCDataAll
	 */  
	LVCDataAll ^SnapAll();

	/** 
	 * \brief Query LVC for current values of a ALL market data records
	 *   
	 * \return Current values in LVCDataAll
	 */  
	LVCDataAll ^ViewAll();

	/** 
	 * \brief Release resources associated with last call to ViewAll() 
	 * or SnapAll().
	 */  
	void FreeAll();

	/** \brief Backwards compatibility - Calls GetSchema() */
	rtEdgeSchema ^GetSchema( String ^notUsed ) { return GetSchema(); }

	/** \brief Backwards compatibility */
	void Destroy();

};  // class LVC



////////////////////////////////////////////////
//
//        c l a s s   L V C A d m i n
//
////////////////////////////////////////////////
/**
 * \class LVC
 * \brief Admin channel to LVC
 */
public ref class LVCAdmin : public rtEdge
{
protected: 
	RTEDGE::LVCAdmin *_lvc;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * \param pAdmin - host:port of LVC admin channel
	 */
	LVCAdmin( String ^pAdmin );

	/**
	 * \brief Destructor.  Cleans up internally.
	 */
	~LVCAdmin();


	////////////////////////////////////
	// Admin
	////////////////////////////////////
public:
	/**
	 * \brief Returns comma-separated list LVC admin \<host\>:\<port\>
	 *
	 * \return Comma-separated list of LVC admin
	 */
	String ^Admin();

	/**
	 * \brief Add new ( svc,tkr ) record from LVC
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name
	 */
	void AddTicker( String ^svc, String ^tkr );

	/**
	 * \brief Add list of ( Service, Ticker ) to LVC
	 *
	 * \param svc - Service Name
	 * \param tkrs - Array of tickers to add
	 */
	void AddTickers( String ^svc, array<String ^> tkrs ):

	/**
	 * \brief Delete existing ( svc,tkr ) record from LVC
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 */
	void DelTicker( String ^svc, String ^tkr );

};  // class LVCAdmin


////////////////////////////////////////////////
//
//      c l a s s   L V C S n a p
//
////////////////////////////////////////////////
/**
 * \class LVCSnap
 * \brief Useful utility class for interrogating the contents of 
 * the LVCData structure by either field ID or name.
 */
public ref class LVCSnap
{
private: 
	LVC         ^_lvc;
	LVCData     ^_data;
	rtEdgeField ^_nullFld;
	Hashtable   ^_fidIdx;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * \param lvc - LVC
	 * \param data - LVCData
	 */
	LVCSnap( LVC ^lvc, LVCData ^data );

	/** \brief Destructor.  Cleans up internally. */
	~LVCSnap();


	////////////////////////////////////
	// Operations
	////////////////////////////////////
public:
	/** 
	 * \brief Return true if LVCData contains field by name
	 *   
	 * \param fld - Field name (e.g., BID )
	 * \return true if LVCData has field named fld
	 */  
	bool HasField( String ^fld );

	/** 
	 * \brief Return true if LVCData contains field by ID
	 *   
	 * \param fid - Field ID (e.g., 22 for BID )
	 * \return true if LVCData has field ID
	 */  
	bool HasField( int fid );

	/** 
	 * \brief Return field contents as String
	 *   
	 * \param fld - Field name (e.g., BID )
	 * \return Field contents
	 */  
	String ^GetField( String ^fld );

	/** 
	 * \brief Return true if LVCData contains field by ID
	 *   
	 * \param fid - Field ID (e.g., 22 for BID )
	 * \return Field contents
	 */  
	String ^GetField( int fid );

	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	rtEdgeField ^_Field( int fid );
	int          _Fid( String ^fld );
	void         _Parse();

};  // class LVCSnap


////////////////////////////////////////////////
//
//      c l a s s   L V C V o l a t i l e
//
////////////////////////////////////////////////
/**
 * \class LVCVolatile
 * \brief A high-performance class for accessing LVC data over and over.  
 *
 * LVCVolatile takes advantage of the fact that LVC records are not added 
 * or removed very frequently.  Typically you seed the LVC once per day 
 * or trading session.  Since the dataset changes infrequently, 
 * LVCVolatile discovers at constructor time the cache index of every 
 * item, stores the indices internally, then allows you to query by 
 * index.  Since querying by index is extremely quick, the performance 
 * benefit it passed onto your application.
 */
public ref class LVCVolatile : public LVC
{
private:
	Dictionary<String ^, int> ^_idxs;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * \param pFile - LVC filename
	 */
	LVCVolatile( String ^pFile );

	/** \brief Destructor.  Cleans up internally. */
	~LVCVolatile();


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/** 
	 * \brief Returns the index in the LVCDataAll for ( svc, tkr )
	 *   
	 * \param svc - Service name (e.g., BLOOMBERG)
	 * \param tkr - Ticker name (e.g., EUR CURNCY)
	 * \return Index in the LVCDataAll struct
	 */  
	int GetIdx( String ^svc, String ^tkr );

	/** 
	 * \brief Snap view of data stream referenced by index
	 *   
	 * \param idx - Index of view from GetIdx() above
	 * \return Data view in LVCData object
	 */  
	LVCData ^ViewByIdx( int idx );


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	void BuildIdxDb();

};  // class LVCVolatile

} // namespace librtEdge
