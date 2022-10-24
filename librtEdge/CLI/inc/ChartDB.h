/******************************************************************************
*
*  ChartDB.h
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*     13 OCT 2015 jcs  Build 32: CDBTable / ViewTable()
*     10 SEP 2020 jcs  Build 44: MDDResult
*     23 OCT 2022 jcs  Build 58: cli::array<>
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <librtEdge.h>
#include <rtEdge.h>
#endif // DOXYGEN_OMIT

namespace librtEdge
{
/////////////
// Forwards
/////////////
ref class ChartDB;

////////////////////////////////////////////////
//
//         c l a s s   C D B D a t a
//
////////////////////////////////////////////////

/**
 * \class CDBData
 * \brief View of the historical time-series data for a single record
 * from the ChartDB time-series database.
 *
 * This class is reused by ChartDB via ChartDB::View()
 */
public ref class CDBData
{
private: 
	ChartDB         ^_cdb;
	RTEDGE::CDBData *_data;
	String          ^_svc;
	String          ^_tkr;
	String          ^_err;
	cli::array<float>    ^_fdb;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/**
	 * \brief Reusable message object constructor
	 *
	 * This (reusable) object is created once by the ChartDb and reused on 
	 * every ChartDb::View() call.
	 *
	 * \include CDBData_OnData.h
	 */
public:
	CDBData( ChartDB ^ );
	~CDBData();

	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return native CDBData object populating this object
	 *
	 * \return Native CDBData object populating this object
	 */
	RTEDGE::CDBData *data();

	/**
	 * \brief Returns formatted time of n'th data point in this series 
	 * formatted as HH:MM:SS
	 *
	 * \param n - Data point selection
	 * \return Time as HH:MM:SS
	 */
	String ^SeriesTime( int n );

	/**
	 * \brief Dumps time-series to 2-column CSV string, only non-zero
	 * values.
	 *
	 * \return Time-series as 2-column string
	 */
	String ^DumpNonZero();

	/**
	 * \brief Dumps entire time-series to 2-column CSV string
	 *
	 * \return Time-series as 2-column string
	 */
	String ^Dump();


	/////////////////////////////////
	//  Operations
	/////////////////////////////////
	/**
	 * \brief Called by ChartDB::View() to state of this reusable messsage.
	 *
	 * \param data - Current time-series contents from ChartDB
	 */
	void Set( RTEDGE::CDBData &data );

	/**
	 * \brief Clear out internal state so this message may be reused.
	 */
	void Clear();


	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns name of service supplying this update */
	property String ^_pSvc
	{
	   String ^get() {
	      if ( _svc == nullptr )
	         _svc = gcnew String( _data->pSvc() );
	      return _svc;
	   }
	}

	/** \brief Returns ticker name of this update */
	property String ^_pTkr
	{
	   String ^get() {
	      if ( _tkr == nullptr )
	          _tkr = gcnew String( _data->pTkr() );
	      return _tkr;
	   }
	}

	/** \brief Returns error (if any) from this update */
	property String ^_pErr
	{
	   String ^get() {
	      if ( _err == nullptr )
	          _err = gcnew String( _data->pErr() );
	      return _err;
	   }
	}

	/** \brief Returns time-series field ID */
	property u_int _fid
	{
	   u_int get() { return (u_int)_data->Fid(); }
	}

	/** \brief Returns time-series interval */
	property u_int _interval
	{
	   u_int get() { return (u_int)_data->data()._interval; } 
	}

	/** \brief Returns current time-series position */
	property u_int _curTick
	{
	   u_int get() { return (u_int)_data->Size(); }
	}

	/** \brief Returns total length of time series */
	property u_int _numTick
	{
	   u_int get() { return (u_int)_data->data()._numTick; } 
	}

	/** \brief Returns create time in ChartDb (Unix time) */
	property u_int _tCreate
	{
	   u_int get() { return (u_int)_data->data()._tCreate; } 
	}

	/** \brief Returns update time in ChartDb (Unix time) */
	property u_int _tUpd
	{
	   u_int get() { return (u_int)_data->data()._tUpd; } 
	}

	/** \brief Returns update time micros in ChartDb */
	property u_int _tUpdUs
	{
	   u_int get() { return (u_int)_data->data()._tUpdUs; } 
	}

	/** \brief Returns age in ChartDb in seconds */
	property double _dAge
	{
	   double get() { return _data->data()._dAge; } 
	}

	/** \brief Returns time ticker became DEAD in ChartDb (Unix time) */
	property u_int _tDead
	{
	   u_int get() { return (u_int)_data->data()._tDead; } 
	}

	/** \brief Returns num update in ChartDb */
	property u_int _nUpd
	{
	   u_int get() { return (u_int)_data->data()._nUpd; } 
	}

	/** \brief Returns snap time in seconds */
	property double _dSnap
	{
	   double get() { return _data->data()._dSnap; } 
	}

	/** \brief Returns Field List from this update */
	property cli::array<float> ^_flds
	{
	   cli::array<float> ^get() {
	      u_int i, nf;

	      // Create once per query

	      nf = _curTick;
	      if ( _fdb == nullptr ) {
	         _fdb = gcnew cli::array<float>( nf );
	         for ( i=0; i<nf; _fdb[i]=_data->flds()[i], i++ );
	      } 
	      return _fdb;
	   }
	}

};  // class CDBData



