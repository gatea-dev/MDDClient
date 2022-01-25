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
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>


/////////////////////
// Forwards
/////////////////////
PyMethodDef *_pMethods = (PyMethodDef *)0;


/////////////////////
// Helpers
/////////////////////
static PyObject *_PyReturn( PyObject *obj )
{
   Py_INCREF( obj );
   return obj;
}



////////////////////////////////////////////////////////////
//
//       Python 'C' Extension Implementation Methods
//
////////////////////////////////////////////////////////////

static MDDpySubChan *_subs[_MAX_PYCHAN];
static MDDpyLVC     *_lvcs[_MAX_PYCHAN];

static MDDpySubChan *_GetSub( int cxt )
{
   MDDpySubChan *ch;

   ch = InRange( 0, cxt, _MAX_PYCHAN-1 ) ? _subs[cxt] : (MDDpySubChan *)0;
   return ch;
}

static void _DelSub( MDDpySubChan *ch )
{
   int cxt;

   cxt = ch->cxt();
   if ( InRange( 0, cxt, _MAX_PYCHAN-1 ) )
      _subs[cxt] = (MDDpySubChan *)0;
   delete ch;
}

static MDDpyLVC *_GetLVC( int cxt )
{
   MDDpyLVC *ch;

   ch = InRange( 0, cxt, _MAX_PYCHAN-1 ) ? _lvcs[cxt] : (MDDpyLVC *)0;
   return ch;
}

static void _DelLVC( MDDpyLVC *ch )
{
   int cxt;

   cxt = ch->cxt();
   if ( InRange( 0, cxt, _MAX_PYCHAN-1 ) )
      _lvcs[cxt] = (MDDpyLVC *)0;
   delete ch;
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
   const char *pHost, *pUser, *pc;
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

   ch         = new MDDpySubChan( pHost, pUser, bBin );
   pc         = ch->Start( pHost, pUser );
   cxt        = ch->cxt();
   _subs[cxt] = ch;
   return PyInt_FromLong( cxt );
}

static PyObject *StartSlice( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *pHost, *pUser, *pc;
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
   pc         = ch->Start( pHost, pUser );
   cxt        = ch->cxt();
   _subs[cxt] = ch;
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
   bool        bTape;
   int         cxt, bDir;

   // Usage : SetTapeDir( cxt, bTape )

   bTape = false;
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
         ch->StartTapeSlice( t0, t1 );
      else
         ch->StartTape();
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

static PyObject *Stop( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   int         cxt;

   // Usage : Stop( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   if ( (ch=_GetSub( cxt )) ) {
      ch->Stop();
      _DelSub( ch );
   }
   return _PyReturn( ch ? Py_True : Py_False );
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

   lvc        = new MDDpyLVC( file );
   cxt        = lvc->cxt();
   _lvcs[cxt] = lvc;
   return PyInt_FromLong( cxt );
}

static PyObject *LVCSchema( PyObject *self, PyObject *args )
{
   MDDpyLVC *lvc;
   int       cxt;

   // Usage : LVCSchema( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return Py_None;
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PySchema();
   return Py_None;
}

static PyObject *LVCGetTkrs( PyObject *self, PyObject *args )
{
   MDDpyLVC *lvc;
   int       cxt;

   // Usage : LVCGetTickers( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return Py_None;
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PyGetTickers();
   return Py_None;
}

static PyObject *LVCSnap( PyObject *self, PyObject *args )
{
   MDDpyLVC   *lvc;
   const char *svc, *tkr;
   int         cxt;

   // Usage : LVCSnap( cxt, svc, tkr )

   if ( !PyArg_ParseTuple( args, "iss", &cxt, &svc, &tkr ) )
      return Py_None;
   if ( (lvc=_GetLVC( cxt )) )
      return lvc->PySnap( svc, tkr );
   return Py_None;
}

