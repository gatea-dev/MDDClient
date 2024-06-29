/******************************************************************************
*
*  LVC.hpp
*     librtEdge LVC read-only access
*
*  REVISION HISTORY:
*     14 NOV 2014 jcs  Created.
*      4 MAY 2015 jcs  Build 31: Fully-qualified std:: (compiler)
*      5 FEB 2016 jcs  Build 32: LVC : public rtEdge
*     25 SEP 2017 jcs  Build 35: No mo admin; Use LVCAdmin.hpp instead
*     14 JAN 2018 jcs  Build 39: _FreeSchema() / _mtx
*      8 MAR 2023 jcs  Build 62: Re-entrant SnanpAll( LVCAll * )
*     14 AUG 2023 jcs  Build 65: _nameMap
*     26 JUN 2024 jcs  Build 72: SetFilter( flds, svcs )
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_LVC_H
#define __RTEDGE_LVC_H
#include <vector>

namespace RTEDGE
{

// Forward declarations

class LVC;
class Message;
class Schema;

typedef std::vector<Message *>     Messages;
typedef hash_map<std::string, int> NameMap;


////////////////////////////////////////////////
//
//        c l a s s   L V C A l l
//
////////////////////////////////////////////////

/**
 * \class LVCAll
 * \brief Wrapper around LVCDataAll
 *
 */
class LVCAll
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor.  Initializes LVC internals */
	LVCAll( LVC &lvc, Schema &schema ) :
	   _lvc( lvc ),
	   _schema( schema ),
	   _msgs(),
	   _nameMap()
	{
	   ::memset( &_all, 0, sizeof( _all ) );
	}

	virtual ~LVCAll()
	{
	   reset();
	}


	////////////////////////////////////
	// Access / Mutator
	////////////////////////////////////
	/** 
	 * \brief Return LVC sourcing us
	 *
	 * \return LVC sourcing us
	 */
	LVC &lvc()
	{
	   return _lvc;
	}

	/** 
	 * \brief Returns LVCData by name
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \return Message by name if found; NULL if not
	 */
	Message *GetRecord( const char *svc, const char *tkr )
	{
	   Message *rc;
	   int      ix;

	   rc = (Message *)0;
	   if ( GetRecordIndex( svc, tkr, ix ) ) 
	      rc  = _msgs[ix];
	   return rc;
	}

	/** 
	 * \brief Returns LVCData index by name
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \param idx - Index if found; -1 if not 
	 * \return true if found and result in idx param; false if not
	 */
	bool GetRecordIndex( const char *svc, const char *tkr, int &idx )
	{
	   NameMap           ndb = _nameMap;
	   NameMap::iterator it;
	   std::string       s( _Key( svc, tkr ) );

	   idx = -1;
	   if ( (it=ndb.find( s )) != ndb.end() ) {
	      idx = (*it).second;
	      return true;
	   }
	   return false;
	}

	/** 
	 * \brief Returns true if LVC file is binary
	 *
	 * \return true if LVC file is binary
	 */
	bool IsBinary()
	{
	   return _all._bBinary ? true : false;
	}

	/** 
	 * \brief Return array of Message's
	 *
	 * \return Array of Message's
	 */
	Messages &msgs()
	{
	   return _msgs;
	}

	/**
	 * \brief Return result size
	 *
	 * \return Result size
	 */
	int Size()
	{
	   return _msgs.size();
	}

	/**
	 * \brief Return snap time in seconds
	 *
	 * \return Snap time in seconds
	 */
	double dSnap()
	{
	   return _all._dSnap;
	}

	/** \brief Reset guts */
	void reset()
	{
	   int i;

	   for ( i=0; i<Size(); delete _msgs[i++] );
	   _msgs.clear();
	   _nameMap.clear();
	   ::LVC_FreeAll( &_all );
	   ::memset( &_all, 0, sizeof( _all ) );
	}

	/** 
	 * \brief Set internal guts from LVCDataAll
	 *
	 * \return Array of Message's
	 */
	LVCAll &Set( LVC_Context cxt, LVCDataAll la )
	{
	   Message    *msg;
	   LVCData    *ld;
	   std::string s;
	   int         i;

	   reset();
	   _all = la;
	   for ( i=0; i<la._nTkr; i++ ) {
	      msg = new Message( cxt, true );
	      ld  = &la._tkrs[i];
	      msg->Set( ld, _schema.Get() );
	      _msgs.push_back( msg );
	      s           = ld->_pTkr;
	      _nameMap[s] = i;
	   }
	   return *this;
	}

	////////////////////////
	// Helpers
	////////////////////////
private:
	std::string _Key( const char *svc, const char *tkr )
	{
	   std::string rc;

	   rc  = svc;
	   rc += "|";
	   rc += tkr;
	   return rc;
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	LVC       &_lvc;
	Schema    &_schema;
	Messages   _msgs;
	LVCDataAll _all;
	NameMap    _nameMap;

};  // class LVCAll



////////////////////////////////////////////////
//
//        c l a s s   L V C
//
////////////////////////////////////////////////

