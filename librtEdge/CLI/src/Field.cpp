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
*
*  (c) 1994-2020 Gatea Ltd.
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
RTEDGE::Field *rtEdgeField::cpp()
{   
   return _fld;
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

long rtEdgeField::GetAsInt64()
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

   pf = _fld->Dump();
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

DateTime ^rtEdgeField::GetAsDateTime()
{
   DateTime ^dt = gcnew DateTime();

   dt = dt->Now;
/*
            string DMY;
            string[] ff, hms, ss;
            int nf, dmy;
            int Y, M, D, h, m, s, ms;
            rtFldType tmp;
            DateTime rtn, now;
            string[] mons = { "JAN", "FEB", "MAR", "APR",
                              "MAY", "JUN", "JUL", "AUG",
                              "SEP", "OCT", "NOV", "DEC" };

            // Native -> DateTime

            switch (_type)
            {
                case rtFldType.rtFld_date:
                {
                    //
                     / Hack for librtEdge Build 26a and earlier;
                    // librtEdgeBuild 27 and later are all native
                    ///
                    if ( _iBld == 26 ) {
                       tmp   = _type;
                       _type = rtFldType.rtFld_string;
                       rtn   = GetAsDateTime();
                       _type = tmp;
                    }
                    else
                       rtn = _GetAsDateTime( _r64 );
                    return rtn;
                }
                case rtFldType.rtFld_time:
                case rtFldType.rtFld_timeSec:
                    return _GetAsDateTime( _r64 );
            }

            //
            // 1) 12:23:56.mmm
            // 2) 2011-01-15 12:23:56.mmm
            // 3) 20110115 12:23:56.mmm
            // 4) 15 JAN 2011 12:23:56.mmm
            // 
            now = DateTime.Now;
            ff  = Data().Split(' ');
            nf  = ff.Length;
            Y   = now.Year;
            M   = now.Month;
            D   = now.Day;
            h   = 0;
            m   = 0;
            s   = 0;
            ms  = 0;
            hms = null;
            switch (nf)
            {
                case 1:   // 1) above
                    hms = ff[0].Split(':');
                    break;
                case 2:   // 2) or 3) above
                    DMY = ff[0].Replace("-", "");   // "20110115"
                    dmy = Convert.ToInt32(DMY);     // 20110115
                    Y   = Convert.ToInt32(dmy / 10000);
                    M   = Convert.ToInt32(dmy / 100) % 100;
                    D   = dmy % 100;
                    hms = ff[1].Split(':');
                    break;
                case 4:   // 4) above
                    D = Convert.ToInt32(ff[0]);
                    M = Array.IndexOf(mons, ff[1]) + 1;
                    Y = Convert.ToInt32(ff[2]);
                    hms = ff[3].Split(':');
                    break;
            }
            if ((hms != null) && (hms.Length == 3))
            {
                h  = Convert.ToInt32( hms[0] );
                m  = Convert.ToInt32( hms[1] );
                ss = hms[2].Split('.');
                s  = Convert.ToInt32(ss[0]);
                ms = (ss.Length > 1) ? Convert.ToInt32(ss[1]) : 0;
            }
            return new DateTime(Y, M, D, h, m, s, ms);
 */
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

} // namespace librtEdge