static PyObject *LVCClose( PyObject *self, PyObject *args )
{
   MDDpyLVC *lvc;
   int       cxt;

   // Usage : LVCClose( cxt )

   if ( !PyArg_ParseTuple( args, "i", &cxt ) )
      return _PyReturn( Py_False );
   if ( (lvc=_GetLVC( cxt )) )
      _DelLVC( lvc );
   return _PyReturn( lvc ? Py_True : Py_False );
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
      rtn = PyTuple_Pack( 2, pyd, Py_None );
      Py_DECREF( pyd );
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
   rtn      = PyTuple_Pack( 2, pyd, data );
   Py_DECREF( pyd );
   if ( data )
      Py_DECREF( data );
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


////////////////////////////
// Book.py
////////////////////////////
static PyObject *GetCleanBook( PyObject *self, PyObject *args )
{
   MDDpySubChan *ch;
   const char *pSvc, *pTkr;
   PyObject   *lst;
   PyObject   *pyRtn, *pyBid, *pyAsk, *pyBr, *pyAr, *pyd, *pyn;
   Book       *bk;
   BookRow    *br, *ar;
   char       *ecnKO[K];
   int         cxt, i, nf, rtn;
   BookRtn     bRtn;
   double      dInversion, dMaturity, d0, dd;
   bool        bOK;

   /*
    * Usage : GetCleanBook( cxt,
    *                       'BBO',
    *                       'EUR/USD',
    *                        ecns[], 
    *                        inversion_threshold, 
    *                        maturity_threshold )
    */
   d0  = ::rtEdge_TimeNs();
   rtn = PyArg_ParseTuple( args, 
                           "issO!dd", 
                           &cxt, 
                           &pSvc, 
                           &pTkr, 
                           &PyList_Type, &lst,
                           &dInversion,
                           &dMaturity );
   bOK  = ( rtn != 0 );
   bOK &= ( (ch=_GetSub( cxt )) != (MDDpySubChan *)0 );
   bk   = bOK ? ch->FindBook( pSvc, pTkr ) : (Book *)0;
   if ( !bk ) {
      pyBr  = PyInt_FromLong( -1 ); 
      pyAr  = PyInt_FromLong( -1 ); 
      dd    = 1000000.0 * ( ::rtEdge_TimeNs() - d0 );
      pyd   = PyFloat_FromDouble( dd );
      pyn   = PyInt_FromLong( 0 );
      pyRtn = PyTuple_Pack( 7, Py_None, Py_None, pyBr, pyAr, lst, pyd, pyn );
      Py_DECREF( pyBr );
      Py_DECREF( pyAr );
      Py_DECREF( pyd );
      Py_DECREF( pyn );
      return pyRtn;
   }

   // Parse Python List

   nf = PyList_Size( lst );
i = 0;
#ifdef TODO_KO_AS_STRING
   for ( i=0; i<nf; i++ ) {
      pyKO     = PyList_GetItem( lst, i );
      ecnKO[i] = PyString_AsString( pyKO );
   }
#endif // TODO_KO_AS_STRING
   ecnKO[i] = (char *)0;
   bRtn  = bk->GetCleanBook( dInversion, dMaturity, ecnKO );
   br    = bRtn._bid;
   ar    = bRtn._ask;
   pyBid = br ? PyFloat_FromDouble( br->GetPrc() ) : Py_None;
   pyAsk = ar ? PyFloat_FromDouble( ar->GetPrc() ) : Py_None;
   pyBr  = PyInt_FromLong( br ? br->_nRow : -1 );
   pyAr  = PyInt_FromLong( ar ? ar->_nRow : -1 );
   dd    = 1000000.0 * ( ::rtEdge_TimeNs() - d0 );
   pyd   = PyFloat_FromDouble( dd );
   pyn   = PyInt_FromLong( bRtn._nItr );

   // [ dBid, dAsk, br, ar, ecnKO, dLatency ]

   pyRtn = PyTuple_Pack( 7, pyBid, pyAsk, pyBr, pyAr, lst, pyd, pyn );
   if ( br ) {
      Py_DECREF( pyBid );
   }
   if ( ar ) {
      Py_DECREF( pyAsk );
   }
   Py_DECREF( pyBr );
   Py_DECREF( pyAr );
   Py_DECREF( pyd );
   Py_DECREF( pyn );
   return pyRtn;
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
    { "OpenByteStr",   OpenByStr, _PY_ARGS, "Open MDD ( svc,tkr ) byte-stream." },
    { "Read",          Read,      _PY_ARGS, "Read MDD update - Subscribe." },
    { "ReadFilter",    ReadFltr,  _PY_ARGS, "Read() Event filter." },
    { "Close",         Close,     _PY_ARGS, "Close MDD ( svc,tkr ) stream." },
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
    { "LVCClose",      LVCClose,   _PY_ARGS, "Close LVC File" },
    /*
     * Library Utilities
     */
    { "GetFields",     GetFields, _PY_ARGS, "Get Field List." },
    { "Log",           Log,       _PY_ARGS, "Set MDD library logger." },
    { "CPU",           CPU,       _PY_ARGS, "Get process CPU usage." },
    { "MemSize",       MemSize,   _PY_ARGS, "Get process memory usage." },
    /* 
     * Book
     */
    { "GetCleanBook",   GetCleanBook, _PY_ARGS, "Book.GetCleanBook" },
    /* 
     * Utility
     */
    { "HasMethod",   HasMethod,  _PY_ARGS, "True if this DLL exports the fcn." },
    { NULL, NULL, 0, NULL}        /* Sentinel */
};

#ifdef _MDD_PYTHON3
static PyModuleDef mddModule = { PyModuleDef_HEAD_INIT,
                                 "MDDirect39",
                                 "MD-Direct for Python 3.x",
                                 -1,
                                 EdgeMethods
                               };

PyMODINIT_FUNC PyInit_MDDirect39( void )
{
   _pMethods = EdgeMethods;
   ::memset( _subs, 0, sizeof( _subs ) );
   ::memset( _lvcs, 0, sizeof( _lvcs ) );
   return PyModule_Create( &mddModule );
} 
#else
PyMODINIT_FUNC initMDDirect27( void )
{
   _pMethods = EdgeMethods;
   ::memset( _subs, 0, sizeof( _subs ) );
   ::memset( _lvcs, 0, sizeof( _lvcs ) );
   Py_InitModule( "MDDirect27", EdgeMethods );
}
#endif // _MDD_PYTHON3 
