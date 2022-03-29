/****************************************************************************** *
*  Data.cpp
*     MD-Direct data base class
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     12 OCT 2013 jcs  Build  3: mddIoctl_fixedXxxx
*     12 NOV 2014 jcs  Build  8: -Wall
*     17 JAN 2015 jcs  Build  9: _XML_Parse() : h = b._hdr if non-zero
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*     29 MAR 2022 jcs  Build 13: mddIoctl_unpacked
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>
#include <GLedgDTD.h>

using namespace MDDWIRE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      D a t a
//
/////////////////////////////////////////////////////////////////////////////

Logger      *Data::_log = (Logger *)0;
mddBuf       Data::_bz;
mddFieldList Data::_fz;
Str2IntMap   Data::_mons;
char        *Data::_pMons[] = { "JAN", "FEB", "MAR", "APR",
                                "MAY", "JUN", "JUL", "AUG",
                                "SEP", "OCT", "NOV", "DEC",
                                (char *)0 };

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Data::Data( bool bPub ) :
   _bPub( bPub ), 
   _proto( mddProto_XML ), 
   _schema( (Schema *)0 ),
   _bOurSchema( true ),
   _xml(),
   _xName(),
   _bPackFlds( true ),
   _bParseInC( true ),
   _bNativeFld( false ),
   _hLib( (HINSTANCE)0 ),
   _fcn( (mddDriverFcn)0 )
{
   int    i;
   bool   bBld;
   string s;

   // Once

   bBld = ( _mons.size() == 0 );
   if ( bBld ) {
      for ( i=0; _pMons[i]; i++ ) {
         s        = _pMons[i];
         _mons[s] = i+1;
      }
      ::memset( &_bz, 0, sizeof( _bz ) );
      ::memset( &_fz, 0, sizeof( _fz ) );
   }

   // Instance

   ::memset( &_mf, 0, sizeof( _mf ) );
}

Data::~Data()
{
   if ( _hLib )
      ::FreeLibrary( _hLib );
   if ( _mf._data )
      delete[] _mf._data;
   if ( _schema && _bOurSchema )
      delete _schema;
}


////////////////////////////////////////////
// API
////////////////////////////////////////////
mddProtocol Data::proto()
{
   return _proto;
}

mddBuf Data::Ping()
{
   mddBuf      r;
   mddBinHdr   bh;
   mddProtocol pro;
   Binary      bin( _bPackFlds );
   int         bSz;
   u_char     *bp;

   // XML protocol if Subscribe

   pro     = _bPub ? _proto : mddProto_XML;
   r._data = 0;
   r._dLen = 0;
   switch( pro ) {
      case mddProto_Undef:
         break;
      case mddProto_XML:
         r._data = (char *)"<Ping/>\n";
         r._dLen = strlen( r._data );
         break;
      case mddProto_MF:
         r._data = _InitBuf( _mf, K );
         sprintf( r._data, "%c%d%c%s%c", FS, MT_PING, GS, "All OK", FS );
         r._dLen = strlen( r._data );
         break;
      case mddProto_Binary:
         r._data      = _InitBuf( _mf, K );
         bp           = (u_char *)r._data;
         ::memset( &bh, 0, sizeof( bh ) );
         bh._dt       = mddDt_Control;
         bh._mt       = mddMt_ping;
         bh._protocol = mddProto_Binary;
         bh._tag      = -1; 
         bh._RTL      = -1;
         bSz          = bin.Set( bp, bh );
         bh._len      = bSz;
         bin.Set( bp, bh, true );
         r._dLen      = bSz;
         break;
   }
   return r;
}

void Data::Ioctl( mddIoctl ctl, void *arg )
{
   bool  bArg;
   char *pCfg;

   bArg = ( arg != (void *)0 );
   switch( ctl ) {
      case mddIoctl_unpacked:    _bPackFlds   = !bArg; break;
      case mddIoctl_nativeField: _bNativeFld  = bArg;  break;
      case mddIoctl_fixedLibrary:
         if ( (pCfg=(char *)arg) )
            LoadFixedLibrary( pCfg );
         break; 
   }
}

void Data::SetProtocol( mddProtocol pro )
{
   _proto = pro;
}

int Data::SetSchema( const char *ps )
{
   if ( _schema && _bOurSchema )
      delete _schema;
   _schema     = new Schema( ps );
   _bOurSchema = true;
   return _schema->Size();
}

int Data::CopySchema( Schema *schema )
{
   if ( _schema && _bOurSchema )
      delete _schema;
   _schema     = schema;
   _bOurSchema = false;
   return _schema->Size();
}

Schema *Data::schema()
{
   return _schema;
}

mddFieldList Data::GetSchema()
{
   mddFieldList rtn;

   ::memset( &rtn, 0, sizeof( rtn ) );
   if ( _schema )
      rtn = _schema->Get();
   return rtn;
}

mddField *Data::GetDef( int fid )
{
   mddFldDef *def;

   def = _GetDef( fid );
   return def ? &def->_mdd : (mddField *)0;
}  
   
mddField *Data::GetDef( const char *pFld )
{  
   mddFldDef *def;

   def = _GetDef( pFld );
   return def ? &def->_mdd : (mddField *)0;
}


////////////////////////////////////////////
// XML
////////////////////////////////////////////
int Data::_XML_Parse( mddMsgBuf b, mddWireMsg &r )
{
   mddMsgHdr h;
   mddBuf   &svc = r._svc;
   mddBuf   &tkr = r._tkr;
   mddBuf   &err = r._err;
   mddBuf   &raw = r._rawData;
   int      nb;

   // 1) Parse

   if ( !b._hdr && !_XML_ParseHdr( b, h ) )
      return 0;
   h  = b._hdr ? *b._hdr : h;
   nb = _xml.byteIndex();

   // 2) mddWireMsg

   GLxmlElem &x   = *_xml.root();

   r._proto  = mddProto_XML;
   r._state  = mdd_up;
   svc._data = (char *)x.getAttr( _mdd_pAttrSvc );
   svc._dLen = strlen( svc._data );
   tkr._data = (char *)x.getAttr( _mdd_pAttrName );
   tkr._dLen = strlen( tkr._data );
   err._data = (char *)x.getAttr( _mdd_pError );
   err._dLen = strlen( err._data );
   r._tag    = h._iTag;
   r._mt     = h._mt;
   r._dt     = h._dt;
   raw._data = b._data;
   raw._dLen = nb;

   /*
    * 3) All Elements -> FieldList
    *       ASSUME : No support for FieldName="YES"
    */
   mddField       f, *def;
   mddBuf        &s   = f._val._buf;
   mddFieldList &fl   = r._flds;
   mddField     *flds = fl._flds;
