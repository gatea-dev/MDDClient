/******************************************************************************
*
*  EventPump.h
*     MDDirect Event Pump
*
*  REVISION HISTORY:
*      7 APR 2011 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     17 OCT 2023 jcs  Build 12: No mo Book
*      5 FEB 2025 jcs  Build 14: _adm
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_EVTPMP_H
#define __MDDPY_EVTPMP_H
#include <MDDirect.h>


//////////////
// Forwards
//////////////
class MDDpySubChan;
class MDDpyLVCAdmin;
class rtMsg;


/////////////////////////////////////////
// Event Pump
/////////////////////////////////////////
class EventPump
{
protected:
	MDDpySubChan  *_sub;
	MDDpyLVCAdmin *_adm;
	RTEDGE::Mutex  _mtx;
	Updates        _upds;  // Conflated
	UpdateFifo     _updFifo;
	rtMsgs         _msgs;  // Unconflated
	volatile bool  _Notify;
	int            _SleepMillis;

	// Constructor / Destructor
public:
	EventPump( MDDpySubChan & );
	EventPump( MDDpyLVCAdmin & );
	~EventPump();

	// Access / Operations

	int  nMsg();
	void Add( Update & );
	void Add( rtMsg * );
	bool GetOneUpd( Update & );
	void Drain( int );
	void Close( Record * );

	// Threading Synchronization

	void SleepMillis( int );
	void Notify();
	void Wait( double );
private:
	void _Sleep( double );
 
}; // class EventPump


/////////////////////////////////////////
// Real-Time Message
/////////////////////////////////////////
class rtMsg : public mddBuf
{
public:
	string *_err;
	char    _upd[K];
	int     _oid;

	// Constructor / Destructor
public:
	rtMsg( const char *, int, int );
	rtMsg( const char *, int );
	~rtMsg();

}; // class rtMsg

#endif // __MDDPY_EVTPMP_H
