/******************************************************************************
*
*  RecCache.h
*     MDDirect record cache
*
*  REVISION HISTORY:
*      7 APR 2011 jcs  Created.
*      . . . 
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     22 NOV 2020 jcs  Build  3: PyObjects
*     17 OCT 2023 jcs  Build 12: No mo Book
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_RECORD_H
#define __MDDPY_RECORD_H
#include <MDDirect.h>


namespace MDDPY
{

/////////////////////////////////////////
// Cached Field
/////////////////////////////////////////
class Field
{
private:
	mddField _fld;
	mddBuf   _b;
	int      _nAlloc;
public:
	bool     _bUpd;

	// Constructor / Destructor
public:
	Field( mddField, bool bSchema=false );
	Field();
	~Field();

	// Access

	mddField   &fld();
	char       *data();
	int         dLen();
	int         Fid();
	const char *name();
	mddFldType  type();
	PyObject   *GetValue( int & );

	// Mutator

	void Update( mddField & );
	void ClearUpd();

	// Helpers
private:
	char *_InitBuf( int );
	int   _bufcpy( char *, mddBuf );

};  // class Field


typedef hash_map<int, Field *>  FldMap; 
typedef vector<Field *>         FldUpds;
typedef FldUpds                 FldList;

/////////////////////////////////////////
// Schema
/////////////////////////////////////////
class Schema
{
protected:
	RTEDGE::Mutex _mtx;
	FldList       _flds;

	// Constructor / Destructor
public:
	Schema();
	~Schema();

	// Access / Operations

	int  GetUpds( PyObjects & );
	void Update( rtEdgeData & );
	void Clear();

}; // class Schema

} // namespace MDDPY


/////////////////////////////////////////
// Cached Record
/////////////////////////////////////////
class Record
{
protected:
	string         _svc;
	string         _tkr;
	MDDPY::FldMap  _flds;
	MDDPY::FldUpds _upds;
	int            _oid;
public:
	int            _userReqID;
	int            _nUpd;
	double         _tUpd;

	// Constructor / Destructor
public:
	Record( const char *, const char *, int, int );
	virtual ~Record();

	// Access

	const char   *pSvc();
	const char   *pTkr();
	int           oid();
	int           nFld();
	MDDPY::Field *GetField( int );
	int           TouchAllFields();
	int           GetUpds( PyObjects & );

	// Real-Time Data

	virtual void Update( mddFieldList );

};  // class Record

#endif // __MDDPY_RECORD_H
