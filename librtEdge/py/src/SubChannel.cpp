/******************************************************************************
*
*  SubChannel.cpp
*     MD-Direct Subscription Channel
*
*  REVISION HISTORY:
*     19 MAR 2016 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     20 NOV 2020 jcs  Build  2: OnStreamDone()
*      1 DEC 2020 jcs  Build  3: SnapTape() / PyTapeSnapQry
*      3 FEB 2022 jcs  Build  5: PyList, not PyTuple
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>

using namespace MDDPY;

#define INT_PTR int)(size_t

////////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      M D D p y S u b C h a n
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
MDDpySubChan::MDDpySubChan( const char *host, const char *user, bool bBin ) :
   RTEDGE::SubChannel(),
   _pmp( *this ),
   _host( host ),
   _user( user ),
   _tapeId2Arg(),
   _bStrByOid(),
   _bStrByName(),
   _py_schema( new MDDPY::Schema() ),
   _byName(),
   _byOid(),
   _mtx(),
   _iFilter( EVT_ALL ),
   _bRTD( true ),
   _snap( (PyTapeSnap *)0 ),
   _bTapeUpd( false )
{
   SetBinary( bBin );
}

MDDpySubChan::~MDDpySubChan()
{
   ByteStreamByOid          &bdb = _bStrByOid;
   ByteStreamByName         &ndb = _bStrByName;
   ByteStreamByOid::iterator it;
   PyByteStream             *bStr;

   // Stop streams; Clear out guts

   Stop();
   for ( it=bdb.begin(); it!=bdb.end(); it++ ) {
      bStr = (*it).second; 
      delete bStr; 
   }
   bdb.clear();
   ndb.clear();
   delete _py_schema;
}


///////////////////////////////
// Operations
///////////////////////////////
const char *MDDpySubChan::Protocol()
{
   const char *ty;
   mddProtocol pro;
   
   pro = GetProtocol();
   ty  = "???";
   switch( pro ) {
      case mddProto_Undef:  ty = "???"; break;
      case mddProto_Binary: ty = "BIN"; break;
      case mddProto_MF:     ty = "MF";  break;
      case mddProto_XML:    ty = "XML"; break;
   }
   return ty;
}

int MDDpySubChan::Open( const char *svc,
                        const char *tkr,
                        int         userID )
{
   RTEDGE::Locker l( _mtx );
   BookByOid     &bdb = _byOid;
   BookByName    &ndb = _byName;
   string         s( _Key( svc, tkr ) );
   Book          *bk;
   Update         u = _INIT_MDDPY_UPD;
   int            oid, nf;

   // Pre-condition(s)

   if ( (bk=FindBook( svc, tkr )) ) {
      if ( !(nf=bk->TouchAllFields()) )
         return true;
      u._mt  = EVT_UPD;
      u._bk  = bk;
      _pmp.Add( u );
      return true;
   }

   // Safe to create and add to cache

   oid      = ReqID();
   bk       = new Book( svc, tkr, userID, oid );
   ndb[s]   = bk;
   bdb[oid] = bk;
// printf( "Subscribe( %s,%s,%d )\n", svc, tkr, oid ); fflush( stdout );
   return Subscribe( svc, tkr, (VOID_PTR)oid );
}

int MDDpySubChan::OpenByteStream( const char *svc, 
                                const char *tkr, 
                                int         userID )
{
   RTEDGE::Locker    l( _mtx );
   ByteStreamByOid  &bdb = _bStrByOid;
   ByteStreamByName &ndb = _bStrByName;
   string            s( _Key( svc, tkr ) );
   PyByteStream     *bStr;
   int               sid;

   // ByteStream Defaults all the way thru ...
 
   bStr            = new PyByteStream( *this, svc, tkr, userID );
   sid             = Subscribe( *bStr );
   bStr->_StreamID = sid;
   bdb[sid]        = bStr;
   ndb[s]          = bStr;
   return sid;
}

Book *MDDpySubChan::FindBook( const char *pSvc, const char *pTkr )
{
   RTEDGE::Locker       l( _mtx );
   BookByName::iterator it;
   string               s( _Key( pSvc, pTkr ) );
   Book                *bk;

   bk = (Book *)0;
   if ( (it=_byName.find( s )) != _byName.end() )
      bk = (*it).second;
   return bk;
}

Update MDDpySubChan::ToUpdate( rtMsg &m )
{
   Update      u = _INIT_MDDPY_UPD;
#ifdef TODO
   mddWireMsg  w;
   bool        bErr = ( m._err != (string *)0 );
   const char *pErr = bErr ? m._err->data() : NULL;
   rtFIELD    *fdb;
   int         nf;

   if ( pErr ) {
      fdb = (rtFIELD *)0;
      nf  = 0;
   }
   else {
      m._hdr  = (mddMsgHdr *)0;
      w._flds = _fl;
      ::MDDpySub_ParseMsg( _mdd, m, &w );
      _fl = w._flds;
      nf  = _fl._nFld;
      fdb = (rtFIELD *)_fl._flds;
   }
   return _ToUpdate( fdb, nf, m._oid, pErr, bErr );
#endif // TODO
   return u;
}

int MDDpySubChan::Close( const char *pSvc, const char *pTkr )
{
   RTEDGE::Locker             l( _mtx );
   ByteStreamByOid           &bdb = _bStrByOid;
   ByteStreamByName          &ndb = _bStrByName;
   ByteStreamByOid::iterator  bt;
   ByteStreamByName::iterator nt;
   string                     s( _Key( pSvc, pTkr ) );
   PyByteStream              *bStr;
   int                        sid;

   // 1) Normal

   sid = Unsubscribe( pSvc, pTkr );

   // 2) ByteStream ...

   if ( (nt=ndb.find( s )) != ndb.end() ) {
      bStr = (*nt).second;
      sid  = bStr->_StreamID;
      if ( (bt=bdb.find( sid )) != bdb.end() ) 
         bdb.erase( bt );
      ndb.erase( nt );
      Unsubscribe( *bStr );
      delete bStr;
   }
   return sid;
}

PyObject *MDDpySubChan::Filter( int iFilter )
{
   // Set filter; Drain

   _iFilter = iFilter;
   _pmp.Drain( iFilter );
   return PyInt_FromLong( iFilter );
}

PyObject *MDDpySubChan::Read( double dWait )
{
   PyObject *rtn;

   // First check if there is data

   if ( (rtn=_Get1stUpd()) != Py_None )
      return rtn;

   // Safe to wait for it

   _pmp.Wait( dWait );
   return _Get1stUpd();
}

PyObject *MDDpySubChan::GetData( const char *pSvc,
                                 const char *pTkr,
                                 int        *fids )
{
   RTEDGE::Locker l( _mtx );
   Book          *bk;
   PyObject      *rtn, *pyV;
   MDDPY::Field  *fld;
   char          *pc;
   int            i, nf;

   // !fids[i] implies end of FID request list

   for ( nf=0; fids[nf]; nf++ );
   bk  = FindBook( pSvc, pTkr );
   rtn = bk ? ::PyList_New( nf ) : Py_None;
   for ( i=0; bk && i<nf; i++ ) {
      fld = bk->GetField( fids[i] );
      pc  = fld ? fld->data() : (char *)"";
      pyV = PyString_FromString( pc );
      ::PyList_SetItem( rtn, i, pyV );
   }
   return rtn;
}

PyObject *MDDpySubChan::QueryTape()
{
   PyObject   *rc, *kv;
   ::MDDResult res;
   ::MDDRecDef rd;
   int         i, nr, nm;

   // Query

   rc  = (PyObject *)0;
   res = Query();
   if ( (nr=res._nRec) ) {
      rc = ::PyList_New( nr );
      for ( i=0; i<nr; i++) {
         rd = res._recs[i];
         nm = rd._interval; // NumMsg in tape
         kv = ::PyList_New( 3 );
         ::PyList_SetItem( kv, 0, PyString_FromString( rd._pSvc ) );
         ::PyList_SetItem( kv, 1, PyString_FromString( rd._pTkr ) );
         ::PyList_SetItem( kv, 2, PyInt_FromLong( nm ) );
         ::PyList_SetItem( rc, i, kv );
      }
   }
   FreeResult();
   return rc;
}

PyObject *MDDpySubChan::SnapTape( PyTapeSnapQry &qry )
{
   PyTapeSnap snp( *this, qry );
   PyObjects &ldb = snp._objs;
   PyObject  *rc;
   double     d0, age;
   size_t     i, n;
   int        tInt;

   // Pre-condition

   if ( !IsTape() )
      return (PyObject *)0; 
   /*
    * Sleep in 10 milli chunks
    */
   _snap = &snp;
   Subscribe( qry._svc, qry._tkr, 0 );
   if ( qry._t0 && qry._t1 ) {
      if ( (tInt=qry._tSample) && qry._flds ) {
         SetTapeDirection( false );
         StartTapeSliceSample( qry._t0, qry._t1, tInt, qry._flds );
      }
      else
         StartTapeSlice( qry._t0, qry._t1 );
   }
   else
      StartTape();
   d0  = TimeNs();
   age = 0.0;
   for ( ; age<qry._tmout && !snp._bDone; ) {
      Sleep( 0.010 );
      age = TimeNs() - d0;
   }
   /*
    * Format result as PyList
    */
   RTEDGE::Locker l( _mtx );

   _snap = (PyTapeSnap *)0;
   n     = ldb.size();
   rc    = n ?  ::PyList_New( n ) : (PyObject *)0;
   for ( i=0; rc && i<n; ::PyList_SetItem( rc, i, ldb[i] ), i++ );
   return rc;
}


