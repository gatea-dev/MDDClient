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

*
*  (c) 1994-2018 Gatea Ltd.
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

typedef std::vector<Message *> Messages;


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
	   _msgs()
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
	 *\brief Return LVC sourcing us
	 *
	 *\return LVC sourcing us
	 */
	LVC &lvc()
	{
	   return _lvc;
	}

	/** 
	 *\brief Returns true if LVC file is binary
	 *
	 *\return true if LVC file is binary
	 */
	bool IsBinary()
	{
	   return _all._bBinary ? true : false;
	}

	/** 
	 *\brief Return array of Message's
	 *
	 *\return Array of Message's
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
	   ::LVC_FreeAll( &_all );
	   ::memset( &_all, 0, sizeof( _all ) );
	}

	/** 
	 *\brief Set internal guts from LVCDataAll
	 *
	 *\return Array of Message's
	 */
	LVCAll &Set( LVC_Context cxt, LVCDataAll la )
	{
	   Message *msg;
	   LVCData *ld;
	   int      i;

	   reset();
	   _all = la;
	   for ( i=0; i<la._nTkr; i++ ) {
	      msg = new Message( cxt, true );
	      ld  = &la._tkrs[i];
	      msg->Set( ld, _schema.Get() );
	      _msgs.push_back( msg );
	   }
	   return *this;
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	LVC       &_lvc;
	Schema    &_schema;
	Messages   _msgs;
	LVCDataAll _all;
	int        _itr;

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
	 * \return Current contents of LVC Cache in LVCAll struct
	 */
	LVCAll &SnapAll()
	{
	   Locker lck( _qryMtx );

	   return _all->Set( _cxt, ::LVC_SnapAll( _cxt ) );
	}

	/**
	 * \brief Query LVC for current values for ALL tickers
	 *
	 * \return Current values in Message[] list
	 */
	LVCAll &ViewAll()
	{
	   return SnapAll();
	}

	/**
	 * \brief Release resources associated with the last call to SnapAll() 
	 * or ViewAll().
	 */
	void FreeAll()
	{
	   _all->reset();
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
