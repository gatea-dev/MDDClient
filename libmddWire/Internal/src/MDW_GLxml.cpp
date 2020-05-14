/******************************************************************************
*
*  MDW_GLxml.cpp
*     XML parsing class
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge)
*     12 NOV 2014 jcs  Build  8: -Wall
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>
#include <xmlparse.h>

using namespace MDDWIRE_PRIVATE;

#define _HasRoom( x )  ( (x) > K )
#define XML_MAX_SIZ    K*K


////////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      G L k e y V a l u  e
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLkeyValue::GLkeyValue( char *k, char *v ) : 
   _key( k ),
   _val( v )
{
}

GLkeyValue::GLkeyValue( GLkeyValue &kv ) :
   _key( kv._key ),
   _val( kv._val )
{
}

GLkeyValue::~GLkeyValue()
{
}


////////////////////////////////////////////
// Access / Mutator
////////////////////////////////////////////
string &GLkeyValue::stdKey()
{
   return _key;
}

string &GLkeyValue::stdVal()
{
   return _val;
}

const char *GLkeyValue::key()
{
   return _key.c_str();
}

const char *GLkeyValue::value()
{
   return _val.c_str();
}

void GLkeyValue::set( char *pVal )
{
   _val = pVal;
}




////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      G L x m l E l e m
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLxmlElem::GLxmlElem( GLxmlElem *p, char *pName ) :
   GLkeyValue( pName, "" ),
   _parent( p ),
   _elems(),
   _elemsH(),
   _attrs(),
   _attrsH()
{
}

GLxmlElem::~GLxmlElem()
{
   int i, sz;

   _elemsH.clear();
   _attrsH.clear();
   for ( i=0,sz=_elems.size(); i<sz; delete _elems[i++] );
   for ( i=0,sz=_attrs.size(); i<sz; delete _attrs[i++] );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
const char *GLxmlElem::name()
{
   return key();
}

const char *GLxmlElem::pData()
{
   return value();
}

bool GLxmlElem::isRoot()
{
   return( _parent == (GLxmlElem *)0 );
}

GLxmlElem *GLxmlElem::parent()
{
   return _parent;
}

GLvecXmlElem &GLxmlElem::elements()
{
   return _elems;
}

GLvecKeyValue &GLxmlElem::attributes()
{
   return _attrs;
}

GLxmlElem *GLxmlElem::find( char *pLkup, bool bRecurse )
{
   GLmapXmlElem::iterator it;
   GLmapXmlElem          &db = _elemsH;
   GLxmlElem             *rtn;
   GLxmlElem             *glx;
   int                    i, sz;

   // 1) Us?

   if ( !::strcmp( name(), pLkup ) )
      return this;

   // 2) Walk through our tree

   if ( !bRecurse )
      return (GLxmlElem *)0;
   sz        = _elems.size();
   bRecurse &= ( sz > 0 );
   rtn = (GLxmlElem *)0;
   if ( (it=db.find( pLkup )) != db.end() ) 
      rtn = (*it).second;
   for ( i=0; !rtn && bRecurse && i<sz; i++ ) {
      glx = _elems[i];
      rtn = glx->find( pLkup, bRecurse );
   }
   return rtn;
}

const char *GLxmlElem::getAttr( char *pLkup )
{
   GLmapKeyValue::iterator it;
   GLkeyValue             *kv;
   string                  s( pLkup );

   if ( (it=_attrsH.find( s )) != _attrsH.end() ) {
      kv = (*it).second;
      return kv->value();
   }
   return "";
}

int GLxmlElem::nElem()
{
   int    rtn;
   size_t i; 

   for ( i=0,rtn=0; i<_elems.size(); i++ )
      rtn += _elems[i]->nElem();
   return rtn;
}

int GLxmlElem::nAttr()
{
   int    rtn;
   size_t i;

   rtn = _attrsH.size();
   for ( i=0,rtn=0; i<_elems.size(); i++ )
      rtn += _elems[i]->nAttr();
   return rtn;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
GLxmlElem *GLxmlElem::addElement( char *pName )
{
   GLxmlElem *rtn;
   string     key;

   rtn = new GLxmlElem( this, pName );
   key = rtn->stdKey();
   _elems.push_back( rtn );
   _elemsH[key] = rtn;
   return rtn;
}

GLkeyValue *GLxmlElem::addAttr( char *pKey, char *pVal )
{
   GLkeyValue *rtn;
   string      key;

   rtn = new GLkeyValue( pKey, pVal );
   key = rtn->stdKey();
   _attrs.push_back( rtn );
   _attrsH[key] = rtn;
   return rtn;
}

int GLxmlElem::header( char *bp, int nLeft, int nDepth, bool bSpaces )
{
   GLxmlNest   n( nDepth, bSpaces );
   GLkeyValue *kv;
   char       *cp;
   int         len;
   size_t      i;
   char       *pEnd = ( _elems.size() == 0 ) ? (char *)"/" : (char *)"";

   // 1) Tag Header

   sprintf( (cp=bp), "%s<%s", n.spaces(), name() );
   cp += strlen( cp );

   // 2) Attributes

   for ( i=0; i<_attrs.size() && _HasRoom( nLeft ); i++ ) {
      kv = _attrs[i];
      sprintf( cp, " %s=\"%s\"", kv->key(), kv->value() );
      len    = strlen( cp );
      nLeft -= len;
      cp    += len;
   }

   // 3) Tag trailer

   sprintf( cp, "%s>\n", pEnd );
   cp += strlen( cp );
   return( cp - bp );
}

int GLxmlElem::build( char *bp, int nLeft, int nDepth, bool bSp )
{
   GLxmlElem *glx;
   char      *cp;
   size_t     i;
   int        j, len;

   // Elements

   cp = bp;
   for ( i=0; i<_elems.size() && _HasRoom( nLeft ); i++ ) {
      glx = _elems[i];
      for ( j=0; j<3 && _HasRoom( nLeft ); j++ ) {
         switch( j ) {
            case 0: len = glx->header( cp,  nLeft, nDepth+1, bSp ); break;
            case 1: len = glx->build( cp,   nLeft, nDepth+1, bSp ); break;
            case 2: len = glx->trailer( cp, nLeft, nDepth+1, bSp ); break;
         }
         nLeft -= len;
         cp    += len;
      }
   }
   return( cp - bp );
}

int GLxmlElem::trailer( char *bp, int nLeft, int nDepth, bool bSpaces )
{
   char *cp;

   cp = bp;
   if ( ( _elems.size() > 0 ) && _HasRoom( nLeft ) ) {
      GLxmlNest n( nDepth, bSpaces );

      sprintf( cp, "%s</%s>\n", n.spaces(), name() );
      cp += strlen( cp );
   }
   return( cp - bp );
}


////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      G L x m l
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLxml::GLxml( bool bSpaces ) :
   _p( (XML_Parser)0 ),
   _root( (GLxmlElem *)0 ),
   _curElem( (GLxmlElem *)0 ),
   _bDone( false ),
   _nRead( 0 ),
   _bSpaces( bSpaces )
{
   // Use J Clark's expat parsing library

   reset();
}

GLxml::GLxml( char *pTag, bool bSpaces ) :
   _p( (XML_Parser)0 ),
   _root( new GLxmlElem( (GLxmlElem *)0, pTag ) ),
   _curElem( (GLxmlElem *)0 ),
   _bDone( false ),
   _nRead( 0 ),
   _bSpaces( bSpaces )
{
   // Use J Clark's expat parsing library

   reset();
}

GLxml::~GLxml()
{
   // Free this bad-boy

   reset( false );
   if ( _p != (XML_Parser)0 )
      ::XML_ParserFree( _p );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
bool GLxml::isComplete()
{
   return _bDone;
}

bool GLxml::isValid()
{
   return( _p != (XML_Parser)0 );
}

GLxmlElem *GLxml::root()
{
   return _root;
}

GLxmlElem *GLxml::curElem()
{
   return _curElem;
}

int GLxml::byteIndex()
{
   if ( isValid() )
      return ::XML_GetCurrentByteIndex( _p );
   return -1;
}

int GLxml::nRead()
{
   return _nRead;
}

int GLxml::nElem()
{
   if ( _root != (GLxmlElem *)0 )
      return _root->nElem();
   return 0;
}

int GLxml::nAttr()
{
   if ( _root != (GLxmlElem *)0 )
      return _root->nAttr();
   return 0;
}

GLxmlElem *GLxml::find( char *pLkup, bool bRecurse )
{
   if ( _root )
      return _root->find( pLkup, bRecurse );
   return (GLxmlElem *)0;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
GLxmlElem *GLxml::addElement( char *pTag, char *pParent, bool bRecurse )
{
   GLxmlElem *par, *rtn;

   // 1) Find it

   par = (GLxmlElem *)0;
   if ( pParent )
      par = find( pParent, bRecurse );
   if ( !par )
      par = _root;

   // 2) Build it; Return

   if ( par )
      rtn = par->addElement( pTag );
   else
      rtn = new GLxmlElem( par, pTag );
   if ( !_root )
      _root = rtn;
   return rtn;
}

string GLxml::build()
{
   string rtn;
   char   *bp, *cp;
   int     i, len, nLeft;

   // 1) Build it

   bp    = new char[XML_MAX_SIZ];
   cp    = bp;
   nLeft = XML_MAX_SIZ;
   for ( i=0; i<3 && _HasRoom( nLeft ); i++ ) {
      switch( i ) {
         case 0: len = _root->header( cp,  nLeft, 0, _bSpaces ); break;
         case 1: len = _root->build( cp,   nLeft, 0, _bSpaces ); break;
         case 2: len = _root->trailer( cp, nLeft, 0, _bSpaces ); break;
      }
      nLeft -= len;
      cp    += len;
   }
   *cp = '\0';

   // 3) Return string

   rtn = bp;
   delete[] bp;
   return rtn;
}

int GLxml::parse( const char *s, int len, int isFinal )
{
   // Pre-condition

   if ( !isValid() )
      return -1;

   // Parse at will

   if ( ::XML_Parse( _p, s, len, isFinal ) == 0 ) {
      _nRead = ::XML_GetCurrentByteIndex( _p );
      return nRead();
   }
   return -1;
}

void GLxml::reset( bool bCreate )
{
   // 1) Our guts

   if ( _root )
      delete _root;
   _root    = (GLxmlElem *)0;
   _curElem = (GLxmlElem *)0;
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
   ::XML_SetElementHandler( _p,
                            &GLxml::xmlStart,
                            &GLxml::xmlEnd );
   ::XML_SetCharacterDataHandler( _p,
                                  &GLxml::xmlData );
}


/////////////////////////////////////////
// Class-Wide Callbacks
/////////////////////////////////////////
void GLxml::xmlStart( void *arg, const char *name, const char **atts )
{
   GLxml     *glx = (GLxml *)arg;
   GLxmlElem *elem;
   int        i;
   char      *pName = (char *)name;

   // 1) Add Element

   if ( !glx->_root ) {
      glx->_root = new GLxmlElem( (GLxmlElem *)0, pName );
      elem       = glx->_root;
   }
   else
      elem = glx->_curElem->addElement( pName );
   glx->_curElem = elem;

   // 2) Fill in attributes

   for ( i=0; atts[i]; i+=2 )
      elem->addAttr( (char *)atts[i], (char *)atts[i+1] );
}

void GLxml::xmlData( void *arg, const char *pData, int len )
{
   string  tmp( pData, len );
   GLxml  *glx = (GLxml *)arg;
   string &rwc = glx->_curElem->stdVal();

   rwc += tmp;
}

void GLxml::xmlEnd( void *arg, const char *name )
{
   GLxml     *glx = (GLxml *)arg;
   GLxmlElem *par = glx->_curElem->parent();

   // 1) Bring 'er back down

   glx->_curElem = par;
   glx->_bDone   = ( glx->_curElem == (GLxmlElem *)0 );
}


/////////////////////////////////////////
// Class-Wide Callbacks
/////////////////////////////////////////
char *GLxml::xTranslate( char *pOut, char *pInp )
{
   xTranslate( pOut, pInp, strlen( pInp ) );
   return pOut;
}

int GLxml::xTranslate( char *pOut, char *pInp, int len, bool bNws )
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
         {
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
   }
   *op = '\0';
   return( op-pOut );
}



////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      G L x m l N e s t
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLxmlNest::GLxmlNest( int nDepth, bool bSpaces ) :
   _pSpaces( (char *)0 ),
   _nDepth( nDepth ),
   _bSpaces( bSpaces )
{
   int len;

   // Set spaces

   len = _nDepth*3;
   if ( bSpaces && ( _nDepth > 0 ) ) {
      _pSpaces = new char[len+2];
      ::memset( _pSpaces, ' ', len );
      _pSpaces[len] = '\0';
   }
}

GLxmlNest::~GLxmlNest()
{
   if ( _pSpaces != (char *)0 )
      delete[] _pSpaces;
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
char *GLxmlNest::spaces()
{
   return ( _pSpaces != (char *)0 ) ? _pSpaces : (char *)"";
}

int GLxmlNest::nDepth()
{
   return _nDepth;
}
