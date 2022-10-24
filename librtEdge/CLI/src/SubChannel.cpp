/******************************************************************************
*
*  SubChannel.cpp
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*     13 NOV 2014 jcs  QueryCache()
*      7 JAN 2015 jcs  Build 29: Chain
*      6 JUL 2015 jcs  Build 31: GetSocket()
*     15 APR 2016 jcs  Build 32: IsSnapshot(); OnDead(); OnIdle()
*     14 JUL 2017 jcs  Build 34: class Channel
*     13 OCT 2017 jcs  Build 36: Tape
*     10 DEC 2018 jcs  Build 41: VS2017
*     29 APR 2020 jcs  Build 43: BDS 
*     10 SEP 2020 jcs  Build 44: SetTapeDirection(); Query()
*     30 SEP 2020 jcs  Build 45: Parse() / ParseView()
*      3 DEC 2020 jcs  Build 47: XxxxPumpFullTape()
*     23 MAY 2022 jcs  Build 54: OnError()
*     22 SEP 2022 jcs  Build 56: Rename StartTape() to PumpTape()
*     23 OCT 2022 jcs  Build 58: cli::array<>
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <SubChannel.h>


namespace librtEdgePRIVATE
{

////////////////////////////////////////////////
//
//     c l a s s   S u b C h a n n e l
//
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
SubChannel::SubChannel( IrtEdgeSubscriber ^cli ) :
   _cli( cli ),
   _schema( gcnew librtEdge::rtEdgeSchema() ),
   _upd( gcnew librtEdge::rtEdgeData() )
{
}

SubChannel::~SubChannel()
{
   _schema = nullptr;
   _upd    = nullptr;
}


//////////////////////////
// Access
//////////////////////////
librtEdge::rtEdgeData ^SubChannel::data()
{
   return _upd;
}

librtEdge::rtEdgeSchema ^SubChannel::schema()
{
   return _schema;
}



//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void SubChannel::OnConnect( const char *msg, bool bUP )
{
   librtEdge::rtEdgeState state;

   if ( bUP )
      state = librtEdge::rtEdgeState::edg_up;
   else
      state = librtEdge::rtEdgeState::edg_down;
   _cli->OnConnect( gcnew String( msg ), state );
}

void SubChannel::OnService( const char *msg, bool bUP )
{
   librtEdge::rtEdgeState state;

   if ( bUP )
      state = librtEdge::rtEdgeState::edg_up;
   else
      state = librtEdge::rtEdgeState::edg_down;
   _cli->OnService( gcnew String( msg ), state );
}

void SubChannel::OnData( RTEDGE::Message &msg )
{
   _upd->Set( msg );
   _cli->OnData( _upd );
}

void SubChannel::OnRecovering( RTEDGE::Message &msg )
{
   _upd->Set( msg );
   _cli->OnRecovering( _upd );
}

void SubChannel::OnStale( RTEDGE::Message &msg )
{
   _upd->Set( msg );
   _cli->OnStale( _upd );
}

void SubChannel::OnDead( RTEDGE::Message &msg, const char *err )
{
   _upd->Set( msg );
   _cli->OnDead( _upd, gcnew String( err ) );
}

void SubChannel::OnStreamDone( RTEDGE::Message &msg )
{
   _upd->Set( msg );
   _cli->OnStreamDone( _upd );
}

void SubChannel::OnSymbol( RTEDGE::Message &msg, const char *err )
{
   _upd->Set( msg );
   _cli->OnSymbol( _upd, gcnew String( err ) );
}

void SubChannel::OnSchema( RTEDGE::Schema &sch )
{
   _schema->Set( sch );
   _cli->OnSchema( _schema );
}

void SubChannel::OnIdle()
{
   _cli->OnIdle();
}

void SubChannel::OnError( const char *err )
{
   _cli->OnError( gcnew String( err ) );
}

} // namespace librtEdgePRIVATE


namespace librtEdge
{

////////////////////////////////////////////////
//
//  c l a s s   r t E d g e S u b s c r i b e r
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
rtEdgeSubscriber::rtEdgeSubscriber( String ^hosts,
                                    String ^user ) :
   _sub( new librtEdgePRIVATE::SubChannel( this ) ),
   _hosts( hosts ),
   _user( user ),
   _bBinary( false ),
   _qry( gcnew rtEdgeData() ),
   _fldGet( gcnew rtEdgeField() ),
   _qryAll( new ::MDDResult() ),
   _parse( gcnew rtEdgeData() )
{
  _chan = _sub;
}

rtEdgeSubscriber::rtEdgeSubscriber( String ^hosts,
                                    String ^user,
                                    bool    bBinary ) :
   _sub( new librtEdgePRIVATE::SubChannel( this ) ),
   _hosts( hosts ),
   _user( user ),
   _bBinary( bBinary ),
   _qry( gcnew rtEdgeData() ),
   _fldGet( gcnew rtEdgeField() ),
   _qryAll( new ::MDDResult() ),
   _parse( gcnew rtEdgeData() )
{
  _chan = _sub;
}

rtEdgeSubscriber::~rtEdgeSubscriber()
{
   Stop();
   delete _qryAll;
   delete _sub;
   _sub    = (librtEdgePRIVATE::SubChannel *)0;
   _hosts  = nullptr;
   _user   = nullptr;
   _qry    = nullptr;
   _fldGet = nullptr;
   _parse  = nullptr;
}


/////////////////////////////////
// Operations
/////////////////////////////////
String ^rtEdgeSubscriber::Start()
{
   _sub->SetBinary( _bBinary );
   _con = gcnew String( _sub->Start( _pStr( _hosts ), _pStr( _user ) ) );
   return pConn();
}

void rtEdgeSubscriber::Stop()
{
   _sub->Stop();
}

bool rtEdgeSubscriber::IsTape()
{
   return _sub->IsTape();
}

void rtEdgeSubscriber::PumpTape()
{
   _sub->PumpTape();
}

void rtEdgeSubscriber::PumpTapeSlice( DateTime ^tStart, DateTime ^tEnd )
{
   const char *t0 = _pStr( TapeTimeString( tStart ) );
   const char *t1 = _pStr( TapeTimeString( tEnd ) );

   _sub->PumpTapeSlice( t0, t1 );
}

void rtEdgeSubscriber::PumpTapeSliceSample( DateTime ^tStart, 
                                             DateTime ^tEnd, 
                                             int       tInt, 
                                             String   ^pFlds )
{
   const char *t0 = _pStr( TapeTimeString( tStart ) );
   const char *t1 = _pStr( TapeTimeString( tEnd ) );

   _sub->PumpTapeSliceSample( t0, t1, tInt, _pStr( pFlds ) );
}

void rtEdgeSubscriber::StopTape()
{
   _sub->StopTape();
}

void rtEdgeSubscriber::SetBinary( bool bBin )
{
   _bBinary = bBin;
}
      
void rtEdgeSubscriber::SetRandomize( bool bRandom )
{
   _sub->SetRandomize( bRandom );
}

void rtEdgeSubscriber::SetIdleCallback( bool bIdleCbk )
{
   _sub->SetIdleCallback( bIdleCbk );
}

void rtEdgeSubscriber::SetHeartbeat( int tHbeat )
{
   _sub->SetHeartbeat( tHbeat );
}

void rtEdgeSubscriber::SetTapeDirection( bool bTapeDir )
{
   _sub->SetTapeDirection( bTapeDir );
}


/////////////////////////////////
// Access
/////////////////////////////////
int rtEdgeSubscriber::GetSocket()
{
   return _sub->GetSocket();
}

int rtEdgeSubscriber::SetRxBufSize( int bufSiz )
{
   return _sub->SetRxBufSize( bufSiz );
}

int rtEdgeSubscriber::GetRxBufSize()
{
   return _sub->GetRxBufSize();
}

int rtEdgeSubscriber::SetThreadProcessor( int cpu )
{
   return _sub->SetThreadProcessor( cpu );
}

int rtEdgeSubscriber::GetThreadProcessor()
{
   return _sub->GetThreadProcessor();
}

long rtEdgeSubscriber::GetThreadID()
{
   return _sub->GetThreadID();
}

rtEdgeSchema ^rtEdgeSubscriber::schema()
{
   return _sub->schema();
}

bool rtEdgeSubscriber::IsBinary()
{
   return _sub->IsBinary();
}

bool rtEdgeSubscriber::IsSnapshot()
{
   return _sub->IsSnapshot();
}


/////////////////////////////////
// Get Field 
/////////////////////////////////
bool rtEdgeSubscriber::HasField( String ^fieldName )
{
   RTEDGE::Message *msg;

   msg = _sub->data()->msg();
   return msg->HasField( _pStr( fieldName ) );
}

bool rtEdgeSubscriber::HasField( int fid )
{
   RTEDGE::Message *msg;

   msg = _sub->data()->msg();
   return msg->HasField( fid );
}

rtEdgeField ^rtEdgeSubscriber::GetField( String ^fieldName )
{
   RTEDGE::Message *msg;
   RTEDGE::Field   *fld;

   msg = _sub->data()->msg();
   fld = msg->GetField( _pStr( fieldName ) );
   return fld ? _fldGet->Set( fld ) : nullptr;
}

rtEdgeField ^rtEdgeSubscriber::GetField( int fid )
{
   RTEDGE::Message *msg;
   RTEDGE::Field   *fld;

   msg = _sub->data()->msg();
   fld = msg->GetField( fid );
   return fld ? _fldGet->Set( fld ) : nullptr;
}



/////////////////////////////////
// Subscribe / Unsubscribe
/////////////////////////////////
int rtEdgeSubscriber::Subscribe( String ^svc, String ^tkr, int arg )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   return _sub->Subscribe( pSvc, pTkr, (VOID_PTR)arg );
}

void rtEdgeSubscriber::Unsubscribe( String ^svc, String ^tkr )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _sub->Unsubscribe( pSvc, pTkr );
}


////////////////////////////////////
// ByteStream
////////////////////////////////////
int rtEdgeSubscriber::Subscribe( ByteStream ^bStr )
{
   return _sub->Subscribe( bStr->bStr() );
}

int rtEdgeSubscriber::Unsubscribe( ByteStream ^bStr )
{
   return _sub->Unsubscribe( bStr->bStr() );
}


////////////////////////////////////
// Chain
////////////////////////////////////
int rtEdgeSubscriber::Subscribe( Chain ^chn )
{
   return _sub->Subscribe( chn->chn() );
}

int rtEdgeSubscriber::Unsubscribe( Chain ^chn )
{
   return _sub->Unsubscribe( chn->chn() );
}


////////////////////////////////////
// BDS
////////////////////////////////////
int rtEdgeSubscriber::OpenBDS( String ^svc, String ^bds, int arg )
{
   const char *pSvc, *pBDS;

   pSvc = (const char *)_pStr( svc );
   pBDS = (const char *)_pStr( bds );
   return _sub->OpenBDS( pSvc, pBDS, (VOID_PTR)arg );
}

int rtEdgeSubscriber::CloseBDS( String ^svc, String ^bds )
{
   const char *pSvc, *pBDS;

   pSvc = (const char *)_pStr( svc );
   pBDS = (const char *)_pStr( bds );
   return _sub->CloseBDS( pSvc, pBDS );
}


////////////////////////////////////
// Cache Query
////////////////////////////////////
void rtEdgeSubscriber::SetCache( bool bCache )
{
   _sub->SetCache( bCache );
}

bool rtEdgeSubscriber::IsCache()
{
   return _sub->IsCache();
}

rtEdgeData ^rtEdgeSubscriber::QueryCache( String ^svc, String ^tkr )
{
   const char      *pSvc, *pTkr;
   RTEDGE::Message *msg;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _qry->Clear();
   if ( (msg=_sub->QueryCache( pSvc, pTkr )) )
      _qry->Set( *msg );
   return _qry;
}


////////////////////////////////////
// Database Directory Query
////////////////////////////////////
MDDResult ^rtEdgeSubscriber::Query()
{
   *_qryAll = _sub->Query();
   return gcnew MDDResult( *_qryAll );
}

void rtEdgeSubscriber::FreeResult()
{
   _sub->FreeResult();
}


////////////////////////////////////
// Parse Only
////////////////////////////////////
rtEdgeData ^rtEdgeSubscriber::Parse( cli::array<byte> ^data )
{
   RTEDGE::Message *msg;
   ::rtBUF          b;

   b = _memcpy( data );
   _parse->Clear();
   if ( (msg=_sub->Parse( b._data, b._dLen )) )
      _parse->Set( *msg );
   return _parse;
}

rtEdgeData ^rtEdgeSubscriber::ParseView( IntPtr vw, int dLen )
{
   RTEDGE::Message *msg;
   ::rtBUF          b;

   b._data = (char *)vw.ToPointer();
   b._dLen = dLen;
   _parse->Clear();
   if ( (msg=_sub->Parse( b._data, b._dLen )) )
      _parse->Set( *msg );
   return _parse;
}


////////////////////////////////////
// Tape Only
////////////////////////////////////
int rtEdgeSubscriber::PumpFullTape( u_int64_t off0, int nMsg )
{
   return _sub->PumpFullTape( off0, nMsg );
}

int rtEdgeSubscriber::StopPumpFullTape( int pumpID )
{
   return _sub->StopPumpFullTape( pumpID );
}

} // namespace librtEdge
