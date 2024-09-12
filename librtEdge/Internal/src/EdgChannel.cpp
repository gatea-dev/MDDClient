/******************************************************************************
*
*  EdgChannel.cpp
*     MDDirect subscription channel : rtEdgeCache3
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*      . . .
*     12 NOV 2014 jcs  Build 28: libmddWire; Logger.CanLog(); _cxt; -Wall
*     21 JAN 2015 jcs  Build 29: rtEdgeData._StreamID; int Unsubscribe()
*     18 JUN 2015 jcs  Build 31: ioctl_getFd; valgrind stuff; _SendPing()
*     15 APR 2016 jcs  Build 32: ioctl_isSnapChan; ioctl_setUserPubXx; Bin err
*     12 SEP 2016 jcs  Build 33: ioctl_userDefStreamID
*     26 MAY 2017 jcs  Build 34: Socket "has-a" Thread
*     12 SEP 2017 jcs  Build 35: hash_map not map / GLHashMap
*     13 OCT 2017 jcs  Build 36: TapeChannel
*     10 JUL 2018 jcs  Build 40: Meow
*     13 JAN 2019 jcs  Build 41: TapeChannel._schema
*     12 FEB 2020 jcs  Build 42: bool Ioctl()
*     10 SEP 2020 jcs  Build 44: _bTapeDir; TapeChannel.Query()
*     16 SEP 2020 jcs  Build 45: ParseOnly()
*     22 OCT 2020 jcs  Build 46: PumpTape() / StopPumpTape()
*      3 DEC 2020 jcs  Build 47: StreamDone : both ways
*     29 MAR 2022 jcs  Build 52: ioctl_unpacked
*      7 MAY 2022 jcs  Build 53: Handle empty username
*     19 MAY 2022 jcs  Build 54: Schema : rtVALUE used as _maxLen
*     10 JUN 2022 jcs  Build 55: PumpTicker() : Reload if off > _tape._data
*     23 SEP 2022 jcs  Build 56: GLrpyDailyIdxVw; TapeChannel.GetField( int )
*     12 JAN 2024 jcs  Build 67: Buffer.h; TapeHeader.h
*     12 SEP 2024 jcs  Build 71: Handle !::mddSub_ParseHdr()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      E d g C h a n n e l
//
/////////////////////////////////////////////////////////////////////////////

static rtBUF       _zBUF  = { (char *)0, 0 };
static const char *_undef = "Undefined";
static const char *_err0  = "Invalid message : 0-length header";

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
EdgChannel::EdgChannel( rtEdgeAttr     attr, 
                        rtEdge_Context cxt ) :
   Socket( attr._pSvrHosts ),
   _cxt( cxt ),
   _con( attr._pSvrHosts ),
   _usr(),
   _pwd(),
   _svcs(),
   _recs(),
   _schema( (rtEdgeData *)0 ),
   _subscrID( 61202 ),
   _pumpID( 3381 ),
   _recU( (EdgRec *)0 ),
   _bConflate( false ),
   _bSnapChan( false ),
   _bRawData( true ),
   _bSvcTkr( true ),
   _bUsrStreamID( false ),
   _bTapeDir( true ),
   _Q(),
   _tape( (TapeChannel *)0 )
{
   string s( attr._pUsername );
   char  *ps, *pu, *pp, *rp;

   ps   = (char *)s.data();
   pu   = ::strtok_r( ps, "|", &rp );
   pp   = ::strtok_r( NULL, "|", &rp );
   _usr = pu ? pu : "";
   _pwd = pp ? pp : "";

   // username|password

   // libmddWire / librtEdge

   _mdd             = ::mddSub_Initialize();
   _attr            = attr;
   _attr._pSvrHosts = _con.c_str();
   _attr._pUsername = pUsr();
   pump().AddIdle( EdgChannel::_OnIdle, this );

   // Idle Callback / Stats

   ::memset( &_zzz, 0, sizeof( _zzz ) );
   ::memset( &_dfltStats, 0, sizeof( _dfltStats ) );
   SetStats( &_dfltStats );

   // Tape??

   if ( _attr._bTape )
      _tape = new TapeChannel( *this );
}

EdgChannel::~EdgChannel()
{
   SvcMap::iterator     it;
   RecByIdMap::iterator rt;

   if ( _tape )
      delete _tape;
   for ( it=_svcs.begin(); it!=_svcs.end(); delete (*it).second,it++ );
   for ( rt=_recs.begin(); rt!=_recs.end(); delete (*rt).second,rt++ );
   _svcs.clear();
   _recs.clear();
   _ClearSchema();
   if ( _log )
      _log->logT( 3, "~EdgChannel( %s )\n", dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
rtEdgeAttr EdgChannel::attr()
{
   return _attr;
}

rtEdge_Context EdgChannel::cxt()
{
   return _cxt;
}

const char *EdgChannel::pUsr()
{
   return _usr.data();
}

const char *EdgChannel::pPwd()
{
   return _pwd.data();
}

EdgRec *EdgChannel::GetRec( const char *pSvc, const char *pTkr )
{
   Locker  lck( _mtx );
   EdgSvc *svc;

   if ( (svc=GetSvc( pSvc )) )
      return svc->GetRec( pTkr );
   return (EdgRec *)0;
}

EdgRec *EdgChannel::GetRec( int oid )
{
   Locker               lck( _mtx );
   RecByIdMap::iterator rt;

   rt = _recs.find( oid );
   return ( rt != _recs.end() ) ? (*rt).second : (EdgRec *)0;
}

EdgSvc *EdgChannel::GetSvc( const char *pSvc )
{
   Locker           lck( _mtx );
   string           s( pSvc );
   SvcMap::iterator it;

   if ( (it=_svcs.find( s )) != _svcs.end() )
      return (*it).second;
   return (EdgSvc *)0;
}

bool EdgChannel::IsConflated()
{
   return _bConflate;
}

bool EdgChannel::IsSnapChan()
{
   return _bSnapChan;
}

EventPump &EdgChannel::Q()
{
   return _Q;
}

TapeChannel *EdgChannel::tape()
{
   return _tape;
}


////////////////////////////////////////////
// Schema / Field in Update Msg
////////////////////////////////////////////
rtEdgeData EdgChannel::GetSchema()
{
   mddFieldList fl;
   mddField    *mdb, m;
   rtFIELD     *fdb, f;
   int          i, nf;

   // Once

   if ( _schema )
      return *_schema;

   // Initialize; Get 'em from libmddWire

   _schema = new rtEdgeData();
   ::memset( _schema, 0, sizeof( rtEdgeData ) );
   fl  = ::mddWire_GetSchema( _mdd );
   mdb = fl._flds;
   nf  = fl._nFld;
   fdb = nf ? new rtFIELD[nf] : (rtFIELD *)0;
   for ( i=0; i<nf; i++ ) {
      m       = mdb[i];
      m._name = m._name ? m._name : _undef;
      f._fid  = m._fid;
      f._val  = m._val;
      f._name = m._name;
      f._type = (rtFldType)m._type;
      fdb[i]  = f;
   }
   _schema->_flds = fdb;
   _schema->_nFld = nf;
   return *_schema;
}

mddField *EdgChannel::GetDef( int fid )
{
   mddField *def;

   def = ::mddWire_GetFldDefByFid( _mdd, fid );
   return def;
}

mddField *EdgChannel::GetDef( const char *pFld )
{
   mddField *def;

   def = ::mddWire_GetFldDefByName( _mdd, pFld );
   return def;
}

rtFIELD *EdgChannel::GetField( const char *pFld )
{
   mddField *def;
   int       fid;

   // pFld == fid OR name

   if ( !(fid=atoi( pFld )) ) {
      if ( (def=GetDef( pFld )) )
         fid = def->_fid;
   }
   return GetField( fid );
}

rtFIELD *EdgChannel::GetField( int fid )
{
   rtFIELD *rc;

   rc = (rtFIELD *)0;
   if ( _tape )
      rc = _tape->GetField( fid );
   else if ( _recU )
      rc = _recU->GetField( fid );
   return rc;
}

bool EdgChannel::HasField( const char *pFld )
{
   return( GetField( pFld ) != (rtFIELD *)0 );
}

bool EdgChannel::HasField( int fid )
{
   return( GetField( fid ) != (rtFIELD *)0 );
}


////////////////////////////////////////////
// Cached Data
////////////////////////////////////////////
rtEdgeData EdgChannel::GetCache( const char *svc, const char *tkr )
{
   Locker       lck( _mtx );
   EdgRec      *rec;
   Record      *c;
   rtEdgeData   d;
   mddFieldList fl;

   // Pre-condition

   ::memset( &d, 0, sizeof( d ) );
   if ( (rec=GetRec( svc, tkr )) ) {
      d._pSvc    = rec->pSvc();
      d._pTkr    = rec->pTkr();
      d._pErr    = NULL;
      d._arg     = rec->_arg;
      d._ty      = edg_image;
      d._rawData = NULL;
      d._rawLen  = 0;
      if ( (c=rec->cache()) ) {
         fl      = c->GetCache();
         d._flds = (rtFIELD *)fl._flds;
         d._nFld = fl._nFld; 
      }
   }
   return d;
}

rtEdgeData EdgChannel::GetCache( int StreamID )
{
   Locker       lck( _mtx );
   EdgRec      *rec;
   Record      *c;
   rtEdgeData   d;
   mddFieldList fl; 

   // Pre-condition

   ::memset( &d, 0, sizeof( d ) );
   if ( (rec=GetRec( StreamID )) ) {
      d._pSvc    = rec->pSvc();
      d._pTkr    = rec->pTkr();
      d._pErr    = NULL;
      d._arg     = rec->_arg;
      d._ty      = edg_image;
      d._rawData = NULL;
      d._rawLen  = 0;
      if ( (c=rec->cache()) ) {
         fl      = c->GetCache();
         d._flds = (rtFIELD *)fl._flds;
         d._nFld = fl._nFld;
      }
   }
   return d;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
int EdgChannel::ParseOnly( rtEdgeData &d )
{
   /*
    * Don't want libmddWire to Free /  Alloc mddFieldList : Check below
    */
   mddFieldList    fl  = { (mddField *)d._flds, 0, d._rawLen };
   mddWire_Context cxt = _tape ? _tape->mdd() : _mdd;
   mddMsgBuf       inp = { (char *)d._rawData, (u_int)d._rawLen, NULL };
   mddWireMsg      w;
   mddMsgHdr       h;
   int             nh, nb;

   // 1) Set it up ...

   w._dt     = mddDt_FieldList;
   w._flds   = fl;
   w._svc    = _zBUF;
   w._tkr    = _zBUF;
   ::memset( &h, 0, sizeof( h ) );

   // 2) ... then parse

   if ( (nh=::mddSub_ParseHdr( cxt, inp, &h )) < 0 ) {
      OnDisconnect( _err0 );
      Disconnect( _err0 );
      return 0;
   }
   if ( !nh )
      return 0;
   nb  = ::mddSub_ParseMsg( cxt, inp, &w );
   fl  = w._flds;
