/******************************************************************************
*
*  TapeChannel.cpp
*     MDDirect subscription channel : Tape
*
*  REVISION HISTORY:
*      1 SEP 2022 jcs  Created (from EdgChannel)
*     23 SEP 2022 jcs  GetField()
*     10 OCT 2022 jcs  Multiple tickers
*     14 OCT 2022 jcs  PumpOneMsg( ..., bool &bContinue )
*     12 JAN 2024 jcs  Build 67: TapeHeader.h
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      T a p e C h a n n e l
//
/////////////////////////////////////////////////////////////////////////////

static const char *t_ALL  = "*";
static const char *t_SEP  = "|";
static const char *_undef = "Undefined";
static const char *_pIdx  = ".idx";
static int _mSz8          = sizeof( GLrecTapeMsg );
static int _mSz4          = _mSz8 - 4;
static int _ixSz          = sizeof( u_int64_t );
static struct timeval _zT = { 0, 0 };
static struct timeval _eT = { INFINITEs, 0 };

static rtBUF     _zBUF      = { (char *)0, 0 };
static u_int64_t _iSz       = sizeof( u_int64_t );

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
TapeChannel::TapeChannel( EdgChannel &chan ) :
   _chan( chan ),
   _attr( chan.attr() ),
   _idxFile(),
   _schema(),
   _schemaByName(),
   _vwHdr( (GLmmap *)0 ),
   _hdr( (TapeHeader *)0 ),
   _idx( (GLrpyDailyIdxVw *)0 ),
   _rdb(),
   _tdb(),
   _wl(),
   _dead(),
   _mdd( ::mddSub_Initialize() ),
   _fl( ::mddFieldList_Alloc( K ) ),
   _upds(),
   _err(),
   _nSub( 0 ),
   _sliceMtx(),
   _slice( (TapeSlice *)0 ),
   _bRun( false ),
   _bInUse( false )
{
   string tmp( pTape() );
   char  *pi;

   // 1) Binary Protocol Always

   ::mddWire_SetProtocol( _mdd, mddProto_Binary );

   /*
    * 2) Supported index files for ULTRA.trep_20180304.1 :
    *    ULTRA.trep.20180304.1.idx
    *    ULTRA.trep.idx_20180304.1
    */
   if ( (pi=(char *)::strstr( tmp.data(), "_" )) ) {
      string tmp1( tmp.data(), pi-tmp.data() );
      string tmp2( pi );

      tmp      = tmp1;
      tmp     += _pIdx;
      tmp     += tmp2;
      _idxFile = tmp;
   }
   tmp += _pIdx;
   if ( !_idxFile.length() )
      _idxFile = tmp;
}

TapeChannel::~TapeChannel()
{
   DeadTickers::iterator it;
   int                   i;

   for ( i=0; _bInUse; i++ );
   _bRun = false;
   for ( it=_dead.begin(); it!=_dead.end(); delete (*it).second );
   _ClearFieldMap();
   _wl.clear();
   _dead.clear();
   Unload();
   ::mddFieldList_Free( _fl );
   ::mddSub_Destroy( _mdd );
   for ( size_t ii=0; ii<_tdb.size(); delete _tdb[ii], ii++ );
   _tdb.clear();

   Locker lck( _sliceMtx );

   if ( _slice )
      delete _slice;
}


////////////////////////////////////
// Access
////////////////////////////////////
EdgChannel &TapeChannel::edg()
{
   return _chan;
}

TapeHeader &TapeChannel::hdr()
{
   return *_hdr;
}

mddWire_Context TapeChannel::mdd()
{
   return _mdd;
}

const char *TapeChannel::pTape()
{
   return _attr._pSvrHosts;
}

const char *TapeChannel::pIdxFile()
{
   return _idxFile.data();
}

const char *TapeChannel::err()
{
   return _err.length() ? _err.data() : (const char *)0;
}

bool TapeChannel::HasTicker( const char *svc, 
                             const char *tkr, 
                             int        &idx )
{
   string                s;
   TapeRecords::iterator it;

   s   = _Key( svc, tkr );
   idx = -1;
   if ( (it=_rdb.find( s )) != _rdb.end() ) {
      idx = (*it).second;
      return true;
   }
   return false;
}

int TapeChannel::GetFieldID( const char *fldName )
{
   FieldMapByName          &ndb = _schemaByName;
   FieldMapByName::iterator it;
   string                   s( fldName );
   int                      fid;

   it  = ndb.find( s );
   fid = ( it != ndb.end() ) ? (*it).second._fid : 0;
   return fid;
}

rtFIELD *TapeChannel::GetField( int fid )
{
   FieldMap          &udb = _upds;
   FieldMap::iterator it;
   rtFIELD           *f;

   // Parse (once), then by fid

   if ( !udb.size() )
      _BuildFieldMap();
   it = udb.find( fid );
   f  = ( it != udb.end() ) ? &((*it).second) : (rtFIELD *)0;
   return f;
}

