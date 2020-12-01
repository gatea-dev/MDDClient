/******************************************************************************
*
*  LVC.cpp
*     MD-Direct Subscription Channel
*
*  REVISION HISTORY:
*      3 APR 2019 jcs  Created.
*     19 NOV 2020 jcs  Build  2: PyGetTickers()
*     22 NOV 2020 jcs  Build  3: PyObjects
*
*  (c) 1994-2020 Gatea, Ltd.
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
      vdb.push_back( PyTuple_Pack( 2, pyF, pyN ) );
   }
   nf  = (int)vdb.size();
   rtn = PyTuple_New( nf );
   for ( i=0; i<nf; PyTuple_SetItem( rtn, i, vdb[i] ), i++ );
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
      vdb.push_back( PyTuple_Pack( 2, pyS, pyT ) );
   }
   rtn = PyTuple_New( nm );
   for ( i=0; i<nm; PyTuple_SetItem( rtn, i, vdb[i] ), i++ );
   return rtn;
}

PyObject *MDDpyLVC::PySnap( const char *svc, const char *tkr )
{
   RTEDGE::Message *msg;
   MDDPY::Field     fld;
   mddField        *fdb;
   PyObject        *rtn, *pyF, *pyV, *pyT;
   int              i, nf, ty;

   // Pre-condition(s)

   if ( !(msg=Snap( svc, tkr )) )
      return Py_None;
   if ( !(nf=msg->NumFields()) )
      return Py_None;

   // Iterate

   rtn = PyTuple_New( nf+2 );
   fdb = (mddField *)msg->Fields();
   PyTuple_SetItem( rtn, 0, PyString_FromString( svc ) );
   PyTuple_SetItem( rtn, 1, PyString_FromString( tkr ) );
   for ( i=0; i<nf; i++ ) {
      fld.Update( fdb[i] );
      pyF = PyInt_FromLong( fld.Fid() );
      pyV = fld.GetValue( ty );
      pyT = PyInt_FromLong( ty );
      PyTuple_SetItem( rtn, i+2, PyTuple_Pack( 3, pyF, pyV, pyT ) );
   }
   return rtn;
}

