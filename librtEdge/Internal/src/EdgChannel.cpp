/******************************************************************************
*
*  EdgChannel.cpp
*     rtEdgeCache subscription channel
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
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      E d g C h a n n e l
//
/////////////////////////////////////////////////////////////////////////////

static rtBuf64     _zBuf  = { (char *)0, 0 }; 
static rtBUF       _zBUF  = { (char *)0, 0 };

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
   _usr = pu;
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
static const char *_undef = "Undefined";

rtEdgeData EdgChannel::GetSchema()
{
   mddFieldList fl;
   mddField    *mdb, m;
   rtFIELD     *fdb, f;
   rtBUF       &b = f._val._buf;
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
      b._data = (char  *)m._name;
      b._dLen = strlen( b._data );
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
   return _recU ? _recU->GetField( pFld ) : (rtFIELD *)0;
}

rtFIELD *EdgChannel::GetField( int fid )
{
   return _recU ? _recU->GetField( fid ) : (rtFIELD *)0;
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

   nh  = ::mddSub_ParseHdr( cxt, inp, &h );
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
   bool  bArg, *pbArg;
   char *pArg;

   // 1) Base Socket Class??

   if ( Socket::Ioctl( ctl, arg ) )
      return true;

   // 2) Else us

   bArg  = ( arg != (void *)0 );
   pbArg = (bool *)arg; 
   pArg  = (char *)arg; 
   switch( ctl ) {
      case ioctl_parse:
      case ioctl_nativeField:
      case ioctl_fixedLibrary:
         ::mddWire_ioctl( _mdd, (mddIoctl)ctl, arg );
         return true;
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

   cp = _in._bp;
   sz = _in.bufSz();
   for ( i=0,nMsg=0; i<sz; ) {
      b._data = (char *)cp;
      b._dLen = sz - i;
      b._hdr  = (mddMsgHdr *)0;
      h       = _InitHdr( mddMt_undef );
      nb      = ::mddSub_ParseHdr( _mdd, b, &h );
      b._dLen = nb;
      b._hdr  = &h;
      if ( !nb )
         break; // for-i
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
   _in.Reset();
   _in._cp += nL;
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
rtFIELD *EdgRec::GetField( const char *pFld )
{
   mddField *def;
   int       fid;

   // pFld == fid OR name

   if ( !(fid=atoi( pFld )) ) {
      if ( (def=_svc.ch().GetDef( pFld )) )
         fid = def->_fid;
   }
   return GetField( fid );
}

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



/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      T a p e C h a n n e l
//
/////////////////////////////////////////////////////////////////////////////

static const char *t_ALL  = "*";
static const char *t_SEP  = "|";
static int _thSz          = sizeof( GLrecTapeHdr );
static int _rSz           = sizeof( GLrecTapeRec );
static int _mSz8          = sizeof( GLrecTapeMsg );
static int _mSz4          = _mSz8 - 4;
static int _deSz          = sizeof( GLrecDictEntry );
static int _ixSz          = sizeof( u_int64_t );
static struct timeval _zT = { 0, 0 };
static struct timeval _eT = { INFINITEs, 0 };

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
TapeChannel::TapeChannel( EdgChannel &chan ) :
   _chan( chan ),
   _attr( chan.attr() ),
   _schema(),
   _tape( _zBuf ),
   _hdr( (GLrecTapeHdr *)0 ),
   _rdb(),
   _tdb(),
   _wl(),
   _dead(),
   _mdd( ::mddSub_Initialize() ),
   _fl( ::mddFieldList_Alloc( K ) ),
   _err(),
   _nSub( 0 ),
   _t0( _zT ),
   _t1( _eT ),
   _bRun( false ),
   _bInUse( false )
{
   ::mddWire_SetProtocol( _mdd, mddProto_Binary );
}

TapeChannel::~TapeChannel()
{
   DeadTickers::iterator it;
   int                   i;

   for ( i=0; _bInUse; i++ );
   _bRun = false;
   for ( it=_dead.begin(); it!=_dead.end(); delete (*it).second );
   _wl.clear();
   _dead.clear();
   Unload();
   ::mddFieldList_Free( _fl );
   ::mddSub_Destroy( _mdd );
}


////////////////////////////////////
// Access
////////////////////////////////////
mddWire_Context TapeChannel::mdd()
{
   return _mdd;
}

const char *TapeChannel::pTape()
{
   return _attr._pSvrHosts;
}

const char *TapeChannel::err()
{
   return _err.length() ? _err.data() : (const char *)0;
}

u_int64_t *TapeChannel::tapeIdxDb()
{
   char *cp;

   cp  = _tape._data;
   cp += _DbHdrSize( _hdr->_numDictEntry, 0, 0 );
   return (u_int64_t *)cp;
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
      rr[i]._pSvc     = v[i]->_svc;
      rr[i]._pTkr     = v[i]->_tkr;
      rr[i]._fid      = 0;
      rr[i]._interval = v[i]->_nMsg;
   }
   q._recs = rr;
   q._nRec = (int)i;
   return q;
}


////////////////////////////////////
// Operations
////////////////////////////////////
int TapeChannel::Subscribe( const char *svc, const char *tkr )
{
   string         s;
   struct timeval tmp;
   double         d0, d1;
   char          *ps, *p0, *p1, *rp;
   int            ix;

   // Special Case : Run baby

   if ( !::strcmp( svc, pTape() ) ) {
      if ( ::strcmp( tkr, t_ALL ) ) {
         s   = tkr;
         ps  = (char *)s.data();
         p0  = ::strtok_r( ps,   t_SEP, &rp );
         p1  = ::strtok_r( NULL, t_SEP, &rp );
         _t0 = _str2tv( p0 );
         _t1 = p1 ? _str2tv( p1 ) : _t0;
         d0  = Logger::Time2dbl( _t0 );
         d1  = Logger::Time2dbl( _t1 );
         if ( d1 < d0 ) {
            tmp = _t0;
            _t0 = _t1;
            _t1 = tmp;
         }
      }
      _bRun = true;
      return 0;
   }

   // Regular Subscription

   _nSub += 1;
   if ( HasTicker( svc, tkr, ix ) ) 
      _wl[ix] = ix;
   else  {
      ix        = _hdr->_maxRec + _nSub;
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
   GLrecTapeRec *rec;
   string        s;
   char         *bp, *cp;
   int           i, nr, rSz;
   bool          bReMap;
   static char   _bDeepCopy = 0;

   // Pre-condition(s)

   bReMap = ( _tape._data != (char *)0 );
   if ( bReMap )
      _tape = ::rtEdge_RemapFile( _tape );
   else {
      Unload();
      _tape = ::rtEdge_MapFile( (char *)pTape(), _bDeepCopy );
   }
   if ( !_tape._dLen ) {
      _err  = "Can not map file ";
      _err += pTape();
      return false;
   }
   if ( bReMap )
      return true;

   /*
    * Header
    */
   bp  = _tape._data;
   _hdr = (GLrecTapeHdr *)bp;
   nr   = _hdr->_numRec;
   rSz  = _RecSiz();
   if ( !_hdr->_bMDDirect ) {
      Unload();
      _err  = "Invalid tape file ";
      _err += pTape();
      return false;
   }
   /*
    * Schema (Field Dictionary)
    */
   cp  = bp;
   cp += _LoadSchema();
   cp += _hdr->_numSecIdxT * _ixSz;  // Offset Indices
   /*
    * Records Dictionary
    */
   for ( i=0; i<nr; i++,cp+=rSz ) {
      rec     = (GLrecTapeRec *)cp;
      s       = _Key( rec->_svc, rec->_tkr );
      _rdb[s] = i;
      _tdb.push_back( rec );
   }
   return true;
}