assert( flds );
   GLvecXmlElem  &edb = x.elements();
   const char    *pFid, *pVal;
   int            i;

   _InitFieldList( fl, edb.size() );
assert( flds );
   fl._nFld = edb.size();
   for ( i=0; i<fl._nFld; i++ ) {
      pFid    = edb[i]->name();
      pFid   += 1;  // _3
      pVal    = edb[i]->getAttr( _mdd_pVal );
      f._fid  = atoi( pFid );
      f._name = (def=GetDef( f._fid )) ? def->_val._buf._data : "Undefined";
      f._type = mddFld_string;
assert( flds );
      s._data = (char *)pVal;
      s._dLen = strlen( pVal );
assert( flds );
      flds[i] = f;
   }
   return nb;
}

int Data::_XML_ParseHdr( mddMsgBuf b, mddMsgHdr &h )
{
   mddBuf &nm = h._svc;
   int     i, nb;

   // Pre-condition

   if ( b._hdr ) {
      ::memcpy( &h, b._hdr, sizeof( h ) );
      return h._len;
   }

   /*
    * Parsing XML header == Parsing full XML msg
    */

   // 1) XML parse (once per msg)

   _xml.reset();
   _xml.parse( b._data, b._dLen );
   if ( !_xml.isComplete() )
      return 0;
   nb = _xml.byteIndex();

   // 2) Fill in header

   GLxmlElem  &x    = *_xml.root();
   const char *pTy  = x.name();
   const char *pTkr = x.getAttr( _mdd_pAttrName );
   const char *pTag = x.getAttr( _mdd_pAttrTag );
   const char *pRTL = x.getAttr( _mdd_pAttrRTL );
   const char *pTm  = x.getAttr( _mdd_pAttrTime );

   _xName = pTkr;
   if ( !strcmp( pTy, _mdd_pXmlMsg[xm_update] ) )
      h._mt= mddMt_update;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_image] ) )
      h._mt= mddMt_image;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_status] ) )
      h._mt= mddMt_dead;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_mount] ) ) 
      h._mt= mddMt_mount;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_open] ) )
      h._mt= mddMt_open;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_close] ) )
      h._mt= mddMt_close;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_ioctl] ) )
      h._mt= mddMt_ctl;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_ping] ) )
      h._mt = mddMt_ping;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_insert] ) )
      h._mt = mddMt_insert;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_insAck] ) )
      h._mt = mddMt_insAck;
   else if ( !strcmp( pTy, _mdd_pXmlMsg[xm_query] ) )
      h._mt = mddMt_query;
   h._len    = nb;
   h._dt     = mddDt_FieldList;
   h._iTag   = atoi( pTag );
   h._RTL    = atoi( pRTL );
   h._time   = atof( pTm );
   nm._data  = (char *)_xName.c_str();
   nm._dLen  = _xName.size();
   h._hdrLen = 0;

   // 3) All Attributes

   mddField       f;
   mddBuf        &s   = f._val._buf;
   mddFieldList  &fl  = h._attrs;
   mddField     *flds = fl._flds;
   GLvecKeyValue &adb = x.attributes();

   fl._nFld = adb.size();
   for ( i=0; i<fl._nFld; i++ ) {
      f._name = adb[i]->key();
      f._type = mddFld_string;
      s._data = (char *)adb[i]->value();
      s._dLen = strlen( s._data );
      flds[i] = f;
   }
   return nb;
}



