/******************************************************************************
*
*  PyAPI.cpp
*     MDDirect Python interface
*
*  REVISION HISTORY:
*      7 APR 2011 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     19 NOV 2020 jcs  Build  2: LVCGetTickers()
*      1 DEC 2020 jcs  Build  3: SnapTape() / PyTapeSnapQry
*     25 JAN 2022 jcs  Build  4: MDDirectxy
*      3 FEB 2022 jcs  Build  5: PyList, not PyTuple
*     19 JUL 2022 jcs  Build  8: MDDpyLVCAdmin; XxxMap
*     11 JAN 2023 jcs  Build  9: Python 3.x on Linux
*     29 AUG 2023 jcs  Build 10: LVCSnapAll; Named Schema; OpenBDS()
*     20 SEP 2023 jcs  Build 11: mdd_PyList_PackX()
*     17 OCT 2023 jcs  Build 12: No mo Book
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>


/////////////////////
// Forwards
/////////////////////
PyMethodDef *_pMethods = (PyMethodDef *)0;


/////////////////////
// Helpers
/////////////////////
static const char *_Py_GetString( PyObject *pyo, string &rc )
{
#if PY_MAJOR_VERSION >= 3
   Py_ssize_t sz;
   wchar_t   *pw = ::PyUnicode_AsWideCharString( pyo, &sz );
   wstring    wc( pw );
   string     ss( wc.begin(), wc.end() );

   rc = ss.data();
   ::PyMem_Free( pw );
#else
   rc = ::PyString_AsString( pyo );
#endif // PY_MAJOR_VERSION >= 3
   return rc.data();
}


////////////////////////////////////////////////////////////
//
//       Python 'C' Extension Implementation Methods
//
////////////////////////////////////////////////////////////

typedef hash_map<int, MDDpySubChan *>  PySubChanMap;
typedef hash_map<int, MDDpyLVC *>      PyLVCMap;
typedef hash_map<int, MDDpyLVCAdmin *> PyLVCAdminMap;

static PySubChanMap  _subMap;
static PyLVCMap      _lvcMap;
static PyLVCAdminMap _admMap;

static MDDpySubChan *_GetSub( int cxt )
{
   PySubChanMap          &sdb = _subMap;
   PySubChanMap::iterator it;
   MDDpySubChan          *ch;

   it = sdb.find( cxt );
   ch = ( it != sdb.end() ) ? (*it).second : (MDDpySubChan *)0;
   return ch;
}

static bool _DelSub( int cxt )
{
   PySubChanMap          &sdb = _subMap;
   PySubChanMap::iterator it;
   MDDpySubChan          *ch;

   if ( (it=sdb.find( cxt )) != sdb.end() ) {
      ch = (*it).second;
      sdb.erase( it );
      ch->Stop();
      delete ch;
      return true;
   }
   return false;
}

static MDDpyLVC *_GetLVC( int cxt )
{
   PyLVCMap          &ldb = _lvcMap;
   PyLVCMap::iterator it;
   MDDpyLVC          *lvc;

   it = ldb.find( cxt );
   lvc = ( it != ldb.end() ) ? (*it).second : (MDDpyLVC *)0;
   return lvc;
}

static bool _DelLVC( int cxt )
{
   PyLVCMap          &ldb = _lvcMap;
   PyLVCMap::iterator it;
   MDDpyLVC          *lvc;

   if ( (it=ldb.find( cxt )) != ldb.end() ) {
      lvc = (*it).second;
      ldb.erase( it );
      delete lvc;
      return true;
   }
   return false;
}

static MDDpyLVCAdmin *_GetLVCAdmin( int cxt )
{
   PyLVCAdminMap          &ldb = _admMap;
   PyLVCAdminMap::iterator it;
   MDDpyLVCAdmin          *adm;

   it = ldb.find( cxt );
   adm = ( it != ldb.end() ) ? (*it).second : (MDDpyLVCAdmin *)0;
   return adm; 
}

static bool _DelLVCAdmin( int cxt )
{
   PyLVCAdminMap          &ldb = _admMap;
   PyLVCAdminMap::iterator it; 
   MDDpyLVCAdmin          *adm;

   if ( (it=ldb.find( cxt )) != ldb.end() ) { 
      adm = (*it).second;
      ldb.erase( it );
      delete adm;
      return true;
   }   
   return false;
}


//////////////////////
// External API
//////////////////////
static PyObject *Version( PyObject *self, PyObject *args )
{
   const char *pc;

   pc = RTEDGE::SubChannel::Version();
   return PyString_FromFormat( "%s\n%s", MDDirectID(), pc );
}


////////////////////////////
// Subscription Channel
////////////////////////////
static PyObject *Start( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *pHost, *pUser;
   int         iBin, cxt;
   bool        bBin;

   // Usage : Start( 'localhost:9998', 'Username', [bBinary] )

   if ( !PyArg_ParseTuple( args, "ssi", &pHost, &pUser, &iBin ) ) {
      iBin = 0;
      if ( !PyArg_ParseTuple( args, "ss", &pHost, &pUser ) )
         return _PyReturn( Py_None );
   }
   bBin = iBin ? true : false;

   // MD-Direct Subscription Channel

   ch           = new MDDpySubChan( pHost, pUser, bBin );
   ch->Start( pHost, pUser );
   cxt          = ch->cxt();
   _subMap[cxt] = ch;
   return PyInt_FromLong( cxt );
}

static PyObject *StartSlice( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *pHost, *pUser;
   int         iBin, cxt;
   bool        bBin;

   // Usage : StartSlice( 'localhost:9998', 'Username', [bBinary] )

   if ( !PyArg_ParseTuple( args, "ssi", &pHost, &pUser, &iBin ) ) {
      iBin = 0;
      if ( !PyArg_ParseTuple( args, "ss", &pHost, &pUser ) )
         return _PyReturn( Py_None );
   }
   bBin = iBin ? true : false;

   // MD-Direct Subscription Channel

   ch         = new MDDpySubChan( pHost, pUser, bBin );
   ch->Start( pHost, pUser );
   cxt        = ch->cxt();
   _subMap[cxt] = ch;
   return PyInt_FromLong( cxt );
}

static PyObject *IsTape( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   bool        bTape;
   int         cxt;

   // Usage : IsTape( cxt )

   bTape = false;
   if ( PyArg_ParseTuple( args, "i", &cxt ) ) {
      if ( (ch=_GetSub( cxt )) )
         bTape = ch->IsTape();
   }
   return _PyReturn( bTape ? Py_True : Py_False );
}

static PyObject *SetTapeDir( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         cxt, bDir;

   // Usage : SetTapeDir( cxt, bDir )

   if ( PyArg_ParseTuple( args, "ii", &cxt, &bDir ) ) {
      if ( (ch=_GetSub( cxt )) )
         ch->SetTapeDirection( bDir ? true : false );
   }
   ::PyErr_Clear();
   return _PyReturn( Py_None );
}

static PyObject *PumpTape( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char   *t0, *t1;
   int           cxt;

   // Usage : PumpTape( cxt )

   t0 = (const char *)0;
   t1 = (const char *)0;
   if ( !PyArg_ParseTuple( args, "i|ss", &cxt, &t0, &t1 ) )
      return _PyReturn( Py_None );
   if ( (ch=_GetSub( cxt )) ) {
      if ( t0 && t1 )
         ch->PumpTapeSlice( t0, t1 );
      else
         ch->PumpTape();
   }
   return _PyReturn( ch ? Py_True : Py_False );
}

static PyObject *SnapTape( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   PyObject     *rc;
   PyTapeSnapQry qry;
   int           rtn, cxt;

   // Usage : SnapTape( cxt, svc, tkr, flds, 1000, 2.5 [, t0, t1, [, tSample ] ] )

   rc  = (PyObject *)0;
   ::memset( &qry, 0, sizeof( qry ) );
   rtn = PyArg_ParseTuple( args, "isssid|ssi", 
                           &cxt, 
                           &qry._svc, 
                           &qry._tkr, 
                           &qry._flds, 
                           &qry._maxRow, 
                           &qry._tmout,
                           &qry._t0,
                           &qry._t1,
                           &qry._tSample );
   if ( rtn && (ch=_GetSub( cxt )) )
      rc = ch->SnapTape( qry );
   return rc ? rc : _PyReturn( Py_None );
}

static PyObject *QueryTape( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   PyObject     *rc;
   int           cxt;

   // Usage : QueryTape( cxt )

   rc = (PyObject *)0;
   if ( PyArg_ParseTuple( args, "i", &cxt ) ) {
      if ( (ch=_GetSub( cxt )) )
         rc = ch->QueryTape();
   }
   return rc ? rc : _PyReturn( Py_None );
}

static PyObject *Open( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *svc, *tkr;
   int         rtn, cxt, uid, sid;

   // Usage : Open( cxt, 'BBO', 'EUR/USD', UserReqID )

   if ( !(rtn=PyArg_ParseTuple( args, "issi", &cxt, &svc, &tkr, &uid )) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) ) {
      sid = ch->Open( svc, tkr, uid );
      return _PyReturn( PyInt_FromLong( sid ) );
   }
   return _PyReturn( Py_None );
}

static PyObject *OpenBDS( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *svc, *tkr;    
   int         rtn, cxt, uid, sid;

   // Usage : OpenBDS( cxt, 'factset', 'NYSE', UserReqID )

   if ( !(rtn=PyArg_ParseTuple( args, "issi", &cxt, &svc, &tkr, &uid )) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) ) {
      sid = ch->OpenBDS( svc, tkr, (VOID_PTR)uid );
      return _PyReturn( PyInt_FromLong( sid ) );
   }
   return _PyReturn( Py_None );
}

static PyObject *OpenByStr( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *svc, *tkr;    
   int         rtn, cxt, uid, sid;

   // Usage : OpenByteStream( cxt, 'ChainDB', 'ULTRAFEED|CSCO|22', UserReqID )

   if ( !(rtn=PyArg_ParseTuple( args, "issi", &cxt, &svc, &tkr, &uid )) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) ) {
      sid = ch->OpenByteStream( svc, tkr, uid );
      return _PyReturn( PyInt_FromLong( sid ) );
   }
   return _PyReturn( Py_None );
}

static PyObject *Read( PyObject *self, PyObject *args )
{
   int         cxt;
   MDDpySubChan *ch;
   PyObject   *rtn;
   double      dWait;

   // Usage : Read( cxt, dWait )

   if ( !PyArg_ParseTuple( args, "id", &cxt, &dWait ) )
      return _PyReturn( Py_None );
   if ( !(ch=_GetSub( cxt )) )
      return _PyReturn( Py_None );
   if ( (rtn=ch->Read( dWait )) == Py_None )
      return _PyReturn( Py_None );
   return rtn;
}

static PyObject *ReadFltr( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         iFilter, cxt;

   // Usage : ReadFilter( cxt, iFilter )

   if ( !PyArg_ParseTuple( args, "ii", &cxt, &iFilter ) )
      return _PyReturn( Py_None );
   ch = _GetSub( cxt );
   return ch ? ch->Filter( iFilter ) : (PyObject *)0;
}

static PyObject *Close( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         cxt, sid;
   const char *svc, *tkr;

   // Usage : Close( cxt, 'BBO', 'EUR/USD' )

   if ( !PyArg_ParseTuple( args, "iss", &cxt, &svc, &tkr ) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) ) {
      sid = ch->Close( svc, tkr );
      return _PyReturn( PyInt_FromLong( sid ) );
   }
   return _PyReturn( Py_None );
}

static PyObject *CloseBDS( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         cxt, sid;
   const char *svc, *tkr;

   // Usage : CloseBDS( cxt, 'factset', 'NYSE' )

   if ( !PyArg_ParseTuple( args, "iss", &cxt, &svc, &tkr ) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) ) {
      sid = ch->CloseBDS( svc, tkr );
      return _PyReturn( PyInt_FromLong( sid ) );
   }
   return _PyReturn( Py_None );
}

static PyObject *Stop( PyObject *self, PyObject *args )
{
   int  cxt;
   bool rc;

   // Usage : Stop( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   rc = _DelSub( cxt );
   return _PyReturn( rc ? Py_True : Py_False );
}

static PyObject *Ioctl( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         cxt, cmd, iVal;

   // Usage : Ioctl( cxt, cmd, iVal )

   if ( !PyArg_ParseTuple( args, "iii", &cxt, &cmd, &iVal ) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) )
      ch->Ioctl( (rtEdgeIoctl)cmd, (VOID_PTR)iVal );
   return _PyReturn( ch ? Py_True : Py_False );
}

static PyObject *Protocol( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         cxt;
   const char *ty;

   // Usage : Protocol( cxt )

   ty = "???";
   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return PyString_FromString( ty );
   if ( (ch=_GetSub( cxt )) )
      ty = ch->Protocol();
   return PyString_FromString( ty );
}


////////////////////////////
// Last Value Cache (LVC)
////////////////////////////
static PyObject *LVCOpen( PyObject *self, PyObject *args )
{
   MDDpyLVC   *lvc;
   const char *file;
   int         cxt;

   // Usage : LVCOpen( './DB/lvc.db' )

   if ( !PyArg_ParseTuple( args, "s", &file ) )
      return _PyReturn( Py_None );

   // MD-Direct Subscription Channel

   lvc          = new MDDpyLVC( file );
   cxt          = lvc->cxt();
   _lvcMap[cxt] = lvc;
   return PyInt_FromLong( cxt );
}

static PyObject *LVCSchema( PyObject *self, PyObject *args )
{
   MDDpyLVC *lvc;
   int       cxt;

   // Usage : LVCSchema( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_None );
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PySchema();
   return _PyReturn( Py_None );
}

static PyObject *LVCGetTkrs( PyObject *self, PyObject *args )
{
   MDDpyLVC *lvc;
   int       cxt;

   // Usage : LVCGetTickers( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_None );
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PyGetTickers();
   return _PyReturn( Py_None );
}

static PyObject *LVCSnap( PyObject *self, PyObject *args )
{
   MDDpyLVC   *lvc;
   const char *svc, *tkr;
   int         cxt;

   // Usage : LVCSnap( cxt, svc, tkr )

   if ( !PyArg_ParseTuple( args, "iss", &cxt, &svc, &tkr ) )
      return _PyReturn( Py_None );
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PySnap( svc, tkr );
   return _PyReturn( Py_None );
}

static PyObject *LVCSnapAll( PyObject *self, PyObject *args )
{
   MDDpyLVC   *lvc;
   int         cxt;

   // Usage : LVCSnapAll( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_None );
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PySnapAll();
   return _PyReturn( Py_None );
}

static PyObject *LVCClose( PyObject *self, PyObject *args )
{
   int  cxt;
   bool rc;

   // Usage : LVCClose( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   rc = _DelLVC( cxt );
   return _PyReturn( rc ? Py_True : Py_False );
}


///////////////////////////
// LVCAdmin Channel
////////////////////////////
static PyObject *LVCAdmOpen( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin *adm;
   const char    *file;
   int            cxt;

   // Usage : LVCAdminOpen( 'localhost:7655' );

   if ( !PyArg_ParseTuple( args, "s", &file ) )
      return _PyReturn( Py_None );

   // MD-Direct Subscription Channel

   adm          = new MDDpyLVCAdmin( file );
   cxt          = adm->cxt();
   _admMap[cxt] = adm;
   return PyInt_FromLong( cxt );
}

static PyObject *LVCAdmAddBDS( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin *adm;
   const char    *svc, *bds;
   int            cxt;

   // Usage : LVCAddBDS( cxt, svc, tkr )

   if ( !PyArg_ParseTuple( args, "iss", &cxt, &svc, &bds ) )
      return _PyReturn( Py_None );
   if ( (adm=_GetLVCAdmin( cxt )) )
      adm->PyAddBDS( svc, bds );
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmAddTkr( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin *adm;
   const char    *svc, *tkr, *schema;
   int            cxt;

   // Usage : LVCAddTicker( cxt, svc, tkr, [, schema ] )

   schema = "";
   if ( !PyArg_ParseTuple( args, "iss|s", &cxt, &svc, &tkr, &schema ) )
      return _PyReturn( Py_None );
   if ( (adm=_GetLVCAdmin( cxt )) )
      adm->PyAddTicker( svc, tkr, schema );
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmDelTkr( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin *adm;
   const char    *svc, *tkr, *schema;
   int            cxt;

   // Usage : LVCDelTicker( cxt, svc, tkr, [, schema ] )

   schema = "";
   if ( !PyArg_ParseTuple( args, "iss|s", &cxt, &svc, &tkr, &schema ) )
      return _PyReturn( Py_None );
   if ( (adm=_GetLVCAdmin( cxt )) )
      adm->PyDelTicker( svc, tkr, schema );
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmDelTkrs( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin   *adm;
   const char      *svc, *schema;
   const char     **tkrs;
   PyObject        *lst, *pyK;
   int              cxt, i, nf;
   string          *s;
   vector<string *> sdb;

   // Usage : LVCDelTickers( cxt, svc, tkrs )

   schema = "";
   if ( !PyArg_ParseTuple( args, "isO!|s", &cxt, &svc, &PyList_Type, &lst, &schema ) )
      return _PyReturn( Py_None );
   if ( !(nf=::PyList_Size( lst )) )
      return _PyReturn( Py_None );
   if ( (adm=_GetLVCAdmin( cxt )) ) {
      tkrs = new const char *[nf+4];
      for ( i=0; i<nf; i++ ) {
         s       = new string();
         pyK     = PyList_GetItem( lst, i );
         tkrs[i] = _Py_GetString( pyK, *s );
         sdb.push_back( s );
      }
      tkrs[i] = NULL;
      adm->PyDelTickers( svc, tkrs, schema );
      delete[] tkrs;
      for ( i=0; i<nf; delete sdb[i++] );
   }
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmAddTkrs( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin   *adm;
   const char      *svc, *schema;
   const char     **tkrs;
   PyObject        *lst, *pyK;
   int              cxt, i, nf;
   string          *s;
   vector<string *> sdb;

   // Usage : LVCAddTickers( cxt, svc, tkrs )

   schema = "";
   if ( !PyArg_ParseTuple( args, "isO!|s", &cxt, &svc, &PyList_Type, &lst, &schema ) )
      return _PyReturn( Py_None );
   if ( !(nf=::PyList_Size( lst )) )
      return _PyReturn( Py_None );
   if ( (adm=_GetLVCAdmin( cxt )) ) {
      tkrs = new const char *[nf+4];
      for ( i=0; i<nf; i++ ) {
         s       = new string();
         pyK     = PyList_GetItem( lst, i );
         tkrs[i] = _Py_GetString( pyK, *s );
         sdb.push_back( s );
      }
      tkrs[i] = NULL;
      adm->PyAddTickers( svc, tkrs, schema );
      delete[] tkrs;
      for ( i=0; i<nf; delete sdb[i++] );
   }
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmRfrshTkr( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin *adm;
   const char    *svc, *tkr;
   const char    *tkrs[K];
   int            cxt;

   // Usage : LVCRefreshTicker( cxt, svc, tkr )

   if ( !PyArg_ParseTuple( args, "iss", &cxt, &svc, &tkr ) )
      return _PyReturn( Py_None );
   tkrs[0] = tkr;
   tkrs[1] = NULL;
   if ( (adm=_GetLVCAdmin( cxt )) )
      adm->PyRefreshTickers( svc, tkrs );
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmRfrshTkrs( PyObject *self, PyObject *args )
{
   MDDpyLVCAdmin  *adm;
   const char     *svc;
   const char    **tkrs;
   PyObject       *lst, *pyK;
   int             cxt, i, nf;
   string          *s;
   vector<string *> sdb;

   // Usage : LVCRefreshTickers( cxt, svc, tkrs )

   if ( !PyArg_ParseTuple( args, "isO!", &cxt, &svc, &PyList_Type, &lst ) )
      return _PyReturn( Py_None );
   if ( !(nf=::PyList_Size( lst )) )
      return _PyReturn( Py_None );
   if ( (adm=_GetLVCAdmin( cxt )) ) {
      tkrs = new const char *[nf+4];
      for ( i=0; i<nf; i++ ) {
         pyK     = PyList_GetItem( lst, i );
         s       = new string();
         tkrs[i] = _Py_GetString( pyK, *s );
         sdb.push_back( s );
      }
      tkrs[i] = NULL;
      adm->PyRefreshTickers( svc, tkrs );
      delete[] tkrs;
      for ( i=0; i<nf; delete sdb[i++] );
   }
   return _PyReturn( Py_None );
}

static PyObject *LVCAdmClose( PyObject *self, PyObject *args )
{
   int  cxt;
   bool rc;

   // Usage : LVCClose( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   rc = _DelLVCAdmin( cxt );
   return _PyReturn( rc ? Py_True : Py_False );
}


////////////////////////////
// Publication Channel
////////////////////////////
static PyObject *PubStart( PyObject *self, PyObject *args )
{
#ifdef TODO_PUB
   rtEdgePubAttr attr;
   rmdsPubAttr   attR;
   const char   *pEdgHost, *pPubName, *pc;
   int           bInteractive;

   // Usage : Start( 'localhost:9998', 'MyPublisher', bInteractive )

   if ( !PyArg_ParseTuple( args, "ssi", &pEdgHost, &pPubName, &bInteractive ) )
      return PyString_FromString( "Bad Arguments" );

   return PyString_FromFormat( "%s Connected", pc );
#endif // TODO_PUB
   return PyString_FromFormat( "TODO - Publication" );
}

static PyObject *Publish( PyObject *self, PyObject *args )
{
#ifdef TODO_PUB
   const char *pSvc, *pTkr;
   PyObject   *lst, *pyFidVal, *pyFid, *pyVal;
   rmdsData    r;
   rtEdgeData  e;
   int         i, j, nf, nk;
   rtFIELD    *flds, f;
   rtVALUE    &v = f._val;
   rtBUF      &b = v._buf;

   // Pre-condition

   if ( !_cxtPub )
      return _PyReturn( Py_None );

   // Usage : Publish( 'EUR/USD', [ [ fid1, 'val1' ], [ fid2, 'val2' ], ... ]

   pSvc = _sPubName;
   if ( !PyArg_ParseTuple( args, "sO!", &pTkr, &PyList_Type, &lst ) )
      return _PyReturn( Py_None );

   // Parse Python List

   nf = PyList_Size( lst );
   if ( !nf )
      return PyInt_FromLong( 0 );
   flds = new rtFIELD[nf];
   for ( i=0,j=0; i<nf; i++ ) {
      pyFidVal = PyList_GetItem( lst, i );
      nk       = PyList_Size( pyFidVal );
      if ( nk == 2 ) {
         ::memset( &f, 0, sizeof( f ) );
         pyFid   = PyList_GetItem( pyFidVal, 0 );
         pyVal   = PyList_GetItem( pyFidVal, 1 );
         f._fid  = PyInt_AsLong( pyFid );
         b._data = PyString_AsString( pyVal );
         b._dLen = strlen( b._data );
         flds[j] = f;
         j      += 1;
      }
   }

   // Publish, Clean up

   if ( _rmds ) {
      ::memset( &r, 0, sizeof( r ) );
      r._pSvc = pSvc;
      r._pTkr = pTkr;
      r._arg  = (void *)0; // TODO
      r._ty   = rmds_image;
      r._flds = flds;
      r._nFld = j;
      _rmds->Publish( _cxtPub, r );
   }
   else {
      ::memset( &e, 0, sizeof( e ) );
      e._pSvc = pSvc;
      e._pTkr = pTkr;
      e._arg  = (void *)0; // TODO 
      e._ty   = edg_image;
      e._flds = flds;
      e._nFld = j;
      ::rtEdge_Publish( _cxtPub, e );
   }
   delete[] flds;
   return PyInt_FromLong( j );
#endif // TODO_PUB
   return PyInt_FromLong( 0 );
}

static PyObject *PubRead( PyObject *self, PyObject *args )
{
#ifdef TODO_PUB
   PyObject *rtn;
   double    dWait;

   // Usage : PubRead( dWait )

   if ( !PyArg_ParseTuple( args, "d", &dWait ) )
      return _PyReturn( Py_None );
   if ( (rtn=_ReadPubEvent( dWait )) == Py_None )
      return _PyReturn( Py_None );
   return rtn;
#endif // // TODO_PUB
   return _PyReturn( Py_None );
}

static PyObject *PubStop( PyObject *self, PyObject *args )
{
#ifdef TODO_PUB
   PyObject *rtn;

   // Once

   if ( _cxtPub ) {
      if ( _rmds )
         _rmds->Destroy( _cxtPub );
      else 
         ::rtEdge_PubDestroy( _cxtPub );
   }
   ::memset( _sPubName, 0, sizeof( _sPubName ) );
   rtn     = _cxtPub ? Py_True : Py_False;
   _cxtPub = 0;
   return _PyReturn( rtn );
#endif // // TODO_PUB
   return _PyReturn( Py_None );
}


////////////////////////////
// Library Utilities
////////////////////////////
static PyObject *GetFields( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *svc, *tkr;
   PyObject   *rtn, *lst, *pyFid, *data, *pyd;
   int         cxt, i, nf, fids[K], aRtn;
   double      d0, dd;

   // Usage : GetFields( cxt, 'BBO', 'EUR/USD', fids[] )

   d0  = ::rtEdge_TimeNs();
   aRtn = PyArg_ParseTuple( args,
                            "issO!", 
                            &cxt, 
                            &svc, 
                            &tkr, 
                            &PyList_Type, 
                            &lst );
   ch = aRtn ? _GetSub( cxt ) : (MDDpySubChan *)0;
   if ( !ch ) {
      pyd = PyFloat_FromDouble( 0.0 );
      rtn = ::mdd_PyList_Pack2( pyd, _PyReturn( Py_None ) );
      return rtn;
   }

   // Parse Python List

   nf = PyList_Size( lst );
   for ( i=0; i<nf; i++ ) {
      pyFid   = PyList_GetItem( lst, i );
      fids[i] = PyInt_AsLong( pyFid );
   }
   fids[nf] = 0;
   data     = ch->GetData( svc, tkr, fids );
   dd       = 1000000.0 * ( ::rtEdge_TimeNs() - d0 );
   pyd      = PyFloat_FromDouble( dd );
   rtn      = ::mdd_PyList_Pack2( pyd, data );
   return rtn;
}

static PyObject *Log( PyObject *self, PyObject *args )
{
   const char *pLog;
   int         iLvl;

   // Usage : Log( pFile, debugLevel )

   if ( !PyArg_ParseTuple( args, "si", &pLog, &iLvl ) )
      return _PyReturn( Py_False );
   RTEDGE::rtEdge::Log( pLog, iLvl );
   return _PyReturn( Py_True );
}

static PyObject *CPU( PyObject *self, PyObject *args )
{
   return PyFloat_FromDouble( RTEDGE::rtEdge::CPU() );
}

static PyObject *MemSize( PyObject *self, PyObject *args )
{
   return PyInt_FromLong( RTEDGE::rtEdge::MemSize() );
}

static PyObject *MemFree( PyObject *self, PyObject *args )
{
   int rc;

   rc  = ::PyMethod_ClearFreeList();
   rc += ::PyFloat_ClearFreeList();
   rc += ::PyTuple_ClearFreeList();
   rc += ::PyUnicode_ClearFreeList();
#if PY_MAJOR_VERSION >= 3
   rc += ::PyDict_ClearFreeList();
   rc += ::PyList_ClearFreeList();
#else
   rc += ::PyInt_ClearFreeList();
#endif // PY_MAJOR_VERSION >= 3
   return PyInt_FromLong( rc );
}


////////////////////////////
// Stats.cpp
////////////////////////////
static PyObject *GetBBDailyStats( PyObject *self, PyObject *args )
{
   const char *pFile;
   PyObject   *rc;

   /*
    * Usage : GetBBDailyStats( 'feed.bloomberg.01.stats' )
    */
   rc = _PyReturn( Py_None );
   if ( PyArg_ParseTuple( args, "s", &pFile ) ) {
      MDDpyStats st( pFile );

      rc = st.PyBBDailyStats();
   }
   return rc;
}


