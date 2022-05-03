/******************************************************************************
*
*  XmlParser.h
*     XML parsing class
*
*  REVISION HISTORY:
*      3 MAY 2022 jcs  Created (from elsewhere).
*
*  (c) 1994-2022, Gatea Ltd.
*******************************************************************************/
#if defined(xml) || defined(DOXYGEN_INCLUDE)
#ifndef __RTEDGE_XmlParser_H
#define __RTEDGE_XmlParser_H
#include <hpp/rtEdge.hpp>
#include <xmlparse.h>
#include <algorithm>
#include <functional>

#ifdef WIN32
typedef unsigned int u_int32_t;
#endif // WIN32

using namespace std;

typedef void *XML_Parser;

namespace RTEDGE
{

// Forwards

class XmlElem;
class KeyValue;

// Templatized vector collections

typedef vector<XmlElem *>            GLvecXmlElem;
typedef vector<KeyValue *>           GLvecKeyValue;
typedef hash_map<string, KeyValue *> GLmapKeyValue;
typedef hash_map<string, XmlElem *>  GLmapXmlElem;

////////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      K e y V a l u e
//
////////////////////////////////////////////////////////////////////////////////
/**
 * \class KeyValue
 * \brief A parsed named XML element or attribute
 *
 * The XmlParser and XmlElem classes create KeyValue's as the XML file or 
 * document is parsed.  The user never creates KeyValue's
 * \see XmlParser
 * \see XmlElem
 */
class KeyValue
{
protected:
	string _key;
	string _val;

	/////////////////////////////
	// Constructor / Destructor
	/////////////////////////////
	/**
	 * \brief Constructor from parsed ( key, value ) XML data
	 *
	 * \param k - key
	 * \param v - value
	 */
public:
	KeyValue( const char *k="", const char *v="" ) :
	   _key( k ),
	   _val( v )
	{
	   trim( _val );
	}

	/////////////////////////////
	// Access / Mutator
	/////////////////////////////
	/** \brief Return reference to key as std::string */
	string     &stdKey() { return _key; }
	/** \brief Return reference to value as std::string */
	string     &stdVal() { return _val; }
	/** \brief Return key */
	const char *key()   { return _key.data(); }
	/** \brief Return value */
	const char *value() { return _val.data(); }
	/** \brief Set value */
	void        set( char *val )
	{
	   _val = val;
	   trim( _val );
	}

	/////////////////////////////
	// Class-wide
	/////////////////////////////
public:
#ifndef DOXYGEN_OMIT
	static string &ltrim( string &s )
	{
	   string::iterator beg, end, trim;

	   // trim from start
	
	   beg  = s.begin();
	   end  = s.end();
	   trim = find_if( beg, end, not1( ptr_fun<int, int>( isspace ) ) );
	   s.erase( beg, trim );
	   return s;
	}

	static string &rtrim( string &s )
	{
	   string::reverse_iterator rbeg, rend;
	   string::iterator         end, trim;

	   // trim from end
	
	   rbeg = s.rbegin();
	   rend = s.rend();
	   end  = s.end();
	   trim = find_if( rbeg, rend, not1( ptr_fun<int, int>( isspace ) ) ).base();
	   s.erase( trim, end );
	   return s;
	}

	static string &trim( string &s )
	{
	   // trim from start and end

	   return ltrim( rtrim( s ) );
	}
#endif // DOXYGEN_OMIT

}; // class KeyValue


////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      X m l E l e m
//
////////////////////////////////////////////////////////////////////////////////
/**
 * \class XmlElem
 * \brief A parsed named XML element including all elements and attributes
 */
class XmlElem : public KeyValue
{
private:
	XmlElem    *_parent;
	GLvecXmlElem  _elems;
	GLmapXmlElem  _elemsH;
	GLvecKeyValue _attrs;
	GLmapKeyValue _attrsH;

	/////////////////////////////
	// Constructor / Destructor
	/////////////////////////////
public:
	/**
	 * \brief Constructor from parent XmlElem
	 *
	 * \param par - XML parent
	 * \param name - Element name
	 */
	XmlElem( XmlElem *par, const char *name ) :
	   KeyValue( name, "" ),
	   _parent( par ),
	   _elems(),
	   _elemsH(),
	   _attrs(),
	   _attrsH()
	{ ; }