MDDResult TapeChannel::Query()
{
   TapeRecDb &v = _tdb;
   MDDResult  q;
   MDDRecDef *rr;
   size_t     i, nr;

   ::memset( &q, 0, sizeof( q ) );
   nr = v.size();
   rr = new MDDRecDef[nr];
   for ( i=0; i<nr; i++ ) {
      rr[i]._pSvc     = v[i]->_svc();
      rr[i]._pTkr     = v[i]->_tkr();
      rr[i]._fid      = 0;
      rr[i]._interval = v[i]->_nMsg();
   }
   q._recs = rr;
   q._nRec = (int)i;
   return q;
}


////////////////////////////////////
// PumpTape
////////////////////////////////////
int TapeChannel::StartPumpFullTape( u_int64_t off0, int nMsg )
{
   int    rc;
   {
      Locker lck( _sliceMtx );

      // Pre-condition(s)

      if ( _slice || _bRun || _bInUse )
         return 0;

      // OK to continue

      _slice = new TapeSlice( *this, off0, nMsg );
      rc     = _slice->_ID;
      _bRun  = true;
   }
   /*
    * Wait for slice to kick off
    */
//   for ( ; !_bInUse; ::rtEdge_Sleep( 0.025 ) );
   return rc;
}

int TapeChannel::StopPumpFullTape( int id )
{
   // Pre-condition
   {
      Locker lck( _sliceMtx );

      if ( !_slice || ( _slice->_ID != id ) )
         return 0;
      _bRun = false;
   }
   // 2) Wait for slice to end

   for ( ; _bInUse; ::rtEdge_Sleep( 0.100 ) );

   Locker lck( _sliceMtx );

   delete _slice;
   _slice = (TapeSlice *)0;
   return id;
}


////////////////////////////////////
// Operations
////////////////////////////////////
int TapeChannel::Subscribe( const char *svc, const char *tkr )
{
   int ix;

   // Special Case : Run baby

   if ( !::strcmp( svc, pTape() ) ) {
      Locker lck( _sliceMtx );

      if ( ::strcmp( tkr, t_ALL ) )
         _slice = new TapeSlice( *this, tkr );
      _bRun = true;
      return 0;
   }

   // Regular Subscription

   _nSub += 1;
   if ( HasTicker( svc, tkr, ix ) ) 
      _wl[ix] = ix;
   else  {
      ix        = hdr()._maxRec() + _nSub;
      _dead[ix] = new string( _Key( svc, tkr ) );
   }
   return ix;
}

int TapeChannel::Unsubscribe( const char *svc, const char *tkr )
{
   TapeWatchList::iterator it;
   int                     ix;
   
   // Special Case : Run baby
   
   if ( !::strcmp( svc, pTape() ) ) {
      _bRun = false;
      return 0;    
   }

   // Dead Tickers are removed when we 1st pump

   if ( HasTicker( svc, tkr, ix ) ) {
      if ( (it=_wl.find( ix )) != _wl.end() )
         _wl.erase( it );
      return ix;
   }
   return 0;

}

bool TapeChannel::Load()
{
   TapeRecHdr *rec;
   string      s;
   char       *bp, *cp, *pi;
   int         i, nr, rSz;

   // Pre-condition(s)

   if ( _vwHdr )
      return true;
   _vwHdr = new GLmmap( (char *)pTape(),  (char *)0, 0, sizeof( GLrecTapeHdr ) );
   if ( !_vwHdr->isValid() ) {
      _err  = "Can not map file ";
      _err += pTape();
      Unload();
      return false;
   }
   if ( !_LoadHdr() )
      return false;
   /*
    * Map the whole thing in : TODO - Allow for real-time pumping??
    */
   bp  = _vwHdr->map( 0, hdr()._fileSiz() );
   if ( !_LoadHdr() )
      return false;
   /*
    * Header
    */
   nr  = hdr()._numRec();
   rSz = hdr()._RecSiz();
   if ( !hdr()._bMDDirect() ) {
      Unload();
      _err  = "Invalid tape file ";
      _err += pTape();
      return false;
   }
   /*
    * Daily Index File
    */
   if ( !_idx ) {
      pi   = (char *)_idxFile.data();
      _idx = new GLrpyDailyIdxVw( hdr(), pi );
      if ( !_idx->isValid() ) {
         delete _idx;   
         _idx = (GLrpyDailyIdxVw *)0;
      }
   }
   /*
    * Schema (Field Dictionary)
    */
   cp  = bp;
   cp += _LoadSchema();
   cp += hdr()._numSecIdxT() * _ixSz;  // Offset Indices
   /*
    * Records Dictionary
    */
   for ( i=0; i<nr; i++,cp+=rSz ) {
      rec     = _GetRecHdr( i );
      s       = _Key( rec->_svc(), rec->_tkr() );
      _rdb[s] = i;
      _tdb.push_back( rec );
   }
   return true;
}

