/******************************************************************************
*
*  EdgChannel.h
*     MDDirect subscription channel : rtEdgeCache3
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     . . .
*     12 NOV 2014 jcs  Build 28: libmddWire; _cxt; -Wall
*     13 DEC 2014 jcs  Build 29: int Unsubscribe()
*      6 JUL 2015 jcs  Build 31: GLHashMap
*     12 OCT 2015 jcs  Build 32: IsSnapChan(); EDG_Internal.h
*     11 SEP 2016 jcs  Build 33: _bUserStreamID
*     26 MAY 2017 jcs  Build 34: Socket "has-a" Thread
*     12 SEP 2017 jcs  Build 35: hash_map not map / GLHashMap
*     13 OCT 2017 jcs  Build 36: TapeChannel
*     13 JAN 2019 jcs  Build 41: TapeChannel._schema
*     12 FEB 2020 jcs  Build 42: bool Ioctl()
*     10 SEP 2020 jcs  Build 44: _bTapeDir; TapeChannel.Query()
*     16 SEP 2020 jcs  Build 45: ParseOnly()
*     22 OCT 2020 jcs  Build 46: XxxPumpFullTape()
*      3 DEC 2020 jcs  Build 47: TapeSlice
*     29 MAR 2022 jcs  Build 52: ioctl_unpacked
*     23 SEP 2022 jcs  Build 56: GLrpyDailyIdxVw; TapeChannel.GetField( int )
*     26 JUN 2024 jcs  Build 72: FIDSet in EDG_Internal.h
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_CHANNEL_H
#define __EDGLIB_CHANNEL_H
#include <EDG_Internal.h>
#include <EDG_GLrecDb.h>

#define MAX_FLD 128*K

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class EdgFldDef;
class EdgRec;
class EdgSvc;
class GLrpyDailyIdxVw;
class Logger;
class Schema;
class Socket;
class TapeChannel;
class TapeSlice;
class TapeRun;

typedef hash_map<string, EdgSvc *>    SvcMap;
typedef hash_map<string, EdgRec *>    RecByNameMap;
typedef hash_map<int, EdgRec *>       RecByIdMap;
typedef hash_map<int, rtFIELD>        FieldMap;


/////////////////////////////////////////
// rtEdgeCache Subscription Channel
/////////////////////////////////////////
class EdgChannel : public Socket
{
protected:
	rtEdgeAttr     _attr;
	rtEdge_Context _cxt;
	string         _con;
	string         _usr;
	string         _pwd;
	SvcMap         _svcs;
	RecByIdMap     _recs;
	rtEdgeData    *_schema;
	rtEdgeData     _zzz;
	int            _subscrID;
	int            _pumpID;
	rtFIELD        _flds[MAX_FLD];
	rtFIELD        _fldsU[MAX_FLD];
	EdgRec        *_recU;
	bool           _bConflate;
	bool           _bSnapChan;
	bool           _bRawData;
	bool           _bSvcTkr;
	bool           _bUsrStreamID;
public:
	bool           _bTapeDir;
protected:
	EventPump      _Q;
	TapeChannel   *_tape;

	// Constructor / Destructor
public:
	EdgChannel( rtEdgeAttr, rtEdge_Context );
	~EdgChannel();

	// Access

	rtEdgeAttr     attr();
	rtEdge_Context cxt();
	const char    *pUsr();
	const char    *pPwd();
	EdgRec        *GetRec( const char *, const char * );
	EdgRec        *GetRec( int );
	EdgSvc        *GetSvc( const char * );
	bool           IsConflated();
	bool           IsSnapChan();
	EventPump     &Q();
	TapeChannel   *tape();

	// Schema / Field in Update Msg

	rtEdgeData GetSchema();
	mddField  *GetDef( int );
	mddField  *GetDef( const char * );
	rtFIELD   *GetField( const char * );
	rtFIELD   *GetField( int );
	bool       HasField( const char * );
	bool       HasField( int );

	// Cached Data

	rtEdgeData GetCache( const char *, const char * );
	rtEdgeData GetCache( int );

	// Operations

	int  ParseOnly( rtEdgeData & );
	int  Subscribe( const char *, const char *, void * );
	int  Unsubscribe( const char *, const char * );
	int  Unsubscribe( int );
	void Open( EdgRec & );
	void Close( EdgRec & );
	int  StartPumpFullTape( u_int64_t, int );
	int  StopPumpFullTape( int );

	// Operations - Conflation

	void Conflate( bool );
	int  Dispatch( int, double );
	int  Read( double, rtEdgeRead & );

	// Socket Interface

	virtual bool Ioctl( rtEdgeIoctl, void * );

	// Thread Notifications
protected:
	virtual void OnConnect( const char * );
	virtual void OnDisconnect( const char * );
	virtual void OnRead();

	// TimerEvent Notifications
protected:
	virtual void On1SecTimer();

	// Socket Notifications

	void OnBinary( mddMsgBuf );
	void OnMF( mddMsgBuf, const char *ty="MF" );
	void OnXML( mddMsgBuf & );

	// XML Handlers
protected:
	void OnXMLData( mddMsgBuf &, bool );
	void OnXMLStatus( mddMsgHdr & );
	void OnXMLMount( const char * );
	void OnXMLIoctl( mddMsgHdr & );

	// Helpers
protected:
	void _ClearUpd();
	void _ClearSchema();

	// Idle Loop Processing ...
public:
	void        OnIdle();
	static void _OnIdle( void * );

}; // class EdgChannel


/////////////////////////////////////////
// rtEdgeCache Service
/////////////////////////////////////////
class EdgSvc
{
protected:
	EdgChannel  &_ch;
	Mutex       &_mtx;
	string       _name;
	RecByNameMap _recs;
	bool         _bUp;

	// Constructor / Destructor
public:
	EdgSvc( EdgChannel &, const char * );
	~EdgSvc();

	// Access

	EdgChannel &ch();
	const char *pName();
	bool        IsUp();
	EdgRec     *GetRec( const char * );

	// Operations

	void Add( EdgRec * );
	void Remove( EdgRec * );

	// Notifications

	void OnService( EdgChannel &, bool );

}; // class EdgSvc


/////////////////////////////////////////
// rtEdgeCache Record
/////////////////////////////////////////
class EdgRec
{
public:
	EdgSvc     &_svc;
	Mutex      &_mtx;
	string      _tkr;
	void       *_arg;
	int         _StreamID;
	int         _nUpd;
	bool        _bOpn;
private:
	rtEdgeData  _upd;
	FieldMap    _upds;
	Record     *_cache;

	// Constructor / Destructor
public:
	EdgRec( EdgSvc &, const char *, int, void * );
	~EdgRec();

	// Access

	const char *pSvc();
	const char *pTkr();
	rtEdgeData &upd();
	Record     *cache();

	// Update Processing

	rtFIELD *GetField( int );
	void     ClearUpd();

	// Helpers
private:
	void _Parse();

}; // class EdgRec


} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_CHANNEL_H
