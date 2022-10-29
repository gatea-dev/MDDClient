/******************************************************************************
*
*  Schema.cpp
*     MD-Direct schema
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*      4 JUN 2014 jcs  Build  7: mddFld_real / mddFld_bytestream
*     12 NOV 2014 jcs  Build  8: -Wall
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*     19 MAY 2022 jcs  Build 14: mddFldDef._i32 = _maxLen
*     29 OCT 2022 jcs  Build 16: hash_map; No mo _ddb
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>

using namespace MDDWIRE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      S c h e m a
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Schema::Schema( const char *pDef ) :
   _minFid( INFINITEs ),
   _maxFid( 0 ),
   _gfifId(),
   _gfifStr()
{
   vector<string> v;
   const char    *pTok, *pn;
   mddFldDef     *def;
   char          *rp;
   string         s;
   size_t         i;
   int            fid;

   // DEF1|DEF2|...

   pTok = ::strtok_r( (char *)pDef, "|", &rp );
   for ( i=0; pTok; i++ ) {
      s    = pTok;
      v.push_back( s );
      pTok = ::strtok_r( NULL, "|", &rp );
   }
   for ( i=0; i<v.size(); i++ ) {
      pTok = v[i].c_str();
      def  = new mddFldDef( pTok );
      pn   = def->pName();
      fid  = def->_fid;
      s    = def->_name;
      if ( !GetDef( pn ) && !GetDef( fid ) ) {
         _gfifId[fid] = def;
         _gfifStr[s]  = def;
         _minFid      = gmin( _minFid, fid );
         _maxFid      = gmax( _maxFid, fid );
      }
      else
         delete def;
   }

   // Array / FieldList

   FldDefByIdMap           &vdb = _gfifId;
   FldDefByIdMap::iterator  it;

   ::memset( &_fl, 0, sizeof( _fl ) );
   _fl._nFld = Size();
   _fl._flds = new mddField[Size()];
   for ( i=0,it=vdb.begin(); it!=vdb.end(); i++,it++ ) {
      fid          = (*it).first;
      def          = (*it).second;
      _fl._flds[i] = def->_mdd;
   }
}

Schema::~Schema()
{
   FldDefByIdMap          &v = _gfifId;
   FldDefByIdMap::iterator ft;

   for ( ft=v.begin(); ft!=v.end(); delete (*ft).second,ft++ );
   _gfifId.clear();
   _gfifStr.clear();
   if ( _fl._flds )
      delete[] _fl._flds; 
}



////////////////////////////////////////////
// Access
////////////////////////////////////////////
int Schema::Size()
{
   return _gfifId.size();
}

mddFieldList Schema::Get()
{
   return _fl;
}

mddFldDef *Schema::GetDef( int fid )
{
   FldDefByIdMap          &v = _gfifId;
   FldDefByIdMap::iterator it;
   mddFldDef              *def;

   // Quickest : Array; Next quickest : map

   def = (mddFldDef *)0;
   if ( (it=v.find( fid )) != v.end() )
      def = (*it).second;
   return def;
}

mddFldDef *Schema::GetDef( const char *pFld )
{
   FldDefByNameMap          &v = _gfifStr;
   FldDefByNameMap::iterator it;
   string                    s( pFld );
   mddFldDef                *def;

   def = (mddFldDef *)0;
   if ( (it=v.find( s )) != v.end() )
      def = (*it).second;
   return def;
}




/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      m d d F l d D e f
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
mddFldDef::mddFldDef( const char *pDef ) :
   _name( "Undefined" ),
   _fid( -1 ),
   _type( "Undefined" ),
   _fType( mddFld_undef ),
   _maxLen( 0 )
{
   const char *pn, *pt;
   char       *rp;

   // PROD_CATG 1 INTEGER 5

   ::memset( &_mdd, 0, sizeof( _mdd ) );
   if ( !(pn=::strtok_r( (char *)pDef, " ", &rp )) )
      return;
   _name = pn;
   if ( !(pn=::strtok_r( NULL, " ", &rp )) )
      return;
   _fid = atoi( pn );
   if ( !(pn=::strtok_r( NULL, " ", &rp )) )
      return;
   _type = pn;
   pt    = _type.c_str();
   if ( !(pn=::strtok_r( NULL, " ", &rp )) )
      return;
   _maxLen = atoi( pn );

   // Set type

   if ( !::strcmp( pt, "ALPHANUMERIC" ) )
      _fType = mddFld_string;
   else if ( !::strcmp( pt, "ALPHANUM_XTND" ) )
      _fType = mddFld_string;
   else if ( !::strcmp( pt, "BINARY" ) )
      _fType = mddFld_string;
   else if ( !::strcmp( pt, "DATE" ) )
      _fType = mddFld_date;
   else if ( !::strcmp( pt, "ENUMERATED" ) )
      _fType = mddFld_int32;
   else if ( !::strcmp( pt, "INTEGER" ) )
      _fType = mddFld_int32;
   else if ( !::strcmp( pt, "NUMERIC" ) )
      _fType = mddFld_double;
   else if ( !::strcmp( pt, "PRICE" ) )
      _fType = mddFld_double;
   else if ( !::strcmp( pt, "TIME" ) )
      _fType = mddFld_time;
   else if ( !::strcmp( pt, "TIME_SECONDS" ) )
      _fType = mddFld_timeSec;
   else if ( !::strcmp( pt, "REAL" ) )
      _fType = mddFld_real;
   else if ( !::strcmp( pt, "BYTESTREAM" ) )
      _fType = mddFld_bytestream;

   // mddField

   mddValue &v = _mdd._val;

   _mdd._fid  = _fid;
   _mdd._name = pName();
   _mdd._type = fType();
   v._i32     = _maxLen;
}

mddFldDef::~mddFldDef()
{
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
const char *mddFldDef::pName()
{
   return _name.c_str();
}

const char *mddFldDef::pType()
{
   return _type.c_str();
}

mddFldType mddFldDef::fType()
{
   return _fType;
}

