/***************************************************************************** 
*
*  EDG_GLlvcDb.cpp
*     LVC shared memory (file) layout and reader
*
*  REVISION HISTORY:
*      2 SEP 2010 jcs  Created.
*     18 SEP 2010 jcs  Build  7: Admin channel
*      5 OCT 2010 jcs  Build  8b:_bV2
*     30 DEC 2010 jcs  Build  9: _bV3; 64-bit; SetFilter
*     13 JAN 2011 jcs  Build 10: GetItem( bShallow )
*     16 SEP 2011 jcs  Build 16: AddTicker() : Pipe-delimited
*     20 JAN 2012 jcs  Build 17: LVC_SIG_004; GLlvcFldDef._name / _type
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*      7 MAY 2013 jcs  Build 25: rtBUF
*     12 NOV 2014 jcs  Build 28: _lock; LVCDef; -Wall
*      6 FEB 2016 jcs  Build 32: Linux compatibility in libmddWire; _bBinary
*     12 SEP 2017 jcs  Build 35: hash_map; No mo XxxTicker()
*     20 JAN 2018 jcs  Build 39: mtx()
*     17 MAY 2022 jcs  Build 54: GLlvcDbItem._bActive; rtFld_unixTime
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;

static int _hSz  = sizeof( GLlvcDbHdr );
static int _h3Sz = sizeof( GLlvcDbHdr3 );
static int _fSz  = sizeof( GLlvcFldDef );

/////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L l v c D b
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLlvcDb::GLlvcDb( LVCDef &ld, bool bLock, DWORD waitMillis ) :
   GLmmap( ld.pFile(), (char *)0, 0, _hSz ),
   _def( ld ),
   _fidOffs(),
   _fidFltr(),
   _schemaByFid(),
   _schema(),
   _recs(),
   _name( ld.pFile() ),
   _freeIdx( -1 ),
   _mtx(),
   _bFullCopy( ld._bFullCopy ),
   _lock( (Semaphore *)0 ),
   _locked( true ),
   _bBinary( false ),
   _cockpits()
{
   int         i, off, fSz, nFld, fid, fLen;
   GLlvcFldDef def;
   FidMap     &odb = _fidOffs;
   char       *sig;

   // Lock

   if ( bLock ) {
      _lock = new Semaphore( ld.pFile() );
      if ( !(_locked=_lock->Lock( waitMillis )) )
         return;
   }
   
   // Remap to file size

   if ( !isValid() )
      return;
   nFld = db()._nFlds;
   fSz  = db()._fileSiz;
   map( 0, fSz );
   if ( !isValid() )
      return;
   sig = db()._signature;
   if ( ::strcmp( LVC_SIG_004, sig ) ) {
      _bBinary = ( ::strcmp( LVC_SIG_005, sig ) == 0 );
      if ( !_bBinary ) {
         unmap();
         return;
      }
   }

   // Build fid map

   SchemaByFid &sdb = _schemaByFid;

   for ( i=0,off=0; i<nFld; i++ ) {
      def      = fdb()[i];
      fid      = def._fid;
      fLen     = def._len;
      odb[fid] = off;
      off     += fLen;
      sdb[fid] = def;
      _schema.push_back( def );
   }

   // Load Records

   Load();
}

GLlvcDb::~GLlvcDb()
{
   Cockpits &v = _cockpits;

   for ( size_t i=0; i<v.size(); v[i++]->DetachLVC( true ) );
   v.clear();
   _fidOffs.clear();
   _fidFltr.clear();
   _recs.clear();
   GLmmap::Shutdown();
   if ( _lock ) {
      if ( _locked )
         _lock->Unlock();
      delete _lock;
   }
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
GLlvcDbHdr &GLlvcDb::db()
{
   GLlvcDbHdr *rtn;

   rtn = (GLlvcDbHdr *)data();
   return *rtn;
}

GLlvcDbHdr3 &GLlvcDb::db3()
{
   GLlvcDbHdr3 *rtn;

   rtn = (GLlvcDbHdr3 *)data();
   return *rtn;
}

GLlvcFldDef *GLlvcDb::fdb()
{
   GLlvcFldDef *rtn;
   char        *bp;

   bp  = data();
   bp += _h3Sz;
   rtn = (GLlvcFldDef *)bp;
   return rtn;
}

RecMap &GLlvcDb::recs()
{
   return _recs;
}

bool GLlvcDb::IsLocked()
{
   return _locked;
}

int GLlvcDb::FieldOffset( int fid )
{
   FidMap          &odb = _fidOffs;
   FidMap::iterator it;
   int              rtn;

   rtn = 0;
   if ( (it=odb.find( fid )) != odb.end() )
      rtn = (*it).second;
   return rtn;
}

bool GLlvcDb::CanAddField( int fid )
{
   FidMap          &odb = _fidFltr;
   FidMap::iterator it;
   bool             bRtn;

   bRtn  = ( odb.size() == 0 );
   bRtn |= ( (it=odb.find( fid )) != odb.end() );
   return bRtn;
}

LVCData GLlvcDb::GetItem( const char *pSvc, 
                          const char *pTkr,
                          Bool        bShallow )
{
   Locker           lck( _mtx );
   RecMap::iterator it;
   LVCData          d;
   GLlvcDbItem     *rec;
   GLlvcFldDef      def;
   GLlvcFldHdr     *h;
   char            *bp, *rp, *dp, *fp, *copy;
   Bool             bOK;
   struct timeval   tv, tNow;
   string           s = MapKey( pSvc, pTkr );
   double           dn;
   int              i, n, iSz, nf, idx, fOff, off;
   LVCint          *ip;

   // 1) Initialize return shit

   ::memset( &d, 0, sizeof( d ) );
   d._pSvc     = pSvc;
   d._pTkr     = pTkr;
   d._bShallow = bShallow;

   // 2) Find

   Load();
   if ( (it=_recs.find( s )) == _recs.end() ) {
      d._ty   = edg_dead;
      d._pErr = "Item Not Found";
      return d;
   }
   bp  = data();
   off = (*it).second;
   rp  = bp + off;
   rec = (GLlvcDbItem *)rp;

   // 3) Fill in item stats

   if ( !bShallow && _bFullCopy ) {
      d._copy = new char[rec->_siz];
      ::memcpy( d._copy, rp, rec->_siz );
   }
   d._pSvc    = rec->_svc;
   d._pTkr    = rec->_tkr;
   d._bActive = rec->_bActive;
   d._tCreate = rec->_tCreate;
   d._tUpd    = rec->_tUpd;
   d._tUpdUs  = rec->_tUpdUs;
   tv.tv_sec  = d._tUpd;
   tv.tv_usec = d._tUpdUs;
   tNow       = Logger::tvNow();
   dn         = dNow();
   dn         = Logger::Time2dbl( tNow );
   d._dAge    = dn - Logger::Time2dbl( tv );
   d._tDead   = rec->_tDead;
   d._nUpd    = rec->_nUpd;
   bOK        = ( rec->_tDead < rec->_tUpd );
   d._ty      = bOK ? edg_image : edg_stale;
   if ( !(nf=rec->_nFld) )
      return d;
   d._flds = new rtFIELD[nf];

   // 3) Safe to walk 'em through 

   dp  = rp + sizeof( GLlvcDbItem );
   ip  = (LVCint *)dp;
   iSz = sizeof( GLlvcDbItem ) + ( nf * sizeof( LVCint ) );
   dp  = rp + iSz;
   for ( i=0,n=0,fOff=0; i<nf; i++ ) {
      fp   = dp + fOff;
      h    = (GLlvcFldHdr *)fp;
      fp  += _uSz();
      idx  = ip[i];
      def  = _schema[idx];
      copy = d._copy + ( fp-dp );
      if ( CanAddField( def._fid ) )
         d._flds[n++] = GetField( *h, fp, def._fid, bShallow, copy );
      fOff += def._len;
   }
   d._nFld = n;
   return d;
}

int GLlvcDb::SetFilter( const char *pFids )
{
   string           s( pFids );
   FidMap          &odb = _fidFltr;
   FidMap::iterator it;
   char            *bp, *cp, *rp;
   int              fid;
   const char      *_csv  = ",";

   // Clear out; Walk 'em all adding once

   odb.clear();
   bp = (char *)s.data();
   for ( cp=::strtok_r( bp,_csv,&rp ); cp; cp=::strtok_r( NULL,_csv,&rp ) ) {
      fid = atoi( cp );
      if ( fid && ( odb.find( fid ) == odb.end() ) )
         odb[fid] = fid;
   }
   return odb.size();
}

Bool GLlvcDb::IsBinary()
{
   return _bBinary;
}

Mutex &GLlvcDb::mtx()
{
   return _mtx;
}

int GLlvcDb::_uSz()
{
   return _bBinary ? sizeof( GLlvcFldHdr ) : sizeof( GLlvcFldHdr1 );
}


////////////////////////////////////////////
// Cockpit Operations
////////////////////////////////////////////
void GLlvcDb::Attach( Cockpit *c )
{
   _cockpits.push_back( c );
}

void GLlvcDb::Detach( Cockpit *c )
{
   Cockpits &v = _cockpits;

   for ( size_t i=0; i<v.size(); i++ ) {
      if ( v[i] == c ) {
         v.erase( v.begin()+i );
         return;
      }
   }
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
void GLlvcDb::Load()
{
   Locker           lck( _mtx );
   RecMap::iterator it;
   GLlvcDbItem     *rec;
   string           s;
   char            *bp, *pn;
   int              off;

   // Pre-condition

   if ( db()._freeIdx == _freeIdx )
      return;

   // 1) Blow away existing

   _recs.clear();
   _freeIdx = db()._freeIdx;
   bp = map( 0, _freeIdx );
   if ( !isValid() )
      return;

   // 2) Walk thru GLlvcDbItem's, storing offsets

   off  = _h3Sz;
   off += ( _fSz * db()._nFlds );
   while( off<_freeIdx ) {
      rec = (GLlvcDbItem *)( bp+off );
      s   = MapKey( rec->_svc, rec->_tkr );
      pn  = (char *)s.data();
      _recs[s] = off;
      off += rec->_siz;
   }
}

rtFIELD GLlvcDb::GetField( GLlvcFldHdr &h, 
                           char        *fp, 
                           int          fid, 
                           Bool         bShallow,
                           char        *copy )
{
   SchemaByFid          &sdb = _schemaByFid;
   SchemaByFid::iterator it;
   GLlvcFldDef           def;
   rtFIELD               f;
   rtVALUE              &v = f._val;
   rtBUF                &b = v._buf;
   Bool                  bStr;

   // 1) Field Contents

   f._fid  = fid;
   f._type = _bBinary ? (rtFldType)h._type : rtFld_string;
   bStr    = !_bBinary;
   if ( _bBinary ) {
      switch( f._type ) {
         case mddFld_undef:
         case mddFld_real:
         case mddFld_bytestream:
            // Not supported
            break;
         case mddFld_string:
            bStr = True;
            break;
         case mddFld_int32:
            ::memcpy( &v._i32, fp, sizeof( v._i32 ) );
            break;
         case mddFld_double:
         case mddFld_date:
         case mddFld_time:
         case mddFld_timeSec:
            ::memcpy( &v._r64, fp, sizeof( v._r64 ) );
            break;
         case mddFld_float:
            ::memcpy( &v._r32, fp, sizeof( v._r32 ) );
            break;
         case mddFld_int8:
            ::memcpy( &v._i8, fp, sizeof( v._i8 ) );
            break;
         case mddFld_int16:
            ::memcpy( &v._i16, fp, sizeof( v._i16 ) );
            break;
         case mddFld_int64:
         case mddFld_unixTime:
            ::memcpy( &v._i64, fp, sizeof( v._i64 ) );
            break;
      }
   }
   if ( bStr ) {
      b._dLen = h._len;
      if ( bShallow )
         b._data = fp;
      else {
         if ( _bFullCopy )
            b._data = copy;
         else {
            b._data = new char[b._dLen+4];
            ::memset( b._data, 0, b._dLen+4 );
            ::memcpy( b._data, fp, b._dLen );
         }
      }
   }

   // 2) Field Name

   f._name = "Undefined";
   if ( (it=sdb.find( f._fid )) != sdb.end() ) {
      def     = (*it).second;
      f._name = (*it).second._name;
   }
   return f;
}

void GLlvcDb::SetFieldAttr_OBSOLETE( rtFIELD &f )
{
   SchemaByFid          &sdb = _schemaByFid;
   SchemaByFid::iterator it;
   GLlvcFldDef           def;

   f._name = "Undefined";
   f._type = rtFld_undef;
   if ( (it=sdb.find( f._fid )) != sdb.end() ) {
      def     = (*it).second;
//      f._name = def._name;
      f._name = (*it).second._name;
      f._type = (rtFldType)def._type;
   }
}  

string GLlvcDb::MapKey( const char *pSvc, const char *pTkr )
{
   string s;

   s   = pSvc;
   s  += LVC_SVCSEP;
   s  += pTkr;
   return s;
}



/////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      L V C D e f
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
LVCDef::LVCDef( const char *file, 
                bool        bLock, 
                DWORD       waitMillis ) :
   _file( file ),
   _lvc( (GLlvcDb *)0 ),
   _bFullCopy( false )
{
   _lvc = new GLlvcDb( *this, bLock, waitMillis );
}

LVCDef::~LVCDef()
{
   if ( _lvc )
      delete _lvc;
}


////////////////////////////////////////////
// Access / Operations
////////////////////////////////////////////
GLlvcDb &LVCDef::lvc()
{
   return *_lvc;
}

char *LVCDef::pFile()
{
   return (char *)_file.data();
}

int LVCDef::SetFilter( const char *pFlds )
{
   // TODO
   return 0;
}

void LVCDef::SetCopyType( bool bFullCopy )
{
   _bFullCopy = bFullCopy;
}
