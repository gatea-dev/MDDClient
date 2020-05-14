/******************************************************************************
*
*  PubChannel.cpp
*     rtEdgeCache publication channel
*
*  REVISION HISTORY:
*     23 SEP 2010 jcs  Created (from EdgChannel)
*     30 DEC 2010 jcs  Build  9: 64-bit
*     11 MAY 2011 jcs  Build 12: .NET : _pub
*     24 JAN 2012 jcs  Build 17: _Open() / _Close()
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*     30 DEC 2012 jcs  Build 22: _IncUpd() : Add, if not there
*     11 FEB 2013 jcs  Build 23: NULL's : _in / _cp not _rdData
*     14 FEB 2013 jcs  Build 24: Ioctl()
*      6 MAY 2013 jcs  Build 25: Publish() : Allow user to build MF; rtBUF
*     10 JUL 2013 jcs  Build 26: Schema ; int Publish()
*     11 JUL 2013 jcs  Build 26a:rtEdgeChanStats
*     12 NOV 2014 jcs  Build 28: libmddWire; Logger.CanLog(); RTEDGE_PRIVATE
*     21 JAN 2015 jcs  Build 29: ioctl_getProtocol; <CTL Error="xxx"/>; _wl
*     27 FEB 2015 jcs  Build 30: ioctl_setHeartbeat
*      6 JUL 2015 jcs  Build 31: ioctl_getFd; Buffer; PubGetData()
*     15 APR 2016 jcs  Build 32: EDG_Internal.h; On1SecTimer(); OnQuery()
*     12 SEP 2016 jcs  Build 33: ioctl_userDefStreamID
*     25 MAY 2017 jcs  Build 34: GetSchema() : Handle !fl._nAlloc (empty); UDP
*     12 SEP 2017 jcs  Build 35: No mo GLHashMap 
*      1 NOV 2017 jcs  Build 37: _PubPermQryRsp() : mddPub_BuildRawMsg 
*      6 DEC 2018 jcs  Build 41: VS2017 : size_t not long
*     12 FEB 2020 jcs  Build 42: Socket._tHbeat
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;

// Supported Queries - Connectionless Publisher

static const char *_SymList  = "<SymList/>\n";
static const char *_GetCache = "<GetCache/>\n";


/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      P u b C h a n n e l
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
PubChannel::PubChannel( rtEdgePubAttr  attr,
                        rtEdge_Context cxt ) :
   Socket( attr._pSvrHosts, attr._bConnectionless ? true : false ),
   _cxt( cxt ),
   _schemaCbk( (rtEdgeDataFcn)0 ),
   _schema( (rtEdgeData *)0 ),
   _pub( attr._pPubName ),
   _authReq(),
   _authRsp( _mdd_pNoAuth ),
   _wlByName(),
   _wlByID(),
   _mddXml( ::mddSub_Initialize() ),
   _hopCnt( 0 ),
   _bUserMsgTy( false ),
   _bPerm( false ),
   _tLastMsgRX( 0 ),
   _preBuilt( (rtPreBuiltBUF *)0 )
{
   _mdd  = ::mddPub_Initialize();
   _attr = attr;
   if ( attr._bConnectionless ) {
      _udpPort = attr._udpPort;
      ::mddWire_SetProtocol( _mdd, mddProto_Binary );
   }
   pump().AddIdle( PubChannel::_OnIdle, this );
}

PubChannel::~PubChannel()
{
   WatchListByName          &ndb = _wlByName;
   WatchListByName::iterator nt;

   for ( nt=ndb.begin(); nt!=ndb.end(); delete (*nt++).second );
   ndb.clear();
   _wlByID.clear();
   ::mddSub_Destroy( _mddXml );
   _ClearSchema();
   if ( _log )
      _log->logT( 3, "~PubChannel( %s )\n", dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
rtEdgePubAttr PubChannel::attr()
{
   return _attr;
}

rtEdgeData PubChannel::GetSchema()
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
   nf  = gmin( fl._nAlloc, fl._nFld );  // Empty Schema
   for ( i=0; i<nf; i++ ) {
      m       = mdb[i];
      f._fid  = m._fid;
      b._data = (char *)m._name;
      b._dLen = strlen( b._data );
      f._name = m._name;
      f._type = (rtFldType)m._type;
      fdb[i]  = f;
   }
   _schema->_flds = fdb;
   _schema->_nFld = nf;
   return *_schema;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
void PubChannel::InitSchema( rtEdgeDataFcn schemaCbk )
{
   _schemaCbk = schemaCbk;
}

int PubChannel::Publish( rtEdgeData &d )
{
   Locker           lck( _mtx );
   rtEdgeChanStats &st = stats();
   mddBuf           b, pb;
   mddMsgHdr        h;
   mddMsgType       mt;
   mddFieldList     fl;
   struct timeval   tv;
   PubRec          *pub;
   bool             bImg;
   int              nf, rtn, strID;

   // Pre-condition(s)

   if ( d._ty == edg_permQuery )
      return _PubPermQryRsp( d );
   if ( !d._nFld && !_preBuilt )
      return 0;

   /*
    * Find PubRec from cache and figure out msg type:
    *   1) If not found, create iff broadcast; Else done
    *   2) Image if 1st message
    *   3) > 1st msg : Update, unless _nOpn > _nImg
    */
   strID = (PTRSZ)d._arg;
   pub   = _GetRec( d._pTkr, strID );
   if ( !pub && !_attr._bInteractive )
      pub = _Open( d._pTkr, strID );  // broadcast channel
   if ( !pub )
      return 0;
   bImg = False;
   if ( _bUserMsgTy )
      bImg = ( d._ty == edg_image );
   else if ( !pub->_nUpd )
      bImg = True;
   else if ( d._ty == edg_image )
      bImg = ( pub->_nImg < pub->_nOpn );
   mt          = bImg ? mddMt_image : mddMt_update;
   pub->_nImg += bImg ? 1 : 0;
   pub->_nUpd += 1;

   // OK to fill in mddBufHdr

   h       = _InitHdr( mt );
   h._iTag = strID;
   h._svc  = _SetBuf( d._pSvc );
   h._tkr  = _SetBuf( d._pTkr );
   h._RTL  = pub->_nUpd;
   h._dt   = mddDt_FieldList;

   // Fields, then build

   if ( _preBuilt ) {
      h._dt     = _preBuilt->_dataType;
      pb        = _preBuilt->_payload;
      b         = ::mddPub_BuildRawMsg( _mdd, h, pb, &_bldBuf );
      _preBuilt = (rtPreBuiltBUF *)0;
   }
   else {
      fl._flds = (mddField *)d._flds;
      fl._nFld = d._nFld;
      nf       = ::mddPub_AddFieldList( _mdd, fl );
      b        = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   }

   // Write, stats, return bytes written

   rtn           = Write( b._data, b._dLen ) ? b._dLen : 0;
   tv            = _tvNow();
   st._lastMsg   = tv.tv_sec;
   st._lastMsgUs = tv.tv_usec;
   st._nMsg     += 1;
   st._nByte    += b._dLen;
   st._nImg     += bImg ? 1 : 0;
   st._nUpd     += bImg ? 0 : 1;
   return rtn;
}

