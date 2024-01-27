/******************************************************************************
*
*  LVCAdmin.hpp
*     LVCAdmin Cockpit Channel
*
*  REVISION HISTORY:
*     25 SEP 2017 jcs  Created.
*     21 JAN 2018 jcs  Build 39: LVC
*     17 MAR 2022 jcs  Build 51: AddTickers()
*     26 APR 2022 jcs  Build 53: _dtdBDS
*     17 MAY 2022 jcs  Build 54: RefreshTickers() / RefreshAll()
*     26 OCT 2022 jcs  Build 58: CockpitMap
*      4 SEP 2023 jcs  Build 64: Named Schema; DelTickers()
*     26 JAN 2024 jcs  Build 68: Cockpit._cMtx; AddFilteredTickers(); ADD-BDS
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __LVCAdmin_H
#define __LVCAdmin_H
#include <hpp/LVC.hpp>
#include <hpp/Cockpit.hpp>

typedef enum {
	lvcAdm_undefined = 0,
	lvcAdm_ACK       = 1,
	lvcAdm_NAK       = 2
} LVCAdminMsgType;

// DTD

static const char *_dtdADD     = "ADD";
static const char *_dtdBDS     = "ADD-BDS";
static const char *_dtdREFR    = "REFRESH";
static const char *_dtdREFRALL = "REFRESH-ALL";
static const char *_dtdDEL     = "DEL";
static const char *_dtdACK     = "ACK";
static const char *_dtdNAK     = "NAK";
static const char *_attrSchema = "Schema";
static const char *_tkrALL     = "*";

typedef hash_set<std::string>  StringSet;

namespace RTEDGE
{

#ifndef DOXYGEN_OMIT
////////////////////////////////////////////////
//
//     c l a s s   L V C A d m i n M s g
//
////////////////////////////////////////////////

/**
 * \class LVCAdminMsg
 * \brief A cracked message from the LVC Cockpit channel
 */
class LVCAdminMsg
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor - Initialized data */
	LVCAdminMsg( RTEDGE::CockpitMsg &msg ) :
	   _msg( msg )
	{ ; }


	////////////////////////////////////
	// Access - this Message
	////////////////////////////////////
public:
	/**
	 * \brief Return message type
	 *
	 * \return Message type
	 */
	LVCAdminMsgType MsgType()
	{
	   const char *pn;

	   pn = _msg.Name();
	   if ( !::strcmp( pn, _dtdACK ) )
	      return lvcAdm_ACK;
	   if ( !::strcmp( pn, _dtdNAK ) )
	      return lvcAdm_NAK;
	   return lvcAdm_undefined;
	}

	/**
	 * \brief Return true if ADD; false if DEL
	 *
	 * \return true if ADD; false if DEL
	 */
	bool IsAdd()
	{
	   return( ::strcmp( _msg.GetAttribute( _mdd_pType ), _dtdADD ) == 0 );
	}

	/**
	 * \brief Return Service Name
	 *
	 * \return Service Name
	 */
	const char *Service()
	{
	   return _msg.GetAttribute( _mdd_pAttrSvc );
	}

	/**
	 * \brief Return Ticker Name
	 *
	 * \return Ticker Name
	 */
	const char *Ticker()
	{
	   return _msg.GetAttribute( _mdd_pAttrName );
	}


	////////////////////////
	// Protected Members
	////////////////////////
protected:
	RTEDGE::CockpitMsg &_msg;

}; // class LVCAdminMsg

#endif // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//       c l a s s   L V C A d m i n
//
////////////////////////////////////////////////

/**
 * \class LVCAdmin
 * \brief XML admin channel to LVC
 */
class LVCAdmin : public RTEDGE::Cockpit
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  Call Start() to connect to LVCAdmin.
	 *
	 * \param pAdmin - \<host>\:\<port\> of LVC admin server
	 */
	LVCAdmin( const char *pAdmin ) :
	   _admin( pAdmin )
	{ ; }


	////////////////////////////////////
	// Cockpit Operations
	////////////////////////////////////
