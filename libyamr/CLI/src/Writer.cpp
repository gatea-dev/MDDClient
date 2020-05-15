/******************************************************************************
*
*  Writer.cpp
*
*  REVISION HISTORY:
*     19 JUN 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Writer.h>


namespace libyamrPRIVATE
{

////////////////////////////////////////////////
//
//     c l a s s   _ W r i t e r
//
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
_Writer::_Writer( IWriter ^cli ) :
   _cli( cli ),
   _msg( gcnew libyamr::yamrMsg() )
{
}

_Writer::~_Writer()
{
   _msg = nullptr;
}


//////////////////////////
// Access
//////////////////////////
libyamr::yamrMsg ^_Writer::msg()
{
   return _msg;
}


//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void _Writer::OnConnect( const char *msg, bool bUP )
{
   _cli->OnConnect( gcnew String( msg ), bUP );
}

void _Writer::OnStatus( ::yamrStatus sts )
{
   libyamr::yamrStatus ySts;

   if ( sts == ::yamr_QloMark )
      ySts = libyamr::yamrStatus::yamr_QloMark;
   else
      ySts = libyamr::yamrStatus::yamr_QhiMark;
   _cli->OnStatus( ySts );
}

void _Writer::OnIdle()
{
   _cli->OnIdle();
}

} // namespace libyamrPRIVATE


namespace libyamr
{

////////////////////////////////////////////////
//
//          c l a s s   W r i t e r
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
Writer::Writer() :
   _sub( new libyamrPRIVATE::_Writer( this ) )
{
}

Writer::~Writer()
{
   Stop();
   _sub = (libyamrPRIVATE::_Writer *)0;
}


/////////////////////////////////
// Operations
/////////////////////////////////
String ^Writer::Start( String ^hosts, int SessionID )
{
   const char *rc;

   _sub->SetIdleCallback( true );
   rc = _sub->Start( _pStr( hosts ), SessionID, true );
   return gcnew String( rc );
}

void Writer::Stop()
{
   _sub->Stop();
}

bool Writer::Send( array<Byte> ^data, short WireProto, short MsgProto )
{
   ::yamrBuf b;

   b = _memcpy( data );
   return _sub->Send( b, WireProto, MsgProto );
}

bool Writer::Send( array<Byte> ^data, short MsgProto )
{
   return Send( data, MsgProto, MsgProto );
}


/////////////////////////////////
// Access
/////////////////////////////////
String ^Writer::pSvrHosts()
{
   return gcnew String( _sub->pSvrHosts() );
}

int Writer::SessionID()
{
   return _sub->SessionID();
}

void Writer::Randomize( bool bRandom )
{
   _sub->Randomize( bRandom );
}

int Writer::SetRxBufSize( int bufSiz )
{
   return _sub->SetRxBufSize( bufSiz );
}

int Writer::GetRxBufSize()
{
   return _sub->GetRxBufSize();
}

int Writer::SetTxBufSize( int bufSiz )
{
   return _sub->SetTxBufSize( bufSiz );
}

int Writer::GetTxBufSize()
{
   return _sub->GetTxBufSize();
}

int Writer::GetTxQueueSize()
{
   return _sub->GetTxQueueSize();
}

int Writer::SetThreadProcessor( int cpu )
{
   return _sub->SetThreadProcessor( cpu );
}

int Writer::GetThreadProcessor()
{
   return _sub->GetThreadProcessor();
}

long Writer::GetThreadID()
{
   return _sub->GetThreadID();
}

void Writer::SetHiLoBand( int hiLoBand )
{
   _sub->SetHiLoBand( hiLoBand );
}

bool Writer::IsUDP()
{
   return _sub->IsUDP();
}

} // namespace libyamr
