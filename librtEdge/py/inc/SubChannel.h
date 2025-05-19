/******************************************************************************
*
*  SubChannel.h
*     MD-Direct Subscription Channel
*
*  REVISION HISTORY:
*     19 MAR 2016 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     20 NOV 2020 jcs  Build  2: Tape; OnStreamDone()
*      1 DEC 2020 jcs  Build  3: SnapTape() / PyTapeSnapQry
*      3 FEB 2022 jcs  Build  5: De-lint
*     29 AUG 2023 jcs  Build 10: BDS
*     17 OCT 2023 jcs  Build 12: No mo Book
*     19 MAY 2025 jcs  Build 77: OnIdle()
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_SUBCHAN_H
#define __MDDPY_SUBCHAN_H
#include <MDDirect.h>

using namespace MDDPY;

//////////////
// Forwards
//////////////
class Record;
class Schema;
class rtMsg;
class PyByteStream;
class PyTapeSnap;
class PyTapeSnapQry;

typedef hash_map<int, PyByteStream *>    ByteStreamByOid;
typedef hash_map<string, PyByteStream *> ByteStreamByName;

/////////////////////////////////////////
// Edge3 Subscription Channel
/////////////////////////////////////////
class MDDpySubChan : public RTEDGE::SubChannel
{
private:
	EventPump        _pmp;
	string           _host;
	string           _user;
	IntMap           _tapeId2Arg;
	ByteStreamByOid  _bStrByOid;
	ByteStreamByName _bStrByName;
	MDDPY::Schema   *_py_schema;
	RecByName        _byName;
	RecByOid         _byOid;
	RTEDGE::Mutex    _mtx;
	int              _iFilter;
	bool             _bRTD;
	PyTapeSnap      *_snap;
	volatile bool    _bTapeUpd;

	// Constructor / Destructor
public:
	MDDpySubChan( const char *, const char *, bool );
	~MDDpySubChan();

	// Operations
public:
	const char *Protocol();
	int         Open( const char *, const char *, int );
	int         OpenByteStream( const char *, const char *, int );
	Record     *FindRecord( const char *, const char * );
	Update      ToUpdate( rtMsg & );
	int         Close( const char *, const char * );
	PyObject   *Filter( int );
	PyObject   *Read( double );
	PyObject   *GetData( const char *, const char *, int * );
	PyObject   *QueryTape();
	PyObject   *SnapTape( PyTapeSnapQry & );

	// RTEDGE::SubChannel Notifications
protected:
	virtual void OnConnect( const char *, bool );
	virtual void OnService( const char *, bool );
	virtual void OnData( RTEDGE::Message & );
	virtual void OnRecovering( RTEDGE::Message & );
	virtual void OnDead( RTEDGE::Message &, const char * );
	virtual void OnStreamDone( RTEDGE::Message & );
	virtual void OnSymbol( RTEDGE::Message &, const char * );
	virtual void OnSchema( RTEDGE::Schema & );
	virtual void OnIdle();

	// PyByteStream Notifications
public:
	void OnByteStream( PyByteStream & );
	void OnByteStreamError( PyByteStream &, const char * );

	// Helpers
protected:
	Update    _ToUpdate( int, RTEDGE::Message & );
	Update    _ToUpdate( int, rtBUF );
	Update    _ToUpdate( int, const char * );
private:
	PyObject *_Get1stUpd();

	// Class-wide
public:
	static int    ReqID();
	static string _Key( const char *, const char * );

};  // class MDDpySubChan


/////////////////////////////////////////
// ByteStream
/////////////////////////////////////////
class PyByteStream : public RTEDGE::ByteStream
{
public:
	MDDpySubChan &_ch;
	int           _userID;
	int           _StreamID;

	// Constructor / Destructor
public:
	PyByteStream( MDDpySubChan &, const char *, const char *, int );
	~PyByteStream();

	// RTEDGE::ByteStream Notifications
protected:
	virtual void OnData( rtBUF );
	virtual void OnError( const char * );
	virtual void OnSubscribeComplete();

}; // class PyByteStream


/////////////////////////////////////////
// Tape Snap Request
/////////////////////////////////////////
class PyTapeSnapQry
{
public:
	const char *_svc;
	const char *_tkr;
	const char *_flds;
	int         _maxRow;
	double      _tmout;
	const char *_t0;
	const char *_t1;
	int         _tSample;

}; // class PyTapeSnapQry


/////////////////////////////////////////
// Tape Snap
/////////////////////////////////////////
class PyTapeSnap
{
public:
	MDDpySubChan &_ch;
	string        _svc;
	string        _tkr;
	Ints          _fids;
	int           _maxRow;
	PyObjects     _objs;
	bool          _bDone;
	RTEDGE::Field _uFld;

	// Constructor / Destructor
public:
	PyTapeSnap( MDDpySubChan &, PyTapeSnapQry & );
	~PyTapeSnap();

	// RTEDGE::Channel Notifications
public:
	void OnData( RTEDGE::Message & );
	void OnStreamDone( RTEDGE::Message & );

}; // class PyTapeSnap

#endif // __MDDPY_SUBCHAN_H