assert( fl._nFld <= d._nFld );
   d._nFld = fl._nFld;
   return d._nFld;
}

int EdgChannel::Subscribe( const char *pSvc, 
                           const char *pTkr, 
                           void       *arg )
{
   Locker  lck( _mtx );
   EdgSvc *svc;
   EdgRec *rec; 
   string  s;
   int     id, uid;

   // Tape

   if ( _tape )
      return _tape->Subscribe( pSvc, pTkr );

   // Create record and service, if necessary

   id  = 0;
   uid = (PTRSZ)arg;
   if ( !(rec=GetRec( pSvc, pTkr )) ) {
      s = pSvc;
      if ( !(svc=GetSvc( pSvc )) ) {
         svc      = new EdgSvc( *this, pSvc );
         _svcs[s] = svc;
      }
      id        = _bUsrStreamID ? uid : _subscrID++;
      rec       = new EdgRec( *svc, pTkr, id, arg );
      _recs[id] = rec;
      svc->Add( rec );
   }

   // Open on rtEdgeCache

   if ( id && svc->IsUp() )
      Open( *rec );
   return id;
}

int EdgChannel::Unsubscribe( const char *pSvc, const char *pTkr )
{
   Locker  lck( _mtx );
   EdgRec *rec;

   if ( _tape )
      return _tape->Unsubscribe( pSvc, pTkr );
   else if ( (rec=GetRec( pSvc, pTkr )) )
      return Unsubscribe( rec->_StreamID );
   return 0;
}