int PubChannel::PubError( rtEdgeData &d, const char *err )
{
   Locker           lck( _mtx );
   rtEdgeChanStats &st   = stats();
   mddBuf           b;
   mddMsgHdr        h;
   struct timeval   tv;
   PubRec          *pub;
   char            *cp;
   int              rtn, strID;

   /*
    * Find PubRec from cache and figure out msg type:
    *   1) If not found, create iff broadcast; Else done
    *   2) Image if 1st message
    *   3) > 1st msg : Update, unless _nOpn > _nImg
    */
   strID = (PTRSZ)d._arg;
   pub   = _GetRec( d._pTkr, strID );
   if ( !pub && !_attr._bInteractive )
      pub = _Open( d._pTkr, strID );  // broadcast channel
   if ( !pub )
      return 0;
   pub->_nUpd += 1;

   // mddBufHdr

   h       = _InitHdr( mddMt_dead );
   h._iTag = strID;
   h._svc  = _SetBuf( d._pSvc );
   h._tkr  = _SetBuf( d._pTkr );
   h._RTL  = pub->_nUpd;
   h._dt   = mddDt_FieldList;

   // Error, then build

   cp    = h._tag;
   sprintf( cp, "-1 : %s", d._pErr );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );

   // Write, stats, return bytes written

   rtn           = Write( b._data, b._dLen ) ? b._dLen : 0;
   tv            = _tvNow();
   st._lastMsg   = tv.tv_sec;
   st._lastMsgUs = tv.tv_usec;
   st._nMsg     += 1;
   st._nDead    += 1;
   st._nByte    += b._dLen;
   return rtn;
}