	~XmlElem()
	{
	   int i, sz;

	   _elemsH.clear();
	   _attrsH.clear();
	   for ( i=0,sz=_elems.size(); i<sz; delete _elems[i++] );
	   for ( i=0,sz=_attrs.size(); i<sz; delete _attrs[i++] );
	}

	/////////////////////////////
	// Access
	/////////////////////////////
	/** \brief Return element name */
	const char   *name()     { return key(); }
	/** \brief Return element data payload, if any */
	const char   *pData()    { return value(); }
	/** \brief Return true if this is root element in XML document */
	bool          isRoot()   { return ( _parent == (XmlElem *)0 ); }
	/** \brief Return reference to parent XmlElem */
	XmlElem    *parent()     { return _parent; }
	/** \brief Return list (vector) of child elements */
	GLvecXmlElem &elements() { return _elems; }
	/** 
	 * \brief Search for child XML element by name
	 *
	 * \param lkup - Element name to find
	 * \param bRecurse - Search only 1st-level children if false
	 * \return Element reference if found; NULL if not.
	 */
	XmlElem    *find( const char *lkup, bool bRecurse=true )
	{
	   GLmapXmlElem::iterator it;
	   GLmapXmlElem          &db = _elemsH;
	   XmlElem             *rtn;
	   XmlElem             *glx;
	   int                    i, sz;

	   // 1) Us?

	   if ( !::strcmp( name(), lkup ) )
	      return this;

	   // 2) Walk through our tree

	   if ( !bRecurse )
	      return (XmlElem *)0;
	   sz        = _elems.size();
	   bRecurse &= ( sz > 0 );
	   rtn = (XmlElem *)0;
	   if ( (it=db.find( lkup )) != db.end() )
	      rtn = (*it).second;
	   for ( i=0; !rtn && bRecurse && i<sz; i++ ) {
	      glx = _elems[i];
	      rtn = glx->find( lkup, bRecurse );
	   }
	   return rtn;
	}

#ifndef DOXYGEN_OMIT
	const char   *getAttr( const char *lkup )
	{
	   GLmapKeyValue::iterator it;
	   KeyValue               *kv;
	   string                  s( lkup );

	   if ( (it=_attrsH.find( s )) != _attrsH.end() ) {
	      kv = (*it).second;
	      return kv->value();
	   }
	   return "";
	}

	bool hasAttr( const char *lkup )
	{
	   GLmapKeyValue::iterator it;
	   string                  s( lkup );

	   return ( (it=_attrsH.find( s )) != _attrsH.end() );
	}

	int nElem()
	{
	   size_t i;
	   int    rtn;

	   for ( i=0,rtn=0; i<_elems.size(); i++ )
	      rtn += _elems[i]->nElem();
	   return rtn;
	}

	int nAttr()
	{
	   size_t i;
	   int    rtn;

	   rtn = _attrsH.size();
	   for ( i=0,rtn=0; i<_elems.size(); i++ )
	      rtn += _elems[i]->nAttr();
	   return rtn;
	}
#endif // DOXYGEN_OMIT

	////////////////////////////////////////////
	// Configurable Properties - Elements
	////////////////////////////////////////////
	/**
	 * \brief Return element value as string, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as string, returning default if not found
	 */
	const char *getElemValue( const char *prop, string &dflt )
	{
	   return getElemValue( prop, dflt.data() );
	}

	/**
	 * \brief Return element value as string, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as string, returning default if not found
	 */
	const char *getElemValue( const char *prop, const char *dflt )
	{
	   XmlElem *xe;

	   return (xe=find( prop )) ? xe->pData() : dflt;
	}

	/**
	 * \brief Return element value as bool, returning default if not found
	 *
	 * Allowable boolean values in the XML document are true and false.  
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as bool, returning default if not found
	 */
	bool getElemValue( const char *prop, bool dflt )
	{
	   XmlElem *xe;

	   return (xe=find( prop )) ? !::strcmp( xe->pData(), "true" ) : dflt;
	}

	/**
	 * \brief Return element value as integer, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as integer, returning default if not found
	 */
	int getElemValue( const char *prop, int dflt )
	{
	   XmlElem *xe;

	   return (xe=find( prop )) ? atoi( xe->pData() ) : dflt;
	}

	/**
	 * \brief Return element value as u_int32_t, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as u_int32_t, returning default if not found
	 */
	u_int32_t getElemValue( const char *prop, u_int32_t dflt )
	{
	   XmlElem *xe;

	   return (xe=find( prop )) ? atoi( xe->pData() ) : dflt;
	}