int EdgChannel::Unsubscribe( int StreamID )
{
   Locker               lck( _mtx );
   RecByIdMap::iterator rt;
   EdgRec              *rec;

   // Pre-condition

   if ( !(rec=GetRec( StreamID )) )
      return 0;

   // Close on rtEdgeCache; Blow away ...

   Close( *rec );
   if ( (rt=_recs.find( StreamID )) != _recs.end() )
      _recs.erase( rt );
   rec->_svc.Remove( rec );
   if ( rec->cache() )
      _Q.Close( rec->cache() );
   if ( _recU && ( StreamID == _recU->_StreamID ) )
      _recU = (EdgRec *)0;
   delete rec;
   return StreamID;
}

void EdgChannel::Open( EdgRec &rec )
{
   rtEdgeChanStats &st = stats();
   mddMsgHdr        h;
   mddBuf           b;
   char             buf[K];

   // Open on rtEdgeCache

   h = _InitHdr( mddMt_open );
   _AddAttr( h, _mdd_pAttrSvc, (char *)rec.pSvc() );
   _AddAttr( h, _mdd_pAttrName, (char *)rec.pTkr() );
   sprintf( buf, "%d", rec._StreamID );
   _AddAttr( h, _mdd_pAttrTag, buf );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );
   rec._bOpn  = true;
   st._nOpen += 1;
}

void EdgChannel::Close( EdgRec &rec )
{
   rtEdgeChanStats &st = stats();
   mddMsgHdr        h;
   mddBuf           b;
   char             buf[K];

   // Close on rtEdgeCache

   h = _InitHdr( mddMt_close );
   _AddAttr( h, _mdd_pAttrSvc, (char *)rec.pSvc() );
   _AddAttr( h, _mdd_pAttrName, (char *)rec.pTkr() );
   sprintf( buf, "%d", rec._StreamID );
   _AddAttr( h, _mdd_pAttrTag, buf );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );
   rec._bOpn   = false;
   st._nClose += 1;
}

int EdgChannel::StartPumpFullTape( u_int64_t off0, int nMsg )
{
   return _tape ? _tape->StartPumpFullTape( off0, nMsg ) : 0;
}

int EdgChannel::StopPumpFullTape( int pumpID )
{
   return _tape ? _tape->StopPumpFullTape( pumpID ) : 0;
}


////////////////////////////////////////////
// Operations - Conflate / RTD
////////////////////////////////////////////
void EdgChannel::Conflate( bool bConflate )
{
   _bConflate = bConflate;
}

