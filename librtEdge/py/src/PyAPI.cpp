/***************************************************************************** 
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
*      5 FEB 2025 jcs  Build 13: 3.11
*      8 JUL 2025 jcs  Build 77: PubChannel; Publish( .., bImg ); ChartDbSvr
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>


/////////////////////
// Forwards
/////////////////////
PyMethodDef *_pMethods = (PyMethodDef *)0;


////////////////////////////////////////////////////////////
//
//       Python 'C' Extension Implementation Methods
//
////////////////////////////////////////////////////////////

typedef hash_map<int, MDDpyChartDB *>  PyChartMap;
typedef hash_map<int, MDDpySubChan *>  PySubChanMap;
typedef hash_map<int, MDDpyPubChan *>  PyPubChanMap;
typedef hash_map<int, MDDpyLVC *>      PyLVCMap;
typedef hash_map<int, MDDpyLVCAdmin *> PyLVCAdminMap;

static PyChartMap    _cdbMap;
static PySubChanMap  _subMap;
static PyPubChanMap  _pubMap;
static PyLVCMap      _lvcMap;
static PyLVCAdminMap _admMap;

static MDDpyChartDB *_GetCDB( int cxt )
{
   PyChartMap          &sdb = _cdbMap;
   PyChartMap::iterator it;
   MDDpyChartDB        *ch;

   it = sdb.find( cxt );
   ch = ( it != sdb.end() ) ? (*it).second : (MDDpyChartDB *)0;
   return ch;
}

static MDDpySubChan *_GetSub( int cxt )
{
   PySubChanMap          &sdb = _subMap;
   PySubChanMap::iterator it;
   MDDpySubChan          *ch;

   it = sdb.find( cxt );
   ch = ( it != sdb.end() ) ? (*it).second : (MDDpySubChan *)0;
   return ch;
}

static MDDpyPubChan *_GetPub( int cxt )
{
   PyPubChanMap          &pdb = _pubMap;
   PyPubChanMap::iterator it;
   MDDpyPubChan          *ch;

   it = pdb.find( cxt );
   ch = ( it != pdb.end() ) ? (*it).second : (MDDpyPubChan *)0;
   return ch;
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

static bool _DelCDB( int cxt )
{
   PyChartMap          &sdb = _cdbMap;
   PyChartMap::iterator it;
   MDDpyChartDB        *ch;

   if ( (it=sdb.find( cxt )) != sdb.end() ) {
      ch = (*it).second;
      sdb.erase( it );
      delete ch;
      return true;
   }
   return false;
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

static bool _DelPub( int cxt )
{
   PyPubChanMap          &pdb = _pubMap;
   PyPubChanMap::iterator it;
   MDDpyPubChan          *ch;

   if ( (it=pdb.find( cxt )) != pdb.end() ) {
      ch = (*it).second;
      pdb.erase( it );
      ch->Stop();
      delete ch;
      return true;
   }
   return false;
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
   const char   *pHost, *pUser;
   int           cxt;

   // Usage : Start( 'localhost:9998', 'Username' )

   if ( !PyArg_ParseTuple( args, "ss", &pHost, &pUser ) )
      return _PyReturn( Py_None );

   // MD-Direct Subscription Channel

   ch           = new MDDpySubChan( pHost, pUser, true );
   ch->Start( pHost, pUser );
   cxt          = ch->cxt();
   _subMap[cxt] = ch;
   return PyInt_FromLong( cxt );
}

static PyObject *StartSlice( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char   *pHost, *pUser;
   int           iBin, cxt;
   bool          bBin;

   // Usage : StartSlice( 'localhost:9998', 'Username', [bBinary] )

   if ( !PyArg_ParseTuple( args, "ssi", &pHost, &pUser, &iBin ) ) {
      iBin = 1;
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
   int           cxt, bDir;

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
   const char   *svc, *tkr;
   int           rtn, cxt, uid, sid;

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
   const char   *svc, *tkr;    
   int           rtn, cxt, uid, sid;

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
   int           cxt;
   MDDpySubChan *ch;
   PyObject     *rtn;
   double        dWait;

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
   int           cxt, sid;
   const char   *svc, *tkr;

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
   int           cxt;
   const char   *ty;

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

   // MD-Direct LVC

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

static PyObject *LVCAdmRead( PyObject *self, PyObject *args )
{
   int            cxt;
   MDDpyLVCAdmin *adm;
   PyObject      *rtn;
   double         dWait;

   // Usage : Read( cxt, dWait )

   if ( !PyArg_ParseTuple( args, "id", &cxt, &dWait ) )
      return _PyReturn( Py_None );
   if ( !(adm=_GetLVCAdmin( cxt )) )
      return _PyReturn( Py_None );
   if ( (rtn=adm->Read( dWait )) == Py_None )
      return _PyReturn( Py_None );
   return rtn;
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
// ChartDbSvr
////////////////////////////
static PyObject *CDBOpen( PyObject *self, PyObject *args )
{
   MDDpyChartDB *cdb;
   const char   *file;
   int           cxt;

   // Usage : CDBOpen( './DB/cdb.db' )

   if ( !PyArg_ParseTuple( args, "s", &file ) )
      return _PyReturn( Py_None );

   // ChartDbSvr Object

   cdb          = new MDDpyChartDB( file );
   cxt          = cdb->cxt();
   _cdbMap[cxt] = cdb;
   return PyInt_FromLong( cxt );
}

static PyObject *CDBGetTkrs( PyObject *self, PyObject *args )
{
   MDDpyChartDB *cdb;
   int           cxt;

   // Usage : CDBGetTickers( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_None );
   if ( (cdb=_GetCDB( cxt )) )
      return cdb->PyGetTickers();
   return _PyReturn( Py_None );
}

static PyObject *CDBSnap( PyObject *self, PyObject *args )
{
   MDDpyChartDB *cdb;
   const char   *svc, *tkr;
   int           cxt, fid, num;

   // Usage : CDBSnap( cxt, svc, tkr, fid [, num ] )

   num = 0;
   if ( !PyArg_ParseTuple( args, "issi|i", &cxt, &svc, &tkr, &fid, &num ) )
      return _PyReturn( Py_None );
   if ( (cdb=_GetCDB( cxt )) )
      return cdb->PySnap( svc, tkr, fid, num );
   return _PyReturn( Py_None );
}


static PyObject *CDBClose( PyObject *self, PyObject *args )
{
   int  cxt;
   bool rc;

   // Usage : CDBClose( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   rc = _DelCDB( cxt );
   return _PyReturn( rc ? Py_True : Py_False );
}


////////////////////////////
// Publication Channel
////////////////////////////
static PyObject *PubStart( PyObject *self, PyObject *args )
{
   PyPubChanMap &pdb = _pubMap;
   MDDpyPubChan *ch;
   const char   *host, *svc;
   int           cxt;

   // Usage : PubStart( 'localhost:9998', 'service' )

   if ( !PyArg_ParseTuple( args, "ss", &host, &svc ) )
      return _PyReturn( Py_None );

   // MD-Direct Publication Channel

   ch       = new MDDpyPubChan( host, svc );
   ch->SetCircularBuffer( true );
   ch->SetCache( false );
   ch->SetBinary( true );
   ch->SetUnPacked( true );
   ch->Start( svc, host );
   cxt      = ch->cxt();
   pdb[cxt] = ch;
   ch->SetTxBufSize( 40*K*K ); // 40 MB
   return PyInt_FromLong( cxt );
}

static PyObject *Publish( PyObject *self, PyObject *args )
{
   MDDpyPubChan *ch;
   PyObject     *lst, *pyImg;
   const char   *tkr;
   bool          bImg;
   int           cxt, ReqID;

   // Usage : Publish( 'EUR CURNCY', StreamID, [ [ fid1, val1 ], [ fid2, val2 ] ... ], bImg )

   if ( !PyArg_ParseTuple( args, "isiO!O", &cxt, &tkr, &ReqID, &PyList_Type, &lst, &pyImg ) )
      return _PyReturn( Py_None );

   // MD-Direct Publication Channel

   bImg = ::PyObject_IsTrue( pyImg ) ? true : false;
   if ( (ch=_GetPub( cxt )) )
      return PyInt_FromLong( ch->pyPublish( tkr, ReqID, lst, bImg ) );
   return _PyReturn( Py_None );
}

static PyObject *PubError( PyObject *self, PyObject *args )
{
   MDDpyPubChan *ch;
   const char   *tkr, *err;
   int           cxt, ReqID;

   // Usage : PubError( 'EUR CURNCY', StreamID, err )

   if ( !PyArg_ParseTuple( args, "isis", &cxt, &tkr, &ReqID, &err ) )
      return _PyReturn( Py_None );

   // MD-Direct Publication Channel

   if ( (ch=_GetPub( cxt )) )
      return PyInt_FromLong( ch->pyPubError( tkr, ReqID, err ) );
   return _PyReturn( Py_None );
}

static PyObject *PubRead( PyObject *self, PyObject *args )
{
   int           cxt;
   MDDpyPubChan *ch;
   PyObject     *rtn;
   double        dWait;

   // Usage : Read( cxt, dWait )

   if ( !PyArg_ParseTuple( args, "id", &cxt, &dWait ) )
      return _PyReturn( Py_None );
   if ( !(ch=_GetPub( cxt )) )
      return _PyReturn( Py_None );
   if ( (rtn=ch->Read( dWait )) == Py_None )
      return _PyReturn( Py_None );
   return rtn;
}
static PyObject *PubStop( PyObject *self, PyObject *args )
{
   int  cxt;
   bool rc;

   // Usage : PubStop( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   rc = _DelPub( cxt );
   return _PyReturn( rc ? Py_True : Py_False );
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

#if ( PY_MAJOR_VERSION >= 3 ) && ( PY_MINOR_VERSION >= 9 )
   rc  = 0;
#else 
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
#endif //  ( PY_MAJOR_VERSION >= 3 ) && ( PY_MINOR_VERSION >= 9 )
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
    { "PubError",      PubError, _PY_ARGS, "Publish Error" },
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
     * ChartDbSvr
     */
    { "ChartDbOpen",   CDBOpen,    _PY_ARGS, "Open ChartDbSvr File" },
    { "ChartDbQuery",  CDBGetTkrs, _PY_ARGS, "Get All Tickers from ChartDbSvr" },
    { "ChartDbSnap",   CDBSnap,    _PY_ARGS, "Snap from ChartDbSvr File" },
    { "ChartDbClose",  CDBClose,   _PY_ARGS, "Close ChartDbSvr File" },
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
    { "LVCAdminRead",           LVCAdmRead,      _PY_ARGS, "Read update - LVCAdmin." },
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
/*
 * Python 3.x
 */