////////////////////////////////////////////
// Utilities
////////////////////////////////////////////
mddFldDef *Data::_GetDef( int fid )
{
   mddFldDef *def;

   def = _schema ? _schema->GetDef( fid ) : (mddFldDef *)0;
   return def;
}

mddFldDef *Data::_GetDef( const char *pFld )
{
   mddFldDef *def;

   def = _schema ? _schema->GetDef( pFld ) : (mddFldDef *)0;
   return def;
}

int Data::_strcat( char *cp, const char *str )
{
   return _strcat( cp, (char *)str );
}

int Data::_strcat( char *cp, char *str )
{
   int len;

   len = str ? strlen( str ) : 0;
   if ( len ) {
      ::memcpy( cp, str, len );
      cp += len;
      *cp = '\0';
   }
   return len;
}

int Data::_int2str( char *bp, int num )
{
   char *cp;
   char  ch[K];  // ASSUME : < 10^^1024
   int   i, rem, n, _base;

   // Base 10

   _base = 10;
   cp    = bp;
   if ( num < 0 ) {
      *cp++ = '-';
      num   = -num;
   }
   for ( n=0; num; n++ ) {
      rem   = num % 10;
      ch[n] = rem + '0';
      num  /= 10;
   }
   for ( i=n-1; i>=0; *cp++ = ch[i], i-- );
   return( cp-bp );
}

int Data::_bufcpy( char *bp, mddBuf b )
{
   if ( b._dLen )
      ::memcpy( bp, b._data, b._dLen );
   return b._dLen;
}

char *Data::_TrimTrailingZero( char *str )
{
   char *cp, *np;
   char  trimch;

   // Cut trailing trimch's

   trimch = '0';
   cp     = str;
   np     = str + strlen( str ) - 1;
   while( *np == trimch ) {
      *np = '\0';
      np--;
   }
   if ( *np == '.' ) {
      np   += 1;
      *np++ = '0';
      *np   = '\0';
   }
   return cp;
}

mddBuf Data::_SetBuf( char *data, int len )
{
   mddBuf b;

   b._data = data;
   b._dLen = ( data && !len ) ? strlen( data ) : len;
   return b;
}

char *Data::_InitBuf( mddBldBuf &buf, int estSz )
{
   if ( (u_int)estSz > buf._nAlloc ) {
      ::mddBldBuf_Free( buf );
      buf = ::mddBldBuf_Alloc( estSz );
   }
   return buf._data;
}

void Data::_InitFieldList( mddFieldList &fl, int estFlds )
{
   int nf;

   if ( estFlds > fl._nAlloc ) {
      ::mddFieldList_Free( fl );
      nf       = fl._nFld;
      fl       = ::mddFieldList_Alloc( estFlds );
      fl._nFld = nf;
   }
}

void Data::_InitMsg( mddWireMsg &r, mddMsgHdr &h )
{  
   mddBuf       &svc = r._svc; 
   mddBuf       &tkr = r._tkr;
   mddBuf       &err = r._err;
   mddBuf       &raw = r._rawData;
   mddFieldList &fl  = r._flds; 
   
   r._proto  = _proto;
   r._state  = mdd_up;
   svc._dLen = 0;
   tkr._dLen = 0;
   err._dLen = 0;
   r._tag    = h._iTag;
   r._mt     = h._mt;
   r._dt     = h._dt;
   fl._nFld  = 0;
   raw._dLen = 0;
}  

void Data::_InitMsg( mddWireMsg &r, mddBinHdr &bh )
{
   mddBuf       &svc = r._svc;
   mddBuf       &tkr = r._tkr;
   mddBuf       &err = r._err;
   mddBuf       &raw = r._rawData;
   mddFieldList &fl  = r._flds;

   r._proto  = mddProto_Binary;
   r._state  = mdd_up;
   svc._dLen = 0;
   tkr._dLen = 0;
   err._dLen = 0;
   r._tag    = bh._tag;
   r._mt     = bh._mt;
   r._dt     = bh._dt;
   fl._nFld  = 0;
   raw._dLen = bh._len;
}