int TapeChannel::Pump()
{
   TapeWatchList::iterator it;
   GLrecTapeMsg           *msg;
   struct timeval          t0;
   char                   *bp, *cp;
   mddBuf                  m;
   u_int64_t               off;
   size_t                  nt;
   bool                    bPmp;
   int                     n, mSz, rc;

   // Pre-condition(s)

   if ( !_hdr || !_bRun )
      return 0;

   // Specific ticker(s)??

   TapeRun run( *this );

   if ( (nt=_wl.size()) ) {
      it = _wl.begin();
      for ( rc=0; it!=_wl.end(); rc += PumpTicker( (*it).first ), it++ );
      return rc;
   }
   /*
    * PumpTpe( off, nMsg )??
    */
   {
      Locker lck( _sliceMtx );

      if ( _slice && !_slice->_bByTime )
         return _PumpSlice( _slice->_off0, _slice->_NumMsg );
   }
   /*
    * All Tickers??
    */
   bp   = _vwHdr->data();
   off  = hdr()._hdrSiz();
   t0   = _slice ? _slice->_t0 : _zT;
   msg  = (GLrecTapeMsg *)0;
   bPmp = true;
   _PumpDead();
   if ( t0.tv_sec )
      off = _tapeOffset( t0 );
   for ( n=0; !_nSub && bPmp && _bRun && off<_vwHdr->siz(); ) {
      cp      = bp+off;
      msg     = (GLrecTapeMsg *)cp;
      mSz     = msg->_bLast4 ? _mSz4 : _mSz8;
      m._data = cp + mSz;
      m._dLen = msg->_msgLen - mSz;
      n      += _PumpOneMsg( *msg, m, false, bPmp );
      off    += msg->_msgLen;
   }
   _PumpComplete( msg, off );  

   // Return number pumped

   return n;
}

int TapeChannel::PumpTicker( int ix )
{
   TapeRecHdr   *rec;
   GLrecTapeMsg *msg;
   char         *bp, *cp, sts[K];
   mddBuf        m;
   Offsets       odb;
   u_int64_t     off, diff, nMsg;
   size_t        i, j, nr;
   bool          bPmp, notUsed;
   int           n, mSz, pct;
   static int    _pct[] = { 10, 25, 50, 75 };

   // Pre-condition(s)

   if ( !_hdr )
      return 0;

   // One Ticker

   rec  = _tdb[ix];
   off  = rec->_loc();
   nMsg = rec->_nMsg();
   msg  = (GLrecTapeMsg *)0;
   /*
    * Remap if too big
    */
   if ( off > _vwHdr->siz() ) {
      Unload();
      Load();
      rec = _tdb[ix];
   }
   _PumpDead();
   bPmp = true;
   bp   = _vwHdr->data();
   for ( i=0,n=0; bPmp && _bRun && off; i++ ) {
      cp      = bp+off;
      msg     = (GLrecTapeMsg *)cp;
      mSz     = msg->_bLast4 ? _mSz4 : _mSz8;
      m._data = cp + mSz;
      m._dLen = msg->_msgLen - mSz;
      if ( !_chan._bTapeDir ) {
         if ( !i ) {
#if (_MSC_VER >= 1910)
            sprintf( sts, "Reversing about %lld msgs", nMsg );
#else
            sprintf( sts, "Reversing about %ld msgs", nMsg );
#endif // (_MSC_VER >= 1910)
            _PumpStatus( msg, sts );
         }
         else {
            for ( j=0; j<4; j++ ) {
               pct = 100 - _pct[j];
               if ( i == ( nMsg / pct ) ) {
                  sprintf( sts, "%d%% msgs reversed", _pct[j] );
                  _PumpStatus( msg, sts );
               }
            }
         }
         if ( _InTimeRange( *msg ) )
            odb.push_back( off );
      }
      else
         n   += _PumpOneMsg( *msg, m, true, bPmp );
      if ( msg->_bLast4 )
         diff = (u_int64_t)_get32( msg->_last );
      else
         diff = _get64( msg->_last );
      off -= diff;
   }
   /*
    * Reverse??
    */
   nr = odb.size();
   for ( i=0; _bRun && i<nr; i++ ) {
      off     = odb[nr-1-i];
      cp      = bp+off;
      msg     = (GLrecTapeMsg *)cp;
      mSz     = msg->_bLast4 ? _mSz4 : _mSz8;
      m._data = cp + mSz;
      m._dLen = msg->_msgLen - mSz;
      n      += _PumpOneMsg( *msg, m, true, notUsed );
   }
   _PumpStatus( msg, "Stream Complete", edg_streamDone );

   // Return number pumped

   return n;
}

void TapeChannel::Stop()
{
   _bRun = false;
}