#if PY_MINOR_VERSION >= 11
/*
 * Python 3.11
 */
static PyModuleDef _mddModule = { PyModuleDef_HEAD_INIT,
                                  "MDDirect311",
                                  "MD-Direct for Python 3.11",
                                  -1,
                                  EdgeMethods
                                };

PyMODINIT_FUNC PyInit_MDDirect311( void )
{
   return ::PyModule_Create( &_mddModule );
} 

#elif PY_MINOR_VERSION >= 9
/*
 * Python 3.9
 */
static PyModuleDef _mddModule = { PyModuleDef_HEAD_INIT,
                                  "MDDirect39",
                                  "MD-Direct for Python 3.9",
                                  -1,
                                  EdgeMethods
                                };

PyMODINIT_FUNC PyInit_MDDirect39( void )
{
   return ::PyModule_Create( &_mddModule );
}
 
#else
/*
 * Python 3.6
 */
static PyModuleDef _mddModule = { PyModuleDef_HEAD_INIT,
                                  "MDDirect36",
                                  "MD-Direct for Python 3.6",
                                  -1,
                                  EdgeMethods
                                };

PyMODINIT_FUNC PyInit_MDDirect36( void )
{
   return ::PyModule_Create( &_mddModule );
} 

#endif // PY_MINOR_VERSION >= 9

#else
/*
 * Python 2.7
 */
PyMODINIT_FUNC initMDDirect27( void )
{
   _pMethods = EdgeMethods;
   Py_InitModule( "MDDirect27", EdgeMethods );
}

#endif // PY_MAJOR_VERSION >= 3
