/******************************************************************************
*
*  ChartDB.hpp
*     librtEdge ChartDB read-only access
*
*  REVISION HISTORY:
*     14 NOV 2014 jcs  Created.
*     12 OCT 2015 jcs  Build 32: class CDBData; CDBTable
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_CDB_H
#define __RTEDGE_CDB_H
#include <hpp/rtEdge.hpp>

typedef std::vector<std::string> VecString;

#define _DZERO               (float)0.00001
#define _IsZero(c)            InRange( -_DZERO, (c), _DZERO )

namespace RTEDGE
{

// Forward declarations

class ChartDB;


////////////////////////////////////////////////
//
//        c l a s s   C D B D a t a
//
////////////////////////////////////////////////

/**
 * \class CDBData
 * \brief Wrapper around ::CDBData
 *
 */
class CDBData : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor.  Initializes CDBData internals */
	CDBData( ChartDB &chartDb ) :
	   _ChartDb( chartDb ),
	   _dump()
	{
		::memset( &_data, 0, sizeof( _data ) );
	}

	virtual ~CDBData()
	{
		reset();
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/** 
	 * \brief Return ChartDb sourcing us
	 *
	 * \return ChartDb sourcing us
	 */
	ChartDB &chartDb()
	{
		return _ChartDb;
	}

	/** 
	 * \brief Return ::CDBData struct
	 *
	 * \return ::CDBData struct
	 */
	::CDBData &data()
	{
		return _data;
	}

	/**
	 * \brief Return DB record Service name
	 *
	 * \return DB record Service name
	 */
	const char *pSvc()
	{
		return _data._pSvc;
	}

	/**
	 * \brief Return DB record Ticker name
	 *
	 * \return DB record Ticker name
	 */
	const char *pTkr()
	{
		return _data._pTkr;
	}

	/**
	 * \brief Return query error string
	 *
	 * \return query error string
	 */
	const char *pErr()
	{
		return _data._pErr;
	}

	/**
	 * \brief Returns Field ID being recorded
	 *
	 * \return Field ID being recorded
	 */
	int Fid()
	{
		return _data._fid;
	}

	/**
	 * \brief Returns time series array
	 *
	 * \return Time series array
	 */
	float *flds()
	{
		return _data._flds;
	}

	/**
	 * \brief Returns Size of flds() array
	 *
	 * \return Size of flds() array
	 */
	int Size()
	{
		return _data._curTick;
	}

	/**
	 * \brief Returns Unix time of the n'th data point in this time series
	 *
	 * \param n - Data point selection
	 * \return Unix time of the n'th data point in this time series
	 */
	time_t SeriesTime( int n )
	{
		time_t     rtn, tUpd;
		struct tm *lt, lBuf;
		int        ns;

		// 1) Unix time of last update

		tUpd        = _data._tUpd ? _data._tUpd : TimeSec();
		lt          = ::localtime_r( &tUpd, &lBuf );
		ns          = n * _data._interval;
		lt->tm_hour = ( ns / 3600 );
		lt->tm_min  = ( ns % 3600 ) / 60;
		lt->tm_sec  = ( ns % 60 );
		rtn         = ::mktime( lt );
		return rtn;
   }

	/**
	 * \brief Returns formatted time of n'th data point in this series
	 * formatted as HH:MM:SS.
	 *
	 * \param n - Data point selection
	 * \param s - std::string to hold   point selection
	 * \return Formatted time of n'th data point
	 */
	const char *pSeriesTime( int n, std::string &s )
	{
		struct tm *lt, lBuf;
		time_t     tm;
		char       buf[K];

		// 1) Unix time of last update

		tm = SeriesTime( n );
		lt = ::localtime_r( &tm, &lBuf );
		sprintf( buf, "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec );
		s = buf;
		return s.data();
   }

	/** 
	 * \brief Dumps time-series to 2-column CSV string
	 *
	 * \param bNonZero - true to only show non-zero values
	 * \return Time-series as 2-column string
	 */
	const char *Dump( bool bNonZero=true )
	{
		::CDBData  &t = _data;
		std::string tm;
		char       *bp, *cp;
		float      *fp;
		int         i, sz;

		// Figure about 40 bytes / data point

		sz  = gmax( K, Size() );
		sz *= 40;
		bp  = new char[sz];
		cp  = bp;
		*cp = '\0';

		// Do it

		fp = flds();
		cp += sprintf( cp, "Time,%s,\n", pTkr() );
		for ( i=t._curTick-1; i>=0; i-- ) {
			if ( _IsZero( fp[i] ) && bNonZero )
				break; // for-i
			cp += sprintf( cp, "%s,", pSeriesTime( i, tm ) );
			cp += sprintf( cp, "%.4f\n", fp[i] ); // TODO : Trim
		}

		// Stuff into std::string

		_dump = bp;
		delete[] bp;
		return _dump.data();
	}


	////////////////////////////////////
	// Mutator
	////////////////////////////////////
	/** \brief Reset guts */
	void reset()
	{
		::CDB_Free( &_data );
		::memset( &_data, 0, sizeof( _data ) );
	}

	/** 
	 * \brief Set internal guts from LVCDataAll
	 *
	 * \return Array of Message's
	 */
	CDBData &Set( ::CDBData data )
	{
		_data = data;
		return *this;
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	ChartDB    &_ChartDb;
	::CDBData   _data;
	std::string _dump;

};  // class CDBData




////////////////////////////////////////////////
//
//        c l a s s   C D B T a b l e
//
////////////////////////////////////////////////

/**
 * \class CDBTable
 * \brief Container for a query for time-series from multiple tickers.
 */
class CDBTable : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor.  Initializes CDBTable internals */
	CDBTable( ChartDB &chartDb ) :
	   _ChartDb( chartDb ),
	   _svc(),
	   _tkrs(),
	   _err(),
	   _dump()
	{
		::memset( &_tbl, 0, sizeof( _tbl ) );
	}

	virtual ~CDBTable()
	{
		reset();
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/** 
	 * \brief Return ChartDb sourcing us
	 *
	 * \return ChartDb sourcing us
	 */
	ChartDB &chartDb()
	{
		return _ChartDb;
	}

	/** 
	 * \brief Return ::CDBData struct
	 *
	 * \return ::CDBData struct
	 */
	::CDBData &data()
	{
		return _tbl;
	}

	/**
	 * \brief Return DB record Service name
	 *
	 * \return DB record Service name
	 */
	const char *pSvc()
	{
		return _svc.data();
	}

	/**
	 * \brief Return DB record Ticker name
	 *
	 * \param nt - Ticker index
	 * \return DB record Ticker name
	 */
	const char *pTkr( int nt )
	{
		return InRange( 0, nt, NumTkr()-1 ) ? _tkrs[nt].data() : NULL;
	}

	/**
	 * \brief Return number of tickers in result set
	 *
	 * \return Number of tickers in result set
	 */
	int NumTkr()
	{
		return _tkrs.size();
	}

	/**
	 * \brief Return query error string
	 *
	 * \return query error string
	 */
	const char *pErr()
	{
		return _tbl._pErr;
	}

	/**
	 * \brief Returns Field ID being recorded
	 *
	 * \return Field ID being recorded
	 */
	int Fid()
	{
		return _tbl._fid;
	}

	/**
	 * \brief Returns time series array
	 *
	 * \return Time series array
	 */
	float *flds()
	{
		return _tbl._flds;
	}

	/**
	 * \brief Returns Size of flds() array
	 *
	 * \return Size of flds() array
	 */
	int Size()
	{
		return( _tbl._curTick * NumTkr() );
	}

	/**
	 * \brief Returns Unix time of the n'th data point in this time series
	 *
	 * \param n - Data point selection
	 * \return Unix time of the n'th data point in this time series
	 */
	time_t SeriesTime( int n )
	{
		time_t     rtn, tUpd;
		struct tm *lt, lBuf;
		int        ns;

		// 1) Unix time of last update

		tUpd        = _tbl._tUpd ? _tbl._tUpd : TimeSec();
		lt          = ::localtime_r( &tUpd, &lBuf );
		ns          = n * _tbl._interval;
		lt->tm_hour = ( ns / 3600 );
		lt->tm_min  = ( ns % 3600 ) / 60;
		lt->tm_sec  = ( ns % 60 );
		rtn         = ::mktime( lt );
		return rtn;
   }

	/**
	 * \brief Returns formatted time of n'th data point in this series
	 * formatted as HH:MM:SS.
	 *
	 * \param n - Data point selection
	 * \param s - std::string to hold   point selection
	 * \return Formatted time of n'th data point
	 */
	const char *pSeriesTime( int n, std::string &s )
	{
		struct tm *lt, lBuf;
		time_t     tm;
		char       buf[K];

		// 1) Unix time of last update

		tm = SeriesTime( n );
		lt = ::localtime_r( &tm, &lBuf );
		sprintf( buf, "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec );
		s = buf;
		return s.data();
   }


	////////////////////////////////////
	// Operations
	////////////////////////////////////
	/** \brief Reset guts */
	void reset()
	{
		::CDB_Free( &_tbl );
		_svc = "";
		_err = "";
		_tkrs.clear();
	}

	/** 
	 * \brief Set internal guts from mutliple calls to CDB_View()
	 *
	 * \param cxt - Context
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkrs - List of Tickers (e.g. EUR CURNCY)
	 * \param nTkr - Number of tkrs
    * \param fid - Field ID
	 * \return Array of Message's
	 */
	CDBTable &View( CDB_Context  cxt, 
	                const char  *svc, 
	                const char **tkrs, 
	                int          nTkr, 
	                int          fid )
	{
		::CDBData  &t = _tbl;
		::CDBData   d;
		double      d0;
		char        err[K];
		const char *fmt;
		int         i, nPt, tblIntvl;
		size_t      fSz;
		float      *fp;

		// Rock and Roll

		reset();
		d0      = rtEdge::TimeNs(); 
		_svc    = svc;
		t._pSvc = _svc.data();
		t._fid  = fid;
		fSz     = sizeof( float );
		nPt     = nTkr+1;
		if ( nTkr ) {
			_tkrs.push_back( std::string( tkrs[0] ) );
			d           = ::CDB_View( cxt, svc, tkrs[0], fid );
			t._interval = d._interval;
			t._curTick  = d._curTick;
			t._numTick  = d._numTick;
			t._nUpd     = d._nUpd;
			fSz        *= t._curTick;
			nPt        *= t._curTick;
			fp          = new float[nPt];
			t._flds     = fp;
			::memcpy( fp, d._flds, fSz );
			fp         += t._curTick;
		}
		tblIntvl = t._interval;
		for ( i=1; i<nTkr; i++ ) {
			d = ::CDB_View( cxt, svc, tkrs[0], fid );
			if ( d._interval != tblIntvl ) {
				reset();
				fmt = "Series interval from %s = %d; [%d] %s interval = %d";
				sprintf( err, fmt, tkrs[0], tblIntvl, i+1, tkrs[i], d._interval );
				_err    = err;
				t._pErr = _err.data();
				return *this;
			}
			_tkrs.push_back( std::string( tkrs[i] ) );
			::memcpy( fp, d._flds, fSz );
			t._nUpd += d._nUpd;
			fp      += d._curTick;
			::CDB_Free( &d );
		}
		t._dSnap = rtEdge::TimeNs() - d0;
		return *this;
	}

	/** 
	 * \brief Dumps table to CSV string
	 *
	 * \param byTime - true for row = time; false for row = ticker
	 * \return Table as CSV string
	 */
	const char *Dump( bool byTime=true )
	{
		CDBData     d( _ChartDb );
		::CDBData  &t = _tbl;
		std::string tm;
		char       *bp, *cp, *tp;
		float      *fp;
		float       dv;
		bool        bZ;
		int         i, j, sz, nt;

		// Figure about 40 bytes / data point

		sz  = gmax( K, Size() );
		sz *= 40;
		bp  = new char[sz];
		cp  = bp;
		*cp = '\0';

		// Do it

byTime = true;
		d.Set( _tbl );
		fp = flds();
		nt = t._curTick;
		if ( byTime ) {
			cp += sprintf( cp, "Time," );
			for ( i=0; i<NumTkr(); cp += sprintf( cp, "%s,", pTkr( i++ ) ) );
			cp += sprintf( cp, "\n" );
			for ( i=t._curTick-1; i>=0; i-- ) {
				tp  = cp;
				cp += sprintf( cp, "%s,", d.pSeriesTime( i, tm ) );
				bZ  = true;
				for ( j=0; j<NumTkr(); j++ ) {
					dv  = fp[i+(nt*j)];
					cp += sprintf( cp, "%.4f,", dv ); // TODO : Trim
					bZ &= _IsZero( dv );
				}
				cp += sprintf( cp, "\n" );
				cp  = bZ ? tp : cp;
			}
		}
		else {
		}

		// Stuff into std::string

		_dump = bp;
		delete[] bp;
		return _dump.data();
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	ChartDB    &_ChartDb;
	::CDBData   _tbl;
	std::string _svc;
	VecString   _tkrs;
	std::string _err;
	std::string _dump;

};  // class CDBTable



////////////////////////////////////////////////
//
//         c l a s s    C h a r t D B
//
////////////////////////////////////////////////

/**
 * \class ChartDB
 * \brief Read-only view on the Chart DB historical DB file
 */

class ChartDB
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  Initializes ChartDB internals
	 * 
	 * \param pFile - ChartDB filename
	 * \param pAdmin - host:port of ChartDB admin channel 
	 */
	ChartDB( const char *pFile, const char *pAdmin="localhost:8765" ) :
		_file( pFile ),
		_admin( pAdmin ),
		_cxt( (CDB_Context)0 ),
		_qry( *this ),
		_tbl( *this )
	{
		::memset( &_qryAll, 0, sizeof( _qryAll ) );
		_cxt = ::CDB_Initialize( pFile, pAdmin );
	}

	virtual ~ChartDB()
	{
		Free();
		_tbl.reset();
		if ( _cxt )
			::CDB_Destroy( _cxt );
		_cxt = (CDB_Context)0;
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/**
	 * \brief Returns CDB_Context associated with this read-only viewer
	 *
	 * \return CDB_Context associated with this read-only viewer
	 */
	CDB_Context cxt()
	{
		return _cxt;
	}

	/**
	 * \brief Returns ChartDB filename
	 *
	 * \return ChartDB filename
	 */
	const char *pFilename()
	{
		return _file.c_str();
	}

	/**
	 * \brief Returns comma-separated list ChartDB admin \<host\>:\<port\>
	 *
	 * \return Comma-separated list of ChartDB admin
	 */
	const char *pAdmin()
	{
		return _admin.c_str();
	}


	////////////////////////////////////
	// ChartDB Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return true is we are Start()'ed
	 *
	 * \return  true if we have Start()'ed but not Stop()'ed.
	 */
	bool IsValid()
	{
		return( _cxt != (CDB_Context)0 );
	}


	////////////////////////////////////
	// Database Directory Query
	////////////////////////////////////
	/**
	 * \brief Query ChartDB for directory of all tickers
	 *
	 * \return ::CDBQuery struct with list of all tickers
	 */
	::CDBQuery Query()
	{
		FreeQry();
		_qryAll = ::CDB_Query( _cxt );
		return _qryAll;
	}

	/**
	 * \brief Release resources associated with the last call to Query(). 
	 */
	void FreeQry()
	{
		::CDB_FreeQry( &_qryAll );
		::memset( &_qryAll, 0, sizeof( _qryAll ) );
	}


	////////////////////////////////////
	// Time-Series Query
	////////////////////////////////////
	/**
	 * \brief Query ChartDB for current values for spcecific ticker.
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \param fid - Field ID
	 * \return CDBData class with time-series values
	 */
	CDBData &View( const char *svc, const char *tkr, int fid )
	{
		::CDBData d;

		Free();
		d = ::CDB_View( _cxt, svc, tkr, fid );
		_qry.Set( d );
		return _qry;
	}

	/**
	 * \brief Query ChartDB for current values for spcecific ticker.
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkrs - List of Tickers (e.g. EUR CURNCY)
	 * \param nTkr - Number of tkrs
	 * \param fid - Field ID
	 * \return CDBTable class with time-series values
	 */
	CDBTable &ViewTable( const char *svc, const char **tkrs, int nTkr, int fid )
	{
		_tbl.reset();
		_tbl.View( _cxt, svc, tkrs, nTkr, fid );
		return _tbl;
	}

	/**
	 * \brief Release resources associated with the last call to View(). 
	 */
	void Free()
	{
		_qry.reset();
	}


	////////////////////////////////////
	// DB Modification
	////////////////////////////////////
	/**
	 * \brief Add new ( svc,tkr,fid ) time-series stream to ChartDB
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 * \param fid - Field ID
	 */
	void AddTicker( const char *svc, const char *tkr, int fid )
	{
		::CDB_AddTicker( _cxt, svc, tkr, fid );
	}

	/**
	 * \brief Remove ( svc,tkr,fid ) time-series stream to ChartDB
	 *
	 * \param svc - Service name
	 * \param tkr - Ticker name; Comma-separated for multiple tickers
	 * \param fid - Field ID
	 */
	void DelTicker( const char *svc, const char *tkr, int fid )
	{
		::CDB_DelTicker( _cxt, svc, tkr, fid );
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	std::string _file;
	std::string _admin;
	CDB_Context _cxt;
	CDBData     _qry;
	CDBTable    _tbl;
	::CDBQuery  _qryAll;

};  // class ChartDB

} // namespace RTEDGE

#endif // __RTEDGE_CDB_H 