///////////////////////////////
// RTEDGE::SubChannel Notifications
///////////////////////////////
void MDDpySubChan::OnConnect( const char *pConn, bool bUp )
{
   const char *ty;
   char        buf[K];
   Update      u = _INIT_MDDPY_UPD;
   int         mt;

   // Pre-condition

   if ( !( (mt=EVT_CONN) & _iFilter ) )
      return;

   // Update queue

   ty  = bUp ?  "UP  " : "DOWN";
   sprintf( buf, "%s|%s", ty, pConn );
   u._mt  = mt;
   u._msg = new string( buf );
   _pmp.Add( u );
}

void MDDpySubChan::OnService( const char *pSvc, bool bUp )
{
   const char *ty;
   char        buf[K];
   Update      u = _INIT_MDDPY_UPD;
   int         mt;

   // Pre-condition(s)

   if ( !::strcmp( pSvc, _pGblSts ) )
      return;
   if ( !( (mt=EVT_SVC) & _iFilter ) )
      return;

   // Update queue

   ty  = bUp ?  "UP  " : "DOWN";
   sprintf( buf, "%s|%s", ty, pSvc );
   u._mt  = mt;
   u._msg = new string( buf );
   _pmp.Add( u );
}

void MDDpySubChan::OnData( RTEDGE::Message &msg )
{
   IntMap::iterator it;
   Book            *bk;
   IntMap          &idb = _tapeId2Arg;
   int              oid = (INT_PTR)msg.arg();
   int              sid = msg.StreamID();
   rtBUF            raw = msg.rawData();
   Update           u   = _INIT_MDDPY_UPD;
   bool             bAdd;
   int              i;

   /*
    * TapeChannel::_PumpOneMsg() doesn't keep rtEdgeData._arg 
    *   Hence, we store map of StreamID vs userArg 
    */
   if ( IsTape() ) {
      RTEDGE::Locker l( _mtx );

      if ( _snap ) {
         _snap->OnData( msg );
         return;
      }
      else if ( (it=idb.find( sid )) != idb.end() )
         oid = (*it).second;
      else if ( (bk=FindBook( msg.Service(), msg.Ticker() )) ) {
         oid      = bk->oid();
         idb[sid] = oid;
      }
      else
         m_breakpoint();
   }

   // Unconflated

   if ( !_bRTD ) {
      _pmp.Add( new rtMsg( raw._data, raw._dLen, oid ) );
      return;
   }
 
   // Conflated / Filtered : Update queue

   u         = _ToUpdate( oid, msg );
   bAdd      = ( ( u._mt & _iFilter ) && u._bk && !u._nUpd );
   _bTapeUpd = IsTape();
   if (  bAdd || _bTapeUpd )
      _pmp.Add( u );
   _pmp.Notify();
   /*
    * Wait for Python thread to drain; Check every 25 millis
    */
   for ( i=0; _bTapeUpd && !IsStopping(); Sleep( 0.025 ), i++ );
}