	/**
	 * \brief Return element value as u_int64_t, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as u_int64_t, returning default if not found
	 */
	u_int64_t getElemValue( const char *prop, u_int64_t dflt )
	{
	   XmlElem  *xe;
	   const char *attr;
	   u_int64_t   rtn;
	   bool        bHex;

	   rtn = dflt;
	   if ( (xe=find( prop )) ) {
	      attr = xe->pData();
	      bHex = ( ::strstr( attr, "0x" ) == attr );
	      rtn  = bHex ? ::strtoull( attr, NULL, 16 ) : ::atol( attr );
	   }
	   return rtn;
	}

	/**
	 * \brief Return element value as double, returning default if not found
	 *
	 * \param prop - Element name to find
	 * \param dflt - Default return value if not found
	 * \return Element value as double, returning default if not found
	 */
	double getElemValue( const char *prop, double dflt )
	{
	   XmlElem *xe;

	   return (xe=find( prop )) ? atof( xe->pData() ) : dflt;
	}

	////////////////////////////////////////////
	// Configurable Properties - Attributes
	////////////////////////////////////////////
	/**
	 * \brief Return attribute value as string, returning default if not found
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as string, returning default if not found
	 */
	const char *getAttrValue( const char *prop, string &dflt )
	{
	   return getAttrValue( prop, dflt.data() );
	}

	/**
	 * \brief Return attribute value as string, returning default if not found
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as string, returning default if not found
	 */
	const char *getAttrValue( const char *prop, const char *dflt )
	{
	   return hasAttr( prop ) ? getAttr( prop ) : dflt;
	}

	/**
	 * \brief Return attribute value as bool, returning default if not found
	 *
	 * Allowable boolean values in the XML document are true and false.  
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as bool, returning default if not found
	 */
	bool getAttrValue( const char *prop, bool dflt )
	{
	   return hasAttr( prop ) ? !::strcmp( getAttr( prop ), "true" ) : dflt;
	}

	/**
	 * \brief Return attribute value as integer, returning default if not found
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as integer, returning default if not found
	 */
	int getAttrValue( const char *prop, int dflt )
	{
	   return hasAttr( prop ) ? atoi( getAttr( prop ) ) : dflt;
	}

	/**
	 * \brief Return attribute value as u_int32_t, returning default if not found
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as u_int32_t, returning default if not found
	 */
	u_int32_t getAttrValue( const char *prop, u_int32_t dflt )
	{
	   return hasAttr( prop ) ? (u_int32_t)atoi( getAttr( prop ) ) : dflt;
	}

	/**
	 * \brief Return attribute value as u_int64_t, returning default if not found
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as u_int64_t, returning default if not found
	 */
	u_int64_t getAttrValue( const char *prop, u_int64_t dflt )
	{
	   const char *attr;
	   u_int64_t   rtn;
	   bool        bHex;

	   rtn = dflt;
	   if ( hasAttr( prop ) ) {
	      attr = getAttr( prop );
	      bHex = ( ::strstr( attr, "0x" ) == attr );
	      rtn  = bHex ? ::strtoull( attr, NULL, 16 ) : ::atol( attr );
	   }
	   return rtn;
	}

	/**
	 * \brief Return attribute value as double, returning default if not found
	 *
	 * \param prop - Attribute name to find
	 * \param dflt - Default return value if not found
	 * \return Attribute value as double, returning default if not found
	 */
	double getAttrValue( const char *prop, double dflt )
	{
	   return hasAttr( prop ) ? atof( getAttr( prop ) ) : dflt;
	}

	////////////////////////////////////////////
	// Operations
	////////////////////////////////////////////
#ifndef DOXYGEN_OMIT
	XmlElem *addElement( char *pName )
	{
	   XmlElem *rtn;
	   string     key;

	   rtn = new XmlElem( this, pName );
	   key = rtn->stdKey();
	   _elems.push_back( rtn );
	   _elemsH[key] = rtn;
	   return rtn;
	}