public:
	/**
	 * \brief Initialize the Cockpit connection
	 *
	 * Your application is notified via OnConnect() when you have
	 * successfully connnected.
	 *
	 * \param hosts - Comma-separated list of Cockpit \<host\>:\<port\>
	 * to connect to.
	 * \param lvc - Associated LVC to be locked during Cockpit operations
	 * \param lockWaitSec - Max seconds to wait for response from LVC before
	 * unlocking and allowing LVC.ViewAll() / LVC.Snapshot() to proceed.
	 * \return Textual description of the connection state
	 */
	const char *Start( const char *hosts, 
	                   LVC        &lvc,
	                   int         lockWaitSec=5 )
	{
	   Locker lck( _cMtx );

	   // Pre-condition(s)

	   if ( _cxt )
	      return "Already connected";
	   if ( !hosts )
	      return "No hostname specified";

	   // Initialize our channel

	   _hosts = hosts;
	   ::memset( &_attr, 0, sizeof( _attr ) );
	   _attr._pSvrHosts   = pSvrHosts();
	   _attr._cxtLVC      = lvc.cxt();
	   _attr._lockWaitSec = lockWaitSec;
	   _attr._connCbk     = _connCbk;
	   _attr._dataCbk     = _dataCbk;
	   _cxt               = ::Cockpit_Initialize( _attr );
//      _msg               = new Message( _cxt );
	   _cockpits.Add( _cxt, this );
	   SetIdleCallback( _bIdleCbk );
	   return ::Cockpit_Start( _cxt );
	}


	////////////////////////////////////
	// Operations
	////////////////////////////////////
public:
	/**
	 * \brief Returns comma-separated list LVC admin \<host\>:\<port\>
	 *
	 * \return Comma-separated list of LVC admin
	 */
	const char *pAdmin()
	{
	   return _admin.data();
	}

	/**
	 * \brief Add new ( svc,bds ) record from LVC
	 *
	 * A Broadcast Data Stream (BDS) is an updating stream of tickers from
	 * a publisher.  When an LVC is seeded by the BDS, it subscribes to the  
	 * updating stream of tickers (e.g., List of NYSE exchange tickers), then
	 * subscribes to market data from each individual ticker. 
	 * 
	 * In this manner, the LVC only needs to know a single name - i.e., the
	 * name of the BDS - rather than the names of all tickers. 
	 *
	 * \param svc - Service name
	 * \param bds - BDS name
	 */
	void AddBDS( const char *svc, const char *bds )
	{
	   const char *tkrs[] = { bds, (const char *)0 };

	   _DoTickers( _dtdBDS, svc, tkrs );
	}

	/**
	 * \brief Add ( Service, Ticker ) to LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \param schema - (Optional) Schema Name
	 */
	void AddTicker( const char *svc, 
	                const char *tkr,
	                const char *schema="" )
	{
	   const char *tkrs[] = { tkr, (const char *)0 };

	   AddTickers( svc, tkrs, schema );
	}

	/**
	 * \brief Add list of ( Service, Ticker ) to LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkrs - NULL-terminated list of tickers
	 * \param schema - (Optional) Schema Name
	 */
	void AddTickers( const char  *svc, 
	                 const char **tkrs,
	                 const char  *schema="" )
	{
	   _DoTickers( _dtdADD, svc, tkrs, schema );
	}

	/**
	 * \brief Add list of ( Service, Ticker ) to LVC that are not there
	 *
	 * This method calls AddTickers() after calling LVC::SnapAll() to 
	 * determine which tickers are not in the cache
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param lvc - LVC to SnapAll()
	 * \param svc - Service Name
	 * \param tkrs - NULL-terminated list of tickers
	 * \param schema - (Optional) Schema Name
	 * \return Number of tickers added
	 * \see AddTickers()
	 * \see LVC::SnapAll_safe()
	 */
	int AddFilteredTickers( LVC         &lvc,
	                        const char  *svc, 
	                        const char **tkrs,
	                        const char  *schema="" )
	{
	   LVCAll       dst( lvc, lvc.GetSchema( false ) );
	   LVCAll      &all = lvc.ViewAll_safe( dst );
	   Messages    &mdb = all.msgs();
	   StringSet    ldb;
	   std::string  s;
	   const char **fltr;
	   size_t       sz;
	   int          i, ns, nf;

	   // Pre-conditions

	   if ( !tkrs || !tkrs[0] )
	      return 0;

	   // 1) Walk LVC all : Build hash_set

	   for ( size_t ii=0; ii<mdb.size(); ii++ ) {
	      if ( ::strcmp( mdb[ii]->Service(), svc ) )
	         continue; // for-ii
	      ldb.insert( std::string( mdb[ii]->Ticker() ) );
	   }

	   // 2) Filtered Ticker List

	   for ( ns=0; tkrs[ns]; ns++ );
	   sz   = ns * sizeof( const char * );
	   fltr = (const char **)new char[sz];
	   for ( i=0,nf=0; i<ns; i++ ) {
	      s = tkrs[i];
	      if ( ldb.find( s ) == ldb.end() )
	         fltr[nf++] = tkrs[i];
	   }
	   fltr[nf] = '\0';

	   // 2) Filter

	   AddTickers( svc, fltr, schema );
	   delete[] fltr;
	   return nf;
	}


	/**
	 * \brief Remove ( Service, Ticker ) from LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \param schema - (Optional) Schema Name
	 */
	void DelTicker( const char *svc, 
	                const char *tkr,
	                const char *schema="" )
	{
	   const char *tkrs[] = { tkr, (const char *)0 };

	   DelTickers( svc, tkrs, schema );
	}

	/**
	 * \brief Del list of ( Service, Ticker ) to LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkrs - NULL-terminated list of tickers
	 * \param schema - (Optional) Schema Name
	 */
	void DelTickers( const char  *svc, 
	                 const char **tkrs,
	                 const char  *schema="" )
	{
	   _DoTickers( _dtdDEL, svc, tkrs, schema );
	}

	/**
	 * \brief Refresh list of ( Service, Ticker ) to LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkrs - NULL-terminated list of tickers
	 */
	void RefreshTickers( const char  *svc, 
	                     const char **tkrs )
	{
	   _DoTickers( _dtdREFR, svc, tkrs );
	}

	/**
	 * \brief Refresh ALL dead tickers in LVC
	 *
	 * This method automatically calls Start() to connect
	 */
	void RefreshAll()
	{
	   const char *tkrs[] = { _tkrALL, NULL };

	   _DoTickers( _dtdREFRALL, _tkrALL, tkrs );
	}

	////////////////////////
	// Private Helpers
	////////////////////////
