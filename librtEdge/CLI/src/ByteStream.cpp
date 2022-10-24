/******************************************************************************
*
*  ByteStream.cpp
*
*  REVISION HISTORY:
*     13 DEC 2014 jcs  Created.
*     23 OCT 2022 jcs  Build 58: cli::array<>
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <ByteStream.h>


namespace librtEdgePRIVATE
{

////////////////////////////////////////////////
//
//    c l a s s    B y t e S t r e a m C
//
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
ByteStreamC::ByteStreamC( IByteStream ^cli,
                          const char  *svc,
                          const char  *tkr ) :
   _cli( cli ),
   RTEDGE::ByteStream( svc, tkr )
{
   ::memset( &_pubDataC, 0, sizeof( _pubDataC ) );
}

ByteStreamC::ByteStreamC( IByteStream ^cli, 
                          const char  *svc,
                          const char  *tkr,
                          int          fidOff,
                          int          fidLen,
                          int          fidNumFld,
                          int          fidPayload ) :
   _cli( cli ),
   RTEDGE::ByteStream( svc, tkr, fidOff, fidLen, fidNumFld, fidPayload )
{
   ::memset( &_pubDataC, 0, sizeof( _pubDataC ) );
}

ByteStreamC::~ByteStreamC()
{
   ClearPubDataC();
}


//////////////////////////
// Access / Mutator
//////////////////////////
::rtBUF ByteStreamC::pubDataC()
{
   return _pubDataC;
}

::rtBUF ByteStreamC::SetPubDataC( char *data, int dLen )
{
   ClearPubDataC();
   _pubDataC._data = new char[dLen+1];
   _pubDataC._dLen = dLen;
   ::memcpy( _pubDataC._data, data, dLen );
   return pubDataC();
}

void ByteStreamC::ClearPubDataC()
{
   if ( _pubDataC._data )
      delete[] _pubDataC._data;
   ::memset( &_pubDataC, 0, sizeof( _pubDataC ) );
}


//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void ByteStreamC::OnData( ::rtBUF buf )
{
   _cli->OnData( librtEdge::rtEdge::_memcpy( buf ) );
}

void ByteStreamC::OnError( const char *msg )
{
   _cli->OnError( gcnew String( msg ) );
}

void ByteStreamC::OnSubscribeComplete()
{
   _cli->OnSubscribeComplete();
}

void ByteStreamC::OnPublishComplete( int nByte )
{
   _cli->OnPublishComplete( nByte );
}

} // namespace librtEdgePRIVATE


namespace librtEdge
{

////////////////////////////////////////////////
//
//     c l a s s   B y t e S t r e a m
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
ByteStream::ByteStream( String ^svc, String ^tkr ) :
   _bStr( new librtEdgePRIVATE::ByteStreamC( this, 
                                             rtEdge::_pStr( svc ),
                                             rtEdge::_pStr( tkr ) ) )
{
}

ByteStream::ByteStream( String ^svc, 
                        String ^tkr,
                        int     fidOff,
                        int     fidLen,
                        int     fidNumFld,
                        int     fidPayload ) :
   _bStr( new librtEdgePRIVATE::ByteStreamC( this,
                                             rtEdge::_pStr( svc ),
                                             rtEdge::_pStr( tkr ),
                                             fidOff,
                                             fidLen,
                                             fidNumFld,
                                             fidPayload ) )
{
}

ByteStream::~ByteStream()
{
   delete _bStr;
   _bStr = (librtEdgePRIVATE::ByteStreamC *)0;
}


/////////////////////////////////
// Access
/////////////////////////////////
RTEDGE::ByteStream &ByteStream::bStr()
{
   return *_bStr;
}

String ^ByteStream::Service()
{
   return gcnew String( bStr().Service() );
}

String ^ByteStream::Ticker()
{
   return gcnew String( bStr().Ticker() );
}

int ByteStream::fidOff()
{
   return bStr().fidOff();
}

int ByteStream::fidLen()
{
   return bStr().fidLen();
}

int ByteStream::fidNumFld()
{
   return bStr().fidNumFld();
}

int ByteStream::fidPayload()
{
   return bStr().fidPayload();
}

int ByteStream::StreamID()
{
   return bStr().StreamID();
}

cli::array<Byte> ^ByteStream::subBuf()
{
   return rtEdge::_memcpy( bStr().subBuf() );
}

int ByteStream::subBufLen()
{
   return bStr().subBufLen();
}

cli::array<Byte> ^ByteStream::pubBuf()
{
   return rtEdge::_memcpy( bStr().pubBuf() );
}


/////////////////////////////////
// Mutator
/////////////////////////////////
void ByteStream::SetPublishData( cli::array<Byte> ^buf )
{
   pin_ptr<Byte> p   = &buf[0];
   char         *data = (char *)p;
   int           dLen = buf->Length;

   /*
    * SetPubDataC() does deep copy since pin_ptr most likely pins buf[0] 
    * only for the duration of this call.
    */
   bStr().SetPublishData( _bStr->SetPubDataC( data, dLen ) );
}

} // namespace librtEdge