void MDDpySubChan::OnRecovering( RTEDGE::Message &msg )
{
   RTEDGE::Locker l( _mtx );
   int            oid = (INT_PTR)msg.arg();
   Update         u   = _INIT_MDDPY_UPD;

   u      = _ToUpdate( oid, msg.Error() );
   u._mt  = EVT_RECOV;
   _pmp.Add( u );
   _pmp.Notify();
}

void MDDpySubChan::OnStreamDone( RTEDGE::Message &msg )
{
   RTEDGE::Locker l( _mtx );
   int            oid = (INT_PTR)msg.arg();
   Update         u   = _INIT_MDDPY_UPD;

   // Tape Snapshot?

   if ( _snap ) {
      _snap->OnStreamDone( msg );
      return;
   }

   // Normal update

   u      = _ToUpdate( oid, "Stream Done" );
   u._mt  = EVT_DONE;
   _pmp.Add( u );
   _pmp.Notify();
}

void MDDpySubChan::OnDead( RTEDGE::Message &msg, const char *pErr )
{
   RTEDGE::Locker l( _mtx );
   int            oid = (INT_PTR)msg.arg();
   Update         u   = _INIT_MDDPY_UPD;

   // Unconflated

   if ( !_bRTD ) {
      _pmp.Add( new rtMsg( pErr, oid ) );
      return;
   }

   // Conflated / Filtered : Update queue

   u = _ToUpdate( oid, pErr );
   if ( ( u._mt & _iFilter ) && u._bk && !u._nUpd )
      _pmp.Add( u );
   _pmp.Notify();
}