int TapeChannel::Pump()
{
   TapeWatchList::iterator it;
   GLrecTapeMsg           *msg;
   char                   *bp, *cp;
   mddBuf                  m;
   u_int64_t               off;
   size_t                  nt;
   int                     n, mSz, idx;

   // Pre-condition(s)

   if ( !_hdr || !_bRun )
      return 0;

   // One ticker??

   _bInUse = true;
   if ( (nt=_wl.size()) == 1 ) {
      it = _wl.begin();
      return PumpTicker( (*it).first );
   }

   // All Tickers

   bp  = _tape._data;
   off = _hdr->_hdrSiz;
   _PumpDead();
   if ( _t0.tv_sec ) {
      idx = _tapeIdx( _t0 );
      off = tapeIdxDb()[idx];
   }
   for ( n=0; !_nSub && _bRun && off<_tape._dLen; ) {
      cp      = bp+off;
      msg     = (GLrecTapeMsg *)cp;
      mSz     = msg->_bLast4 ? _mSz4 : _mSz8;
      m._data = cp + mSz;
      n      += _PumpOneMsg( *msg, m, false );
      off    += msg->_msgLen;
   }
   _bRun   = false;
   _bInUse = false;

   // Return number pumped

   return n;
}

int TapeChannel::PumpTicker( int ix )
{
   GLrecTapeRec *rec;
   GLrecTapeMsg *msg;
   char         *bp, *cp, *rp, sts[K];
   u_int64_t    *idb;
   mddBuf        m;
   Offsets       odb;
   u_int64_t     off, diff, nMsg;
   size_t        i, j, nr;
   int           n, mSz, idx, pct;
   static int    _pct[] = { 10, 25, 50, 75 };

   // Pre-condition(s)

   if ( !_hdr )
      return 0;

   // One Ticker

   _bInUse = true;
   bp      = _tape._data;
   rec     = _tdb[ix];
   off     = rec->_loc;
   nMsg    = rec->_nMsg;
   _PumpDead();
#ifdef FUCK_THIS
   if ( _t1.tv_sec != INFINITEs  ) {
      rp  = (char *)rec;
      rp += _rSz;
      idb = (u_int64_t *)rp;
      idx = _SecIdx( _t1, rec );
      for ( off=idb[idx]; !off && idx; off=idb[--idx] );
      idx = _tapeIdx( _t1 );
      off = tapeIdxDb()[idx];
   }
#endif // FUCK_THIS
   for ( i=0,n=0; _bRun && off; i++ ) {
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
            _PumpStatus( *msg, sts );
         }
         else {
            for ( j=0; j<4; j++ ) {
               pct = 100 - _pct[j];
               if ( i == ( nMsg / pct ) ) {
                  sprintf( sts, "%d%% msgs reversed", _pct[j] );
                  _PumpStatus( *msg, sts );
               }
            }
         }
         if ( _InTimeRange( *msg ) )
            odb.push_back( off );
      }
      else
         n   += _PumpOneMsg( *msg, m, true );
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
   for ( i=0; i<nr; i++ ) {
      off     = odb[nr-1-i];
      cp      = bp+off;
      msg     = (GLrecTapeMsg *)cp;
      mSz     = msg->_bLast4 ? _mSz4 : _mSz8;
      m._data = cp + mSz;
      m._dLen = msg->_msgLen - mSz;
      n      += _PumpOneMsg( *msg, m, true );
   }
   if ( nr )
      _PumpStatus( *msg, "Stream Complete", edg_streamDone );
   _bRun   = false;
   _bInUse = false;

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
   ::rtEdge_UnmapFile( _tape );
   _tape = _zBuf;
   _hdr  = (GLrecTapeHdr *)0;
}


