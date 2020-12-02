/******************************************************************************
*
*  EdgChannel.h
*     rtEdgeCache subscription channel
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
*     22 OCT 2020 jcs  Build 46: PumpTape() / StopPumpTape()
*     27 NOV 2020 jcs  Build 47: TapeSlice
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_CHANNEL_H
#define __EDGLIB_CHANNEL_H
#include <EDG_Internal.h>
#include <EDG_GLrecDb.h>

#define MAX_FLD 128*K

typedef vector<int>    FIDs;
typedef hash_set<int>  FIDSet;

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class EdgFldDef;
class EdgRec;
class EdgSvc;
class Logger;
class Schema;
class Socket;
class TapeChannel;
class TapeSlice;

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
	int  PumpTape( u_int64_t, u_int64_t, const char *, const char * ); 
	int  StopPumpTape( int );

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

	rtFIELD *GetField( const char * );
	rtFIELD *GetField( int );
	void     ClearUpd();

	// Helpers
private:
	void _Parse();

}; // class EdgRec



/////////////////////////////////////////
// Tape Channel
/////////////////////////////////////////

typedef hash_map<string, rtFIELD> FieldMapByName;
typedef hash_map<string, int>     TapeRecords;
typedef vector<GLrecTapeRec *>    TapeRecDb;
typedef hash_map<int, int>        TapeWatchList;
typedef hash_map<int, string *>   DeadTickers;
typedef vector<u_int64_t>         Offsets;

class TapeChannel
{
private:
	EdgChannel     &_chan;
	rtEdgeAttr      _attr;
	FieldMap        _schema;
	FieldMapByName  _schemaByName;
	rtBuf64         _tape;
	GLrecTapeHdr   *_hdr;
	TapeRecords     _rdb;
	TapeRecDb       _tdb;
	TapeWatchList   _wl;
	DeadTickers     _dead;
	mddWire_Context _mdd;
	mddFieldList    _fl;
	string          _err;
	int             _nSub;
	TapeSlice      *_slice;
	volatile bool   _bRun;
	volatile bool   _bInUse;

	// Constructor / Destructor
public:
	TapeChannel( EdgChannel & );
	~TapeChannel();

	// Access
public:
	EdgChannel     &edg();
	GLrecTapeHdr   &hdr();
	mddWire_Context mdd();
	const char     *pTape();
	const char     *err();
	u_int64_t      *tapeIdxDb();
	bool            HasTicker( const char *, const char *, int & );
	int             GetFieldID( const char * );
	MDDResult       Query();

	// Operations
public:
	int  Subscribe( const char *, const char * );
	int  Unsubscribe( const char *, const char * );
	bool Load();
	int  Pump();
	void Stop();
	int  PumpTicker( int );
	void Unload();

	// Helpers
private:
	bool           _InTimeRange( GLrecTapeMsg & );
	bool           _IsWatched( GLrecTapeMsg & );
	int            _LoadSchema();
	bool           _ParseFieldList( mddBuf );
	int            _PumpDead();
	void           _PumpStatus( GLrecTapeMsg &, const char *, rtEdgeType ty=edg_recovering );
	int            _PumpOneMsg( GLrecTapeMsg &, mddBuf, bool );
	string         _Key( const char *, const char * );
	int            _get32( u_char * );
	u_int64_t      _get64( u_char * );
	int            _tapeIdx( struct timeval );
	int            _SecIdx( struct timeval, GLrecTapeRec * );
	u_int64_t      _DbHdrSize( int, int, int );
	int            _RecSiz();

};  // class TapeChannel


/////////////////////////////////////////
// Tape Slice
/////////////////////////////////////////
class TapeSlice
{
public:
	TapeChannel   &_tape;
	double         _td0;
	double         _td1;
	time_t         _tSnap;
	struct timeval _t0;
	struct timeval _t1;
	int            _tInterval;
	FieldMap       _LVC;
	FIDs           _fids;
	FIDSet         _fidSet;
	rtFIELD        _flds[MAX_FLD];

	// Constructor / Destructor
public:
	TapeSlice( TapeChannel &, const char * );

	// Access
public:
	bool IsSampled();
	bool InTimeRange( GLrecTapeMsg & );
	bool CanPump( rtEdgeData & );

	// Helpers
private:
	struct timeval _str2tv( char * );
	void           _Cache( rtEdgeData & );

};  // class TapeSlice

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_CHANNEL_H