	KeyValue *addAttr( char *pKey, char *pVal )
	{
	   KeyValue *rtn;
	   string      key;

	   rtn = new KeyValue( pKey, pVal );
	   key = rtn->stdKey();
	   _attrs.push_back( rtn );
	   _attrsH[key] = rtn;
	   return rtn;
	}
#endif // DOXYGEN_OMIT

};  // class XmlElem


////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      X m l P a r s e r
//
////////////////////////////////////////////////////////////////////////////////
/**
 * \class XmlParser
 * \brief A parsed XML document read from a file
 */
class XmlParser
{
private:
	XML_Parser _p;
	XmlElem   *_root;
	XmlElem   *_curElem;
	bool       _bDone;
	int        _nRead;

	////////////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////////////
public:
	/** \brief Constructor */
	XmlParser() :
	   _p( (XML_Parser)0 ),
	   _root( (XmlElem *)0 ),
	   _curElem( (XmlElem *)0 ),
	   _bDone( false ),
	   _nRead( 0 )
	{ ; }
	
	~XmlParser()
	{
	   // Free this bad-boy
	
	   reset( false );
	   if ( _p )
	      ::XML_ParserFree( _p );
	}

	////////////////////////////////////////////
	// Access
	////////////////////////////////////////////
	/** \brief true if document is read and is valid */
	bool isValid() { return( _root != (XmlElem *)0 ); }
	/** \brief Root XML element reference */
	XmlElem *root() { return _root; }
#ifndef DOXYGEN_OMIT
	XmlElem *curElem() { return _curElem; }
	int      byteIndex() { return _p ? ::XML_GetCurrentByteIndex( _p ) : -1; }
	int      nRead() { return _nRead; }
	int      nElem() { return _root ? _root->nElem() : 0; }
	int      nAttr() { return _root ? _root->nAttr() : 0; }
#endif // DOXYGEN_OMIT
	/**
	 * \brief Search for child XML element by name
	 *
	 * \param lkup - Element name to find
	 * \param bRecurse - Search only 1st-level children if false
	 * \return Element reference if found; NULL if not.
	 *
	 * \see find()
	 */
	XmlElem *find( const char *lkup, bool bRecurse )
	{
	   return _root ? _root->find( lkup, bRecurse ) : (XmlElem *)0;
	}

	////////////////////////////////////////////
	// Operations
	////////////////////////////////////////////
	/** 
	 * \brief Load XML document from file 
	 *
	 * \return isValid()
	 */
	bool Load( const char *pFile )
	{
	   string      s;
	   const char *xmlData;
	   int         i, sz, off, nb;
	
	   // Use J Clark's expat parsing library
	
	   reset();
	
	   // Parse
	
	   xmlData = ReadFile( pFile, s ); 
	   sz      = strlen( xmlData );
	   for ( i=0,off=0; off<sz; i++ ) {
	      parse( xmlData+off, sz-off );
	      if ( _bDone )
	         break;
	      if ( !(nb=byteIndex()) )
	         break;
	      off  += nb;
	      _root = (XmlElem *)0;
	      reset();
	   }
	   return isValid();
	}
	
#ifndef DOXYGEN_OMIT
private:
	XmlElem *addElement( char *pTag, 
	                     char *pParent  = (char *)0, 
	                     bool  bRecurse = true )
	{
	   XmlElem *par, *rtn;
	
	   // 1) Find it
	
	   par = (XmlElem *)0;
	   if ( pParent )
	      par = find( pParent, bRecurse );
	   if ( !par )
	      par = _root;
	
	   // 2) Build it; Return
	
	   if ( par )
	      rtn = par->addElement( pTag );
	   else
	      rtn = new XmlElem( par, pTag );
	   if ( !_root )
	      _root = rtn;
	   return rtn;
	}
	
	int parse( const char *s, int len, int isFinal=0 )
	{
	   // Pre-condition
	
	   if ( !_p )
	      return -1;
	
	   // Parse at will
	
	   if ( ::XML_Parse( _p, s, len, isFinal ) == 0 ) {
	      _nRead = ::XML_GetCurrentByteIndex( _p );
	      return nRead();
	   }
	   return -1;
	}
	
	void reset( bool bCreate=true )
	{
	   // 1) Our guts
	
	   if ( _root )
	      delete _root;
	   _root    = (XmlElem *)0;
	   _curElem = (XmlElem *)0;
	   _bDone   = false;
	   _nRead   = 0;
	
	   // 2) J Clark's guts
	
	   if ( _p )
	      ::XML_ParserFree( _p );
	   _p = (XML_Parser)0;
	   if ( !bCreate )
	      return;
	
	   // 2a) Create
	
	   _p = ::XML_ParserCreate( NULL );
	   if ( !_p )
	      return;
	   ::XML_SetUserData( _p, this );
	   ::XML_SetElementHandler( _p, &XmlParser::xmlStart, &XmlParser::xmlEnd );
	   ::XML_SetCharacterDataHandler( _p, &XmlParser::xmlData );
	}