rtBUF PubChannel::PubGetData()
{
   rtBUF b;

   b._data = _bldBuf._data;
   b._dLen = _bldBuf._dLen;
   return b;
}

int PubChannel::_PubStreamSymList()
{
   Locker                    lck( _mtx );
   WatchListByName          &ndb = _wlByName;
   WatchListByName::iterator it;
   PubRec                   *rec;
   int                       n, nw;

   for ( n=0,it=ndb.begin(); it!=ndb.end(); it++ ) {
      rec = (*it).second;
      nw  = _PubStreamID( rec->pTkr(), rec->_StreamID );
      n  += nw ? 1 : 0;
   }
   return n;
}

void PubChannel::_GetStreamCache()
{
   Locker                    lck( _mtx );
   WatchListByName          &ndb = _wlByName;
   WatchListByName::iterator it;
   PubRec                   *rec;
   int                       n;

   _PubStreamSymList();
   for ( n=0,it=ndb.begin(); it!=ndb.end(); it++ ) {
      rec = (*it).second;
      if ( _attr._imgQryCbk )
         (*_attr._imgQryCbk)( _cxt, rec->pTkr(), (VOID_PTR)rec->_StreamID );
   }
}

int PubChannel::_PubStreamID( const char *tkr, int StreamID )
{
   Locker       lck( _mtx );
   mddBuf       b;
   mddMsgHdr    h;
   mddField     flds[K];
   mddFieldList fl;
   int          nf, nw;

   // mddMt_dbTable w/ 2 fields : _FID_STREAMID and _FID_TICKER

   mddWire_InitFieldList( fl, flds, K );
   h       = _InitHdr( mddMt_dbTable );
   h._iTag = StreamID;
   h._svc  = _SetBuf( _pub.data() );
   h._tkr  = _SetBuf( tkr );
   h._RTL  = 0;
   h._dt   = mddDt_FieldList;
   mddWire_AddInt32( fl, _FID_STREAMID, StreamID ); 
   mddWire_AddStringZ( fl, _FID_TICKER, (char *)tkr );
   nf = ::mddPub_AddFieldList( _mdd, fl );
   b  = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );

   // Write, stats, return bytes written

   nw =  Write( b._data, b._dLen ) ? b._dLen : 0;
   return nw;
}

static const char *_NotEmpty( const char *str )
{
   return strlen( str ) ? str : " ";
}
 
int PubChannel::_PubPermQryRsp( rtEdgeData &d )
{
   rtFIELD     f;
   rtBUF      &b   = f._val._buf;
   u_int      &i32 = f._val._i32;
   u_char     &i8  = f._val._i8;
   const char *svc, *tkr, *usr, *loc, *err;
   int         i, reqID;
   u_char      bAck;
   char       *cp, rsp[K];

   /*
    * 1) Special packing in rtEdgeData; 1st 7 fields:
    *       svc   (str)
    *       tkr   (str)
    *       usr   (str)
    *       loc   (str)
    *       err   (str)
    *       reqID (i16)
    *       bAck  (i8)
    */
   svc   = tkr = usr = loc = err = "";
   reqID = 0;
   bAck  = false;
   for ( i=0; i<7; i++ ) {
      f = d._flds[i];
      svc   = ( i == 0 ) ? b._data : svc;
      tkr   = ( i == 1 ) ? b._data : tkr;
      usr   = ( i == 2 ) ? b._data : usr;
      loc   = ( i == 3 ) ? b._data : loc;
      err   = ( i == 4 ) ? b._data : err;
      reqID = ( i == 5 ) ? i32     : reqID;
      bAck  = ( i == 6 ) ? i8      : bAck;
   }

   // 2) Build Payload : MATCH w/ GLmdXmlSink._CrackPermResp()

   d._pSvc = tkr;
   cp      = rsp;
   cp     += sprintf( cp, "%s|", _NotEmpty( svc ) );
   cp     += sprintf( cp, "%s|", _NotEmpty( tkr ) );
   cp     += sprintf( cp, "%s|", _NotEmpty( usr ) );
   cp     += sprintf( cp, "%s|", _NotEmpty( loc ) );
   cp     += sprintf( cp, "%s|", _NotEmpty( err ) );
   cp     += sprintf( cp, "%d|", reqID );
   cp     += sprintf( cp, "%s|", bAck ? _mdd_pYes : _mdd_pNo );

   // 3) Publish response

   mddBuf    pb, r;
   mddMsgHdr h; 

   pb._data = rsp;
   pb._dLen = ( cp - rsp );
   h        = _InitHdr( mddMt_ctl );
   h._iTag  = reqID;
   h._svc   = _SetBuf( svc );
   h._tkr   = _SetBuf( tkr );
   h._RTL   = 1;
   h._dt    = mddDt_FixedMsg;  // Doesn't matter really
   r        = ::mddPub_BuildRawMsg( _mdd, h, pb, &_bldBuf );
   Write( r._data, r._dLen );
   return r._dLen;
}