int EdgChannel::Dispatch( int maxUpd, double dWait )
{
   double     d0, d1, dw;
   rtEdgeRead rd;
   int        i, n, mt;

   d0 = ::rtEdge_TimeNs();
   dw = dWait;
   for ( i=0,n=0; n<maxUpd && dw > 0.0; i++ ) {
      _Q.Wait( dw );
      mt = Read( dw, rd );
      n += ( mt != EVT_NONE ) ? 1 : 0;
      switch( mt ) {
         case EVT_IMG:
         case EVT_UPD:
            if ( _attr._dataCbk )
               (*_attr._dataCbk)( _cxt, rd._d );
            break;
         case EVT_CONN:
            if ( _attr._connCbk )
               (*_attr._connCbk)( _cxt, rd._msg, rd._state );
            break;
         case EVT_SVC:
            if ( _attr._svcCbk )
               (*_attr._svcCbk)( _cxt, rd._msg, rd._state );
            break;
         case EVT_SCHEMA:
            if ( _attr._schemaCbk )
               (*_attr._schemaCbk)( _cxt, rd._d );
            break;
      }
      d1 = ::rtEdge_TimeNs();
      dw = gmax( 0.0, dWait - ( d1-d0 ) );
   }
   return n;
}

int EdgChannel::Read( double dWait, rtEdgeRead &rd )
{
   Update      upd;
   Record     *rec;
   bool        bUpd;
   string     *s;
   const char *pc;

   // Wait for it

   _Q.Wait( dWait );
   bUpd = _Q.GetOneUpd( upd );
   if ( !bUpd )
      return EVT_NONE;

   // Pull it out

   s = upd._msg;
   ::memset( &rd, 0, sizeof( rd ) );
   switch( upd._mt ) {
      case EVT_IMG:
      case EVT_UPD:
         if ( !(rec=upd._rec) )
            return EVT_NONE;
         rd._d._tMsg     = ::rtEdge_TimeNs();
         rd._d._rawData  = "Conflated";
         rd._d._rawLen   = 9;
         rd._d._flds     = _fldsU;
         rd._d._nFld     = rec->GetUpds( _fldsU );
         rd._d._pSvc     = rec->pSvc();
         rd._d._pTkr     = rec->pTkr();
         rd._d._pErr     = "OK";
         rd._d._arg      = (VOID_PTR)rec->StreamID();
         rd._d._ty       = ( upd._mt == EVT_IMG ) ? edg_image : edg_update;
         rd._d._StreamID = rec->StreamID();
         rd._d._RTL      = 0; // _recU->_nUpd
         break;
     case EVT_CONN:
     case EVT_SVC:
         pc        = s ? s->c_str() : "Undefined";
         rd._state = upd._state;
         safe_strcpy( rd._msg, pc );
         break;
     case EVT_SCHEMA:
         rd._d = GetSchema();
         break;
   }
   if ( s )
      delete s;
   return upd._mt;
}


////////////////////////////////////////////
// Socket Interface
////////////////////////////////////////////
bool EdgChannel::Ioctl( rtEdgeIoctl ctl, void *arg )
{
   bool       bArg, *pbArg;
   char      *pArg;
   u_int64_t *i64;

   // 1) Base Socket Class??

   if ( Socket::Ioctl( ctl, arg ) )
      return true;

   // 2) Else us

   bArg  = ( arg != (void *)0 );
   pbArg = (bool *)arg; 
   pArg  = (char *)arg; 
   i64   = arg ? (u_int64_t *)arg : (u_int64_t *)0;
   switch( ctl ) {
      case ioctl_rawData:
         _bRawData = bArg;
         return true;
      case ioctl_svcTkrName:
         _bSvcTkr = bArg;
         return true;
      case ioctl_isSnapChan:
         *pbArg = IsSnapChan();
         return true;
      case ioctl_getPubAuthToken:
         pArg[0] = '\0';
         return true;
      case ioctl_userDefStreamID:
         _bUsrStreamID = bArg;
         return true;
      case ioctl_tapeDirection:
         _bTapeDir = bArg;
         return true;
      case ioctl_tapeStartTime:
         if ( _tape && i64 )
            *i64 = _tape->hdr()._tCreate();
         return( _tape && i64 );
      case ioctl_tapeEndTime:
         if ( _tape && i64 )
            *i64 = _tape->hdr()._curTime().tv_sec;
         return( _tape && i64 );
      default:
         break;
   }
   return false;
}


////////////////////////////////////////////
// Thread Notifications
////////////////////////////////////////////
void EdgChannel::OnConnect( const char *pc )
{
   rtEdgeChanStats &st = stats();
   Update           u;

   // Notify

   st._lastMsg = _tvNow().tv_sec;
   if ( _attr._connCbk ) {
      if ( _bConflate ) {
         ::memset( &u, 0, sizeof( u ) );
         u._mt    = EVT_CONN;
         u._state = edg_up;
         u._msg   = new string( pc );
         _Q.AddAndNotify( u );
      }
      else
         (*_attr._connCbk)( _cxt, pc, edg_up );
   }
}

void EdgChannel::OnDisconnect( const char *reason )
{
   Locker           lck( _mtx );
   SvcMap::iterator it;
   EdgSvc          *svc;
   Update           u;

   // Blast to all services ...

   for ( it=_svcs.begin(); it!=_svcs.end(); it++ ) {
      svc = (*it).second;
      svc->OnService( *this, false );
   }
   _proto = mddProto_XML;
   ::mddWire_SetProtocol( _mdd, _proto ); 

   // Notify interested parties ...

   if ( _attr._connCbk ) {
      if ( _bConflate ) {
         ::memset( &u, 0, sizeof( u ) );
         u._mt    = EVT_CONN;
         u._state = edg_down;
         u._msg   = new string( reason );
         _Q.AddAndNotify( u );
      }
      else
         (*_attr._connCbk)( _cxt, reason, edg_down );
   }
}