////////////////////////////////////
// Helpers
////////////////////////////////////
bool TapeChannel::_InTimeRange( GLrecTapeMsg &m )
{
   return InRange( _t0.tv_sec, m._tv_sec, _t1.tv_sec );
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
   FieldMap          &sdb = _schema;
   FieldMap::iterator it;
   GLrecDictEntry    *de;
   rtEdgeData         d;
   char              *bp, *cp;
   rtFIELD           *fdb, f;
   rtBUF             &b = f._val._buf;
   int                i, nd, ni, nr, dSz, iSz;

   // Pre-condition(s)

   if ( !_hdr || !_tape._dLen )
      return 0;
   /*
    * Header / Sizes
    */
   bp   = _tape._data;
   cp   = bp;
   cp  += _thSz;
   nd   = _hdr->_numDictEntry;
   ni   = _hdr->_numSecIdxT;
   nr   = _hdr->_numRec;
   dSz  = nd * _deSz;
   iSz  = ni * _ixSz;
   /*
    * Schema
    */
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
      if ( (it=sdb.find( f._fid )) == sdb.end() )
         sdb[f._fid] = f;
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

   tv.tv_sec  = _hdr->_tCreate;
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
      if ( _attr._dataCbk )
         (*_attr._dataCbk)( _chan.cxt(), d );
      delete (*it).second;
   }
   _dead.clear();
   return n;
}

