/******************************************************************************
*
*  EDG_GLchtDb.h
*     ChartDB shared memory (file) layout and reader
*
*  REVISION HISTORY:
*     20 OCT 2010 jcs  Created (OVERWRITE from REAL ChartDB!!!).
*     12 NOV 2014 jcs  Build 28: RTEDGE_PRIVATE
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*     10 SEP 2020 jcs  Build 44: MDDResult
*      6 FEB 2023 jcs  Build 62: GLchtDbItem._idx
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_CHARTDB_DB_H
#define __EDGLIB_CHARTDB_DB_H
#include <EDG_Internal.h>
#include <EDG_GLmmap.h>

#define CDB_SIG_002 "002 ChartDb"
#define CDB_SVCLEN   64
#define CDB_ITEMLEN 256
#define CDB_SVCSEP  "|"

namespace RTEDGE_PRIVATE
{

/////////////////////////////////////////
// File layout
/////////////////////////////////////////
class GLchtDbHdr
{
public:
   CDBlong      _fileSiz;
   CDBint       _freeIdx;
   CDBint       _nTkr;
   CDBint       _hdrSiz;
   CDBlong      _date;    // YYYYMMDD
   char         _signature[16];
//   GLchtDbItem  _items[0];
};

class GLchtDbItem
{
public:
   char   _svc[CDB_SVCLEN];
   char   _tkr[CDB_ITEMLEN];
   int    _idx;
   int    _fid;
   int    _interval;    // Seconds
   int    _curTck;
   int    _nTck;        // SECPERDAY / _interval
   CDBint _siz;         // Total size = GLchtDbItem + _flds + _data
   CDBint _tCreate;     // Time Created
   CDBint _tUpd;        // Time Updated
   CDBint _tUpdUs;      // Time Updated - Microseconds
   CDBint _nUpd;        // Number of Updates
   CDBint _tDead;       // Time of Last Drop; _bAlive == ( _tDead < _tUpd )
//   float  _data[_nTck];

}; // class GLchtDbHdr


/////////////////////////////////////////
// ChartDB memory-mapped file
/////////////////////////////////////////
class GLchtDb : public RTEDGE_PRIVATE::GLmmap
{
protected:
	string _admin;
	RecMap _recs;
	string _name;
	CDBint _freeIdx;
	Mutex  _mtx;
	bool   _bFullCopy;

	// Constructor / Destructor
public:
	GLchtDb( char *, const char * );
	~GLchtDb();

	// Access

	GLchtDbHdr &db();
	RecMap     &recs();
	MDDResult   Query();
	CDBData     GetItem( const char *, const char *, int );
	void        AddTicker( const char *, const char *, int );
	void        DelTicker( const char *, const char *, int );

	// Helpers
private:
	void   Load();
	string MapKey( const char *, const char *, int );

}; // class GLchtDb

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_CHARTDB_DB_H