void TapeChannel::Unload()
{
   _rdb.clear();
   _tdb.clear();
   if ( _hdr )
      delete _hdr;
   if ( _vwHdr )
      delete _vwHdr;
   if ( _idx )
      delete _idx;
   _vwHdr = (GLmmap *)0;
   _hdr   = (TapeHeader *)0;
   _idx   = (GLrpyDailyIdxVw *)0;
}


////////////////////////////////////
// Helpers
////////////////////////////////////
bool TapeChannel::_LoadHdr()
{
   if ( _hdr ) 
      delete _hdr;
   _hdr = new TapeHeader( *_vwHdr );
   if ( !hdr().Map() ) {
      delete _hdr;      
      _hdr  = (TapeHeader *)0;
      _err  = "Can not map header ";
      _err += pTape();
      Unload();
   }
   return _hdr ? true : false;
}    

TapeRecHdr *TapeChannel::_GetRecHdr( int dbIdx )
{
   TapeHeader &h = hdr();
   TapeRecHdr *rec;
   char       *bp, *cp;
   u_int64_t   rSz, hSz;

   // Already Built?

   if ( ( dbIdx < (int)_tdb.size() ) && (rec=_tdb[dbIdx]) )
      return rec;

   // Calcs

   hSz = h._DbHdrSize( h._numDictEntry(), h._numSecIdxT(), 0 );
   rSz = h._RecSiz();

   // No range boundary protection

   bp  = _vwHdr->data();
   cp  = bp;
   cp += hSz;
   cp += ( rSz * dbIdx );
   rec = new TapeRecHdr( h, cp );
   return rec;
}

bool TapeChannel::_InTimeRange( GLrecTapeMsg &m )
{
   return _slice ? _slice->InTimeRange( m ) : true;
}

bool TapeChannel::_IsWatched( GLrecTapeMsg &m )
{
   int    ix;
   size_t sz;

   ix = (int)m._dbIdx;
   sz = _wl.size();
   return !sz ? true : ( _wl.find( ix ) != _wl.end() );
}

int TapeChannel::_LoadSchema()
{
   FieldMap       &sdb = _schema;
   FieldMapByName &ndb = _schemaByName;
   TapeHeader     &h   = hdr();
   GLrecDictEntry *de;
   rtEdgeData      d;
   char           *bp, *cp;
   rtFIELD        *fdb, f;
   rtBUF          &b = f._val._buf;
   int             i, nd, dSz;
   string          s;

   // Pre-condition(s)

   if ( !_hdr || !_vwHdr->siz() )
      return 0;
   /*
    * Schema
    */
   bp  = _vwHdr->data();
   cp  = bp + h._DbHdrSize( 0, 0, 0 );
   nd  = h._numDictEntry();
   dSz = nd * sizeof( GLrecDictEntry );
   ::memset( &d, 0, sizeof( d ) );
   de  = (GLrecDictEntry *)cp;
   fdb = new rtFIELD[nd];
   for ( i=0; i<nd; i++ ) {
      f._fid  = de[i]._fid;
      b._data = (char  *)de[i]._acronym;
      b._dLen = strlen( b._data );
      f._name = b._data;
      f._type = (rtFldType)de[i]._type;
      fdb[i]  = f;
      /*
       * Schema Lookup - Persistent
       */
      s = f._name;
      if ( sdb.find( f._fid ) == sdb.end() )
         sdb[f._fid] = f;
      if ( ndb.find( s ) == ndb.end() )
         ndb[s] = f;
   }
   d._flds = fdb;
   d._nFld = nd;
   d._nFld = nd;
   if ( _attr._schemaCbk )
      (*_attr._schemaCbk)( _chan.cxt(), d );
   delete[] fdb;
   cp += dSz;
   return( cp-bp );
}

bool TapeChannel::_ParseFieldList( mddBuf raw )
{
   FieldMap          &sdb = _schema;
   FieldMap::iterator it;
   mddMsgBuf          inp  = { raw._data, raw._dLen, (mddMsgHdr *)0 };
   mddMsgHdr          h;
   mddWireMsg         w;
   mddField           f;
   int                i, nb, nh, nf;

   _fl._nFld = 0;
   w._dt     = mddDt_FieldList;
   w._flds   = _fl;
   w._svc    = _zBUF;
   w._tkr    = _zBUF;
   ::memset( &h, 0, sizeof( h ) );

   // Parse : Header 1st, then full enchilada

   nh  = ::mddSub_ParseHdr( _mdd, inp, &h );
   nb  = ::mddSub_ParseMsg( _mdd, inp, &w );
   _fl = w._flds;
   nf  = _fl._nFld;
   for ( i=0; i<nf; i++ ) {
      f       = _fl._flds[i];
      f._name = _undef;
      if ( (it=sdb.find( f._fid )) != sdb.end() )
         f._name = (*it).second._name;
      _fl._flds[i] = f;
   }
   return( nf > 0 );
}