/**
 * \class LVC
 * \brief Read-only view on Last Value Cache (LVC) file
 *
 */
class LVC : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  Initializes LVC internals
	 * 
	 * \param pFile - LVC filename
	 */
	LVC( const char *pFile ) :
	   _file( pFile ),
	   _cxt( (LVC_Context)0 ),
	   _all( (LVCAll *)0 ),
	   _qryMtx(),
	   _msg( (Message *)0 ),
	   _schema()
	{
	   ::memset( &_qry, 0, sizeof( _qry ) );
	   ::memset( &_qrySchema, 0, sizeof( _qrySchema ) );
	   _cxt = ::LVC_Initialize( pFile );
	   GetSchema();
	   _all = new LVCAll( *this, GetSchema( false ) );
	   _msg = new Message( _cxt, true );
	}

	virtual ~LVC()
	{
	   Free();
	   _FreeSchema();
	   if ( _cxt )
	      ::LVC_Destroy( _cxt );
	   if ( _all )
	      delete _all;
	   if ( _msg )
	       delete _msg;
	   _cxt = (LVC_Context)0;
	   _msg = (Message *)0;
	   _all = (LVCAll *)0;
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/**
	 * \brief Returns LVC_Context associated with this read-only viewer
	 *
	 * \return LVC_Context associated with this read-only viewer
	 */
	LVC_Context cxt()
	{
	   return _cxt;
	}

	/**
	 * \brief Returns LVC filename
	 *
	 * \return LVC filename
	 */
	const char *pFilename()
	{
	   return _file.data();
	}

	/**
	 * \brief Returns Query Mutex
	 *
	 * \return Query Mutex
	 */
	Mutex &QueryMtx()
	{
	   return _qryMtx;
	}

	////////////////////////////////////
	// Access - Schema
	////////////////////////////////////
public:
	/**
	 * \brief (Optionally) Query LVC for Schema and return reference
	 *
	 * \param bQry - true to query; false to simply return reference
	 * \return Current schema from LVC
	 */
	Schema &GetSchema( bool bQry=true )
	{
	   Message msg( 0 );

	   if ( bQry ) {
	      _FreeSchema();
	      ::LVC_GetSchema( _cxt, &_qrySchema );
	      _schema.Initialize( msg.Set( &_qrySchema ) );
	   }
	   return _schema;
	}

	////////////////////////////////////
	// Mutator - Filter
	////////////////////////////////////
public:
	/**
	 * \brief Set query response field list and/or service list filter.
	 *
	 * Use this to improve query performance if you know you only want to view
	 * certain fields and/or services, but the LVC might contain many more.
	 *
	 * Examples:
	 * Requirement | Filter
	 * --- | ---
	 * BID, ASK, TRDPRC_1 from all services | SetFilter( "BID,ASK,TRDPRC_1", NULL )
	 * BID, ASK, TRDPRC_1 from bloomberg | SetFilter( "BID,ASK,TRDPRC_1", [ "bloomberg", NULL ] )
	 * All fields from bloomberg | SetFilter( "", [ "bloomberg", NULL ] )
	 *
	 * The filter is used on the following APIs:
	 * API | Field Filter | Service Filter
	 * --- | --- | ---
	 * LVC_Snapshot() | Y | N
	 * LVC_View() | Y | N
	 * LVC_SnapAll() | Y | Y
	 * LVC_ViewAll() | Y | Y
	 *
	 * \param flds - Comma-separated list of field names or ID's
	 * \param svc - NULL-terminated array of allowable service names
	 * \return Number of fields and services in filter
	 * \see ClearFilter()
	 */
	int SetFilter( const char *flds, const char **svcs=NULL )
	{
	   return ::LVC_SetFilter( _cxt, flds, svcs );
	}

	/** 
	 * \brief Clear response filter set via SetFilter()
	 *   
	 * \see SetFilter()
	 */  
	void ClearFilter()
	{
	   SetFilter( (const char *)0,  (const char **)0 );
	}

	////////////////////////////////////
	// Query - Single Ticker
	////////////////////////////////////
	/**
	 * \brief Query LVC for current values for spcecific ticker.
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \return Current values in Message
	 */
	Message *Snap( const char *svc, const char *tkr )
	{
	   Locker   lck( _qryMtx );
	   Message *rtn;

	   Free();
	   _qry = ::LVC_Snapshot( _cxt, svc, tkr );
	   rtn  = &_msg->Set( &_qry );
	   return rtn;
	}

	/**
	 * \brief Query LVC for current values for spcecific ticker.
	 *
	 * This is for backwards-compatibility only.
	 *
	 * \param svc - Service name (e.g. BLOOMBERG)
	 * \param tkr - Ticker name (e.g. EUR CURNCY)
	 * \return Current values in Message
	 */
	Message *View( const char *svc, const char *tkr )
	{
	   return Snap( svc, tkr );
	}

	/**
	 * \brief Release resources associated with the last call to Snap() 
	 * or View().
	 */
	void Free()
	{
	   ::LVC_Free( &_qry );
	   ::memset( &_qry, 0, sizeof( _qry ) );
	}


	////////////////////////////////////
	// Query - All
	////////////////////////////////////
	/**
	 * \brief Query LVC for current values for ALL tickers
	 *
	 * This method is not re-entrant and unsafe
	 *
	 * \return Current contents of LVC Cache in LVCAll struct
	 */
	LVCAll &SnapAll()
	{
	   Locker  lck( _qryMtx );

	   return _all->Set( _cxt, ::LVC_SnapAll( _cxt ) );
	}

	/**
	 * \brief Query LVC for current values for ALL tickers
	 *
	 * This method is not re-entrant and unsafe
	 *
	 * \return Current contents of LVC Cache in LVCAll struct
	 * \see SnapAll()
	 */
	LVCAll &ViewAll()
	{
	   return SnapAll();
	}

	/**
	 * \brief Query LVC for current values for ALL tickers
	 *
	 * This method may be called simultaneously by multiple threads. 
	 *
	 * \param dst : User-supplied LVCAll instance to hold LVC Values
	 * \return dst
	 */
	LVCAll &SnapAll_safe( LVCAll &dst )
	{
	   return dst.Set( _cxt, ::LVC_SnapAll( _cxt ) );
	}

	/**
	 * \brief Query LVC for current values for ALL tickers
	 *
	 * This method may be called simultaneously by multiple threads. 
	 *
	 * \param dst : User-supplied LVCAll instance to hold LVC Values
	 * \return dst
	 * \see SnapAll_aafe()
	 */
	LVCAll &ViewAll_safe( LVCAll &dst )
	{
	   return SnapAll_safe( dst );
	}

	/**
	 * \brief Release resources associated with the last call to SnapAll() 
	 * or ViewAll().
	 */
	void FreeAll()
	{
	   _all->reset();
	}

	////////////////////////////////////
	// Query - Filtered
	////////////////////////////////////
	/**
	 * \brief Query LVC for specific service
	 *
	 * This method is not re-entrant and unsafe
	 *
	 * \param svc - Service to snap
	 * \return Current contents of svc in LVC Cache in LVCAll struct
	 * \see SnapServices()
	 */
	LVCAll &SnapService( std::string &svc )
	{
	   Strings sdb;

	   sdb.push_back( svc );
	   return SnapServices( sdb );
	}

	/**
	 * \brief Query LVC for specific services
	 *
	 * This method is not re-entrant and unsafe
	 *
	 * \param svcs - List of services to snap
	 * \return Current contents of svcs in LVC Cache in LVCAll struct
	 * \see SetFilter()
	 * \see SnapAll()
	 */
	LVCAll &SnapServices( Strings &svcs )
	{
	   std::vector<const char *> svcsV;
	   size_t                    i, n;

	   n = svcs.size();
	   for ( i=0; i<n; svcsV.push_back( svcs[i].data() ), i++ );
	   svcsV.push_back( (const char *)0 );
	   SetFilter( (const char *)0, svcsV.data() );
	   return SnapAll();
	}

	/**
	 * \brief Query LVC for specific fields
	 *
	 * This method is not re-entrant and unsafe
	 *
	 * \param flds - CSV list of field IDs or names to snap
	 * \return Current contents of flds in LVC Cache in LVCAll struct
	 * \see SetFilter()
	 * \see SnapAll()
	 */
	LVCAll &SnapFields( std::string &flds )
	{
	   SetFilter( flds.data(), (const char **)0 );
	   return SnapAll();
	}

	/**
	 * \brief Query LVC for specific fields from single service
	 *
	 * This method is not re-entrant and unsafe
	 *
	 * \param flds - CSV list of field IDs or names to snap
	 * \param svc - Service to snap
	 * \return Current contents of flds from svc in LVC Cache
	 * \see SetFilter()
	 * \see SnapAll()
	 */
	LVCAll &SnapFieldsFromService( std::string &flds, std::string &svc )
	{
	   std::vector<const char *> svcsV;

	   svcsV.push_back( svc.data() );
	   svcsV.push_back( (const char *)0 );
	   SetFilter( flds.data(), svcsV.data() );
	   return SnapAll();
	   SetFilter( flds.data(), (const char **)0 );
	   return SnapAll();
	}

	////////////////////////
	// Helpers
	////////////////////////
private:
	void _FreeSchema()
	{
	   LVCData &qs = _qrySchema;
	   int      i;

	   for ( i=0; i<qs._nFld; qs._flds[i++]._type = rtFld_string );
	   ::LVC_Free( &_qrySchema );
	   ::memset( &_qrySchema, 0, sizeof( _qrySchema ) );
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	std::string _file;
	LVC_Context _cxt;
	LVCData     _qry;
	LVCAll     *_all;
	LVCData     _qrySchema;
	Mutex       _qryMtx;
	Message    *_msg;
	Schema      _schema;

};  // class LVC

} // namespace RTEDGE

#endif // __RTEDGE_LVC_H 