void TapeChannel::_PumpStatus( GLrecTapeMsg &msg, 
                               const char   *sts,
                               rtEdgeType    ty )
{
   rtEdgeData d;
   int        ix;

   // Fill in rtEdgeData and dispatch

   ::memset( &d, 0, sizeof( d ) );
   ix          = msg._dbIdx;
   d._tMsg     = Logger::Time2dbl( Logger::tvNow() );
   d._pSvc     = _tdb[ix]->_svc;
   d._pTkr     = _tdb[ix]->_tkr;
   d._pErr     = sts;
   d._ty       = ty;
   if ( _attr._dataCbk )
      (*_attr._dataCbk)( _chan.cxt(), d );
}

int TapeChannel::_PumpOneMsg( GLrecTapeMsg &msg, mddBuf m, bool bRev )
{
   rtEdgeData     d;
   struct timeval tv;
   int            ix;

   // Pre-condition(s)

   if ( !_InTimeRange( msg ) ) {
      if ( bRev )
         _bRun &= ( msg._tv_sec >= _t0.tv_sec );
      else
         _bRun &= ( msg._tv_sec <= _t1.tv_sec );
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
   d._tMsg     = Logger::Time2dbl( tv );
   d._pSvc     = _tdb[ix]->_svc;
   d._pTkr     = _tdb[ix]->_tkr;
   d._pErr     = "OK";
   d._ty       = edg_update;
   d._flds     = (rtFIELD *)_fl._flds;
   d._nFld     = _fl._nFld;
   d._rawData  = m._data;
   d._rawLen   = m._dLen;
   d._StreamID = ix;
   if ( _attr._dataCbk )
      (*_attr._dataCbk)( _chan.cxt(), d );
   return 1;
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

int TapeChannel::_tapeIdx( struct timeval tv )
{
   return _SecIdx( tv, (GLrecTapeRec *)0 );
}

int TapeChannel::_SecIdx( struct timeval now, GLrecTapeRec *rec )
{
   GLrecTapeHdr  &h  = *_hdr;
   struct timeval tv;
   time_t         tt, tn;
   struct tm     *tm, lt;
   bool           bRec;
   int            rtn, cmp, curIdx, secPerIdx;

   // 1) Tape or Record?

   bRec      = ( rec != (GLrecTapeRec *)0 );
   curIdx    = bRec ? rec->_curIdx   : h._curIdx;
   secPerIdx = bRec ? h._secPerIdxR  : h._secPerIdxT;
   tv        = bRec ? rec->_curIdxTm : h._curIdxTm;
   
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

u_int64_t TapeChannel::_DbHdrSize( int numDictEntry,
                                   int numSecIdx,
                                   int numRec )
{
   u_int64_t rtn;

   rtn  = _thSz;
   rtn += ( _deSz * numDictEntry );
   rtn += ( _ixSz * numSecIdx );
   rtn += ( _RecSiz() * numRec );
   return rtn;
}

int TapeChannel::_RecSiz()
{
   return _rSz + ( _hdr->_numSecIdxR * _ixSz );
}

struct timeval TapeChannel::_str2tv( char *hms )
{
   struct tm     *tm, lt;
   struct timeval tv;
   time_t         t;
   char          *h, *m, *s, *u, *z, *rp;
   size_t         sz, mul;

   // File Create Time

   z   = (char *)"0";
   t   = _hdr->_tCreate;
   tm  = ::localtime_r( (time_t *)&t, &lt );
   h   = ::strtok_r( hms,  ":", &rp );
   m   = h ? ::strtok_r( NULL, ":", &rp ) : z;
   s   = m ? ::strtok_r( NULL, ":", &rp ) : z;
   u   = s ? ::strtok_r( NULL, ".", &rp ) : z;
   sz  = strlen( u );
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
