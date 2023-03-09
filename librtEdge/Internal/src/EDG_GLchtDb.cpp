/******************************************************************************
*
*  EDG_GLchtDb.cpp
*     ChartDB shared memory (file) layout and reader
*
*  REVISION HISTORY:
*     20 OCT 2010 jcs  Created (OVERWRITE from REAL ChartDB!!!).
*     12 NOV 2014 jcs  Build 28: -Wall
*     20 MAR 2016 jcs  Build 32: Linux compatibility in libmddWire
*     10 SEP 2020 jcs  Build 44: MDDResult
*      9 MAR 2023 jcs  Build 62: GLchtDbItem._idx; u_int64_t _fileSiz
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;

static int _hSz  = sizeof( GLchtDbHdr );

/////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L c h t D b
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLchtDb::GLchtDb( char *pn, const char *pAdmin ) :
   GLmmap( pn, (char *)0, 0, _hSz ),
   _admin( pAdmin ? pAdmin : "localhost:8775"  ),
   _recs(),
   _name( pn ),
   _freeIdx( -1 ),
   _mtx(),
   _bFullCopy( false )
{
   u_int64_t fSz;

   // Remap to file size / Load

   if ( !isValid() )
      return;
   fSz  = db()._fileSiz;
   map( 0, fSz );
   if ( ::strcmp( CDB_SIG_002, db()._signature ) )
      unmap();
   else
      Load();
}

GLchtDb::~GLchtDb()
{
   _recs.clear();
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
GLchtDbHdr &GLchtDb::db()
{
   GLchtDbHdr *rtn;

   rtn = (GLchtDbHdr *)data();
   return *rtn;
}

RecMap &GLchtDb::recs()
{
   return _recs;
}

MDDResult GLchtDb::Query()
{
   Locker           lck( _mtx );
   RecMap          &v = recs();
   RecMap::iterator it;
   MDDResult        q;
   MDDRecDef       *rr, rd;
   GLchtDbItem     *rec;
   char            *bp, *rp;
   int              i, nr, off;

   ::memset( &q, 0, sizeof( q ) );
   bp = data();
   nr = v.size();
   rr = new MDDRecDef[nr];
   for ( i=0,it=v.begin(); it!=v.end(); i++,it++ ) {
      off = (*it).second;
      if ( !off )
         break; // for-i
      rp  = bp + off;
      rec = (GLchtDbItem *)rp;
      rd._pSvc     = rec->_svc;
      rd._pTkr     = rec->_tkr;
      rd._fid      = rec->_fid;
      rd._interval = rec->_interval;
      rr[i]        = rd;
   }
   q._recs = rr;
   q._nRec = i;
   return q;
}

CDBData GLchtDb::GetItem( const char *pSvc, 
                          const char *pTkr,
                          int         fid )
{
   Locker           lck( _mtx );
   RecMap::iterator it;
   CDBData          d;
   GLchtDbItem     *rec;
   char            *bp, *rp;
   struct timeval   tv, tNow;
   string           s = MapKey( pSvc, pTkr, fid );
   float           *dp;
   int              i, nf, off;

   // 1) Initialize return shit

   ::memset( &d, 0, sizeof( d ) );
   d._pSvc = pSvc;
   d._pTkr = pTkr;
   d._pErr = "Item Not Found";

   // 2) Find
#ifdef OBSOLETE_CHECK_THIS_OUT
   Load();
#endif // OBSOLETE_CHECK_THIS_OUT
   if ( (it=_recs.find( s )) == _recs.end() )
      return d;
   bp  = data();
   off = (*it).second;
   rp  = bp + off;
   rec = (GLchtDbItem *)rp;
   rp += sizeof( GLchtDbItem );
   dp  = (float *)rp;

   // 3) Fill in item stats

   nf          = rec->_nTck;
   d._pTkr     = pTkr;
   d._pSvc     = rec->_svc;
   d._pTkr     = rec->_tkr;
   d._pErr     = "";
   d._fid      = rec->_fid;
   d._interval = rec->_interval;
   d._curTick  = rec->_curTck;
   d._numTick  = nf;
   d._tCreate  = rec->_tCreate;
   d._tUpd     = rec->_tUpd;
   d._tUpdUs   = rec->_tUpdUs;
   tv.tv_sec   = d._tUpd;
   tv.tv_usec  = d._tUpdUs;
   tNow        = Logger::tvNow();
   d._dAge     = Logger::Time2dbl( tNow ) - Logger::Time2dbl( tv );
   d._nUpd     = rec->_nUpd;
   d._tDead    = rec->_tDead;
   d._flds     = new float[nf];
   for ( i=0; i<d._curTick; i++ )
      d._flds[i] = dp[i];
   return d;
}

void GLchtDb::AddTicker( const char *pSvc, const char *pTkr, int fid )
{
   const char *ph  = _admin.c_str();
   const char *sep = "|";
   string      toks( pTkr );
   Socket      s( ph );
   char       *cp, *rp;
   char        buf[K];

   // Pipe-delimited string ...

   cp = (char *)toks.c_str(); 
   s.Connect();
   for ( cp=::strtok_r( cp,sep,&rp ); cp; cp=::strtok_r( NULL,sep,&rp ) ) {
      sprintf( buf, "<ADD %s=\"%s\" %s=\"%s\" %s=\"%d\"/>\n", 
         _mdd_pAttrSvc, pSvc, 
         _mdd_pAttrName, cp,
         _mdd_pAttrTag, fid );
      s.Write( buf, strlen( buf ) );
   }
   SLEEP( 0.25 ); // 250 mS to flush the socket
}

void GLchtDb::DelTicker( const char *pSvc, const char *pTkr, int fid )
{
   const char *ph  = _admin.c_str();
   const char *sep = "|";
   string      toks( pTkr );
   Socket      s( ph );
   char       *cp, *rp;
   char        buf[K];

   // Pipe-delimited string ...

   ph  = _admin.c_str();
   sep = "|";
   cp  = (char *)toks.c_str();
   s.Connect();
   for ( cp=::strtok_r( cp,sep,&rp ); cp; cp=::strtok_r( NULL,sep,&rp ) ) {
      sprintf( buf, "<DEL %s=\"%s\" %s=\"%s\" %s=\"%d\"/>\n",
         _mdd_pAttrSvc, pSvc, 
         _mdd_pAttrName, cp,
         _mdd_pAttrTag, fid );
      s.Write( buf, strlen( buf ) );
   }
   SLEEP( 0.25 ); // 250 mS to flush the socket
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
void GLchtDb::Load()
{
   Locker           lck( _mtx );
   RecMap::iterator it;
   GLchtDbItem     *rec;
   string           s;
   char            *bp, *pn;
   u_int64_t        off;

   // Pre-condition

   if ( db()._freeIdx == _freeIdx )
      return;

   // 1) Blow away existing

   _recs.clear();
   _freeIdx = db()._freeIdx;
   bp = map( 0, _freeIdx );

   // 2) Walk thru GLchtDbItem's, storing offsets

   off = _hSz;
   while( off<_freeIdx ) {
      rec = (GLchtDbItem *)( bp+off );
      s   = MapKey( rec->_svc, rec->_tkr, rec->_fid );
      pn  = (char *)s.c_str();
      _recs[s] = off;
      off += rec->_siz;
   }
}

string GLchtDb::MapKey( const char *pSvc, const char *pTkr, int fid )
{
   string      s;
   char        buf[K];
   const char *sep = CDB_SVCSEP;

   // Svc|Tkr|FID

   sprintf( buf, "%s%s%s%s%d", pSvc, sep, pTkr, sep, fid );
   s   = buf;
   return s;
}
