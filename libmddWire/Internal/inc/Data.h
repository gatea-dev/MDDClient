/******************************************************************************
*
*  Data.h
*     MD-Direct data base class
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     12 OCT 2013 jcs  Build  3: LoadLibrary(), etc.
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_GLxml.h; MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __MDD_DATA_H
#define __MDD_DATA_H
#include <MDW_Internal.h>
#include <MDW_GLxml.h>

//////////////////////
// Loadable Library
//////////////////////
#if !defined(WIN32)
#define HINSTANCE      void *
#define GetProcAddress dlsym
#define LoadLibrary(c) dlopen( c, RTLD_NOW )
#define FreeLibrary(c) dlclose( c )
#include <dlfcn.h>
#endif // WIN32

namespace MDDWIRE_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class GLxmlElem;
class Logger;
class mddFldDef;
class Schema;
class Subscribe;
class Publish;

/////////////////////////////////////////
// MD-Direct Data
/////////////////////////////////////////
class Data
{
public:
	static Logger      *_log;
	static mddBuf       _bz;
	static mddFieldList _fz;
	static Str2IntMap   _mons;
	static char        *_pMons[];
protected:
	bool        _bPub;
	mddProtocol _proto;
	Schema     *_schema;
	bool        _bOurSchema;
	GLxml       _xml;
	string      _xName;
	bool        _bParseInC;
	bool        _bNativeFld;
	mddBldBuf   _mf;

	// Fixed-to-FieldList 

	HINSTANCE    _hLib;
	mddDriverFcn _fcn;

	// Constructor / Destructor
protected:
	Data( bool );
public:
	virtual ~Data();

	// API

	mddProtocol  proto();
	mddBuf       Ping();
	void         Ioctl( mddIoctl, void * );
	void         SetProtocol( mddProtocol );
	int          SetSchema( const char * );
	int          CopySchema( Schema * );
	Schema      *schema();
	mddFieldList GetSchema();
	mddField    *GetDef( int );
	mddField    *GetDef( const char * );

	// API - Data Interface

	virtual int Parse( mddMsgBuf, mddWireMsg & ) = 0;
	virtual int ParseHdr( mddMsgBuf, mddMsgHdr & ) = 0;

	// XML
protected:
	int _XML_Parse( mddMsgBuf, mddWireMsg & );
	int _XML_ParseHdr( mddMsgBuf, mddMsgHdr & );

	// Utilities
protected:
	mddFldDef *_GetDef( int );
	mddFldDef *_GetDef( const char * );
	int        _strcat( char *, const char * );
	int        _strcat( char *, char * );
	int        _int2str( char *, int );
	int        _bufcpy( char *, mddBuf );
	char      *_TrimTrailingZero( char * );
	mddBuf     _SetBuf( char *, int len=0 );
	char      *_InitBuf( mddBldBuf &, int );
	void       _InitFieldList( mddFieldList &, int );
	void       _InitMsg( mddWireMsg &, mddMsgHdr & );
	void       _InitMsg( mddWireMsg &, mddBinHdr & );
	void       _InitMsgHdr( mddMsgHdr &, int, mddMsgType, mddDataType );
	char      *_memrchr( char *, int, size_t );

	// Loadable Fixed-to-FieldList
protected:
	void LoadFixedLibrary( char * );
};

//////////////////////////////
// The Main Class
//////////////////////////////
class mddWire
{
public:
	mddWire_Context _cxt;
	Subscribe      *_sub;
	Publish        *_pub;

	// Constructor / Destructor
public:
	mddWire( mddWire_Context, bool );
	~mddWire();

	// API Operations
public:
	int          ParseHdr( mddMsgBuf, mddMsgHdr & );
	mddBuf       Ping();
	void         Ioctl( mddIoctl, void * );
	void         SetProtocol( mddProtocol );
	mddProtocol  GetProtocol();
	int          SetSchema( const char * );
	int          CopySchema( Schema * );
	Schema      *schema();
	mddFieldList GetSchema();
	mddField    *GetFldDef( int );
	mddField    *GetFldDef( const char * );
};

} // namespace MDDWIRE_PRIVATE

#endif // __MDD_DATA_H
