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
VectorC::VectorC( IVector    ^cli,
                  const char *svc,
                  const char *tkr ) :
   _cli( cli ),
   RTEDGE::Vector( svc, tkr )
{
   ::memset( &_pubDataC, 0, sizeof( _pubDataC ) );
}

VectorC::~VectorC()
{
}


//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void VectorC::OnData( RTEDGE::VectorImage &img )
{
   array<double> ^ddb;
   size_t         sz = img.size();

   idb = gcnew array<double>( sz );
   for ( size_t i=0; i<sz; ddb[i]=img[i], i++ );
   _cli->OnData( ddb );
}

void VectorC::OnData( RTEDGE::VectorUpdate &upd )
{
   array<VectorValue> ^udb; 
   size_t              sz = upd.size();

   udb = gcnew array<VectorValue>( sz );
   for ( size_t i=0; i<sz; i++ )
      udb[i] = gcnew VectorValue( upd[i]._position, upd[i]._value );
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
Vector::Vector( String ^svc, String ^tkr ) :
   _vec( new librtEdgePRIVATE::VectorC( this, _pStr( svc ), _pStr( tkr ) ) )
{ ; }

Vector::~Vector()
{
   delete _vec;
   _vec = (librtEdgePRIVATE::VectorC *)0;
}


/////////////////////////////////
// Access
/////////////////////////////////
String ^Vector::svc()
{
   return gcnew String( _vec->svc() );
}

String ^Vector::tkr()
{
   return gcnew String( _vec->tkr() );
}

array<double> ^Vector::Get()
{
   VectorImage    img;
   array<double> ^ddb;
   size_t         sz = _vec->Get( img ).size();

   ddb = gcnew array<double>( sz );
   for ( size_t i=0; i<sz; ddb[i]=img[i], i++ );
   return img;
}


/////////////////////////////////
// Mutator
/////////////////////////////////
int Vector::Update( array<double> ^ddb )
{
   RTEDGE::VectorImage img;

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
   return _vec->Subscribe( sub.cpp() );
}

void Vector::Unsubscribe( librtEdge::rtEdgeSubscriber ^sub )
{
   return _vec->Unsubscribe( sub.cpp() );
}

int Vector::Publish( librtEdge::rtEdgePublisher ^pub, int StreamID, bool bImg )
{
   return _vec->Publish( pub.cpp(), StreamID, bImg );
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

String ^Dump( array<VectorValue> ^upd, bool bPage )
{
   RTEDGE::VectorUpdate udb;
   RTEDGE::VectorValue  v;
   string               s;
   int                  i;

   for ( i=0; i<upd.Length; i++ ) {
      v._position = upd[i]._position;
      v._value    = upd[i]._value;
      udb.push_back( v );
   }
   s = _vec->Dump( udb, bPage );
   return gcnew String( s.data() );
}

} // namespace librtEdge