void Data::_InitMsgHdr( mddMsgHdr  &h, 
                        int         len, 
                        mddMsgType  mt, 
                        mddDataType dt )
{
   mddBuf       &svc = h._svc;
   mddBuf       &tkr = h._tkr;
   mddFieldList &fl  = h._attrs;

   h._len    = len;
   h._mt     = mt;
   h._dt     = dt;
   h._tag[0] = '\0';
   h._iTag   = -1;
   h._RTL    = -1;
   h._time   = 0.0;
   svc._dLen = 0;
   tkr._dLen = 0;
   fl._nFld  = 0;
}

char *Data::_memrchr( char *buf, int ch, size_t cnt )
{
   char  *cp;
   size_t i;

   cp = buf + cnt;
   for ( i=1; i<=cnt; cp--,i++ ) {
      if ( *cp == (char)ch )
         return cp;
   }
   return NULL;
}


////////////////////////////////////////////
// Loadable Fixed-to-FieldList
////////////////////////////////////////////
void Data::LoadFixedLibrary( char *pLib )
{
   if ( _hLib )
      ::FreeLibrary( _hLib );
   _hLib = (HINSTANCE)0;
   _fcn  = (mddDriverFcn)0;
   _hLib = (HINSTANCE)::LoadLibrary( pLib );
   if ( _hLib ) {
      _fcn = (char (*)(mddBuf *, mddFieldList *))
               ::GetProcAddress( _hLib, _pDriverFcn );
   }
}



/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      m d d W i r e
//
/////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
mddWire::mddWire( mddWire_Context cxt, bool bSub ) :
   _cxt( cxt ),
   _sub( bSub ? new Subscribe() : (Subscribe *)0 ),
   _pub( bSub ? (Publish *)0 : new Publish() )
{
}

mddWire::~mddWire()
{
   if ( _pub )
      delete _pub;
   if ( _sub )
      delete _sub;
}


////////////////////////////////////////////
// API Operations
////////////////////////////////////////////
int mddWire::ParseHdr( mddMsgBuf b, mddMsgHdr &h )
{
   if ( _sub )
      return _sub->ParseHdr( b, h );
   else if ( _pub )
      return _pub->ParseHdr( b, h );
   return 0;
}

mddBuf mddWire::Ping()
{
   mddBuf b;

   b = Data::_bz;
   if ( _sub )
      b = _sub->Ping();
   else if ( _pub )
      b = _pub->Ping();
   return b;
}

void mddWire::Ioctl( mddIoctl ctl, void *arg )
{
   if ( _sub )
      _sub->Ioctl( ctl, arg );
   else if ( _pub )
      _pub->Ioctl( ctl, arg );
}

void mddWire::SetProtocol( mddProtocol pro )
{
   if ( _sub )
      _sub->SetProtocol( pro );
   if ( _pub )
      _pub->SetProtocol( pro );
}

mddProtocol mddWire::GetProtocol()
{
   if ( _sub )
      return _sub->proto();
   if ( _pub )
      return _pub->proto();
   return mddProto_Undef;
}

int mddWire::SetSchema( const char *ps )
{
   if ( _sub )
      return _sub->SetSchema( ps );
   if ( _pub )
      return _pub->SetSchema( ps );
   return 0;
}

int mddWire::CopySchema( Schema *sch )
{
   if ( _sub )
      return _sub->CopySchema( sch );
   if ( _pub )
      return _pub->CopySchema( sch );
   return 0;
}

Schema *mddWire::schema()
{
   if ( _sub )
      return _sub->schema();
   if ( _pub )
      return _pub->schema();
   return (Schema *)0;
}

mddFieldList mddWire::GetSchema()
{
   if ( _sub )
      return _sub->GetSchema();
   if ( _pub )
      return _pub->GetSchema();
   return Data::_fz;
}
         
mddField *mddWire::GetFldDef( int fid )
{
   mddField *rtn;

   rtn = (mddField *)0;
   if ( _sub )
      rtn = _sub->GetDef( fid );
   else if ( _pub )
      rtn = _pub->GetDef( fid );
   return rtn;
}
 
mddField *mddWire::GetFldDef( const char *pFld )
{
   mddField *rtn;

   rtn = (mddField *)0;
   if ( _sub )
      rtn = _sub->GetDef( pFld );
   else if ( _pub )
      rtn = _pub->GetDef( pFld );
   return rtn;
}

