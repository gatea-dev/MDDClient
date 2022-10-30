/******************************************************************************
*
*  Vector.cpp
*
*  REVISION HISTORY:
*     21 OCT 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Vector.h>


namespace librtEdgePRIVATE
{

////////////////////////////////////////////////
//
//       c l a s s    V e c t o r C
//
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
VectorC::VectorC( librtEdgePRIVATE::IVector ^cli,
                  const char                *svc,
                  const char                *tkr,
                  int                        precision ) :
   RTEDGE::Vector( svc, tkr, precision ),
   _cli( cli )
{ ; }


//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void VectorC::OnData( RTEDGE::Doubles &img )
{
   cli::array<double> ^ddb;
   size_t              sz = img.size();

   ddb = gcnew cli::array<double>( sz );
   for ( size_t i=0; i<sz; ddb[i]=img[i], i++ );
   _cli->OnData( ddb );
}

void VectorC::OnData( RTEDGE::VectorUpdate &upd )
{
   cli::array<librtEdge::VectorValue ^> ^udb; 
   size_t                                sz = upd.size();

   udb = gcnew cli::array<librtEdge::VectorValue ^>( sz );
   for ( size_t i=0; i<sz; i++ )
      udb[i] = gcnew librtEdge::VectorValue( upd[i]._position, upd[i]._value );
   _cli->OnData( udb );
}

void VectorC::OnError( const char *msg )
{
   _cli->OnError( gcnew String( msg ) );
}

void VectorC::OnPublishComplete( int nByte )
{
   _cli->OnPublishComplete( nByte );
}

} // namespace librtEdgePRIVATE


namespace librtEdge
{

////////////////////////////////////////////////
//
//         c l a s s   V e c t o r
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
Vector::Vector( String ^svc, String ^tkr, int precision ) :
   _vec( new librtEdgePRIVATE::VectorC( this, 
                                        rtEdge::_pStr( svc ), 
                                        rtEdge::_pStr( tkr ),
                                        precision ) )
{ ; }

Vector::~Vector()
{
   delete _vec;
   _vec = (librtEdgePRIVATE::VectorC *)0;
}


/////////////////////////////////
// Access
/////////////////////////////////
String ^Vector::Service()
{
   return gcnew String( _vec->Service() );
}

String ^Vector::Ticker()
{
   return gcnew String( _vec->Ticker() );
}

cli::array<double> ^Vector::Get()
{
   RTEDGE::Doubles     img;
   cli::array<double> ^ddb;
   size_t              sz = _vec->Get( img ).size();

   ddb = gcnew cli::array<double>( sz );
   for ( size_t i=0; i<sz; ddb[i]=img[i], i++ );
   return ddb;
}


/////////////////////////////////
// Mutator
/////////////////////////////////
int Vector::Update( cli::array<double> ^ddb )
{
   RTEDGE::Doubles img;

   for ( int i=0; i<ddb->Length; img.push_back( ddb[i++] ) );
   return (int)_vec->Update( img );
}

int Vector::UpdateAt( int idx, double val )
{
   return _vec->UpdateAt( idx, val );
}

int Vector::ShiftLeft( int num, bool bRollToEnd )
{
   return _vec->ShiftLeft( num, bRollToEnd );
}

int Vector::ShiftRight( int num, bool bRollToFront )
{
   return _vec->ShiftRight( num, bRollToFront );
}


////////////////////////////////////
// Operations
////////////////////////////////////
int Vector::Subscribe( librtEdge::rtEdgeSubscriber ^sub )
{
   return _vec->Subscribe( sub->cpp() );
}

void Vector::Unsubscribe( librtEdge::rtEdgeSubscriber ^sub )
{
   return _vec->Unsubscribe( sub->cpp() );
}

int Vector::Publish( librtEdge::rtEdgePublisher ^pub, int StreamID, bool bImg )
{
   return _vec->Publish( pub->upd(), StreamID, bImg );
}


////////////////////////////////////
// Debugging
////////////////////////////////////
String ^Vector::Dump( bool bPage )
{
   string s;

   s = _vec->Dump( bPage );
   return gcnew String( s.data() );
}

String ^Vector::Dump( cli::array<VectorValue ^> ^upd, bool bPage )
{
   RTEDGE::VectorUpdate udb;
   RTEDGE::VectorValue  v;
   string               s;
   int                  i;

   for ( i=0; i<upd->Length; i++ ) {
      v._position = upd[i]->_position;
      v._value    = upd[i]->_value;
      udb.push_back( v );
   }
   s = _vec->Dump( udb, bPage );
   return gcnew String( s.data() );
}

} // namespace librtEdge
