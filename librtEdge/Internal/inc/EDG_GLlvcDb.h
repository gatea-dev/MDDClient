/******************************************************************************
*
*  EDG_GLlvcDb.h
*     LVC shared memory (file) layout and reader
*
*  REVISION HISTORY:
*      2 SEP 2010 jcs  Created.
*     18 SEP 2010 jcs  Build  7: Admin channel
*      5 OCT 2010 jcs  Build  8b:_bV2
*     30 DEC 2010 jcs  Build  9: _bV3; 64-bit; SetFilter
*     13 JAN 2011 jcs  Build 10: GetItem( bShallow )
*     20 JAN 2012 jcs  Build 17: GLlvcFldDef._name / _type
*      3 FEB 2012 jcs  Build 17a:LVC_SIG_004 : LVC_ITEMLEN = 256
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*     20 OCT 2012 jcs  Build 20: FidMap / RecMap in Internal.h
*      7 MAY 2013 jcs  Build 25: SchemaList
*     10 SEP 2014 jcs  Build 28: _lock; LVCDef; RTEDGE_PRIVATE
*      6 FEB 2016 jcs  Build 32: EDG_Internal.h; Binary; GLlvcFldHdr
*     12 SEP 2017 jcs  Build 35: hash_map; No mo XxxTicker()
*     20 JAN 2018 jcs  Build 39: mtx()
*     17 MAY 2022 jcs  Build 54: GLlvcDbItem._bActive
*     17 MAY 2022 jcs  Build 54: GLlvcDbItem._bActive
*     13 MAR 2023 jcs  Build 62: GetItem_safe()
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_LVC_DB_H
#define __EDGLIB_LVC_DB_H
#include <EDG_Internal.h>
#include <EDG_GLmmap.h>

#define LVC_SIG_001 "002 LVC"
#define LVC_SIG_002 "003 LVC"
#define LVC_SIG_003 "004 LVC"
#define LVC_SIG_004 "005 LVC"
#define LVC_SIG_005 "006 LVC-BINARY"
#define LVC_MAX_FLD 64*K
#define LVC_SVCLEN   64
#define LVC_ITEMLEN 256
#define LVC_MAXSVC  256
#define LVC_SVCSEP  "|"

namespace RTEDGE_PRIVATE
{

// Forwards

class Cockpit;
class LVCDef;

typedef vector<Cockpit *> Cockpits;

/////////////////////////////////////////
// File layout
/////////////////////////////////////////
class GLlvcFldDef
{
public:
   int  _fid;
   int  _len;
   int  _type; // rtFldType
   char _name[64];
};

class GLlvcChanDef
{
public:
   LVClong _addr;
   LVCint  _port;
   LVClong _tUp;
   LVClong _tDown;  // bUP = _tUP > _tDown
   char    _bRMDS;
   char    _pad[3];
};

class GLlvcSvcDef
{
public:
   char    _svc[LVC_SVCLEN];
   LVClong _tUp;
   LVClong _tDown;  // bUP = _tUP > _tDown
};

class GLlvcDbHdr
{
public:
   LVClong     _fileSiz;
   LVCint      _freeIdx;
   LVCint      _hdrSiz;
   char        _signature[16];
   LVCint      _nFlds;
   LVCint      _pad;
//   GLlvcFldDef _schema[_nFlds];
//   GLlvcDbItem _items[0];
};

class GLlvcDbHdr3 : public GLlvcDbHdr
{
public:
   GLlvcChanDef _chan;
   LVClong      _tSvcMod;
   LVCint       _nSvc;
   GLlvcSvcDef  _svcs[LVC_MAXSVC];
//   GLlvcFldDef _schema[_nFlds];
//   GLlvcDbItem _items[0];
};

class GLlvcDbItem
{
public:
   char    _svc[LVC_SVCLEN];
   char    _tkr[LVC_ITEMLEN-1];
   char    _bActive;
   LVCint  _siz;         // Total size = GLlvcDbItem + _flds + _data
   LVCint  _tCreate;     // Time Created
   LVCint  _tUpd;        // Time Updated
   LVCint  _tUpdUs;      // Time Updated - Microseconds
   LVCint  _nUpd;        // Number of Updates
   LVCint  _tDead;       // Time of Last Drop; _bAlive == ( _tDead < _tUpd )
   LVCint  _nFld;        // Number of fields
//   LVCint  _flds[_nFld]; // Index into GLlvcDbHdr._flds
//   char    _data[_siz];
};

class GLlvcFldHdr1
{
public:
   u_short _len;
};

class GLlvcFldHdr : public GLlvcFldHdr1
{
public:
   u_char  _type;  // (u_char)mddFldType
};


/////////////////////////////////////////
// LVC memory-mapped file
/////////////////////////////////////////

typedef hash_map<int, GLlvcFldDef> SchemaByFid;
typedef vector<GLlvcFldDef>        SchemaList;

class GLlvcDb : public RTEDGE_PRIVATE::GLmmap
{
protected:
	LVCDef     &_def;
	FidMap      _fidOffs;
	FidMap      _fidFltr;
	SchemaByFid _schemaByFid;
	SchemaList  _schema;
	RecMap      _recs;
	string      _name;
	LVCint      _freeIdx;
	Mutex       _mtx;
	bool        _bFullCopy;
	Semaphore  *_lock;
	bool        _locked;
	bool        _bBinary;
	Cockpits    _cockpits;

	// Constructor / Destructor
public:
	GLlvcDb( LVCDef &, bool, DWORD waitMillis=INFINITE );
	~GLlvcDb();

	// Access

	GLlvcDbHdr  &db();
	GLlvcDbHdr3 &db3();
	GLlvcFldDef *fdb();
	RecMap      &recs();
	bool         IsLocked();
	int          FieldLen( int );
	int          FieldOffset( int );
	bool         CanAddField( int );
	LVCData      GetItem( const char *, const char *, Bool );
	LVCData      GetItem_safe( const char *, const char *, Bool );
	int          SetFilter( const char * );
	Bool         IsBinary();
	Mutex       &mtx();
	int          _uSz();

	// Cockpit Operations
public:
	void Attach( Cockpit * );
	void Detach( Cockpit * );

	// Helpers
private:
	void    Load();
	rtFIELD GetField( GLlvcFldHdr &, char *, int, Bool, char * );
	void    SetFieldAttr_OBSOLETE( rtFIELD & );
	string  MapKey( const char *, const char * );

}; // class GLlvcDb


/////////////////////
// LVC Config Def'n
/////////////////////
class LVCDef
{
private:
	string   _file;
	GLlvcDb *_lvc;
public:
	bool     _bFullCopy;

	// Constructor / Destructor
public:
	LVCDef( const char *, bool, DWORD );
	~LVCDef();

	// Access / Operations
public:
	GLlvcDb &lvc();
	char    *pFile();
	int      SetFilter( const char * );
	void     SetCopyType( bool );

}; // class LVCDef

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_LVC_DB_H
