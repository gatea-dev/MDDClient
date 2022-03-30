/******************************************************************************
*
*  PubChannel.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*      7 JAN 2015 jcs  Build 29: ByteStreamFld
*     28 FEB 2015 jcs  Build 30: SetHeartbeat()
*      6 JUL 2015 jcs  Build 31: GetSocket(); OnOverflow()
*     15 APR 2016 jcs  Build 32: SetUserPubMsgTy(); OnIdle()
*     14 JUL 2017 jcs  Build 34: class Channel
*     29 APR 2020 jcs  Build 43: BDS
*     30 MAR 2022 jcs  Build 52: SetUnPacked()
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <PubChannel.h>
#include <Update.h>


namespace librtEdgePRIVATE
{

////////////////////////////////////////////////
//
//     c l a s s   P u b C h a n n e l
//
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
PubChannel::PubChannel( IrtEdgePublisher ^cli, const char *pubName ) :
   RTEDGE::PubChannel( pubName ),
   _cli( cli ),
   _schema( gcnew librtEdge::rtEdgeSchema() )
{
}

PubChannel::~PubChannel()
{
   _schema = nullptr;
}


//////////////////////////
// Schema Stuff
//////////////////////////
librtEdge::rtEdgeSchema ^PubChannel::schema()
{
   return _schema;
}


//////////////////////////
// RTEDGE::PubChannel Interface
//////////////////////////
RTEDGE::Update *PubChannel::CreateUpdate()
{
   return new RTEDGE::Update( *this );
}


//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void PubChannel::OnConnect( const char *msg, bool bUP )
{
   librtEdge::rtEdgeState state;

   if ( bUP )
      state = librtEdge::rtEdgeState::edg_up;
   else
      state = librtEdge::rtEdgeState::edg_down;
   _cli->OnConnect( gcnew String( msg ), state );
}

void PubChannel::OnPubOpen( const char *tkr, void *arg )
{
   IntPtr StreamID;

   StreamID = IntPtr( arg );
   _cli->OnOpen( gcnew String( tkr ), StreamID );
}

void PubChannel::OnPubClose( const char *tkr )
{
   _cli->OnClose( gcnew String( tkr ) );
}

void PubChannel::OnOpenBDS( const char *tkr, void *arg )
{
   IntPtr StreamID;

   StreamID = IntPtr( arg );
   _cli->OnOpenBDS( gcnew String( tkr ), StreamID );
}

void PubChannel::OnCloseBDS( const char *tkr )
{
   _cli->OnCloseBDS( gcnew String( tkr ) );
}

void PubChannel::OnSchema( RTEDGE::Schema &sch )
{
   _schema->Set( sch );
   _cli->OnSchema( _schema );
}

void PubChannel::OnOverflow()
{
   _cli->OnOverflow();
}

void PubChannel::OnIdle()
{
   _cli->OnIdle();
}

void PubChannel::OnSymListQuery( int nSym )
{
   _cli->OnSymListQuery( nSym );
}

void PubChannel::OnRefreshImage( const char *tkr, void *arg )
{
   IntPtr StreamID;

   StreamID = IntPtr( arg );
   _cli->OnRefreshImage( gcnew String( tkr ), StreamID );
}


} // namespace librtEdgePRIVATE


namespace librtEdge
{

////////////////////////////////////////////////
//
//  c l a s s   r t E d g e P u b l i s h e r
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
rtEdgePublisher::rtEdgePublisher( String ^hosts, String ^pubName ) :
   _pub( new librtEdgePRIVATE::PubChannel( this, _pStr( pubName ) ) ),
   _hosts( hosts ),
   _bInteractive( true ),
   _bSchema( false ),
   _bBinary( false )
{
   _chan = _pub;
}

rtEdgePublisher::rtEdgePublisher( String ^hosts,
                                  String ^pubName,
                                  bool    bInteractive ) :
   _pub( new librtEdgePRIVATE::PubChannel( this, _pStr( pubName ) ) ),
   _hosts( hosts ),
   _bInteractive( bInteractive ),
   _bSchema( false ),
   _bBinary( false )
{
   // RTEDGE::PubChannel::Start() calls rtEdge_PubInitSchema()

   _chan = _pub;
   PubStart();
}

rtEdgePublisher::rtEdgePublisher( String ^hosts,
                                  String ^pubName,
                                  bool    bInteractive,
                                  bool    bSchema,
                                  bool    bBinary ) :
   _pub( new librtEdgePRIVATE::PubChannel( this, _pStr( pubName ) ) ),
   _hosts( hosts ),
   _bInteractive( bInteractive ),
   _bSchema( bSchema ),
   _bBinary( bBinary )
{
   // RTEDGE::PubChannel::Start() calls rtEdge_PubInitSchema()

   _chan = _pub;
   PubStart();
}

rtEdgePublisher::~rtEdgePublisher()
{
   Stop();
   delete _pub;
   _hosts = nullptr;
}


/////////////////////////////////
// Operations
/////////////////////////////////
String ^rtEdgePublisher::PubStart()
{
   SetBinary( _bBinary );
   _con = gcnew String( _pub->Start( _pStr( _hosts ), _bInteractive ) );
   return pConn();
}

String ^rtEdgePublisher::PubStartConnectionless()
{
   _con = gcnew String( _pub->StartConnectionless( _pStr( _hosts ) ) );
   return pConn();
}

void rtEdgePublisher::Stop()
{
   _pub->Stop();
}

void rtEdgePublisher::SetBinary( bool bBin )
{
   _pub->SetBinary( bBin );
}

void rtEdgePublisher::SetUnPacked( bool bUnPacked )
{
   _pub->SetUnPacked( bUnPacked );
}

void rtEdgePublisher::SetPerms( bool bPerms )
{
   _pub->SetPerms( bPerms );
}

void rtEdgePublisher::SetUserPubMsgTy( bool bUserMsgTy )
{
   _pub->SetUserPubMsgTy( bUserMsgTy );
}

void rtEdgePublisher::SetRandomize( bool bRandom )
{
   _pub->SetRandomize( bRandom );
}

void rtEdgePublisher::SetHeartbeat( int tHbeat )
{
   _pub->SetHeartbeat( tHbeat );
}

void rtEdgePublisher::SetIdleCallback( bool bIdleCbk )
{
   _pub->SetIdleCallback( bIdleCbk );
}


/////////////////////////////////
// Access
/////////////////////////////////
int rtEdgePublisher::GetSocket()
{
   return _pub->GetSocket();
}

int rtEdgePublisher::SetRxBufSize( int bufSiz )
{
   return _pub->SetRxBufSize( bufSiz );
}

int rtEdgePublisher::GetRxBufSize()
{
   return _pub->GetRxBufSize();
}

int rtEdgePublisher::SetThreadProcessor( int cpu )
{
   return _pub->SetThreadProcessor( cpu );
}

int rtEdgePublisher::GetThreadProcessor()
{
   return _pub->GetThreadProcessor();
}

long rtEdgePublisher::GetThreadID()
{
   return _pub->GetThreadID();
}

String ^rtEdgePublisher::pPubName()
{
   return gcnew String( _pub->pPubName() );
}

bool rtEdgePublisher::IsBinary()
{
   return _pub->IsBinary();
}

bool rtEdgePublisher::IsUnPacked()
{
   return _pub->IsUnPacked();
}

void rtEdgePublisher::PubSetHopCount( int hopCnt )
{
   _pub->PubSetHopCount( hopCnt );
}

int rtEdgePublisher::PubGetHopCount()
{
   return _pub->PubGetHopCount();
}

array<Byte> ^rtEdgePublisher::PubGetData()
{
   return rtEdge::_memcpy( _pub->PubGetData() );
}


/////////////////////////////////
// IrtEdgePublisher Interface - Publish 
/////////////////////////////////
int rtEdgePublisher::GetFid( String ^fldName )
{
   return _pub->schema()->Fid( fldName );
}


/////////////////////////////////
// BDS Publication
/////////////////////////////////
int rtEdgePublisher::PublishBDS( String          ^bds, 
                                 int              StreamID, 
                                 array<String ^> ^symbols )
{
   char *syms[K];
   int   i, n, nb;

   // 1K at a time ...

   nb = 0;
   n  = 0;
   for ( i=0; i<symbols->Length; i++ ) {
      syms[n++] = (char *)_pStr( symbols[i] );
      if ( n == K-1 ) {
         syms[n] = (char *)0;
         nb     += _pub->PublishBDS( _pStr( bds ), StreamID, syms );
         n       = 0;
      }
   }
   if ( n ) {
      syms[n] = (char *)0;
      nb     += _pub->PublishBDS( _pStr( bds ), StreamID, syms );
   }
   return nb;
}

} // namespace librtEdge
