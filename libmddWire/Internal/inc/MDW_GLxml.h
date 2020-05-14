/******************************************************************************
*
*  MDW_GLxml.h
*     XML parsing class
*
*  REVISION HISTORY:
*     13 AUG 2000 jcs  Created.
*     18 SEP 2013 jcs  Created (from librtEdge)
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_GLxml.h
*
*  (c) 1994-2015 Gatea Ltd.
*******************************************************************************/
#ifndef __MDD_GLXML_H
#define __MDD_GLXML_H
#include <sys/types.h>
#include <map>
#include <string>
#include <vector>
// #include <xmlparse.h>

typedef void *XML_Parser;

namespace MDDWIRE_PRIVATE
{

//////////////////////
// Forwards
//////////////////////
class GLkeyValue;
class GLxmlElem;

// Templatized vector collections

typedef vector<GLxmlElem *>       GLvecXmlElem;
typedef vector<GLkeyValue *>      GLvecKeyValue;
typedef map<string, GLxmlElem *>  GLmapXmlElem;
typedef map<string, GLkeyValue *> GLmapKeyValue;

#define _XML_MAXSTR 100

/////////////////////////////////////////
// Key/Value Pair
/////////////////////////////////////////
class GLkeyValue
{
protected:
	string _key;
	string _val;

	// Constructor / Destructor
public:
	GLkeyValue( char *k="", char *v="" );
	GLkeyValue( GLkeyValue & );
	~GLkeyValue();

	// Access / Mutator

	string     &stdKey();
	string     &stdVal();
	const char *key();
	const char *value();
	void        set( char * );
};

/////////////////////////////////////////
// XML Element
/////////////////////////////////////////
class GLxmlElem : public GLkeyValue
{
private:
	GLxmlElem    *_parent;
	GLvecXmlElem  _elems;
	GLmapXmlElem  _elemsH;
	GLvecKeyValue _attrs;
	GLmapKeyValue _attrsH;

	// Constructor / Destructor
public:
	GLxmlElem( GLxmlElem *, char * );
	~GLxmlElem();

	// Access

	const char    *name();
	const char    *pData();
	bool           isRoot();
	GLxmlElem     *parent();
	GLvecXmlElem  &elements();
	GLvecKeyValue &attributes();
	GLxmlElem     *find( char *, bool bRecurse=true );
	const char    *getAttr( char * );
	int            nElem();
	int            nAttr();

	// Operations

	GLxmlElem  *addElement( char * );
	GLkeyValue *addAttr( char *, char * );
	int         header( char *, int, int, bool );
	int         build( char *, int, int, bool );
	int         trailer( char *, int, int, bool );
};

/////////////////////////////////////////
// XML parsing class
/////////////////////////////////////////
class GLxml
{
private:
	XML_Parser _p;
	GLxmlElem *_root;
	GLxmlElem *_curElem;
	bool       _bDone;
	int        _nRead;
	bool       _bSpaces;

	// Constructor / Destructor
public:
	GLxml( bool bSpaces=true );
	GLxml( char *, bool bSpaces=true );
	~GLxml();

	// Access

	bool       isComplete();
	bool       isValid();
	GLxmlElem *root();
	GLxmlElem *curElem();
	int        byteIndex();
	int        nRead();
	int        nElem();
	int        nAttr();
	GLxmlElem *find( char *, bool bRecurse=true );

	// Operations

	GLxmlElem *addElement( char *, 
			char *pParent  = (char *)0, 
			bool  bRecurse = true );
	string     build();
	int        parse( const char *, int, int isFinal=0 );
	void       reset( bool bCreate=true );

	// Class-wide Callbacks
private:
	static void xmlStart( void *, const char *, const char ** );
	static void xmlData( void *, const char *, int );
	static void xmlEnd( void *, const char * );

	// Class-wide Helpers
public:
	static char *xTranslate( char *, char * );
	static int   xTranslate( char *, char *, int, bool bNws=false );
};

/////////////////////////////////////////
// XML nesting class
/////////////////////////////////////////
class GLxmlNest
{
private:
	char *_pSpaces;
	int   _nDepth;
	bool  _bSpaces;

	// Constructor / Destructor
public:
	GLxmlNest( int, bool );
	~GLxmlNest();

	// Access

	char *spaces();
	int   nDepth();
};

} // namespace MDDWIRE_PRIVATE

#endif   // __MDD_GLXML_H