int TapeChannel::_PumpDead()
{
   DeadTickers::iterator it;
   rtEdgeData            d;
   const char           *s;
   char                 *rp;
   int                   n;
   struct timeval        tv;

   // Msg Time

   tv.tv_sec  = hdr()._tCreate();
   tv.tv_usec = 0;

   // Fill in rtEdgeData and dispatch

   ::memset( &d, 0, sizeof( d ) );
   for ( n=0,it=_dead.begin(); it!=_dead.end(); n++,it++ ) {
      s           = (*it).second->data();
      d._tMsg     = Logger::Time2dbl( tv );
      d._pSvc     = ::strtok_r( (char *)s, t_SEP, &rp );
      d._pTkr     = ::strtok_r( NULL, t_SEP, &rp );
      d._pErr     = "non-existent item";
      d._ty       = edg_dead;
      d._StreamID = (*it).first;
      d._RTL      = 0;
      if ( _attr._dataCbk )
         (*_attr._dataCbk)( _chan.cxt(), d );
      delete (*it).second;
   }
   _dead.clear();
   return n;
}

void TapeChannel::_PumpStatus( GLrecTapeMsg *msg, 
                               const char   *sts,
                               rtEdgeType    ty,
                               u_int64_t     off )
{
   TapeRecHdr *rec;
   rtEdgeData  d;
   int         ix;

   // Fill in rtEdgeData and dispatch

   ::memset( &d, 0, sizeof( d ) );
   ix          = msg ? msg->_dbIdx : 0;
   rec         = _tdb[ix];
   d._tMsg     = Logger::Time2dbl( Logger::tvNow() );
   d._pSvc     = rec->_svc();
   d._pTkr     = rec->_tkr();
   d._pErr     = sts;
   d._ty       = ty;
   d._TapePos  = off;
   if ( _attr._dataCbk )
      (*_attr._dataCbk)( _chan.cxt(), d );
}

int TapeChannel::_PumpSlice( u_int64_t off0, int nMsg )
{
   TapeHeader    &h  = hdr();
   Sentinel       ss = h._sentinel();
   GLrecTapeMsg  *msg;
   char          *bp, *cp;
   bool           bPmp;
   mddBuf         m;
   u_int64_t      off;
   int            i, n, mSz;

   // 1) Quick offset check : Are 1st 5 msgs valid?

   bp = _vwHdr->data();
   if ( !off0 )
      off0 += h._hdrSiz();
   for ( i=0,off=off0; i<5 && off<_vwHdr->siz(); i++ ) {
      cp   = bp+off;
      msg  = (GLrecTapeMsg *)cp;
      mSz  = msg->_bLast4 ? _mSz4 : _mSz8;
      off += msg->_msgLen;
      /*
       * Requires valid a) dbIdx and b) Timestamp
       */
      if ( !InRange( 0, msg->_dbIdx, (u_int32_t)h._numRec() ) )
         return 0;
      if ( !InRange( ss._tStart, msg->_tv_sec, h._curTime().tv_sec ) )
         return 0;
   }

   // Pump up to nMsg ...

   off  = off0;
   bPmp = true;
   for ( i=0,n=0; bPmp && _bRun && off<_vwHdr->siz() && i<nMsg; i++ ) {
      cp      = bp+off;
      msg     = (GLrecTapeMsg *)cp;
      mSz     = msg->_bLast4 ? _mSz4 : _mSz8;
      m._data = cp + mSz;
      m._dLen = msg->_msgLen - mSz;
      n      += _PumpOneMsg( *msg, m, false, bPmp );
      off    += msg->_msgLen;
   }
   /*
    * ... plus streamDone w/ offset = **NEXT** msg 
    */
   off = ( off < _vwHdr->siz() ) ? off : 0;
   cp  = off ? bp + off : (char *)0;
   msg = (GLrecTapeMsg *)cp;
   _PumpComplete( msg, off );

   // Return number pumped

   return n;
}

int TapeChannel::_PumpOneMsg( GLrecTapeMsg &msg, mddBuf m, bool bRev, bool &bPmp )
{
   TapeRecHdr    *rec;
   rtEdgeData     d;
   struct timeval tv;
   int            ix, n;

   // Pre-condition(s)

   if ( !_InTimeRange( msg ) ) {
      if ( bRev )
         bPmp &= ( msg._tv_sec >= _slice->_t0.tv_sec );
      else
         bPmp &= ( msg._tv_sec <= _slice->_t1.tv_sec );
      return 0;
   }
   if ( !_IsWatched( msg ) )
      return 0;
   if ( !_ParseFieldList( m ) )
      return 0;

   // Msg Time

   tv.tv_sec  = msg._tv_sec;
   tv.tv_usec = msg._tv_usec;
   ::memset( &d, 0, sizeof( d ) );

   // Fill in rtEdgeData and dispatch

   ix          = msg._dbIdx;
   rec         = _tdb[ix];
   d._tMsg     = Logger::Time2dbl( tv );
   d._pSvc     = rec->_svc();
   d._pTkr     = rec->_tkr();
   d._pErr     = "OK";
   d._ty       = edg_update;
   d._flds     = (rtFIELD *)_fl._flds;
   d._nFld     = _fl._nFld;
   d._rawData  = m._data;
   d._rawLen   = m._dLen;
   d._StreamID = ix;
   d._TapePos  = (char *)&msg - _vwHdr->data();
   d._RTL      = msg._nUpd;
   if ( _slice ) {
      for ( n=0; _slice->CanPump( n, d ); n++ ) {
         if ( _attr._dataCbk )
            (*_attr._dataCbk)( _chan.cxt(), d );
         d._flds = (rtFIELD *)_fl._flds;
         d._nFld = _fl._nFld;
      }
      return n;
   }
   else {
      n = 1;
      if ( _attr._dataCbk )
         (*_attr._dataCbk)( _chan.cxt(), d );
   }
   return n;
}