	////////////////////////////////////////////
	// Class-wide Callbacks
	////////////////////////////////////////////
private:
	static void xmlStart( void *arg, const char *name, const char **atts )
	{
	   XmlParser *glx = (XmlParser *)arg;
	   XmlElem   *elem;
	   int        i;
	   char      *pName = (char *)name;
	
	   // 1) Add Element
	
	   if ( !glx->_root ) {
	      glx->_root = new XmlElem( (XmlElem *)0, pName );
	      elem       = glx->_root;
	   }
	   else
	      elem = glx->_curElem->addElement( pName );
	   glx->_curElem = elem;
	
	   // 2) Fill in attributes
	
	   for ( i=0; atts[i]; i+=2 )
	      elem->addAttr( (char *)atts[i], (char *)atts[i+1] );
	}
	
	static void xmlData( void *arg, const char *pData, int len )
	{
	   string     tmp( pData, len );
	   XmlParser *glx = (XmlParser *)arg;
	   string    &rwc = glx->_curElem->stdVal();
	
	   rwc += KeyValue::trim( tmp );
	}
	
	static void xmlEnd( void *arg, const char *name )
	{
	   XmlParser *glx = (XmlParser *)arg;
	   XmlElem   *par = glx->_curElem->parent();
	
	   // 1) Bring 'er back down
	
	   glx->_curElem = par;
	   glx->_bDone   = ( glx->_curElem == (XmlElem *)0 );
	}

	////////////////////////////////////////////
	// Class-wide Helpers
	////////////////////////////////////////////
private:
	static char *xTranslate( char *pOut, char *pInp )
	{
	   xTranslate( pOut, pInp, strlen( pInp ) );
	   return pOut;
	}

	static int xTranslate( char *pOut, char *pInp, int len, bool bNws=false )
	{
	   char *ip, *op;
	   int   i;
	   bool  bEsc, bCR;
	
	   // New (quicker) translation
	
	   ip  = pInp;
	   op  = pOut;
	   for ( i=0; i<len; i++,ip++ ) {
	      bCR = false;
	      switch( *ip ) {
	         case 0x3e: // >
	            ::memcpy( op, "&gt;", 4 );
	            op += 4; 
	            break;
	         case 0x3c: // <
	            ::memcpy( op, "&lt;", 4 );
	            op += 4; 
	            break;
	         case 0x22: // "
	            ::memcpy( op, "&quot;", 6 );
	            op += 6; 
	            break;
	         case 0x26: // &
	            ::memcpy( op, "&amp;", 5 );
	            op += 5; 
	            break;
	         case 0x27: // '
	            ::memcpy( op, "&apos;", 6 );
	            op += 6; 
	            break;
	         case 0x09: // \t
	         case 0x0a: // \n
	         case 0x0d: // \r
	            bCR = true;
	            if ( bNws ) {
	               *op++ = *ip;
	               break;
	            }
	            // Fall-through if !bNws
	         default:
	            if ( *ip < 0x20 && !bCR )
	               break;
	            if ( bNws )
	               bEsc = !( *ip >= ' ' && *ip != '@' && *ip < 127 );
	            else
	               bEsc = ( ( *ip & 0x80 ) == 0x80 );
	            if ( bEsc ) {
	               sprintf( op, "&#%d;", (int)( 0x00ff & *ip ) );
	               op += strlen( op );
	            }
	            else
	               *op++ = *ip;
	            break;
	      }
	   }
	   *op = '\0';
	   return( op-pOut );
	}
	
	static const char *ReadFile( const char *filename, string &s )
	{
	    FILE *fp;
	    char  buf[K];
	
	    // Stupid way to read in file - Use mmap() going forward
	
	    s  = "";
	    fp = ::fopen( filename, "rb" );
	    for ( ; fp && ::fgets( buf,K,fp ); s += buf );
	    if ( fp )
	        ::fclose( fp );
	    return s.data();
	}

#endif // DOXYGEN_OMIT

}; // class XmlParser

}  // namespace RTEDGE

#endif // __RTEDGE_XmlParser_H

#endif // defined(xml) || defined(DOXYGEN_INCLUDE)
