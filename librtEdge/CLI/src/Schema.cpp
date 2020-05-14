/******************************************************************************
*
*  Schema.cpp
*
*  REVISION HISTORY:
*     14 NOV 2014 jcs  Created.
*     14 JAN 2018 jcs  Build 39: gcnew / delete
*      9 FEB 2020 jcs  Build 42: GetDef()
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Schema.h>

namespace librtEdge 
{
////////////////////////////////////////////////
//
//     c l a s s   r t E d g e S c h e m a
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
rtEdgeSchema::rtEdgeSchema() :
   _schema( (RTEDGE::Schema *)0 ),
   _fldC( new RTEDGE::Field ),
   _fld( gcnew rtEdgeField() )
{
}

rtEdgeSchema::~rtEdgeSchema()
{
   delete _fld;
   delete _fldC;
}


////////////////////////////////////
// Access - Iterate All Fields
////////////////////////////////////
void rtEdgeSchema::reset() 
{
   _schema->reset();
}

bool rtEdgeSchema::forth()
{
   return (*_schema)();
}

rtEdgeField ^rtEdgeSchema::field()
{
   RTEDGE::Field *fld;

   _fld->Clear();
   if ( (fld=_schema->field()) )
      _fld->Set( fld );
   return _fld;
}


/////////////////////////////////
// Access
/////////////////////////////////
int rtEdgeSchema::Size()
{
   return _schema->Size();
}

rtEdgeField ^rtEdgeSchema::GetDef( String ^name )
{
   RTEDGE::FieldDef *def;
   const char       *pFld = rtEdge::_pStr( name );

   if ( (def=_schema->GetDef( pFld )) ) {
      _fldC->Set( *_schema, def->field() );
      _fld->Set( _fldC ); 
      return _fld;
   }
   return nullptr;
}

rtEdgeField ^rtEdgeSchema::GetDef( int fid )
{
   RTEDGE::FieldDef *def;

   if ( (def=_schema->GetDef( fid )) ) {
      _fldC->Set( *_schema, def->field() );
      _fld->Set( _fldC ); 
      return _fld;
   }
   return nullptr;
}

int rtEdgeSchema::Fid( String ^fld )
{
   RTEDGE::FieldDef *def;

   def = _schema->GetDef( rtEdge::_pStr( fld ) );
   return def ? def->Fid() : 0;
}

String ^rtEdgeSchema::Name( int fid )
{
   RTEDGE::FieldDef *def;
   const char       *pn;
      
   def = _schema->GetDef( fid );
   pn  = def ? def->pName() : "";
   return gcnew String( pn );
}

rtFldType rtEdgeSchema::Type( int fid )
{
   RTEDGE::FieldDef *def;

   def = _schema->GetDef( fid );
   return def ? (rtFldType)def->fType() : rtFldType::rtFld_undef;
}

rtFldType rtEdgeSchema::Type( String ^fld )
{
   RTEDGE::FieldDef *def;

   def = _schema->GetDef( rtEdge::_pStr( fld ) );
   return def ? (rtFldType)def->fType() : rtFldType::rtFld_undef;
}


/////////////////////////////////
// Mutator
/////////////////////////////////
void rtEdgeSchema::Set( RTEDGE::Schema &sch )
{      
   _schema = &sch;
}

} // namespace librtEdge
