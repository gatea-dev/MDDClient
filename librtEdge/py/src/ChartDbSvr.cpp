/******************************************************************************
*
*  ChartDbSvr.h
*     MD-Direct ChartDbSvr.cpp
*
*  REVISION HISTORY:
*      8 JUL 2025 jcs  Created.
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>


////////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      M D D p y L V C
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
MDDpyChartDB::MDDpyChartDB( const char *file ) :
   RTEDGE::ChartDB( file )
{
}


///////////////////////////////
// Operations
///////////////////////////////
PyObject *MDDpyChartDB::PyGetTickers()
{
   ::MDDResult  mdb = Query();
   ::MDDRecDef *rdb = mdb._recs;
   PyObjects    vdb;
   PyObject    *rtn, *pyS, *pyT, *pyF, *pyI;
   int          i, nm;

   // Pre-condition(s)

   if ( !(nm=mdb._nRec) )
      return _PyReturn( Py_None );

   // [ [ Svc, Tkr, FID, Interval ], [ Svc, Tkr, FID, Interval ], ... ]

   for ( i=0; i<mdb._nRec; i++ ) {
      pyS = PyString_FromString( rdb[i]._pSvc );
      pyT = PyString_FromString( rdb[i]._pTkr );
      pyF = PyInt_FromLong( rdb[i]._fid );
      pyI = PyInt_FromLong( rdb[i]._interval );
      vdb.push_back( ::mdd_PyList_Pack4( pyS, pyT, pyF, pyI ) );
   }
   rtn = ::PyList_New( nm );
   for ( i=0; i<nm; ::PyList_SetItem( rtn, i, vdb[i] ), i++ );
   return rtn;
}

PyObject *MDDpyChartDB::PySnap( const char *svc, 
                                const char *tkr, 
                                int         fid, 
                                size_t      num )
{
   RTEDGE::CDBData &d   = View( svc, tkr, fid );

   if ( d.Size() )
      return _cpp2py( d, num );
   return _PyReturn( Py_None );
}


///////////////////////////////
// Helpers
///////////////////////////////
PyObject *MDDpyChartDB::_cpp2py( RTEDGE::CDBData &d, size_t num )
{
   ::CDBData &cpp = d.data();
   float     *vdb = d.flds();
   PyObject  *rtn, *udb, *fdb;
   size_t     i, i0, n, sz;

   // Support for last 'num' values; 0 means all

   sz  = d.Size();
   n   = num ? num : sz;
   if ( !n )
      return _PyReturn( Py_None );

   /*
    * [ Age, [ tUnix1, tUnix2, ... ], [ Val1, Val2, ... ] ]
    */
   rtn = ::PyList_New( 3 );
   udb = ::PyList_New( n );
   fdb = ::PyList_New( n );
   i0  = num ? gmax( 0, sz-num ) : 0;
   for ( i=0; i<n; i++, i0++ ) {
      ::PyList_SetItem( udb, i, PyInt_FromLong( d.SeriesTime( i0 ) ) );
      ::PyList_SetItem( fdb, i, PyFloat_FromDouble( vdb[i0] ) );
   }
   ::PyList_SetItem( rtn, 0, PyFloat_FromDouble( cpp._dAge ) );
   ::PyList_SetItem( rtn, 1, udb );
   ::PyList_SetItem( rtn, 2, fdb );
   return rtn;
}