////////////////////////////////////////////
// Socket Interface      
////////////////////////////////////////////
bool PubChannel::Ioctl( rtEdgeIoctl ctl, void *arg )
{
   size_t lVal;
   int   *iArg;
   bool  *pbArg;
   char  *pArg;
   bool   bArg;

   // 1) Base Socket Class??

   if ( Socket::Ioctl( ctl, arg ) )
      return true;

   // 2) Else us

   lVal  = (size_t)arg;
   iArg  = (int *)arg;
   pbArg = (bool *)arg;
   bArg  = ( arg != (void *)0 );
   pArg  = (char *)arg;
   switch( ctl ) {
      case ioctl_setPubHopCount:
         _hopCnt = (int)lVal;
         return true;
      case ioctl_getPubHopCount:
         *iArg = _hopCnt;
         return true;
      case ioctl_setPubDataPayload:
         _preBuilt = (rtPreBuiltBUF *)arg;
         return true;
      case ioctl_isSnapChan:
         *pbArg = false;
         return true;
      case ioctl_setUserPubMsgTy:
         _bUserMsgTy = bArg;
         return true;
      case ioctl_setPubAuthToken:
         _authReq = pArg;
         return true;
      case ioctl_getPubAuthToken:
         strcpy( pArg, _authRsp.data() );
         return true;
      case ioctl_enablePerm:
         _bPerm = bArg;
         return true;
      default:
         break;
   }
   return false;
}


////////////////////////////////////////////
// Thread Notifications
////////////////////////////////////////////
void PubChannel::OnConnect( const char *pc )
{
   mddMsgHdr   h;
   mddBuf      b;
   char        hop[K], *pa, *pv;
   int         i;
   const char *pSrc;

   // UDP??

   if ( _bConnectionless ) {
      if ( _attr._connCbk )
         (*_attr._connCbk)( _cxt, pc, edg_up );
      return;
   }

   // Mount string : BROADCAST or Interactive; Cache always

   _tLastMsgRX = _tvNow().tv_sec;
   pSrc = _pub.data();
   h = _InitHdr( mddMt_mount );
   _AddAttr( h, _mdd_pAttrName, (char *)pSrc );
   _AddAttr( h, _mdd_pAttrBcst, _attr._bInteractive ? _mdd_pNo : _mdd_pYes );
   _AddAttr( h, _mdd_pAttrPerm, _bPerm ? _mdd_pYes : _mdd_pNo );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );

   // Attributed : Ping / Data Dict

   const char *attrs[] = { _mdd_pAttrDict, 0 };
   const char *vals[]  = { _mdd_pYes, 0 };

   for ( i=0; attrs[i]; i++ ) {
      h  = _InitHdr( mddMt_ctl );
      pa = (char *)attrs[i];
      pv = (char *)vals[i];
      _AddAttr( h, pa, pv );
      b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
      Write( b._data, b._dLen );
   }

   /*
    * OnSrcReady() : More attributes:
    * 1) Cache    = _bCache ? YES : NO
    * 2) Hop      = _hopCnt
    * 3) SrcReady = <PubName>
    */

   const char *rAttrs[] = { _mdd_pAttrCache, _mdd_pAttrHop, _mdd_pAttrRdy, 0 };
   const char *rVals[]  = { IsCache() ? _mdd_pYes : _mdd_pNo, hop, pSrc, 0 };

   // OnSrcReady()

   sprintf( hop, "%d", _hopCnt );
   h = _InitHdr( mddMt_ctl );
   for ( i=0; rAttrs[i]; i++ ) {
      pa = (char *)rAttrs[i];
      pv = (char *)rVals[i];
      _AddAttr( h, pa, pv );
   }
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );

   // Protocol

   h  = _InitHdr( mddMt_ctl );
   _AddAttr( h, _bBinary ? _mdd_pAttrBin : _mdd_pAttrMF, _mdd_pYes );
   b = ::mddPub_BuildMsg( _mdd, h, &_bldBuf );
   Write( b._data, b._dLen );

   // Notify

   if ( _attr._connCbk )
      (*_attr._connCbk)( _cxt, pc, edg_up );
}