void TapeChannel::_PumpComplete( GLrecTapeMsg *msg, u_int64_t off )
{
   const char *sts;

   // 1) Done

   sts = _bRun ? "Stream Complete" : "Stream terminated";
   _PumpStatus( msg, sts, edg_streamDone, off );

   // 2) Clean up

   _bRun   = false;
   _bInUse = false;
   if ( _idx )
      delete _idx;
   _idx = (GLrpyDailyIdxVw *)0;
}

string TapeChannel::_Key( const char *svc, const char *tkr )
{
   string s( svc );

   s += t_SEP;
   s += tkr;
   return string( s );
}

int TapeChannel::_get32( u_char *bp )
{
   int *i32 = (int *)bp;

   return *i32;
}

u_int64_t TapeChannel::_get64( u_char *bp )
{
   u_int64_t *i64 = (u_int64_t *)bp;

   return *i64;
}

u_int64_t TapeChannel::_tapeOffset( struct timeval tv )
{
   int        ix;
   char      *cp;
   u_int64_t *idb;

   // 1) Current Index

   ix = _SecIdx( tv, (GLrecTapeRec *)0 );

   // 2) tapeIdxDb() : TODO - GLrpyDailyIdxVw

   cp  = _vwHdr->data();
   cp += hdr()._DbHdrSize( hdr()._numDictEntry(), 0, 0 );
   idb = (u_int64_t *)cp;
   return idb[ix];
}

int TapeChannel::_SecIdx( struct timeval now, GLrecTapeRec *rec )
{
   TapeHeader    &h  = hdr();
   struct timeval tv;
   time_t         tt, tn;
   struct tm     *tm, lt;
   bool           bRec;
   int            rtn, cmp, curIdx, secPerIdx;

   // 1) Tape or Record?

   bRec      = ( rec != (GLrecTapeRec *)0 );
   curIdx    = bRec ? rec->_curIdx    : h._curIdx();
   secPerIdx = bRec ? h._secPerIdxR() : h._secPerIdxT();
   tv        = bRec ? rec->_curIdxTm  : h._curIdxTm();
   
   // 2) Same Interval?
   
   rtn = curIdx;
   tt  = tv.tv_sec;
   tn  = now.tv_sec;
   cmp = ( tn - tt );
   if ( rtn && ( cmp > 0 ) && ( cmp < secPerIdx ) ) // 1st time?
      return rtn;
   
   // 3) Else convert; Last resort = localtime
   
   if ( rtn )
      rtn += ( cmp / secPerIdx );
   else {
      tm   = ::localtime_r( &tn, &lt );
      rtn  = ( lt.tm_hour * 3600 ) + ( lt.tm_min * 60 ) + lt.tm_sec;
      rtn /= secPerIdx;
   }
   return rtn;
}

void TapeChannel::_BuildFieldMap()
{
   FieldMap &udb = _upds;
   rtFIELD  *fdb = (rtFIELD *)_fl._flds;
   rtFIELD   f;
   int       i, fid;

   // Once per update

   if ( udb.size() )
      return;

   // Go wild

   for ( i=0; i<_fl._nFld; i++ ) {
      f        = fdb[i];
      fid      = f._fid;
      udb[fid] = f;
   }
}

void TapeChannel::_ClearFieldMap()
{
   _upds.clear();
}



/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      T a p e S l i c e
//
/////////////////////////////////////////////////////////////////////////////

