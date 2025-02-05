/******************************************************************************
*
*  Cockpit.hpp
*     librtEdge Cockpit channel
*
*  REVISION HISTORY:
*     24 AUG 2017 jcs  Created.
*     21 JAN 2018 jcs  Build 39: protected, not private
*     26 OCT 2022 jcs  Build 58: CxtMap
*     22 JAN 2024 jcs  Build 67: Cockpit._cMtx
*      5 FEB 2025 jcs  Build 75: Cockpit : public rtEdge
*
*  (c) 1994-2025, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Cockpit_H
#define __RTEDGE_Cockpit_H
#include <hpp/rtEdge.hpp>

namespace RTEDGE
{

// Forward declarations

class Cockpit;

#ifndef DOXYGEN_OMIT
static CxtMap<Cockpit *> _cockpits;
#endif // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//     c l a s s   C o c k p i t M s g
//
////////////////////////////////////////////////

/**
 * \class CockpitMsg
 * \brief A cracked message from the Cockpit channel
 */

class CockpitMsg : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor - Uninitialized data. */
	CockpitMsg() :
	   _dmp()
	{
	   ::memset( &_d, 0, sizeof( _d ) );
	}

	/** \brief Constructor - Initialized data. */
	CockpitMsg( CockpitData &d ) :
	   _dmp()
	{
	   _Set( d );
	}

	~CockpitMsg()
	{
	}

private:
	void _Set( CockpitData &d )
	{
	   _d = d;
	}


	////////////////////////////////////
	// Access - this Message
	////////////////////////////////////
public:
	/**
	 * \brief Return name of this CockpitMsg
	 *
	 * \return Name of this CockpitMsg
	 */
	const char *Name()
	{
	   return _d._value._name;
	}

	/**
	 * \brief Return true if this CockpitMsg has data
	 *
	 * \return true if this CockpitMsg has data
	 */
	bool HasData()
	{
	   return( strlen( Data() ) > 0 );
	}

	/**
	 * \brief Return data content of this CockpitMsg
	 *
	 * \return Data content of this CockpitMsg
	 */
	const char *Data()
	{
	   return _d._value._value;
	}


	////////////////////////////////////
	// Access - Attributes
	////////////////////////////////////
	/**
	 * \brief Get attribute data
	 *
	 * \param name - Attribute Name
	 * \param dflt - Default value if not found
	 * \return Data content of this CockpitMsg
	 */
	const char *GetAttribute( const char *name, const char *dflt="" )
	{
	   TupleList  &a = _d._attrs;
	   const char *rtn;
	   Tuple       t;

	   rtn = (const char *)0;
	   for ( int i=0; !rtn && i<NumAttrs(); i++ ) {
	      t   = a._tuples[i];
	      rtn = !::strcmp( t._name, name ) ? t._value : rtn;
	   }
	   return rtn ? rtn : dflt;
	}

	/**
	 * \brief Return number of Attributes in this CockpitMsg
	 *
	 * \return Number of Attributes in this CockpitMsg
	 */
	int NumAttrs()
	{
	   return _d._attrs._nTuple;
	}


	////////////////////////////////////
	// Access - Attributes
	////////////////////////////////////
	/**
	 * \brief Get attribute data
	 *
	 * \param name - Element Name
	 * \param rtn - If found, container for element data
	 * \param bRecurse - true for Recursive (nested) search
	 * \return true if found; Data content in rtn
	 */
	bool GetElement( const char *name, 
	                 CockpitMsg &rtn, 
	                 bool        bRecurse = true )
	{
	   Tuple t;

	   for ( int i=0; i<NumElems(); i++ ) {
	      t = _d._elems[i]._value;
	      if ( !::strcmp( t._name, name ) ) {
	         rtn._Set( _d._elems[i] );
	         return true;
	      }
	      else if ( bRecurse ) {
	         CockpitMsg r( _d._elems[i] );

	         if ( r.GetElement( name, rtn, bRecurse ) )
	            return true;
	      }
	   }
	   return false;
	}

	/**
	 * \brief Return number of Elements in this CockpitMsg
	 *
	 * \return Number of Elements in this CockpitMsg
	 */
	int NumElems()
	{
	   return _d._nElem;
	}


	////////////////////////////////////
	// Operations
	////////////////////////////////////
	/**
	 * \brief Dumps message contents as string
	 *
	 * \param indent - Number of spaces to indent
	 * \return Message contents as string
	 */
	const char *Dump( int indent=0 )
	{
	   Tuple t;
	   char  buf[K], sp[K], *cp;
	   int   i;

	   // Spaces

	   ::memset( sp, ' ', sizeof( sp ) );
	   sp[indent] = '\0';

	   // Attributes

	   cp  = buf;
	   cp += sprintf( cp, "%s<%s", sp, Name() );
	   for ( i=0; i<NumAttrs(); i++ ) {
	      t   = _d._attrs._tuples[i];
	      cp += sprintf( cp, " %s=\"%s\"", t._name, t._value );
	   }
	   if ( !HasData() && !NumElems() ) {
	      cp  += sprintf( cp, "/>\n" );
	      _dmp = buf;
	      return _dmp.data();
	   }
	   cp  += sprintf( cp, ">\n" );
	   _dmp = buf;

	   // Data

	   if ( HasData() ) {
	      _dmp += " ";
	      _dmp += Data();
	      _dmp += " ";
	   }

	   // Sub-Elements

	   for ( i=0; i<NumElems(); i++ ) {
	      CockpitMsg xe( _d._elems[i] );

	      _dmp += xe.Dump( indent+3 );
	   }

	   // Closure / Return

	   sprintf( buf, "%s</%s>\n", sp, Name() );
	   _dmp += buf;
	   return _dmp.data();
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	CockpitData _d;
	std::string _dmp;

}; // class CockpitMsg



