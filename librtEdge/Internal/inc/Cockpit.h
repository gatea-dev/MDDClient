/******************************************************************************
*
*  Cockpit.h
*     MD-Direct Cockpit Channel
*
*  REVISION HISTORY:
*     22 AUG 2017 jcs  Created.
*     21 JAN 2018 jcs  Build 39: _LVC
*     22 DEC 2024 jcs  Build 74: ConnCbk()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __MDD_COCKPIT_H
#define __MDD_COCKPIT_H
#include <EDG_Internal.h>
#include "../../../libmddWire/Internal/inc/MDW_GLxml.h"

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class GLlvcDb;
class Logger;
class Socket;

/////////////////////////////////////////
// MD-Direct Cockpit Channel
/////////////////////////////////////////
class Cockpit : public Socket
{
protected:
	CockpitAttr            _attr;
	Cockpit_Context        _cxt;
	string                 _con;
	MDDWIRE_PRIVATE::GLxml _xml;
	CockpitData            _zzz;
	GLlvcDb               *_LVC;
	Mutex                  _lvcMtx;
	bool                   _bLockedLVC;

	// Constructor / Destructor
public:
	Cockpit( CockpitAttr, Cockpit_Context, GLlvcDb * );
	~Cockpit();

	// Access

	CockpitAttr     attr();
	Cockpit_Context cxt();

	// LVC Destruction
public:
	void DetachLVC( bool bForced=false );

	// Socket Interface
public:
	virtual void ConnCbk( const char *, bool );

	// Thread Notifications
protected:
	virtual void OnConnect( const char * );
	virtual void OnDisconnect( const char * );
	virtual void OnRead();
private:
	void         _OnXML();
	CockpitData  _Add( MDDWIRE_PRIVATE::GLxmlElem * ); 
	void         _Free( CockpitData ); 

	// TimerEvent Notifications
protected:
	virtual void On1SecTimer();

	// Idle Loop Processing ...
public:
	void        OnIdle();
	static void _OnIdle( void * );
};

} // namespace RTEDGE_PRIVATE

#endif // __MDD_COCKPIT_H
