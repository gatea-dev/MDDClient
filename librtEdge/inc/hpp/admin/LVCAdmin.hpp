/******************************************************************************
*
*  LVCAdmin.hpp
*     LVCAdmin Cockpit Channel
*
*  REVISION HISTORY:
*     25 SEP 2017 jcs  Created.
*     21 JAN 2018 jcs  Build 39: LVC
*      7 MAR 2022 jcs  Build 51: AddTickers()
*
*  (c) 1994-2022, Gatea Ltd.
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

static const char *_CMD_ADD  = "ADD";
static const char *_CMD_DEL  = "DEL";
static const char *_CMD_ACK  = "ACK";
static const char *_CMD_NAK  = "NAK";

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
	   if ( !::strcmp( pn, _CMD_ACK ) )
	      return lvcAdm_ACK;
	   if ( !::strcmp( pn, _CMD_NAK ) )
	      return lvcAdm_NAK;
	   return lvcAdm_undefined;
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
	   cchans_[_cxt]      = this;
//    _msg               = new Message( _cxt );
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
	 * \brief Add ( Service, Ticker ) to LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 */
	void AddTicker( const char *svc, 
	                const char *tkr )
	{
	   char buf[K], *cp;

	   cp  = buf;
	   cp += sprintf( cp, "<%s ", _CMD_ADD );
	   cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrSvc, svc );
	   cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrName, tkr );
	   cp += sprintf( cp, "/>\n" );
	   Cockpit::Start( pAdmin() );  // TODO : LVC
	   ::Cockpit_Send( cxt(), buf );
	}

	/**
	 * \brief Add list of ( Service, Ticker ) to LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkrs - NULL-terminated list of tickers
	 */
	void AddTickers( const char  *svc, 
	                 const char **tkrs )
	{
	   string s;
	   char   buf[K], *cp;
	   int    i;

	   for ( i=0; tkrs[i]; i++ ) {
	      cp  = buf;
	      cp += sprintf( cp, "<%s ", _CMD_ADD );
	      cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrSvc, svc );
	      cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrName, tkrs[i] );
	      cp += sprintf( cp, "/>\n" );
	      s  += buf;
	   }
	   Cockpit::Start( pAdmin() );  // TODO : LVC
	   ::Cockpit_Send( cxt(), s.data() );
	}

	/**
	 * \brief Remove ( Service, Ticker ) from LVC
	 *
	 * This method automatically calls Start() to connect
	 *
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 */
	void DelTicker( const char *svc, 
	                const char *tkr )
	{
	   char buf[K], *cp;

	   cp  = buf;
	   cp += sprintf( cp, "<%s ", _CMD_DEL );
	   cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrSvc, svc );
	   cp += sprintf( cp, "%s=\"%s\" ", _mdd_pAttrName, tkr );
	   cp += sprintf( cp, "/>\n" );
	   Cockpit::Start( pAdmin() );  // TODO : LVC
	   ::Cockpit_Send( cxt(), buf );
	}


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
public:
	/**
	 * \brief Called asynchronously when an ACK message arrives on 
	 * Cockpit channel.
	 *
	 * \param msgTy - Type of ACK : Add or Delete
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \return true if you processed this msg; false to propogate to 
	 * RTEDGE::Cockpit::OnData handler
	 *
	 * \see RTEDGE::Cockpit::OnData
	 */
	virtual bool OnAdminACK( LVCAdminMsgType msgTy,
	                         const char     *svc,
	                         const char     *tkr )
	{ return false; }

	/**
	 * \brief Called asynchronously when an NAK message arrives on 
	 * Cockpit channel.
	 *
	 * \param msgTy - Type of NAK : Add or Delete
	 * \param svc - Service Name
	 * \param tkr - Ticker Name
	 * \return true if you processed this msg; false to propogate to 
	 * RTEDGE::Cockpit::OnData handler
	 *
	 * \see RTEDGE::Cockpit::OnData
	 */
	virtual bool OnAdminNAK( LVCAdminMsgType msgTy,
	                         const char     *svc,
	                         const char     *tkr )
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
	         return OnAdminACK( e.MsgType(), e.Service(), e.Ticker() );
	      case lvcAdm_NAK:
	         return OnAdminNAK( e.MsgType(), e.Service(), e.Ticker() );
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