void PubChannel::OnDisconnect( const char *reason )
{
   vector<string> v;
   const char    *pTkr;
   size_t         i, sz;

   // 1) Copy under lock
   {
      Locker                    lck( _mtx );
      WatchListByName          &ndb = _wlByName;
      WatchListByID            &idb = _wlByID;
      WatchListByName::iterator nt;
      WatchListByID::iterator   it;

      for ( nt=ndb.begin(); nt!=ndb.end(); nt++ )
         v.push_back( string( (*nt).first ) );
      for ( it=idb.begin(); it!=idb.end(); delete (*it++).second );
      ndb.clear();
      idb.clear();
   }

   // 2) Notify interested parties ...

   _authRsp = _mdd_pNoAuth;
   ::mddWire_SetProtocol( _mdd, mddProto_XML );
   sz = v.size();
   for ( i=0; _attr._closeCbk && i<v.size(); i++ ) {
      pTkr = v[i].data();
      (*_attr._closeCbk)( _cxt, pTkr );
      _Close( pTkr );
   }
   if ( _attr._connCbk )
      (*_attr._connCbk)( _cxt, reason, edg_down );
}

void PubChannel::OnRead()
{
//   Locker      lck( _mtx );
   mddMsgBuf  b;
   mddMsgHdr  h;
   const char *cp;
   int         i, sz, nMsg, nb, nL;

   // 1) Base class drains channel and populates _in / _cp

   Socket::OnRead();
   if ( _attr._bConnectionless ) {
      _OnMPAC();
      return;
   }

   // 2) OK, now we chop up ...

   cp = _in._bp;
   sz = _in.bufSz();
   for ( i=0,nMsg=0; i<sz; ) {
      b._data = (char *)cp;
      b._dLen = sz - i;
      b._hdr  = (mddMsgHdr *)0;
      h       = _InitHdr( mddMt_undef );
      nb      = ::mddSub_ParseHdr( _mddXml, b, &h );
      b._hdr  = &h;
      if ( !nb )
         break; // for-i
      nMsg++;
      if ( _log && _log->CanLog( 2 ) ) {
         Locker lck( _log->mtx() );
         string s( cp, nb );

         _log->logT( 2, "[XML-RX] (%02d of ,%d) ", nb, sz );
         _log->Write( 2, s.data(), nb );
      }
      OnXML( h );
      cp += nb;
      i  += nb;
   }

   // Message spans 2+ network packets?

   nL = WithinRange( 0, sz-i, INFINITEs );
   if ( nMsg && nL )
      _in.Move( sz-nL, nL );
   _in.Reset();
   _in._cp += nL;
}

void PubChannel::_OnMPAC()
{
   const char *cp;
   int         sz, nc;

   // Stupid XML Protocol : 2 cmds (for now)

   cp = _in._bp;
   sz = _in.bufSz();
   if ( ::strstr( cp, _SymList ) == cp ) {
      nc = _PubStreamSymList();
      if ( _attr._symQryCbk )
         (*_attr._symQryCbk)( _cxt, nc );
   }
   else if ( ::strstr( cp, _GetCache ) == cp )
      _GetStreamCache();
}


