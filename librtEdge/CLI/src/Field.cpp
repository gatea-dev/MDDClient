/******************************************************************************
*
*  Field.cpp
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*     13 DEC 2014 jcs  Build 29: ByteStreamFld
*      5 FEB 2016 jcs  Build 32: Dump()
*     11 JAN 2018 jcs  Build 39: _IncObj() / _DecObj()
*      9 MAR 2020 jcs  Build 42: Copy constructor; _bStrCpy
*     11 AUG 2020 jcs  Build 44: GetAsDateTime() filled in
*     30 MAR 2022 jcs  Build 52: long long GetAsInt64(); Native GetAsDateTime()
*      2 JUN 2022 jcs  Build 55: GetAsString() wraps GetAsString(), not Dump()
*     24 OCT 2022 jcs  Build 58: Opaque cpp()
*     30 OCT 2022 jcs  Build 60: rtFld_vector
*     10 NOV 2022 jcs  Build 61: DateTime in vector
*     14 AUG 2023 jcs  Build 64: IsEmpty()
*     20 DEC 2023 jcs  Build 67: TypeFromXxx()
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Field.h>

namespace librtEdge
{

////////////////////////////////////////////////
//
//    c l a s s   B y t e S t r e a m F l d
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
ByteStreamFld::ByteStreamFld() :
   _bStr( (RTEDGE::ByteStreamFld *)0 ),
   _bRaw( nullptr )
{
   rtEdge::_IncObj();
}

ByteStreamFld::~ByteStreamFld()
{
   _bRaw = nullptr;
   rtEdge::_DecObj();
}


////////////////////////////////////
// Access - Misc
////////////////////////////////////
RTEDGE::ByteStreamFld *ByteStreamFld::bStr()
{
   return _bStr;
}


/////////////////////////////////
//  Operations
/////////////////////////////////
ByteStreamFld ^ByteStreamFld::Set( RTEDGE::ByteStreamFld &bStr )
{
   _bStr = &bStr;
   _bRaw = nullptr;
   return this;
}

void ByteStreamFld::Clear()
{
   _bStr = (RTEDGE::ByteStreamFld *)0;
   _bRaw = nullptr;
}


////////////////////////////////////////////////
//
//       c l a s s   r t E d g e F i e l d
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
rtEdgeField::rtEdgeField() :
   _fld( (RTEDGE::Field *)0 ),
   _bCopy( false ),
   _bStrCpy( false ),
   _bStr( gcnew ByteStreamFld() ),
   _val( nullptr ),
   _name( nullptr )
{
   rtEdge::_IncObj();
}

rtEdgeField::rtEdgeField( rtEdgeField ^src ) :
   _fld( (RTEDGE::Field *)0 ),
   _bCopy( false ),
   _bStrCpy( false ),
   _bStr( gcnew ByteStreamFld() ),
   _val( nullptr ),
   _name( nullptr )
{
   ::rtFldType ty;

   rtEdge::_IncObj();
   Copy( src->cpp() );
   ty = _fld->Type();
   if ( ty == ::rtFld_string ) {
      if ( _val != nullptr )
         delete _val;
      _val     = gcnew String( src->GetAsString( false ) );
      _bStrCpy = true;
   }
   Name();
}

rtEdgeField::~rtEdgeField()
{
   delete _bStr;
   if ( _val != nullptr )
      delete _val;
   if ( _name != nullptr )
      delete _name;
   if ( _bCopy )
       delete _fld;
   Clear();
   rtEdge::_DecObj();
}


/////////////////////////////////
// Access
/////////////////////////////////
bool rtEdgeField::IsEmpty()
{
   return _fld->IsEmpty();
}

int rtEdgeField::Fid()
{
   return _fld->Fid();
}

String ^rtEdgeField::Name()
{
   if ( _name == nullptr )
      _name = gcnew String( _fld->Name() );
   return _name;
}

rtFldType rtEdgeField::Type()
{
   return (rtFldType)_fld->Type();
}

rtFldType rtEdgeField::TypeFromMsg()
{
   return (rtFldType)_fld->TypeFromMsg();
}

rtFldType rtEdgeField::TypeFromSchema()
{
   return (rtFldType)_fld->TypeFromSchema();
}

u_char rtEdgeField::GetAsInt8()
{
   return _fld->GetAsInt8();
}

u_short rtEdgeField::GetAsInt16()
{
   return _fld->GetAsInt16();
}

int rtEdgeField::GetAsInt32()
{
   return _fld->GetAsInt32();
}

long long rtEdgeField::GetAsInt64()
{
   return _fld->GetAsInt64();
}

float rtEdgeField::GetAsFloat()
{
   return _fld->GetAsFloat();
}

double rtEdgeField::GetAsDouble()
{
   return _fld->GetAsDouble();
}

String ^rtEdgeField::GetAsString( bool bShowType )
{
   const char *pf, *pt;
   char        buf[K];
   ::rtFldType ty;

   // Deep Copy String??

   if ( _bStrCpy )
      return _val;

   // OK to continue

   pf = _fld->GetAsString();
   ty = _fld->Type();
   if ( bShowType ) {
      switch( ty ) {
         case ::rtFld_undef:      pt = "(und)"; break;
         case ::rtFld_string:     pt = "(str)"; break;
         case ::rtFld_int:        pt = "(i32)"; break;
         case ::rtFld_double:     pt = "(r64)"; break;
         case ::rtFld_date:       pt = "(dat)"; break;
         case ::rtFld_time:       pt = "(tim)"; break;
         case ::rtFld_timeSec:    pt = "(sec)"; break;
         case ::rtFld_float:      pt = "(r32)"; break;
         case ::rtFld_int8:       pt = "(i08)"; break;
         case ::rtFld_int16:      pt = "(i16)"; break;
         case ::rtFld_int64:      pt = "(i64)"; break;
         case ::rtFld_real:       pt = "(rel)"; break;
         case ::rtFld_bytestream: pt = "(byt)"; break;
      }
      sprintf( buf, "%s %s", pt, pf );
      pf = buf;
   }

   if ( _val != nullptr )
      delete _val;
   _val = gcnew String( pf );
   return _val;
}

String ^rtEdgeField::Dump()
{
   return _bStrCpy ? _val : gcnew String( _fld->Dump() );
}

ByteStreamFld ^rtEdgeField::GetAsByteStream()
{
   return _bStr->Set( _fld->GetAsByteStream() );
}

cli::array<double> ^rtEdgeField::GetAsVector()
{
   RTEDGE::DoubleList &vdb = cpp()->GetAsVector();
   size_t              n   = vdb.size();
   cli::array<double> ^vec;

   vec = n ? gcnew cli::array<double>( n ) : nullptr;
   for ( size_t i=0; i<n; vec[i]=vdb[i], i++ );
   return vec;
}

cli::array<DateTime ^> ^rtEdgeField::GetAsDateTimeVector()
{
   RTEDGE::DateTimeList   &vdb = cpp()->GetAsDateTimeVector();
   size_t                  n   = vdb.size();
   cli::array<DateTime ^> ^vec;
   RTEDGE::rtDateTime      dtTm;
   RTEDGE::rtDate         &dt = dtTm._date;
   RTEDGE::rtTime         &tm = dtTm._time;

   vec = n ? gcnew cli::array<DateTime ^>( n ) : nullptr;
   for ( size_t i=0; i<n; i++ ) {
      dtTm = vdb[i];
      vec[i] = gcnew DateTime( _WithinRange( 0, dt._year, 9999 ),
                               _WithinRange( 1, dt._month + 1, 12 ),
                               _WithinRange( 1, dt._mday, 31 ),
                               _WithinRange( 0, tm._hour, 23 ),
                               _WithinRange( 0, tm._minute, 59 ),
                               _WithinRange( 0, tm._second, 59 ),
                               _WithinRange( 0, tm._micros / 1000, 999 ) );
   }
   return vec;
}

DateTime ^rtEdgeField::GetAsDateTime()
{
   DateTime          ^dt;
   RTEDGE::rtDateTime rDtTm = _fld->GetAsDateTime();
   RTEDGE::rtDate    &rDt   = rDtTm._date;
   RTEDGE::rtTime    &rTm   = rDtTm._time;

   dt  = gcnew DateTime( _WithinRange( 0, rDt._year, 9999 ),
                         _WithinRange( 1, rDt._month + 1, 12 ),
                         _WithinRange( 1, rDt._mday, 31 ),
                         _WithinRange( 0, rTm._hour, 23 ),
                         _WithinRange( 0, rTm._minute, 59 ),
                         _WithinRange( 0, rTm._second, 59 ),
                         _WithinRange( 0, rTm._micros / 1000, 999 ) );
   return dt;
}


/////////////////////////////////
// Operations
/////////////////////////////////
void rtEdgeField::DumpToConsole()
{
   Console::WriteLine( "   [{0}] {1}", Fid(), GetAsString( true ) );
}


/////////////////////////////////
//  Mutator
/////////////////////////////////
rtEdgeField ^rtEdgeField::Set( RTEDGE::Field *fld )
{
   Clear();
   _fld = fld;
   return this;
}

void rtEdgeField::Copy( RTEDGE::Field *src )
{
   if ( !_fld ) {
      _bCopy = true;
      _fld   = new RTEDGE::Field();
   }
   _fld->Copy( *src );
}

void rtEdgeField::Clear()
{
   if ( !_bCopy )
      _fld = (RTEDGE::Field *)0;
   if ( _name != nullptr )
      delete _name;
   _name = nullptr;
}


/////////////////////////////////
// Helpers
/////////////////////////////////
int rtEdgeField::_WithinRange( int x0, int x, int x1 )
{
   if ( x0 > x )
      return x0;
   if ( x > x1 )
      return x1;
   return x;
}

} // namespace librtEdge