////////////////////////////////////////////////
//
//         c l a s s   C D B T a b l e
//
////////////////////////////////////////////////

/**
 * \class CDBTable
 * \brief View of the historical time-series data for multiple records
 * from the ChartDB time-series database.
 *
 * This class is reused by ChartDB via ChartDB::ViewTable()
 */
public ref class CDBTable
{
private: 
	ChartDB          ^_cdb;
	RTEDGE::CDBTable *_tbl;
	String           ^_svc;
	String           ^_err;
	cli::array<float>     ^_fdb;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/**
	 * \brief Reusable message object constructor
	 *
	 * This (reusable) object is created once by the ChartDb and reused on 
	 * every ChartDb::ViewTable() call.(
	 *
	 * \include CDBData_OnData.h
	 */
public:
	CDBTable( ChartDB ^ );
	~CDBTable();


	/////////////////////////////////
	// Access
	/////////////////////////////////
	/** 
	 * \brief Return DB record Ticker name
	 *   
	 * \param nt - Ticker index
	 * \return DB record Ticker name
	 */  
	String ^pTkr( int nt );

	/**
	 * \brief Dumps time-series to (N+1)-column CSV string
	 *
	 * \return Time-series as (N+1)-column string
	 */
	String ^DumpByTime();

	/**
	 * \brief Dumps entire time-series to (N+1)-column CSV string
	 *
	 * \return Time-series as (N+1)-column string
	 */
	String ^DumpByTicker();


	/////////////////////////////////
	// Operations
	/////////////////////////////////
	/**
	 * \brief Called by ChartDB::View() to state of this reusable messsage.
	 *
	 * \param tbl - Current time-series contents from ChartDB
	 */
	void Set( RTEDGE::CDBTable &tbl );

	/**
	 * \brief Clear out internal state so this message may be reused.
	 */
	void Clear();


	/////////////////////////////////
	// Properties
	/////////////////////////////////

public:
	/** \brief Returns name of service supplying this update */
	property String ^_pSvc
	{
	   String ^get() {
	      if ( _svc == nullptr )
	         _svc = gcnew String( _tbl->pSvc() );
	      return _svc;
	   }
	}

	/** \brief Returns error (if any) from this update */
	property String ^_pErr
	{
	   String ^get() {
	      if ( _err == nullptr )
	          _err = gcnew String( _tbl->pErr() );
	      return _err;
	   }
	}

	/** \brief Returns time-series field ID */
	property u_int _nTkr
	{
	   u_int get() { return (u_int)_tbl->NumTkr(); }
	}

	/** \brief Returns time-series field ID */
	property u_int _fid
	{
	   u_int get() { return (u_int)_tbl->Fid(); }
	}

	/** \brief Returns size of _flds array */
	property u_int _size
	{
	   u_int get() { return (u_int)_tbl->Size(); }
	}

	/** \brief Returns time-series interval */
	property u_int _interval
	{
	   u_int get() { return (u_int)_tbl->data()._interval; } 
	}

	/** \brief Returns current time-series position */
	property u_int _curTick
	{
	   u_int get() { return (u_int)_tbl->data()._curTick; }
	}

	/** \brief Returns total length of time series */
	property u_int _numTick
	{
	   u_int get() { return (u_int)_tbl->data()._numTick; } 
	}

	/** \brief Returns num update in ChartDb */
	property u_int _nUpd
	{
	   u_int get() { return (u_int)_tbl->data()._nUpd; } 
	}

	/** \brief Returns snap time in seconds */
	property double _dSnap
	{
	   double get() { return _tbl->data()._dSnap; } 
	}

	/** \brief Returns Field List from this update */
	property cli::array<float> ^_flds
	{
	   cli::array<float> ^get() {
	      u_int  i, nf;
	      float *fp;

	      // Create once per query

	      nf = _size;
	      fp = _tbl->flds();
	      if ( _fdb == nullptr ) {
	         _fdb = gcnew cli::array<float>( nf );
	         for ( i=0; i<nf; _fdb[i]=fp[i], i++ );
	      } 
	      return _fdb;
	   }
	}

};  // class CDBTable

////////////////////////////////////////////////
//
//       c l a s s   M D D R e c R e f
//
////////////////////////////////////////////////

/**
 * \class MDDRecDef
 * \brief Definition of a single record in the ChartDB database
 */
