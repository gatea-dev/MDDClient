/******************************************************************************
*
*  TapeChannel.h
*     MDDirect subscription channel : Tape
*
*  REVISION HISTORY:
*      1 SEP 2022 jcs  Created (from EdgChannel)
*     23 SEP 2022 jcs  GetField()
*     14 OCT 2022 jcs  PumpOneMsg( ..., bool &bContinue )
*     12 JAN 2024 jcs  TapeHeader.h
*     26 JUN 2024 jcs  Build 72: FIDSet in EDG_Internal.h
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __TAPE_CHANNEL_H
#define __TAPE_CHANNEL_H
#include <EDG_Internal.h>
#include <TapeHeader.h>

#define MAX_FLD 128*K

namespace RTEDGE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class GLrpyDailyIdxVw;
class Schema;


/////////////////////////////////////////
// Tape Channel
/////////////////////////////////////////

typedef hash_map<int, rtFIELD>    FieldMap;
typedef hash_map<string, rtFIELD> FieldMapByName;
typedef hash_map<string, int>     TapeRecords;
typedef vector<TapeRecHdr *>      TapeRecDb;
typedef hash_map<int, int>        TapeWatchList;
typedef hash_map<int, string *>   DeadTickers;
typedef vector<u_int64_t>         Offsets;

class TapeChannel
{
friend class TapeRun;
private:
	EdgChannel      &_chan;
	rtEdgeAttr       _attr;
	string           _idxFile;
	FieldMap         _schema;
	FieldMapByName   _schemaByName;
	GLmmap          *_vwHdr;
	TapeHeader      *_hdr;
	GLrpyDailyIdxVw *_idx;
	TapeRecords      _rdb;
	TapeRecDb        _tdb;
	TapeWatchList    _wl;
	DeadTickers      _dead;
	mddWire_Context  _mdd;
	mddFieldList     _fl;
	FieldMap         _upds;
	string           _err;
	int              _nSub;
	Mutex            _sliceMtx;
	TapeSlice       *_slice;
	volatile bool    _bRun;
	volatile bool    _bInUse;

	// Constructor / Destructor
public:
	TapeChannel( EdgChannel & );
	~TapeChannel();

	// Access
public:
	EdgChannel     &edg();
	TapeHeader     &hdr();
	mddWire_Context mdd();
	const char     *pTape();
	const char     *pIdxFile();
	const char     *err();
	bool            HasTicker( const char *, const char *, int & );
	int             GetFieldID( const char * );
	rtFIELD        *GetField( int );
	MDDResult       Query();

	// PumpTape
public:
	int StartPumpFullTape( u_int64_t, int );
	int StopPumpFullTape( int );

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
	bool        _LoadHdr();
	TapeRecHdr *_GetRecHdr( int );
	bool        _InTimeRange( GLrecTapeMsg & );
	bool        _IsWatched( GLrecTapeMsg & );
	int         _LoadSchema();
	bool        _ParseFieldList( mddBuf );
	int         _PumpDead();
	void        _PumpStatus( GLrecTapeMsg *, const char *, rtEdgeType ty=edg_recovering, u_int64_t off=0 );
	int         _PumpSlice( u_int64_t, int );
	int         _PumpOneMsg( GLrecTapeMsg &, mddBuf, bool, bool & );
	void        _PumpComplete( GLrecTapeMsg *, u_int64_t );
	string      _Key( const char *, const char * );
	int         _get32( u_char * );
	u_int64_t   _get64( u_char * );
	u_int64_t   _tapeOffset( struct timeval );
	int         _SecIdx( struct timeval, GLrecTapeRec * );
	void        _BuildFieldMap();
	void        _ClearFieldMap();

};  // class TapeChannel

class TapeRun
{
private:
	TapeChannel &_tape;

	/////////////////////////////
	// Constructor / Destructor
	/////////////////////////////
public:
	TapeRun( TapeChannel &tape ) :
	   _tape( tape )
	{
	   _tape._bRun   = true;
	   _tape._bInUse = true;
	}

	~TapeRun()
	{
	   _tape._bRun   = false;
	   _tape._bInUse = false;
	}

}; // TapeRun


/////////////////////////////////////////
// Tape Slice
/////////////////////////////////////////
class TapeSlice
{
public:
	TapeChannel   &_tape;
	long           _ID;
	bool           _bByTime;
	/*
	 * _bByTime
	 */
	double         _td0;
	double         _td1;
	time_t         _tSnap;
	struct timeval _t0;
	struct timeval _t1;
	int            _tInterval;
	FIDs           _fids;
	FIDSet         _fidSet;
	/*
	 * !_bByTime
	 */
	u_int64_t      _off0;
	int            _NumMsg;
	/*
	 * LVC
	 */
	FieldMap       _LVC;
	rtFIELD        _flds[MAX_FLD];

	// Constructor / Destructor
public:
	TapeSlice( TapeChannel &, const char * );
	TapeSlice( TapeChannel &, u_int64_t, int );
	TapeSlice( const TapeSlice & );

	// Access
public:
	bool IsSampled();
	bool InTimeRange( GLrecTapeMsg & );
	bool CanPump( int, rtEdgeData & );

	// Helpers
private:
	struct timeval _str2tv( char * );
	void           _Cache( rtEdgeData & );

};  // class TapeSlice

/////////////////////////////////////////
// View on Daily Index
/////////////////////////////////////////
class GLrpyDailyIdxVw : public GLmmap
{
private:
	TapeHeader &_hdr;
	u_int64_t   _off;
	u_int64_t   _daySz;
	u_int64_t   _fileSz;
	Sentinel   *_ss;
	u_int64_t  *_tapeIdxDb;

	// Constructor
public:
	GLrpyDailyIdxVw( TapeHeader &, char * );
	~GLrpyDailyIdxVw();

	// Access / Operations
public:
	Sentinel  &sentinel();
	u_int64_t *tapeIdxDb();
	Bool       forth();
private:
	Bool       _Set();

}; // class GLrpyDailyIdxVw

} // namespace RTEDGE_PRIVATE

#endif // __TAPE_CHANNEL_H
