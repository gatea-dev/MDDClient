/******************************************************************************
*
*  PubChannel.h
*     rtEdgeCache publication channel
*
*  REVISION HISTORY:
*     23 SEP 2010 jcs  Created (from EdgChannel)
*     11 MAY 2011 jcs  Build 12: .NET : _pub
*     24 JAN 2012 jcs  Build 17: nUpd()
*     23 APR 2012 jcs  Build 19: LIBRTEDGE
*     14 FEB 2013 jcs  Build 24: Ioctl()
*      9 MAY 2013 jcs  Build 25: _BuildFld()
*     10 JUL 2013 jcs  Build 26: Schema ; int Publish()
*     11 JUL 2013 jcs  Build 26a:rtEdgeChanStats
*     13 SEP 2014 jcs  Build 28: libmddWire; _cxt; PubError()
*     23 JAN 2015 jcs  Build 29: WatchList
*      6 JUL 2015 jcs  Build 31: PubGetData(); _preBuilt
*     15 APR 2016 jcs  Build 32: EDG_Internal.h; On1SecTimer(); OnQuery()
*     26 MAY 2017 jcs  Build 34: Socket "has-a" Thread
*     12 SEP 2017 jcs  Build 35: No mo GLHashMap
*     12 FEB 2020 jcs  Build 42: Socket._tHbeat
*     22 DEC 2024 jcs  Build 74: ConnCbk()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_PUB_CHANNEL_H
#define __EDGLIB_PUB_CHANNEL_H
#include <EDG_Internal.h>

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class PubRec;
class Socket;

typedef hash_map<string, PubRec *>    WatchListByName;
typedef hash_map<int, PubRec *>       WatchListByID;

/////////////////////////////////////////
// rtEdgeCache Publication Channel
/////////////////////////////////////////
class PubChannel : public Socket
{
protected:
	rtEdgePubAttr   _attr;
	rtEdge_Context  _cxt;
	rtEdgeDataFcn   _schemaCbk;
	rtEdgeData     *_schema;
	string          _pub;
	string          _authReq;
	string          _authRsp;
	WatchListByName _wlByName;
	WatchListByID   _wlByID;
	mddWire_Context _mddXml;
	int             _hopCnt;
	bool            _bUserMsgTy;
	bool            _bPerm;
	u_int           _tLastMsgRX;
	rtPreBuiltBUF  *_preBuilt;

	// Constructor / Destructor
public:
	PubChannel( rtEdgePubAttr, rtEdge_Context );
	~PubChannel();

	// Access

	rtEdgePubAttr attr();
	rtEdgeData    GetSchema();

	// Operations

	void  InitSchema( rtEdgeDataFcn );
	int   Publish( rtEdgeData & );
	int   PubError( rtEdgeData &, const char * );
	rtBUF PubGetData();
private:
	int   _PubStreamSymList();
	void  _GetStreamCache();
	int   _PubStreamID( const char *, int );
	int   _PubPermQryRsp( rtEdgeData & );

	// Socket Interface
public:
	virtual void ConnCbk( const char *, bool );
	virtual bool Ioctl( rtEdgeIoctl, void * );

	// Thread Notifications
protected:
	virtual void OnConnect( const char * );
	virtual void OnDisconnect( const char * );
	virtual void OnRead();
private:
	void         _OnMPAC();

	// TimerEvent Notifications
protected:
	virtual void On1SecTimer();

	// Socket Notifications

	void OnXML( mddMsgHdr & );

	// XML Handlers
protected:
	void OnPubOpen( mddMsgHdr & );
	void OnPubClose( mddMsgHdr & );
	void OnMount( mddMsgHdr & );
	void OnQuery( mddMsgHdr & );
	void OnCTL( mddMsgHdr & );

	// Helpers
private:
	PubRec *_Open( const char *, int );
	void    _Close( const char * );
	PubRec *_GetRec( const char *, int );
	void    _ClearSchema();

	// Idle Loop Processing ...
public:
	void        OnIdle();
	static void _OnIdle( void * );
};

/////////////////////////////////////////
// rtEdgeCache Pub Record
/////////////////////////////////////////
class PubRec
{
public:
	PubChannel &_pub;
	string      _tkr;
	int         _StreamID;
	int         _nOpn;
	int         _nImg;
	int         _nUpd;

	// Constructor / Destructor
public:
	PubRec( PubChannel &, const char *, int );
	~PubRec();

	// Access

	const char *pSvc();
	const char *pTkr();
};

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_PUB_CHANNEL_H