void EdgChannel::OnRead()
{
   Locker           lck( _mtx );
   rtEdgeChanStats &st = stats();
   const char      *cp;
   mddMsgBuf        b;
   mddMsgHdr        h;
   struct timeval   tv;
   int              i, sz, nMsg, nb, nL;

   // 1) Base class drains channel and populates _in / _cp

   Socket::OnRead();

   // 2) OK, now we chop up ...

   cp = _in.bp();
   sz = _in.bufSz();
   for ( i=0,nMsg=0; i<sz; ) {
      b._data = (char *)cp;
      b._dLen = sz - i;
      b._hdr  = (mddMsgHdr *)0;
      h       = _InitHdr( mddMt_undef );
      if ( (nb=::mddSub_ParseHdr( _mdd, b, &h )) < 0 ) {
         OnDisconnect( _err0 );
         Disconnect( _err0 );
         return;
      }
      if ( !nb )
         break; // for-i
      b._dLen = nb;
      b._hdr  = &h;
      nMsg  += 1;
      if ( _log && _log->CanLog( 2 ) ) {
         Locker lck( _log->mtx() );
         string s( cp, nb );

         _log->logT( 2, "[XML-RX]" );
         _log->Write( 2, s.c_str(), nb );
      }
      switch( _proto ) {
         case mddProto_Undef:  break;
         case mddProto_Binary: OnBinary( b ); break;
         case mddProto_MF:     OnMF( b );     break;
         case mddProto_XML:    OnXML( b );    break;
      }
      _ClearUpd();
      cp += nb;
      i  += nb;

      // Stats

      tv            = _tvNow();
      st._lastMsg   = tv.tv_sec;
      st._lastMsgUs = tv.tv_usec;
      st._nMsg     += 1; 
   }

   // Message spans 2+ network packets?

   nL = WithinRange( 0, sz-i, INFINITEs );
   if ( _log && _log->CanLog( 4 ) ) {
      Locker lck( _log->mtx() );

      _log->logT( 4, "%d of %d bytes processed\n", i, sz );
      if ( nL )
         _log->HexLog( 4, cp, nL );
   }
   if ( nMsg && nL )
      _in.Move( sz-nL, nL );
   _in.Set( nL );
}


////////////////////////////////////////////
// TimerEvent Notifications
////////////////////////////////////////////
void EdgChannel::On1SecTimer()
{
   rtEdgeChanStats &st = stats();

   // 10 Tape 1st

   if ( _tape )
      _tape->Pump();
   else
      Socket::On1SecTimer();

   // 2) Idle Callback

   if ( _bIdleCbk && _attr._dataCbk )
      (*_attr._dataCbk)( _cxt, _zzz );

   // 3) Heartbeat

   _CheckHeartbeat( st._lastMsg );
}


////////////////////////////////////////////
// EdgChannel Notifications
////////////////////////////////////////////
void EdgChannel::OnBinary( mddMsgBuf b )
{
   OnMF( b, "BINARY" );
}