////////////////////////////////////////////
// TimerEvent Notifications
////////////////////////////////////////////
void PubChannel::On1SecTimer()
{
   rtEdgeChanStats &st = stats();
   struct timeval   tv = _tvNow();
   int              age;

   // 1) Normal stuff

   Socket::On1SecTimer();
   if ( _bIdleCbk && _attr._openCbk )
      (*_attr._openCbk)( _cxt, (const char *)0, (void *)0 );

   // 2) Connectionless : Send at least once / second

   age = tv.tv_sec - st._lastMsg;
   if ( _bConnectionless && ( age > 1 ) ) {
      _udp._wire_seqNum -= 1;  // Heartbeats are non-sequenced
      _SendPing();
   }

   // 3) Heartbeat

   _CheckHeartbeat( _tLastMsgRX );
}


////////////////////////////////////////////
// PubChannel Notifications
////////////////////////////////////////////
void PubChannel::OnXML( mddMsgHdr &h )
{
   _tLastMsgRX = _tvNow().tv_sec;
   switch( h._mt ) {
      case mddMt_open:  OnPubOpen( h );  break;
      case mddMt_close: OnPubClose( h ); break;
      case mddMt_ctl:   OnCTL( h );      break;
      case mddMt_ping:  _SendPing();     break;
      case mddMt_undef:
      case mddMt_image:
      case mddMt_update:
      case mddMt_stale:
      case mddMt_recovering:
      case mddMt_dead:
         break;
      case mddMt_mount: OnMount( h );    break;
      case mddMt_query: OnQuery( h );    break;
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
void PubChannel::OnPubOpen( mddMsgHdr &h )
{
   rtEdgeChanStats &st = stats();
   const char      *pTkr;
   PubRec          *rec;
   int              StreamID;

   // 1) Pull from XML / _Open()

   pTkr     = _GetAttr( h, _mdd_pAttrName );
   StreamID = h._iTag;

   // 2) Update Watchlist 
   {
      Locker lck( _mtx );
      rec = _Open( pTkr, StreamID );
      if ( rec )
         rec->_nOpn += 1;
   }

   // 3) Stats / Notify

   st._nOpen += 1;
   if ( _attr._openCbk )
      (*_attr._openCbk)( _cxt, pTkr, (VOID_PTR)StreamID );
}

void PubChannel::OnPubClose( mddMsgHdr &h )
{
   rtEdgeChanStats &st = stats();
   const char      *pTkr;

   // 1) Pull from XML / _Close()

   pTkr = _GetAttr( h, _mdd_pAttrName );
   _Close( pTkr );

   // 2) Stats / Notify

   st._nClose += 1;
   if ( _attr._closeCbk )
      (*_attr._closeCbk)( _cxt, pTkr );
}

void PubChannel::OnMount( mddMsgHdr &h )
{
   Locker      lck( _mtx );
   char        buf[K], auth[K], *ap;
   u_char     *mp;
   int         i, j;
   const char *pID, *pAuth;
   const char *chk[] = { _authReq.data(),
                         _mdd_pNoAuth,
                         "Undefined",
                         "undefined",
                         (char *)0 };

   /*
    * <MNT Name         = "ULTRAFEED" 
    *      BROADCAST    = "NO" 
    *      Cache        = "NO" 
    *      LoginID      = "a30f9df41cc8" 
    *      Authenticate = "e563137a5522106355a0eede506ca1d9"/>
    */
   pID   = _GetAttr( h, _mdd_pLoginID );
   pAuth = _GetAttr( h, _mdd_pAuth );
   if ( !strlen( pID ) || !strlen( pAuth ) )
      return;
   for ( i=0; chk[i]; i++ ) {
      GLmd5 md5;

      sprintf( buf, "%s%s", chk[i], pID );
      md5.update( buf, strlen( buf ) );
      mp = md5.digest();
      ap = auth;
      for ( j=0; j<16; ap+=sprintf( ap, "%02x", mp[j++] ) );
      if ( !::strcmp( auth, pAuth ) ) {
         _authRsp = chk[i];
         return;
      }
   }
}

void PubChannel::OnQuery( mddMsgHdr &h )
{
   const char *tuple[8];
   u_int       reqID, i;

   // Pull out ( Servivce, Ticker, User, Location ) tuple in order

   i          = 0;
   tuple[i++] = _GetAttr( h, _mdd_pAttrSvc );
   tuple[i++] = _GetAttr( h, _mdd_pAttrName );
   tuple[i++] = _GetAttr( h, _mdd_pUser );
   tuple[i++] = _GetAttr( h, _mdd_pAuth );
   tuple[i]   = NULL;
   reqID      = atoi( _GetAttr( h, _mdd_pAttrTag ) );

   // Notify Interested Parties

   if ( _attr._permQryCbk )
      (*_attr._permQryCbk)( _cxt, tuple, reqID );
}

void PubChannel::OnCTL( mddMsgHdr &h )
{
   Locker      lck( _mtx );
   bool        bMF, bBin;
   const char *pAttr;

   /*
    * <CTL MF="YES"/>
    * <CTL DataDict="..."/>
    */

   if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrMF )) ) ) {
      bMF    = ( ::strcmp( pAttr, _mdd_pYes ) == 0 );
      _proto = bMF ? mddProto_MF : _proto;
      ::mddWire_SetProtocol( _mdd, _proto );
   }
   else if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrBin )) ) ) {
      bBin   = ( ::strcmp( pAttr, _mdd_pYes ) == 0 );
      _proto = bBin ? ( _bBinary ? mddProto_Binary : mddProto_MF ) : _proto;
      ::mddWire_SetProtocol( _mdd, _proto );
   }
   if ( strlen( (pAttr=_GetAttr( h, _mdd_pAttrDict )) ) ) {
      _ClearSchema();
      ::mddWire_SetSchema( _mdd, pAttr );
      if ( _schemaCbk )
         (*_schemaCbk)( _cxt, GetSchema() );
   }
   else if ( strlen( (pAttr=_GetAttr( h, _mdd_pError )) ) ) {
      OnDisconnect( pAttr );
      Disconnect( pAttr );
   }