////////////////////////////////////////////////
//
//       c l a s s   C o c k p i t
//
////////////////////////////////////////////////

/**
 * \class Cockpit
 * \brief Cockpit channel
 *
 * Use the (reusable) Message class to parse incoming messages 
 */

class Cockpit : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  Call Start() to connect to your MD-Direct process.
	 * 
	 * The constructor initializes internal variables, including reusable 
	 * data objects passed to the OnData().  You call Start() to connect 
	 * to the rtEdgeCache3 server.
	 */
	Cockpit() :
	   _hosts(),
	   _cxt( (Cockpit_Context)0 ),
	   _cMtx(),
	   _bIdleCbk( false )
	{
	   // Initialize us

	   ::memset( &_attr, 0, sizeof( _attr ) );
	}

	virtual ~Cockpit()
	{
	   Stop();
	}


	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Returns Cockpit_Context associated with this channel
	 *
	 * \return Cockpit_Context associated with this channel
	 */
	Cockpit_Context cxt()
	{
	   return _cxt;
	}

	/**
	 * \brief Return true if this Channel has been initialized and not Stop()'ed
	 *
	 * \return true if this Channel has been initialized and not Stop()'ed
	 */
	bool IsValid()
	{
	   return( _cxt != (Cockpit_Context)0 );
	}

	/**
	 * \brief Returns comma-separated list of rtEdgeCache3 hosts to
	 * connect this subscription channel to.
	 *
	 * This list is defined when you call Start() and is specified as
	 * \<host1\>:\<port1\>,\<host2\>:\<port2\>,...  The library tries to connect 
	 * to \<host1\>; If failure, then \<host2\>, etc.  This 
	 *
	 * \return Comma-separated list of rtEdgeCache3 hosts.
	 */
	const char *pSvrHosts()
	{
	   return _hosts.c_str();
	}

	/**
	 * \brief Allow / Disallow the library timer to call out to OnIdle()
	 * every second or so.
	 *
	 * This is useful for performing tasks in the librtEdge thread
	 * periodically, for example if you need to ensure that you call
	 * Subscribe() ONLY from the subscription channel thread.
	 *
	 * \param bIdleCbk - true to receive OnIdle(); false to disable
	 */
	void SetIdleCallback( bool bIdleCbk )
	{
	   _bIdleCbk = bIdleCbk;
	   if ( _cxt )
	      ::rtEdge_ioctl( _cxt, ioctl_setIdleTimer, (void *)_bIdleCbk );
	}


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
	 * \return Textual description of the connection state
	 */
	const char *Start( const char *hosts )
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
	   _attr._pSvrHosts = pSvrHosts();
	   _attr._connCbk   = _connCbk;
	   _attr._dataCbk   = _dataCbk;
	   _cxt             = ::Cockpit_Initialize( _attr );