void MDDpySubChan::OnSchema( RTEDGE::Schema &sch )
{
   rtEdgeData    d;
   mddFieldList *fl = sch.Get();
   Update        u  = _INIT_MDDPY_UPD;

   // Schema.Update( rtEdgeData ) only cares about fields ...

   d._flds = (rtFIELD *)fl->_flds;
   d._nFld = sch.Size();

   // Update Schema; Then place on queue

   _py_schema->Update( d );
   if ( EVT_SCHEMA & _iFilter ) {
      ::memset( &u, 0, sizeof( u ) );
      u._mt  = EVT_SCHEMA;
      _pmp.Add( u );
   }
   _pmp.Notify();
}


///////////////////////////////
// ByteStream Notifications
///////////////////////////////
void MDDpySubChan::OnByteStream( PyByteStream &bStr )
{
   RTEDGE::Locker l( _mtx );
   int            oid = bStr._userID;
   rtBUF          src = bStr.subBuf();
   Update         u   = _INIT_MDDPY_UPD;
   rtBUF          dst;

   // 1) Deep copy of (perhaps) volatile data

   dst._data = new char[src._dLen+4];
   dst._dLen = src._dLen;
   ::memcpy( dst._data, src._data, dst._dLen );

   // 2) Conflated / Filtered : Update queue

   u = _ToUpdate( oid, dst );
   if ( ( u._mt & _iFilter ) && u._bk )
      _pmp.Add( u );
   _pmp.Notify();
}

