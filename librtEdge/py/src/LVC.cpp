/******************************************************************************
*
*  LVC.cpp
*     MD-Direct Subscription Channel
*
*  REVISION HISTORY:
*      3 APR 2019 jcs  Created.
*     19 NOV 2020 jcs  Build  2: PyGetTickers()
*     22 NOV 2020 jcs  Build  3: PyObjects
*      3 FEB 2022 jcs  Build  5: MDDpyLVC.PySnap() : _tUpd
*
*  (c) 1994-2022, Gatea, Ltd.
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
MDDpyLVC::MDDpyLVC( const char *file ) :
   RTEDGE::LVC( file )
{
}


///////////////////////////////
// Operations
///////////////////////////////
PyObject *MDDpyLVC::PySchema()
{
   RTEDGE::Schema &sch = GetSchema();
   RTEDGE::Field  *fld;
   PyObjects       vdb;
   PyObject       *rtn, *pyF, *pyN;
   int             i, nf;

   // Pre-condition(s)

   if ( !(nf=sch.Size()) )
      return Py_None;

   // Iterate

   for ( sch.reset(); (sch)(); ) {
      fld = sch.field();
      pyF = PyInt_FromLong( fld->Fid() );
      pyN = PyString_FromString( fld->Name() );
      vdb.push_back( ::PyTuple_Pack( 2, pyF, pyN ) );
   }
   nf  = (int)vdb.size();
   rtn = ::PyList_New( nf );
   for ( i=0; i<nf; ::PyList_SetItem( rtn, i, vdb[i] ), i++ );
   return rtn;
}

PyObject *MDDpyLVC::PyGetTickers()
{
   RTEDGE::Messages &mdb = ViewAll().msgs();
   PyObjects         vdb;
   PyObject         *rtn, *pyS, *pyT;
   size_t            i, nm;

   // Pre-condition(s)

   if ( !(nm=mdb.size()) )
      return Py_None;

   // Iterate

   for ( i=0; i<nm; i++ ) {
      pyS = PyString_FromString( mdb[i]->Service() );
      pyT = PyString_FromString( mdb[i]->Ticker() );
      vdb.push_back( ::PyTuple_Pack( 2, pyS, pyT ) );
   }
   rtn = ::PyList_New( nm );
   for ( i=0; i<nm; ::PyList_SetItem( rtn, i, vdb[i] ), i++ );
   return rtn;
}

PyObject *MDDpyLVC::PySnap( const char *svc, const char *tkr )
{
   RTEDGE::Message *msg;
   MDDPY::Field     fld;
   mddField        *fdb;
   PyObject        *rtn, *pyF, *pyV, *pyT;
   double           tm;
   int              i, nf, ty;

   // Pre-condition(s)

   if ( !(msg=Snap( svc, tkr )) )
      return Py_None;
   if ( !(nf=msg->NumFields()) )
      return Py_None;

   // Iterate : [ tUpd, Svc, Tkr, fld1, fld2, ... ]

   ::LVCData &ld = msg->dataLVC();

   rtn = ::PyList_New( nf+3 );
   fdb = (mddField *)msg->Fields();
   tm  = (double)ld._tUpdUs / 1000000.0;
   tm += ld._tUpd;
   ::PyList_SetItem( rtn, 0, PyFloat_FromDouble( tm ) );
   ::PyList_SetItem( rtn, 1, PyString_FromString( svc ) );
   ::PyList_SetItem( rtn, 2, PyString_FromString( tkr ) );
   for ( i=0; i<nf; i++ ) {
      fld.Update( fdb[i] );
      pyF = PyInt_FromLong( fld.Fid() );
      pyV = fld.GetValue( ty );
      pyT = PyInt_FromLong( ty );
      ::PyList_SetItem( rtn, i+3, ::PyTuple_Pack( 3, pyF, pyV, pyT ) );
   }
   return rtn;
}