void EdgChannel::OnMF( mddMsgBuf b, const char *ty )
{
   rtEdgeChanStats &st = stats();
   EdgSvc          *svc;
   bool             bUp, bImg, bBin;
   const char      *pn, *pSvc, *pTkr;
   int              i, sz, fid;
   mddWireMsg       m;
   mddBuf          &bSvc = m._svc;
   mddBuf          &bTkr = m._tkr;
   mddBuf           bErr;
   mddFieldList    &fl = _fl;

   // 1) Log, if required

   if ( _log ) {
      Locker lck( _log->mtx() );
      string s( b._data, b._dLen );

      _log->logT( 1, "[%s-RX %06d bytes]\n    ", ty, b._dLen );
      _log->HexLog( 4, s.c_str(), b._dLen );
   }

   // 2) Parse

   m._flds = _fl;
   if ( !(sz=::mddSub_ParseMsg( _mdd, b, &m )) )
      return;
   _recU = GetRec( m._tag );
   _fl   = m._flds;

   // 3) On-pass by msg type ...

   rtEdgeData  dz;
   rtEdgeData &d   = _recU ? _recU->upd() : dz;
   rtFIELD    *fdb = (rtFIELD *)fl._flds;
   int         nf  = fl._nFld;
   mddField   *def;
   Record     *c;
   Update      u;

   ::memset( &d, 0, sizeof( d ) );
   if ( _bRawData ) {
      d._rawData = b._data; 
      d._rawLen  = b._dLen;
   }
   switch( m._mt ) {
      case mddMt_image:
      case mddMt_update:
      {
         if ( !_recU || !_attr._dataCbk )
            return;
         bImg      = ( m._mt == mddMt_image );
         st._nImg += bImg ? 1 : 0;
         st._nUpd += bImg ? 0 : 1;
         if ( _bConflate ) {
            ::memset( &u, 0, sizeof( u ) );
            u._mt  = bImg ? EVT_IMG : EVT_UPD;
            u._rec = _recU->cache();
            u._rec->Cache( fl );
            _Q.AddAndNotify( u );
            break;
         }
         if ( (c=_recU->cache()) )
            c->Cache( fl );
         for ( i=0; i<nf; i++ ) {
            fid = fdb[i]._fid;
            def = GetDef( fid );
            pn  = def ? def->_name : "Undefined";
            fdb[i]._name = pn;
         }
         d._flds = fdb;
         d._nFld = nf;
         if ( _bSvcTkr ) {
            d._pSvc = _recU->pSvc();
            d._pTkr = _recU->pTkr();
            d._pErr = "OK";
         }
         d._tMsg     = ::rtEdge_TimeNs();
         d._arg      = _recU->_arg;
         d._ty       = bImg ? edg_image : edg_update;
         d._StreamID = _recU->_StreamID;
         d._RTL      = 0; // _recU->_nUpd
         (*_attr._dataCbk)( _cxt, d );
         if ( IsSnapChan() ) {
            _ClearUpd();
            Unsubscribe( d._StreamID );
         }
         break;
      }
      case mddMt_gblStatus: // Global Status
      {
         bUp  = ( m._state == mdd_up );
         if ( !bSvc._dLen )
            return;

         string ss( bSvc._data, bSvc._dLen );

         pSvc = ss.data();
         if ( !(svc=GetSvc( pSvc )) && ::strcmp( pSvc, _mdd_pGlobal ) ) {
            Locker l( _mtx );
            string s( pSvc );

            svc      = new EdgSvc( *this, pSvc );
            _svcs[s] = svc;
         }
         svc->OnService( *this, bUp );
         break;
      }
      case mddMt_dead:    // Item Status
      {
         string sErr;

         if ( !_attr._dataCbk )
            return;
         if ( !_recU ) {
            if ( !bSvc._dLen || !bTkr._dLen )
               return;

            string ss( bSvc._data, bSvc._dLen );
            string tt( bTkr._data, bTkr._dLen );

            pSvc = ss.data();
            pTkr = tt.data();
            if ( !(svc=GetSvc( pSvc )) )
               return;
            _recU = svc->GetRec( pTkr );
         }
         if ( !_recU )
            return;

         bBin = ( _proto == mddProto_Binary );
         if ( bBin && nf ) {
            bErr = fl._flds[0]._val._buf;
            if ( bErr._dLen < b._dLen ) 
               sErr.append( bErr._data, bErr._dLen );
            else
               sErr.append( "unknown error" );
            m._err._data = (char *)sErr.data();
            m._err._dLen = sErr.length();
         }
         st._nDead += 1;
         d._pSvc     = _recU->pSvc();
         d._pTkr     = _recU->pTkr();
         d._pErr     = m._err._data;
         d._arg      = _recU->_arg;
         d._ty       = edg_dead;
         d._StreamID = _recU->_StreamID;
         d._RTL      = 0; // _recU->_nUpd
         (*_attr._dataCbk)( _cxt, d );
         if ( _recU )
            _recU->_bOpn = false;
         if ( IsSnapChan() ) {
            _ClearUpd();
            Unsubscribe( d._StreamID );
         }
         break;
      }
      case mddMt_ping:
         _SendPing();
         break;
      case mddMt_undef:
      case mddMt_stale:
      case mddMt_recovering:
      case mddMt_mount:
      case mddMt_ctl:
      case mddMt_open:
      case mddMt_close:
      case mddMt_query:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }
}

void EdgChannel::OnXML( mddMsgBuf &b )
{
   mddMsgHdr &h = *b._hdr;

   switch( h._mt ) {
      case mddMt_image:  OnXMLData( b, true );  break;
      case mddMt_update: OnXMLData( b, false ); break;
      case mddMt_dead:   OnXMLStatus( h );      break;
      case mddMt_ctl:    OnXMLIoctl( h );       break;
      case mddMt_ping:   _SendPing();           break;
      case mddMt_undef:
      case mddMt_stale:
      case mddMt_recovering:
         break;
      case mddMt_mount:
         OnXMLMount( _GetAttr( h, _mdd_pLoginID ) );
         break;
      case mddMt_open:
      case mddMt_close:
      case mddMt_query:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_gblStatus:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }
}


////////////////////////////////////////////
// XML Handler
////////////////////////////////////////////
void EdgChannel::OnXMLData( mddMsgBuf &b, bool bImg )
{
   Locker     lck( _mtx );
   mddWireMsg m;
   int        sz;

   // 1) Parse

   if ( !(sz=::mddSub_ParseMsg( _mdd, b, &m )) )
      return;
   _fl   = m._flds;
   _recU = GetRec( m._tag );
   if ( !_recU || !_attr._dataCbk )
      return;

   // 2) Process

   rtEdgeData &d = _recU->upd();

   _recU->_nUpd += 1;
   ::memset( &d, 0, sizeof( d ) );
   d._pSvc     = _recU->pSvc();
   d._pTkr     = _recU->pTkr();
   d._pErr     = "OK";
   d._arg      = _recU->_arg;
   d._ty       = bImg ? edg_image : edg_update;
   d._flds     = (rtFIELD *)_fl._flds;
   d._nFld     = _fl._nFld;
   d._StreamID = _recU->_StreamID;
   d._RTL      = 0; // _recU->_nUpd
   (*_attr._dataCbk)( _cxt, d );
}