void MDDpySubChan::OnByteStreamError( PyByteStream &bStr, const char *pErr )
{
   RTEDGE::Locker l( _mtx );
   int            oid = bStr._userID;
   Update         u;

   // Unconflated

   if ( !_bRTD ) {
      _pmp.Add( new rtMsg( pErr, oid ) );
      return;
   }

   // Conflated / Filtered : Update queue

   u = _ToUpdate( oid, pErr );
   if ( ( u._mt & _iFilter ) && u._bk && !u._nUpd )
      _pmp.Add( u );
   _pmp.Notify();
}


///////////////////////////////
// Helpers
///////////////////////////////
Update MDDpySubChan::_ToUpdate( int oid, RTEDGE::Message &msg )
{
   RTEDGE::Locker      l( _mtx );
   BookByOid          &bdb = _byOid;
   BookByOid::iterator it;
   mddFieldList        fl;
   Book               *bk;
   Update              u = _INIT_MDDPY_UPD;
   int                 mt, nUpd;

   // Update Cache : Find by oid

   fl._flds = (mddField *)msg.Fields();
   fl._nFld = msg.NumFields();
   bk       = (Book *)0;
   mt       = EVT_UPD;
   nUpd     = 0;
   if ( (it=bdb.find( oid )) != bdb.end() ) {
      bk   = (*it).second;
      nUpd = bk->_nUpd;
      bk->Update( fl );
   }

   // Update

   u._mt   = mt;
   u._bk   = bk;
   u._nUpd = nUpd;
   u._tMsg = msg.MsgTime();
   return u;
}

Update MDDpySubChan::_ToUpdate( int oid, rtBUF bStr )
{
   RTEDGE::Locker      l( _mtx );
   BookByOid          &bdb = _byOid;
   BookByOid::iterator it;
   Book               *bk;
   Update              u = _INIT_MDDPY_UPD;
   int                 mt, nUpd;

   // Update Cache : Find by oid

   bk   = (Book *)0;
   mt   = EVT_BYSTR;
   nUpd = 0;
   if ( (it=bdb.find( oid )) != bdb.end() )
      bk = (*it).second;

   // Update

   u._mt   = mt;
   u._bk   = bk;
   u._nUpd = nUpd;
   u._bStr = bStr;
   return u;
}

Update MDDpySubChan::_ToUpdate( int oid, const char *pErr )
{
   RTEDGE::Locker      l( _mtx );
   BookByOid          &bdb = _byOid;
   BookByOid::iterator it;
   Book               *bk;
   Update              u = _INIT_MDDPY_UPD;
   int                 mt, nUpd;

   // Update Cache : Find by oid

   bk   = (Book *)0;
   mt   = EVT_STS;
   nUpd = 0;
   if ( (it=bdb.find( oid )) != bdb.end() )
      bk   = (*it).second;

   // Update

   u._mt   = mt;
   u._bk   = bk;
   u._msg  = new string( pErr );
   u._nUpd = nUpd;
   return u;
}