public ref class MDDRecDef
{
private:
	String ^_svc;
	String ^_tkr;
	int     _Fid;
	int     _Interval;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
	/** \brief Constructor */
public:
	MDDRecDef( ::MDDRecDef );
	~MDDRecDef();


	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns name of service supplying this update */
	property String ^_pSvc
	{
	   String ^get() { return _svc; }
	}

	/** \brief Returns ticker name of this update */
	property String ^_pTkr
	{
	   String ^get() { return _tkr; }
	}

	/** \brief Returns time-series field ID */
	property int _fid
	{
	   int get() { return _Fid; }
	}

	/** \brief Returns time-series interval */
	property int _interval
	{
	   int get() { return _Interval; }
	}

};  // class MDDRecDef


////////////////////////////////////////////////
//
//        c l a s s   M D D R e s u l t
//
////////////////////////////////////////////////

/**
 * \class MDDResult
 * \brief Complete list of tickers in the ChartDB time-series DB
 */
public ref class MDDResult
{
private: 
	::MDDResult         &_qry;
	cli::array<MDDRecDef ^> ^_rdb;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/** \brief Constructor */
public:
	MDDResult( ::MDDResult & );
	~MDDResult();


	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns Field List from this update */
	property cli::array<MDDRecDef ^> ^_recs
	{
	   cli::array<MDDRecDef ^> ^get() {
	      u_int i, nf;

	      // Create once per query

	      nf = _nRec;
	      if ( _rdb == nullptr ) {
	         _rdb = gcnew cli::array<MDDRecDef ^>( nf );
	         for ( i=0; i<nf; i++ )
	            _rdb[i] = gcnew MDDRecDef( _qry._recs[i] );
	      } 
	      return _rdb;
	   }
	}

	/** \brief Returns number of time-series streams in ChartDB */
	property u_int _nRec
	{
	   u_int get() { return (u_int)_qry._nRec; }
	}

	/** \brief Returns time to snap in seconds */
	property double _dSnap
	{
	   double get() { return _qry._dSnap; } 
	}

};  // class MDDResult



////////////////////////////////////////////////
//
//          c l a s s   C h a r t D B
//
////////////////////////////////////////////////
/**
 * \class ChartDB
 * \brief  Read-only view on Last Value Cache (ChartDB) file
 */
public ref class ChartDB : public rtEdge
{
private: 
	RTEDGE::ChartDB *_cdb;
	::MDDResult      *_qryAll;
	CDBData         ^_qry;
	CDBTable        ^_tbl;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * The constructor will attach to the ChartDB time-series historical 
	 * database and initialize internal variables.  Your call View() to 
	 * query the time-series data.
	 *
	 * \param pFile - ChartDB filename
	 * \param pAdmin - host:port of ChartDB admin channel
	 */
	ChartDB( String ^pFile, String ^pAdmin );

	/**
	 * \brief Destructor.  Cleans up internally.
	 */
	~ChartDB();


	////////////////////////////////////
	// Cache Query
	////////////////////////////////////
public:
	/** 
	 * \brief Query ChartDB for current values of a single market data record
	 *   
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \param fid - Field ID
	 * \return Current values in CDBData
	 */  
	CDBData ^View( String ^svc, String ^tkr, int fid );

	/** 
	 * \brief Query ChartDB for current values for spcecific ticker.
	 *   
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkrs - List of Tickers (e.g. EUR CURNCY)
	 * \param fid - Field ID
	 * \return CDBTable class with time-series values
	 */  
	CDBTable ^ViewTable( String ^svc, cli::array<String ^> ^tkrs, int fid );

	/**
	 * \brief Release resources associated with last call to View().
	 */
	   void Free();

	/** 
	 * \brief Query ChartDB for complete list of tickers
	 *   
	 * \return Complete list of tickers in MDDResult object
	 */  
	MDDResult ^Query();

	/**
	 * \brief Release resources associated with last call to Query().
	 */
	void FreeResult();


	////////////////////////////////////
	// DB Mutator
	////////////////////////////////////
public:
	/**
	 * \brief Add new ( svc,tkr,fid ) time-series stream to ChartDB
	 *
	 * This is for backwards compatibility with librtEdgeDotNet
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 */
	void AddTicker( String ^svc, String ^tkr );

	/**
	 * \brief Add new ( svc,tkr,fid ) time-series stream to ChartDB
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 * \param fid - Field ID
	 */
	void AddTicker( String ^svc, String ^tkr, int fid );

	/**
	 * \brief Deletes ( svc,tkr,fid ) time-series stream to ChartDB
	 *
	 * This is for backwards compatibility with librtEdgeDotNet
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 */
	void DelTicker( String ^svc, String ^tkr );

	/**
	 * \brief Deletes ( svc,tkr,fid ) time-series stream to ChartDB
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 * \param fid - Field ID
	 */
	void DelTicker( String ^svc, String ^tkr, int fid );

	/** \brief Backwards compatibility */
	void Destroy();

};  // class ChartDB

} // namespace librtEdge
