/******************************************************************************
*
*  Schema.h
*     MD-Direct schema
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*     29 OCT 2022 jcs  Build 16: hash_map; No mo _ddb
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __MDD_SCHEMA_H
#define __MDD_SCHEMA_H
#include <MDW_Internal.h>

namespace MDDWIRE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class mddFldDef;

typedef hash_map<int, mddFldDef *>    FldDefByIdMap;
typedef hash_map<string, mddFldDef *> FldDefByNameMap;

/////////////////////////////////////////
// MD-Direct Schema
/////////////////////////////////////////
class Schema
{
private:
	mddFieldList    _fl;
	int             _minFid;
	int             _maxFid;
	FldDefByIdMap   _gfifId;
	FldDefByNameMap _gfifStr;

	// Constructor / Destructor
public:
	Schema( const char * );
	~Schema();

	// Access

	int          Size();
	mddFieldList Get();
	mddFldDef   *GetDef( int );
	mddFldDef   *GetDef( const char * );

}; // class Schema

/////////////////////////////////////////
// MD-Direct Field Definition
/////////////////////////////////////////
class mddFldDef
{
public:
	mddField   _mdd;
	string     _name;
	int        _fid;
	string     _type;
	mddFldType _fType;
	int        _maxLen;

	// Constructor / Destructor
public:
	mddFldDef( const char * );
	~mddFldDef();

	// Access

	const char *pName();
	const char *pType();
	mddFldType  fType();
};

} // namespace MDDWIRE_PRIVATE

#endif // __MDD_SCHEMA_H