PyObject *MDDpySubChan::_Get1stUpd()
{
   RTEDGE::Locker l( _mtx );
   PyObjects      v;
   PyObject      *rtn, *pym, *pyd, *pyByt;
   rtBUF          b;
   Book          *bk;
   string        *s;
   Update         upd;
   const char    *ps, *svc, *tkr;
   int            i, n, nAgg, uoid;

   // Pull 1st update off _pmp queue

   if ( !_pmp.GetOneUpd( upd ) )
      return Py_None;

   /*
    * ( EVT_CONN, pMsg )
    * ( EVT_SVC, pMsg )
    * ( EVT_UPD,    ( <tMsg> <UID> <Svc>, <Tkr>, <nAgg>, <Fld1>, <Fld2>, ... ) )
    * ( EVT_STS,    ( <tMsg> <UID> <Svc>, <Tkr>, <ErrMsg> )
    * ( EVT_RECOV,  ( <tMsg> <UID> <Svc>, <Tkr>, <ErrMsg> )
    * ( EVT_DONE,   ( <tMsg> <UID> <Svc>, <Tkr>, <ErrMsg> )
    * ( EVT_SCHEMA, ( <Field1>, <Field2>, ... ) )
    */
   _bTapeUpd = false;
   switch( upd._mt ) {
      case EVT_UPD:
         /*
          * [ tUpd, oid, Svc, Tkr, nAgg, fld1, fld2, ... ]
          *   where fldN = [ fidN, valN, tyN ]
          */
         bk   = upd._bk;
         uoid = bk->_userReqID;
         nAgg = bk->_nUpd - 1;
         n    = bk->GetUpds( v );
         pyd  = ::PyList_New( n+5 );
         ::PyList_SetItem( pyd, 0, PyFloat_FromDouble( upd._tMsg ) );
         ::PyList_SetItem( pyd, 1, PyInt_FromLong( uoid ) );
         ::PyList_SetItem( pyd, 2, PyString_FromString( bk->pSvc() ) );
         ::PyList_SetItem( pyd, 3, PyString_FromString( bk->pTkr() ) );
         ::PyList_SetItem( pyd, 4, PyInt_FromLong( nAgg ) );
         for ( i=0; i<n; i++ )
            ::PyList_SetItem( pyd, i+5, v[i] );
         bk->_nUpd = 0;
         break;
      case EVT_BYSTR:
         /*
          * [ tUpd, oid, Svc, Tkr, bytestream ]
          */
         bk    = upd._bk;
         b     = upd._bStr;
         uoid  = bk->_userReqID;
         nAgg  = bk->_nUpd - 1;
         n     = bk->GetUpds( v );
         pyd   = ::PyList_New( 5 );
         pyByt = PyString_FromStringAndSize( b._data, b._dLen );
         ::PyList_SetItem( pyd, 0, PyFloat_FromDouble( upd._tMsg ) );
         ::PyList_SetItem( pyd, 1, PyInt_FromLong( uoid ) );
         ::PyList_SetItem( pyd, 2, PyString_FromString( bk->pSvc() ) );
         ::PyList_SetItem( pyd, 3, PyString_FromString( bk->pTkr() ) );
         ::PyList_SetItem( pyd, 4, pyByt );
         delete[] b._data;
         break;
      case EVT_STS:
      case EVT_RECOV:
      case EVT_DONE:
         /*
          * [ tUpd, oid, Svc, Tkr, sts ]
          */
         bk   = upd._bk;
         uoid = bk ? bk->_userReqID : -1;
         svc  = bk ? bk->pSvc() : "None";
         tkr  = bk ? bk->pTkr() : "None";
         s    = upd._msg;
         ps   = s->data();
         pyd = ::PyList_New( 5 );
         ::PyList_SetItem( pyd, 0, PyFloat_FromDouble( upd._tMsg ) );
         ::PyList_SetItem( pyd, 1, PyInt_FromLong( uoid ) );
         ::PyList_SetItem( pyd, 2, PyString_FromString( svc ) );
         ::PyList_SetItem( pyd, 3, PyString_FromString( tkr ) );
         ::PyList_SetItem( pyd, 4, PyString_FromString( ps ) );
         delete s;
         break;
      case EVT_CONN:
      case EVT_SVC:
         s   = upd._msg;
         ps  = s->data();
         pyd = PyString_FromString( ps );
         delete s;
         break;
      case EVT_SCHEMA:
         n    = _py_schema->GetUpds( v );
         pyd  = ::PyList_New( n );
         for ( i=0; i<n; i++ )
            ::PyList_SetItem( pyd, i, v[i] );
         break;
      default:
         pyd = PyString_FromString( "Unknown msg type" );
         break;
   }
   pym = PyInt_FromLong( upd._mt );
   rtn = ::PyTuple_Pack( 2, pym, pyd );
   return rtn;
}


///////////////////////////////
// Class-wide
///////////////////////////////
int MDDpySubChan::ReqID()
{
   static long _reqID = 1;

   return ATOMIC_INC( &_reqID );
}