////////////////////////////
// Utilities
////////////////////////////
static PyObject *HasMethod( PyObject *self, PyObject *args )
{
   const char *pFcn, *pm;
   int         i;

   // Usage : HasMethod( pFcn )

   if ( !PyArg_ParseTuple( args, "s", &pFcn ) )
      return _PyReturn( Py_False );
   for ( i=0; (pm=_pMethods[i].ml_name); i++ ) {
      if ( !::strcmp( pm, pFcn ) )
         return _PyReturn( Py_True );
   }
   return _PyReturn( Py_False );
}



////////////////////////////////////////////////////////////
//
//          Python 'C' Extension Interface
//
////////////////////////////////////////////////////////////
#define _PY_ARGS METH_VARARGS

static PyMethodDef EdgeMethods[] = 
{
    { "Version",     Version,    _PY_ARGS, "Library Version." },
    /*
     * Subscription
     */
    { "Start",         Start,     _PY_ARGS, "MDD Connect - Subscribe" },
    { "StartSlice",    StartSlice,_PY_ARGS, "MDD Connect - Tape Slice " },
    { "IsTape",        IsTape,    _PY_ARGS, "MDD Subscribe Channel is Tape" },
    { "SetTapeDir",    SetTapeDir,_PY_ARGS, "MDD Tape Dir : True = reverse" },
    { "PumpTape",      PumpTape,  _PY_ARGS, "Pump MDD Tape" },
    { "SnapTape",      SnapTape,  _PY_ARGS, "Snap MDD Tape - ( svc, tkr, fids )" },
    { "QueryTape",     QueryTape, _PY_ARGS, "Query MDD Tape" },
    { "Open",          Open,      _PY_ARGS, "Open MDD ( svc,tkr ) stream." },
    { "OpenBDS",       OpenBDS,   _PY_ARGS, "Open MDD BDS ( svc,tkr ) stream." },
    { "OpenByteStr",   OpenByStr, _PY_ARGS, "Open MDD ( svc,tkr ) byte-stream." },
    { "Read",          Read,      _PY_ARGS, "Read MDD update - Subscribe." },
    { "ReadFilter",    ReadFltr,  _PY_ARGS, "Read() Event filter." },
    { "Close",         Close,     _PY_ARGS, "Close MDD ( svc,tkr ) stream." },
    { "CloseBDS",      CloseBDS,  _PY_ARGS, "Close MDD BDS ( svc,tkr ) stream." },
    { "Stop",          Stop,      _PY_ARGS, "MDD Disconnect - Subscribe" },
    { "Ioctl",         Ioctl,     _PY_ARGS, "MDD Control - Conflate, etc." },
    { "Protocol",      Protocol,  _PY_ARGS, "Channel Protocol" },
    /*
     * Publication
     */
    { "PubStart",      PubStart, _PY_ARGS, "MDD Connect - Publish" },
    { "Publish",       Publish,  _PY_ARGS, "Publish" },
    { "PubRead",       PubRead,  _PY_ARGS, "Read MDD update - Publish." },
    { "PubStop",       PubStop,  _PY_ARGS, "MDD Disconnect - Publish" },
    /*
     * Last Value Cache (LVC)
     */
    { "LVCOpen",       LVCOpen,    _PY_ARGS, "Open LVC File" },
    { "LVCSchema",     LVCSchema,  _PY_ARGS, "LVC File Schema" },
    { "LVCGetTickers", LVCGetTkrs, _PY_ARGS, "Get All Tickers from LVC" },
    { "LVCSnap",       LVCSnap,    _PY_ARGS, "Snap from LVC File" },
    { "LVCSnapAll",    LVCSnapAll, _PY_ARGS, "Snap All Tickers from LVC File" },
    { "LVCClose",      LVCClose,   _PY_ARGS, "Close LVC File" },
    /*
     * LVC Admin Channel
     */
    { "LVCAdminOpen",           LVCAdmOpen,      _PY_ARGS, "Open LVCAdmin Channel" },
    { "LVCAdminAddBDS",         LVCAdmAddBDS,    _PY_ARGS, "Add BDS to LVC" },
    { "LVCAdminAddTicker",      LVCAdmAddTkr,    _PY_ARGS, "Add Ticker to LVC" },
    { "LVCAdminAddTickers",     LVCAdmAddTkrs,   _PY_ARGS, "Add Ticker List to LVC" },
    { "LVCAdminDelTicker",      LVCAdmDelTkr,    _PY_ARGS, "Delete Ticker from LVC" },
    { "LVCAdminDelTickers",     LVCAdmDelTkrs,   _PY_ARGS, "Delete Ticker List to LVC" },
    { "LVCAdminRefreshTicker",  LVCAdmRfrshTkr,  _PY_ARGS, "Refresh Ticker to LVC" },
    { "LVCAdminRefreshTickers", LVCAdmRfrshTkrs, _PY_ARGS, "Refresh Ticker List to LVC" },
    { "LVCAdminClose",          LVCAdmClose,     _PY_ARGS, "Close LVCAdmin Channel" },
    /*
     * Library Utilities
     */
    { "GetFields",     GetFields, _PY_ARGS, "Get Field List." },
    { "Log",           Log,       _PY_ARGS, "Set MDD library logger." },
    { "CPU",           CPU,       _PY_ARGS, "Get process CPU usage." },
    { "MemSize",       MemSize,   _PY_ARGS, "Get process memory usage." },
    { "MemFree",       MemFree,   _PY_ARGS, "Clear Python FreeLists." },
    /* 
     * Stats
     */
    { "GetBBDailyStats",GetBBDailyStats, _PY_ARGS, "Stats.BBDailyStats" },
    /* 
     * Utility
     */
    { "HasMethod",   HasMethod,  _PY_ARGS, "True if this DLL exports the fcn." },
    { NULL, NULL, 0, NULL}        /* Sentinel */
};

#if PY_MAJOR_VERSION >= 3
#if PY_MINOR_VERSION >= 9
static PyModuleDef _mddModule = { PyModuleDef_HEAD_INIT,
                                  "MDDirect39",
                                  "MD-Direct for Python 3.x",
                                  -1,
                                  EdgeMethods
                                };

PyMODINIT_FUNC PyInit_MDDirect39( void )
{
   return ::PyModule_Create( &_mddModule );
} 
#else
static PyModuleDef _mddModule = { PyModuleDef_HEAD_INIT,
                                  "MDDirect36",
                                  "MD-Direct for Python 3.x",
                                  -1,
                                  EdgeMethods
                                };

PyMODINIT_FUNC PyInit_MDDirect36( void )
{
   return ::PyModule_Create( &_mddModule );
} 
#endif // PY_MINOR_VERSION >= 9
#else
PyMODINIT_FUNC initMDDirect27( void )
{
   _pMethods = EdgeMethods;
   Py_InitModule( "MDDirect27", EdgeMethods );
}

#endif // PY_MAJOR_VERSION >= 3