//	   _msg             = new Message( _cxt );
	   _cockpits.Add( _cxt, this );
	   SetIdleCallback( _bIdleCbk );
	   return ::Cockpit_Start( _cxt );
	}

	/**
	 * \brief Destroy connection to the rtEdgeCache3
	 *
	 * Calls ::Cockpit_Destroy() to disconnect from the rtEdgeCache3 server.
	 */
	virtual void Stop()
	{
	   Locker lck( _cMtx );

	   if ( _cxt )
	      ::Cockpit_Destroy( _cxt );
	   _cockpits.Remove( _cxt );
	   _cxt = (Cockpit_Context)0;
	}



	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously when Cockpit channel connects or disconnects
	 *
	 * Override this method in your application to take action when
	 * your Cockpit channel connects or disconnects.
	 *
	 * \param msg - Textual description of connection state
	 * \param bUP - true if connected; false if disconnected
	 */
	virtual void OnConnect( const char *msg, bool bUP )
	{ ; }

	/**
	 * \brief Called asynchronously when a message arrives on the Cockpit channel
	 *
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnData( CockpitMsg &msg )
	{ ; }

	/**
	 * \brief Called asynchronously - roughly once per second - when the
	 * library is idle.
	 *
	 * This is only called if you enable via SetIdleCallback()
	 *
	 * Override this method in your application to perform tasks
	 * in the subscription channel thread.
	 */
	virtual void OnIdle()
	{ ; }


	////////////////////////////////////
	// Asynchronous Callbacks - protected
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously when a message arrives on the Cockpit 
	 * channel.  Return true if a sub-class processed this message; false
	 * to propogate into OnData()
	 *
	 * This protected virtual member allows you to sub-class this base 
	 * Cockpit channel and handle specific messages in your sub-class.
	 *
	 * \param msg - Market data update in a Message object
	 * \return true if message handled by sub-class; false to propogate to
	 * the generic OnData handler.
	 * \see OnData
	 */
	virtual bool _OnData( CockpitMsg &msg )
	{ return false; }



	////////////////////////
	// Private Members
	////////////////////////
protected:
	std::string     _hosts;
	CockpitAttr     _attr;
	Cockpit_Context _cxt;
	Mutex           _cMtx;
	bool            _bIdleCbk;


	////////////////////////////////////
	// Class-wide (private) callbacks
	////////////////////////////////////
protected:
	static void EDGAPI _connCbk( Cockpit_Context cxt, 
	                             const char     *msg, 
	                             rtEdgeState     state )
	{
	   Cockpit *us;

	   if ( (us=_cockpits.Get( cxt )) )
	      us->OnConnect( msg, ( state == edg_up ) );
	}

	static void EDGAPI _dataCbk( Cockpit_Context cxt, CockpitData d )
	{
	   CockpitMsg msg( d );
	   Cockpit   *us;

	   // ( CockpitData._value._name  == 0 ) implies OnIdle()

	   if ( (us=_cockpits.Get( cxt )) ) {
	      if ( !d._value._name )
	         us->OnIdle();
	      else if ( !us->_OnData( msg ) )
	         us->OnData( msg );
	   }
	}

};  // class Cockpit

} // namespace RTEDGE

#endif // __RTEDGE_Cockpit_H 