breakpointE();
}



////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
PubRec *PubChannel::_Open( const char *tkr, int StreamID )
{
   PubRec *pub;
   string  s( tkr );
   bool    bAdd;

   // 1) Add to internal watchlist

   pub  = _GetRec( tkr, StreamID );
   bAdd = ( !pub && _fd );
   if ( bAdd ) {
      pub               = new PubRec( *this, tkr, StreamID );
      _wlByName[s]      = pub;
      _wlByID[StreamID] = pub;
   }

   // 2) Pump down to Edge3 if UDP

   if ( bAdd && _attr._bConnectionless )
      _PubStreamID( tkr, StreamID );
   return pub;
}

void PubChannel::_Close( const char *pTkr )
{
   Locker                    lck( _mtx );
   PubRec                   *pub;
   string                    s( pTkr );
   int                       sid;
   WatchListByName          &ndb = _wlByName;
   WatchListByID            &idb = _wlByID;
   WatchListByName::iterator nt;
   WatchListByID::iterator   it;

   pub = _GetRec( pTkr, 0 );
   sid = pub ? pub->_StreamID : 0;
   if ( (it=idb.find( sid )) != idb.end() )
      idb.erase( it );
   if ( (nt=ndb.find( s )) != ndb.end() )
      ndb.erase( nt );
   if ( pub )
      delete pub;
}

PubRec *PubChannel::_GetRec( const char *pTkr, int StreamID )
{
   PubRec                   *pub;
   WatchListByName          &ndb = _wlByName;
   WatchListByID            &idb = _wlByID;
   WatchListByName::iterator nt;
   WatchListByID::iterator   it;

   // By StreamID 1st

   pub = (PubRec *)0;
   if ( (it=idb.find( StreamID )) != idb.end() )
      pub = (*it).second;
   else {
      string s( pTkr );

      if ( (nt=ndb.find( s )) != ndb.end() )
         pub = (*nt).second;
   }
   return pub;
}

void PubChannel::_ClearSchema()
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
void PubChannel::OnIdle()
{
breakpointE(); // TODO : Re-connect ...
}

void PubChannel::_OnIdle( void *arg )
{
   PubChannel *us;

   us = (PubChannel *)arg;
   us->OnIdle();
}



/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      P u b R e c
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
PubRec::PubRec( PubChannel &pub, 
                const char *tkr,
                int         StreamID ) :
   _pub( pub ),
   _tkr( tkr ),
   _StreamID( StreamID ),
   _nOpn( 0 ),
   _nImg( 0 ),
   _nUpd( 0 )
{
}

PubRec::~PubRec()
{
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
const char *PubRec::pSvc()
{
   return _pub.attr()._pPubName;
}

const char *PubRec::pTkr()
{
   return _tkr.data();
}
