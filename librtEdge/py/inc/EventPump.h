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
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_EVTPMP_H
#define __MDDPY_EVTPMP_H
#include <MDDirect.h>


//////////////
// Forwards
//////////////
class MDDpySubChan;
class rtMsg;


/////////////////////////////////////////
// Event Pump
/////////////////////////////////////////
class EventPump
{
protected:
	MDDpySubChan &_ch;
	RTEDGE::Mutex _mtx;
	Updates       _upds;  // Conflated
	UpdateFifo    _updFifo;
	rtMsgs        _msgs;  // Unconflated
	volatile bool _Notify;
	int           _SleepMillis;

	// Constructor / Destructor
public:
	EventPump( MDDpySubChan & );
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