void EdgChannel::OnXMLStatus( mddMsgHdr &h )
{
   Locker      lck( _mtx );
   EdgSvc     *svc;
   const char *pRIC, *pType, *pCode, *pErr;
   char       *pSvc, *pUp, *rp;
   bool        bUp;

   // Extract status

   pRIC  = _GetAttr( h, _mdd_pAttrName );
   pType = _GetAttr( h, _mdd_pType );
   pCode = _GetAttr( h, _mdd_pCode );
   pErr  = _GetAttr( h, _mdd_pError );
   if ( !::strcmp( pType, _mdd_pGlobal ) ) {
      string s( pErr );

      pSvc = ::strtok_r( (char *)s.data(), ":", &rp );
      pUp  = ::strtok_r( NULL, ":", &rp );

      // Invalid password ea9e565bdfe9835c7fd4cb5673539748

      if ( !pUp ) {
         OnDisconnect( pErr );
         Disconnect( pErr );
         return;
      }

      // IDN_RDF:UP

      bUp  = ( ::strcmp( pUp, _mdd_pUp ) == 0 );

      Locker l( _mtx );

      s = pSvc;
      if ( !(svc=GetSvc( pSvc )) && ::strcmp( pSvc, _mdd_pGlobal ) ) {
         svc      = new EdgSvc( *this, pSvc );
         _svcs[s] = svc;
      }
      svc->OnService( *this, bUp );
   }
}

void EdgChannel::OnXMLMount( const char *pAuth )
{
   Locker    lck( _mtx );
   mddMsgHdr h;
   mddBuf    b;
   char     *pro, *ap;
   char      buf[K], auth[K];
   u_char   *mp;
   GLmd5     md5;
   int       i;

   // 1) Set username; 

   h = _InitHdr( mddMt_mount );
   _AddAttr( h, _mdd_pUser, (char *)pUsr() );
   if ( strlen( pPwd() ) && strlen( pAuth ) )  {
      sprintf( buf, "%s%s", pPwd(), pAuth );
      md5.update( buf, strlen( buf ) );
      mp = md5.digest();
      ap = auth;
      for ( i=0; i<16; ap+=sprintf( ap, "%02x", mp[i++] ) );
      _AddAttr( h, _mdd_pPword, auth );
   }
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );

   // 2) DataDict

   h = _InitHdr( mddMt_ctl );
   _AddAttr( h, _mdd_pAttrDict, _mdd_pYes );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );

   // 3) Protocol

   h   = _InitHdr( mddMt_ctl );
   pro = _bBinary ? _mdd_pAttrBin : _mdd_pAttrMF;
   _AddAttr( h, pro, _mdd_pYes );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );
}

void EdgChannel::OnXMLIoctl( mddMsgHdr &h )
{
   Locker      lck( _mtx );
   const char *pAttr;
   Update      u;
   bool        bMF, bBin;

   /*
    * <CTL MF="YES"/>
    * <CTL DataDict="..."/>
    * <CTL SNAP="YES"/>
    */

   if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrMF )) ) ) {
      bMF    = ( ::strcmp( pAttr, _mdd_pYes ) == 0 );
      _proto = bMF ? mddProto_MF : _proto;
      ::mddWire_SetProtocol( _mdd, _proto );
   }
   else if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrBin )) ) ) {
      bBin   = ( ::strcmp( pAttr, _mdd_pYes ) == 0 );
      _proto = bBin ? mddProto_Binary : _proto;
      ::mddWire_SetProtocol( _mdd, _proto );
   }
   if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrDict )) ) ) {
      _ClearSchema();
      ::mddWire_SetSchema( _mdd, pAttr );
      if ( _attr._schemaCbk ) {
         if ( _bConflate ) {
            ::memset( &u, 0, sizeof( u ) );
            u._mt  = EVT_SCHEMA;
            _Q.AddAndNotify( u );
         }
         else
            (*_attr._schemaCbk)( _cxt, GetSchema() );
      }
   }
   else if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrSnap )) ) )
      _bSnapChan = ( ::strcmp( pAttr, _mdd_pYes ) == 0 );
   else if ( strlen( (pAttr=_GetAttr( h, _mdd_pError )) ) ) {
      OnDisconnect( pAttr );
      Disconnect( pAttr );
   }
}



////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
void EdgChannel::_ClearUpd()
{
   if ( _recU )
      _recU->ClearUpd();
   _recU = (EdgRec *)0;
}

void EdgChannel::_ClearSchema()
{
   rtFIELD *flds;
   
   if ( _schema ) {
      flds = _schema->_flds;
      if ( flds )
         delete[] flds;
      delete _schema;
   }
   _schema = (rtEdgeData *)0;
}



/////////////////////////////////////////
// Idle Loop Processing
////////////////////////////////////////////
void EdgChannel::OnIdle()
{
breakpointE(); // TODO : Re-connect ...
}

void EdgChannel::_OnIdle( void *arg )
{
   EdgChannel *us;

   us = (EdgChannel *)arg;
   us->OnIdle();
}




/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      E d g S v c
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
EdgSvc::EdgSvc( EdgChannel &ch, const char *pSvc ) :
   _ch( ch ),
   _mtx( ch.mtx() ),
   _name ( pSvc ),
   _recs(),
   _bUp( false )
{
}