static long _NumSlice = 1;

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
TapeSlice::TapeSlice( TapeChannel &tape, const char *tkr ) :
   _tape( tape ),
   _ID( ATOMIC_INC( &_NumSlice ) ),
   _bByTime( true ),
   _td0( Logger::Time2dbl( _zT ) ),
   _td1( Logger::Time2dbl( _eT ) ),
   _tSnap( 0 ),
   _t0( _zT ),
   _t1( _eT ),
   _tInterval( 0 ),
   _fids(),
   _fidSet(),
   _off0( 0 ),
   _NumMsg( 0 ),
   _LVC()
{
   string         s( tkr );
   struct timeval tmp;
   char          *ps, *p0, *p1, *p2, *p3, *rp;
   int            fid;

   // tStart|tEnd|tInteral|FIDs

   ps   = (char *)s.data();
   p0   = ::strtok_r( ps,   t_SEP, &rp );
   p1   = ::strtok_r( NULL, t_SEP, &rp );
   p2   = p1 ? ::strtok_r( NULL, t_SEP, &rp ) : (char *)0;
   p3   = p2 ? ::strtok_r( NULL, t_SEP, &rp ) : (char *)0;
   /*
    * tStart, tEnd
    */
   _t0  = _str2tv( p0 );
   _t1  = p1 ? _str2tv( p1 ) : _t0;
   _td0 = Logger::Time2dbl( _t0 );
   _td1 = Logger::Time2dbl( _t1 );
   if ( _td1 < _td0 ) {
      tmp  = _t0;
      _t0  = _t1;
      _t1  = tmp;
      _td0 = Logger::Time2dbl( _t0 );
      _td1 = Logger::Time2dbl( _t1 );
   }
   if ( !p2 || !p3 )
      return;
   /*
    * tInterval, FIDs
    */
   FIDSet &fdb = _fidSet;
   string  ss( p3 );

   _tInterval = atoi( p2 );
   ps = (char *)ss.data();
   for ( p0=::strtok_r( ps, ",", &rp ); p0; p0=::strtok_r( NULL, ",", &rp ) ) {
      if ( !(fid=atoi( p0 )) )
         fid = _tape.GetFieldID( p0 );
      if ( fid && ( fdb.find( fid ) == fdb.end() ) ) {
         _fids.push_back( fid );
         fdb.insert( fid );
      }
   }
}

TapeSlice::TapeSlice( TapeChannel &tape, u_int64_t off0, int NumMsg ) :
   _tape( tape ),
   _ID( ATOMIC_INC( &_NumSlice ) ),
   _bByTime( false ),
   _td0( Logger::Time2dbl( _zT ) ),
   _td1( Logger::Time2dbl( _eT ) ),
   _tSnap( 0 ),
   _t0( _zT ),
   _t1( _eT ),
   _tInterval( 0 ),
   _fids(),
   _fidSet(),
   _off0( off0 ),
   _NumMsg( NumMsg ),
   _LVC()
{
}

TapeSlice::TapeSlice( const TapeSlice &c ) :
   _tape( c._tape ),
   _ID( c._ID ),
   _bByTime( c._bByTime ),
   _td0( c._td0 ),
   _td1( c._td1 ),
   _tSnap( c._tSnap ),
   _t0( c._t0 ),
   _t1( c._t1 ),
   _tInterval( c._tInterval ),
   _fids(),
   _fidSet(),
   _off0( c._off0 ),
   _NumMsg( c._NumMsg ),
   _LVC()
{
}


////////////////////////////////////
// Access
////////////////////////////////////
bool TapeSlice::IsSampled()
{
   return( ( _tInterval > 0 ) && ( _fids.size() > 0 ) );
}

bool TapeSlice::InTimeRange( GLrecTapeMsg &m )
{
   struct timeval tv = { m._tv_sec, m._tv_usec };
   double         dt;

   dt = Logger::Time2dbl( tv );
   return InRange( _td0, dt, _td1 );
}

bool TapeSlice::CanPump( int nDup, rtEdgeData &d )
{
   time_t tMsg;

   // Pre-condition(s)

   if ( !InRange( _td0, d._tMsg, _td1 ) )
      return false;
   if ( !IsSampled() )
      return( nDup == 0 );

   /*
    * 1) Cache
    * 2) Determine if next interval; If so, advance _tSnap and ...
    */
   _Cache( d );
   tMsg = (time_t)d._tMsg;
   if ( _tSnap ) {
      if ( ( _tSnap+_tInterval ) > tMsg )
         return false;
      _tSnap += _tInterval;
   }
   else
      _tSnap = tMsg - ( tMsg % _tInterval );
   /*
    * 3) ... Pump
    */
   FieldMap::iterator it;
   size_t             i, n;
   int                nf;

   d._flds = _flds;
   d._nFld = 0;
   n       = _fids.size();
   for ( i=0,nf=0; i<n; i++ ) {
      if ( (it=_LVC.find( _fids[i] )) != _LVC.end() )
         d._flds[nf++] = (*it).second;
   }
   d._tMsg = _tSnap;
   d._nFld = nf;
   return true;
}


////////////////////////////////////
// Helpers
////////////////////////////////////
struct timeval TapeSlice::_str2tv( char *hmsU )
{
   string         ss( hmsU ), hh;
   struct tm     *tm, lt;
   struct timeval tv;
   time_t         t;
   char          *ps, *ymd, *hms, *h, *m, *s, *u, *z, *r1, *r2;
   char           sH[8], sM[8], sS[8];
   bool           b8, b6;
   size_t         sz, mul;
   rtFIELD        f;
   rtBUF         &b = f._val._buf;
   const char    *_SEP1 = " ";
   const char    *_SEP2 = ".";
   const char    *_SEP3 = ":";