private:
#ifndef DOXYGEN_OMIT
	void _DoTickers( const char  *cmd,
	                 const char  *svc,
	                 const char **tkrs,
	                 const char  *schema=(const char *)0 )
	{
	   std::string s;
	   char        buf[K], *cp;
	   bool        bS;
	   int         i;

	   // Pre-condition(s)

	   if ( !svc || !strlen( svc ) )
	      return;
	   if ( !tkrs || !tkrs[0] || !strlen( tkrs[0] ) )
	      return;

	   bS  = ( schema && strlen( schema ) );
	   for ( i=0; tkrs[i]; i++ ) {
	      if ( !strlen( tkrs[i] ) )
	         continue; // for-i
	      cp  = buf;
	      cp += sprintf( cp, "<%s ", cmd );
	      cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrSvc, svc );
	      cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrName, tkrs[i] );
	      cp += bS ? sprintf( cp, "%s=\"%s\" ", _attrSchema, schema ) : 0;
	      cp += sprintf( cp, "/>\n" );
	      s  += buf;
	   }

	   Locker lck( _cMtx );

	   Cockpit::Start( pAdmin() );  // TODO : LVC
	   ::Cockpit_Send( cxt(), s.data() );
	}
#endif // DOXYGEN_OMIT


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
public:
	/**
	 * \brief Called asynchronously when an ACK message arrives on 
	 * Cockpit channel.
	 *
	 * \param bAdd - true if ADD; false if DEL
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \return true if you processed this msg; false to propogate to 
	 * RTEDGE::Cockpit::OnData handler
	 *
	 * \see RTEDGE::Cockpit::OnData
	 */
	virtual bool OnAdminACK( bool bAdd, const char *svc, const char *tkr )
	{ return false; }

	/**
	 * \brief Called asynchronously when an NAK message arrives on 
	 * Cockpit channel.
	 *
	 * \param bAdd - true if ADD; false if DEL
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \return true if you processed this msg; false to propogate to 
	 * RTEDGE::Cockpit::OnData handler
	 *
	 * \see RTEDGE::Cockpit::OnData
	 */
	virtual bool OnAdminNAK( bool bAdd, const char *svc, const char *tkr )
	{ return false; }



	////////////////////////////////////
	// Asynchronous Callbacks - protected
	////////////////////////////////////
protected:
	virtual bool _OnData( RTEDGE::CockpitMsg &msg )
	{
	   RTEDGE::LVCAdminMsg e( msg );

	   // Supported Msg Types FROM EFSARb

	   switch( e.MsgType() ) {
	      case lvcAdm_undefined:
	         break;
	      case lvcAdm_ACK:
	         return OnAdminACK( e.IsAdd(), e.Service(), e.Ticker() );
	      case lvcAdm_NAK:
	         return OnAdminNAK( e.IsAdd(), e.Service(), e.Ticker() );
	   }
	   return false;
	}

	////////////////////////
	// Private Members
	////////////////////////
private:
	std::string _admin;

};  // class LVCAdmin

};  // namespace RTEDGE

#endif // __LVCAdmin_H 