EdgSvc::~EdgSvc()
{
   _recs.clear();
   if ( _ch._log )
      _ch._log->logT( 3, "~EdgSvc( %s )\n", _ch.dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
EdgChannel &EdgSvc::ch()
{
   return _ch;
}

const char *EdgSvc::pName()
{
   return _name.c_str();
}

bool EdgSvc::IsUp()
{
   return _bUp;
}

EdgRec *EdgSvc::GetRec( const char *pTkr )
{
   Locker                 mtx( _mtx );
   RecByNameMap::iterator it;
   string                 s( pTkr );

   if ( (it=_recs.find( s )) != _recs.end() )
      return (*it).second;
   return (EdgRec *)0;
}



////////////////////////////////////////////
// Operations
////////////////////////////////////////////
void EdgSvc::Add( EdgRec *rec )
{
   Locker mtx( _mtx );
   string s( rec->pTkr() );

   if ( !GetRec( rec->pTkr() ) )
      _recs[s] = rec; 
}

void EdgSvc::Remove( EdgRec *rec )
{
   Locker                 mtx( _mtx );
   RecByNameMap::iterator it;
   string                 s( rec->pTkr() );

   if ( (it=_recs.find( s )) != _recs.end() )
      _recs.erase( it );
}



////////////////////////////////////////////
// Notification
////////////////////////////////////////////
void EdgSvc::OnService( EdgChannel &, bool bUp )
{
   Locker                 lck( _mtx );
   rtEdgeAttr             attr = _ch.attr();
   rtEdgeData             d;
   Update                 u;
   EdgRec                *rec;
   RecByNameMap::iterator it;

   // Set flag; Call service callback 

   _bUp = bUp;
   if ( attr._svcCbk ) {
      if ( _ch.IsConflated() ) {
         ::memset( &u, 0, sizeof( u ) );
         u._mt    = EVT_SVC;
         u._state = bUp ? edg_up : edg_down;
         u._msg   = new string( pName() );
         _ch.Q().AddAndNotify( u );
      }
      else
         (*attr._svcCbk)( _ch.cxt(), pName(), bUp ? edg_up : edg_down );
   }

   // Blast to all ticker callbacks

   for( it=_recs.begin(); it!=_recs.end() && attr._dataCbk; it++ ) {
      rec = (*it).second;
      ::memset( &d, 0, sizeof( d ) );
      d._pSvc     = rec->pSvc();
      d._pTkr     = rec->pTkr();
      d._pErr     = bUp ? "Service UP" : "Service DOWN";
      d._arg      = rec->_arg;
      d._ty       = bUp ? edg_recovering : edg_stale;
      d._flds     = (rtFIELD *)0;
      d._nFld     = 0;
      d._StreamID = rec->_StreamID;
      d._RTL      = 0; // rec->_nUpd;
      (*attr._dataCbk)( _ch.cxt(), d );
      if ( bUp && !rec->_bOpn )
         _ch.Open( *rec );
      else if ( !bUp )
         rec->_bOpn = false;
   }
}




/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      E d g R e c
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
EdgRec::EdgRec( EdgSvc     &svc, 
                const char *pTkr, 
                int         StreamID, 
                void       *arg ) :
   _svc( svc ),
   _mtx( svc.ch().mtx() ),
   _tkr ( pTkr ),
   _arg( arg ),
   _StreamID( StreamID ),
   _nUpd( 0 ),
   _bOpn( false ),
   _upds(),
   _cache( (Record *)0 )
{
   EdgChannel &ch = _svc.ch();

   if ( ch.IsConflated() || ch.IsCache() )
      _cache = new Record( svc.pName(), pTkr, _StreamID );
   ::memset( &_upd, 0, sizeof( _upd ) );
}

EdgRec::~EdgRec()
{
   Logger *lf;

   ClearUpd();
   if ( _cache )
      delete _cache;
   if ( (lf=Socket::_log) )
      lf->logT( 3, "~EdgRec( %s )\n", _svc.ch().dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
const char *EdgRec::pSvc()
{
   return _svc.pName();
}

const char *EdgRec::pTkr()
{
   return _tkr.c_str();
}

rtEdgeData &EdgRec::upd()
{
   return _upd;
}

Record *EdgRec::cache()
{
   return _cache;
}


////////////////////////////////////////////
// Update Processing
////////////////////////////////////////////
rtFIELD *EdgRec::GetField( int fid )
{
   FieldMap::iterator it;
   rtFIELD           *f;

   // Parse (once), then by fid

   _Parse();
   it = _upds.find( fid );
   f  = ( it != _upds.end() ) ? &((*it).second) : (rtFIELD *)0;
   return f;
}

void EdgRec::ClearUpd()
{
   _upds.clear();
}



////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
void EdgRec::_Parse()
{
   rtFIELD f;
   int     i, nf, fid;

   /*
    * Parse once per update
    * ASSUMPTION : EdgChannel::_ClearUpd() called after each update
    */

   nf = _upds.size() ? 0 : _upd._nFld;
   for ( i=0; i<nf; i++ ) {
      f          = _upd._flds[i];
      fid        = f._fid;
      _upds[fid] = f;
   }
}