string MDDpySubChan::_Key( const char *pSvc, const char *pTkr )
{
   string rtn( pSvc );

   rtn += "|";
   rtn += pTkr;
   return rtn;
}



////////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      P y B y t e S t r e a m
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
PyByteStream::PyByteStream( MDDpySubChan &ch, 
                            const char *svc,
                            const char *tkr,
                            int         userID ) :
   RTEDGE::ByteStream( svc, tkr ),  // Defaults all the way thru ...
   _ch( ch ),
   _userID( userID ),
   _StreamID( 0 )
{
}

PyByteStream::~PyByteStream()
{
}


///////////////////////////////
// RTEDGE::ByteStream Notifications
///////////////////////////////
void PyByteStream::OnData( rtBUF )
{
   // Don't care until it's done ...
}

void PyByteStream::OnError( const char *err )
{
   _ch.OnByteStreamError( *this, err );
}

void PyByteStream::OnSubscribeComplete()
{
   _ch.OnByteStream( *this );
}


////////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      P y T a p e S n a p
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
PyTapeSnap::PyTapeSnap( MDDpySubChan &ch, PyTapeSnapQry &qry ) :
   _ch( ch ),
   _svc( qry._svc ),
   _tkr( qry._tkr ),
   _fids(),
   _maxRow( qry._maxRow ),
   _objs(),
   _bDone( false ),
   _uFld()
{
   RTEDGE::Schema   &sch = _ch.schema();
   RTEDGE::FieldDef *fDef;
   string            s( qry._flds );
   char             *cp, *rp;
   int               fid;

   // Tokenize Fields

   cp = ::strtok_r( (char *)s.data(), ",", &rp );
   for ( ; cp; cp=::strtok_r( NULL, ",", &rp ) ) {
      if ( (fid=atoi( cp )) )
         _fids.push_back( fid );
      else if ( (fDef=sch.GetDef( cp )) ) {
         fid = fDef->Fid();
         _fids.push_back( fid );
      }
   }
}

PyTapeSnap::~PyTapeSnap()
{
}


///////////////////////////////
// RTEDGE::SubChannel Notifications
///////////////////////////////
void PyTapeSnap::OnData( RTEDGE::Message &msg )
{
   rtFIELD         *flds, f;
   rtBUF           &b = f._val._buf;
   mddField        *w;
   PyObject        *upd;
   Fields           fdb;
   Fields::iterator it;
   const char      *pv;
   char             buf[K];
   int              i, n, fid;

   // Pre-condition

   if ( (int)_objs.size() >= _maxRow ) {
      OnStreamDone( msg );
      return ;
   }

   // GetField() / HasField() doesn't work w/ tape

   flds = msg.Fields();
   n    = msg.NumFields();
   for ( i=0; i<n; i++ ) {
      f        = flds[i];
      fid      = f._fid;
      fdb[fid] = f;
   }

   // MsgTime, Service, Ticker, Field1, Field2, ...

   n   = (int)_fids.size();
   upd = ::PyList_New( n+3 );
   ::PyList_SetItem( upd, 0, PyFloat_FromDouble( msg.MsgTime() ) );
   ::PyList_SetItem( upd, 1, PyString_FromString( _svc.data() ) );
   ::PyList_SetItem( upd, 2, PyString_FromString( _tkr.data() ) );
   for ( i=0; i<n; i++ ) {
      pv  = "None";
      fid = _fids[i];
      if ( (it=fdb.find( fid )) != fdb.end() ) {
         f = (*it).second;
         w = (mddField *)&f;
         if ( f._type == rtFld_string ) {
            ::memcpy( buf, b._data, b._dLen );
            buf[b._dLen] = '\0';
            pv = buf;
         }
         else {
            _uFld.Set( *w );
            pv = _uFld.GetAsString();
         }
      }
      ::PyList_SetItem( upd, i+3, PyString_FromString( pv ) );
   }
   _objs.push_back( upd );
}

void PyTapeSnap::OnStreamDone( RTEDGE::Message &msg )
{
   _bDone = true;
}