   /*
    * YYYYMMDD HH:MM:SS
    * <FileCreateTime> HH:MM:SS
    */
   ps  = (char *)ss.data();
   z  = (char *)"0";
   t  = _tape.hdr()._tCreate();
   tm = ::localtime_r( (time_t *)&t, &lt );
   ymd = ::strtok_r( ps,   _SEP1, &r1 );
   hms = ::strtok_r( NULL, _SEP1, &r1 );
   hh  = hms ? hms : hmsU;
   if ( hms ) {
      b8         = ( (sz=strlen( ymd )) == 8 );
      f._type    = rtFld_string;
      b._data    = ymd;
      b._dLen    = 4;
      lt.tm_year = ::rtEdge_atoi( f ) - 1900; 
      b._dLen   += b8 ? 0 : 1; // '-'
      b._data   += b._dLen;
      b._dLen    = 2;
      lt.tm_mon  = ::rtEdge_atoi( f ) - 1;
      b._dLen   += b8 ? 0 : 1; // '-'
      b._data   += b._dLen;
      lt.tm_mday = ::rtEdge_atoi( f );
   }
   ps  = (char *)hh.data();
   hms = ::strtok_r( ps,   _SEP2, &r2 );
   u   = ::strtok_r( NULL, _SEP2, &r2 );
   b6  = ( (sz=strlen( hms )) == 6 );
   if ( b6 ) {
      sH[2] = sM[2] = sS[2] = '\0';
      ::memcpy( (h=sH), hms+0, 2 );
      ::memcpy( (m=sM), hms+2, 2 );
      ::memcpy( (s=sS), hms+4, 2 );
   }
   else {
      h = ::strtok_r( hms,  _SEP3, &r2 );
      m = h ? ::strtok_r( NULL, _SEP3, &r2 ) : z;
      s = m ? ::strtok_r( NULL, _SEP3, &r2 ) : z;
   }
   sz  = u ? strlen( u ) : 0;
   mul = 1;
   switch( sz ) {
      case 0:
      case 1:  mul = 100000; break;
      case 2:  mul = 10000;  break;
      case 3:  mul = 1000;   break;
      case 4:  mul = 100;    break;
      case 5:  mul = 10;     break;
      default: mul = 1;      break;
   }
   lt.tm_hour = h ? atoi( h ) : 0;
   lt.tm_min  = m ? atoi( m ) : 0;
   lt.tm_sec  = s ? atoi( s ) : 0;
   tv.tv_sec  = ::mktime( &lt );
   tv.tv_usec = u ? atoi( u ) : 0;
   tv.tv_usec = WithinRange( 0, tv.tv_usec * mul, 999999 );
   return tv;
}

void TapeSlice::_Cache( rtEdgeData &d )
{
   FIDSet  &fdb = _fidSet;
   rtFIELD  f;
   int      i, fid;

   for ( i=0; i<d._nFld; i++ ) {
      f   = d._flds[i];
      fid = f._fid;
      if ( fdb.find( fid ) != fdb.end() )
         _LVC[fid] = f;
   }
}



/////////////////////////////////////////////////////////////////////////////
//
//            c l a s s       G L r p y D a i l y I d x V w
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLrpyDailyIdxVw::GLrpyDailyIdxVw( TapeHeader &hdr, char *pf ) :
   GLmmap( pf, (char *)0, 0, 0 ),
   _hdr( hdr ),
   _off( 0 ),
   _daySz( hdr._DailyIdxSize() ),
   _fileSz( 0 ),
   _ss( (Sentinel *)0 ),
   _tapeIdxDb( (u_int64_t *)0 )
{
   _fileSz = siz();
   _Set();
}

GLrpyDailyIdxVw::~GLrpyDailyIdxVw()
{
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
Sentinel &GLrpyDailyIdxVw::sentinel()
{
   return *_ss;
}

u_int64_t *GLrpyDailyIdxVw::tapeIdxDb()
{
   return _tapeIdxDb;
}

Bool GLrpyDailyIdxVw::forth()
{
   u_int64_t off, nL;
   Bool      bOK;

   // Map to next _daySz chunk; Handle End of File

   off = offset() + siz();
   nL  = _fileSz - off;
   bOK = ( nL > 0 );
   if ( bOK ) {
      map( off, _daySz );
      bOK = _Set();
   }
   return bOK;
}

Bool GLrpyDailyIdxVw::_Set()
{
   char *cp;

   // Pre-condition

   if ( !isValid() )
      return false;

   // Sentinel

   cp  = data();
   cp += _off;
   _ss = (Sentinel *)cp;
   cp += _hdr._sSz();

   // Tape Index

   _tapeIdxDb = (u_int64_t *)cp;
   cp        += ( _hdr._numSecIdxT() * _iSz );
   return isValid();
}
