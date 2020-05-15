/******************************************************************************
*
*  Reader.cpp
*
*  REVISION HISTORY:
*     19 JUN 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Reader.h>


namespace libyamr
{

////////////////////////////////////////////////
//
//          c l a s s   R e a d e r
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
Reader::Reader() :
   _cpp( new YAMR::Reader() ),
   _ym( new ::yamrMsg ),
   _msg( gcnew yamrMsg() )
{
	_msg->Set( *_ym );
}

Reader::~Reader()
{
   Close();
   if ( _cpp )
      delete _cpp;
   _cpp = (YAMR::Reader *)0;
   _ym  = (::yamrMsg *)0;
   _msg = nullptr;
}


/////////////////////////////////
// Debug : Message Dump
/////////////////////////////////
String ^Reader::DumpHeader( yamrMsg ^y )
{
   std::string rc;

   rc = _cpp->DumpHeader( y->cpp() );
   return gcnew String( rc.data() );
}

String ^Reader::DumpBody( yamrMsg ^y )
{
   std::string rc;

   rc = _cpp->DumpBody( y->cpp() );
   return gcnew String( rc.data() );
}

String ^Reader::ProtoName( yamrMsg ^y )
{
   std::string rc;

   rc = _cpp->ProtoName( y->cpp() );
   return gcnew String( rc.data() );
}

String ^Reader::ProtoName( short proto )
{
   std::string rc;

   rc = _cpp->ProtoName( proto );
   return gcnew String( rc.data() );
}

short Reader::ProtoNumber( String ^name )
{
   return _cpp->ProtoNumber( _pStr( name ) );
}


/////////////////////////////////
// Reader Operations
/////////////////////////////////
void Reader::Open( String ^filename )
{
   _cpp->Open( _pStr( filename ) );
}

void Reader::Close()
{
   _cpp->Close();
}

long Reader::Rewind()
{
   return _cpp->Rewind();
}

long Reader::RewindTo( long pos )
{
   return _cpp->RewindTo( pos );
}

long Reader::RewindTo( String ^pTime )
{
   return _cpp->RewindTo( _pStr( pTime ) );
}

yamrMsg ^Reader::Read()
{
   if ( _cpp->Read( *_ym ) )
      return _msg;
   return nullptr;
}

void Reader::Decode( yamrMsg ^y )
{
   _cpp->Decode( y->cpp() );
}

} // namespace libyamr
