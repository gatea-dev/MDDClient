/******************************************************************************
*
*  Data.cpp
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*      5 FEB 2016 jcs  Build 32: _InitHeap()
*     11 JAN 2018 jcs  Build 39: Leak : _FreeHeap()
*      9 FEB 2020 jcs  Build 42: GetColumnAsXxx()
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Data.h>

namespace librtEdge 
{

////////////////////////////////////////////////
//
//       c l a s s   r t E d g e D a t a
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
rtEdgeData::rtEdgeData() :
   _msg( (RTEDGE::Message *)0 ),
   _mt( (rtEdgeType)edg_dead ),
   _NumFld( 0 ),
   _data( (::rtEdgeData *)0 ),
   _svc( nullptr ),
   _tkr( nullptr ),
   _err( nullptr ),
   _fdb( nullptr ),
   _raw( nullptr ),
   _fld( gcnew rtEdgeField() ),
   _heap( nullptr ),
   _cachedFields( nullptr )
{
   int i, nh;

   // Pre-allocate a 'heap' of 1024 reusable rtEdgeFields

   nh    = 1024;
   _heap = gcnew array<rtEdgeField ^>( nh );
   for ( i=0; i<nh; _heap[i++] = gcnew rtEdgeField() );
   rtEdge::_IncObj();
}

rtEdgeData::rtEdgeData( rtEdgeData ^src ) :
   _msg( (RTEDGE::Message *)0 ),
   _mt( src->mt() ),
   _NumFld( src->_nFld ),
   _data( (::rtEdgeData *)0 ),
   _svc( nullptr ),
   _tkr( nullptr ),
   _err( nullptr ),
   _fdb( nullptr ),
   _raw( nullptr ),
   _fld( gcnew rtEdgeField() ),
   _heap( nullptr ),
   _cachedFields( nullptr )
{
   String               ^deepCopy;
   array<rtEdgeField ^> ^sdb;
   rtEdgeField          ^fld;
   Hashtable            ^cdb;
   int                   i, fid, nf;

   // Pre-allocate a 'heap' of 1024 reusable rtEdgeFields

   rtEdge::_IncObj();
   Set( *src->msg() );
   deepCopy = _pSvc;
   deepCopy = _pTkr;
   deepCopy = _pErr;
   /*
    * Deep copy fields
    */
   sdb  = src->_flds;
   nf   = (int)_NumFld;
   _fdb = gcnew array<rtEdgeField ^>( nf );
   for ( i=0; i<nf; i++ ) {
      fld     = sdb[i];
      _fdb[i] = gcnew rtEdgeField( fld );
   }
   /*
    * GetField()
    */
   cdb = gcnew Hashtable();
   for ( i=0; i<_nFld; i++ ) {
      fld      = _fdb[i];
      fid      = fld->Fid();
      cdb[fid] = fld;
   }
   _cachedFields = cdb;
}

rtEdgeData::~rtEdgeData()
{
   if ( _cachedFields )
      delete _cachedFields;
   delete _fld;
   _FreeHeap();
   Clear();
   rtEdge::_DecObj();
}


////////////////////////////////////
// Access - Iterate All Fields
////////////////////////////////////
bool rtEdgeData::forth()
{
   RTEDGE::Field *fld;

   fld = (*_msg)();
   return( fld != 0 );
}

rtEdgeField ^rtEdgeData::field()
{
   RTEDGE::Field *fld;

   _fld->Clear();
   if ( (fld=_msg->field()) )
      _fld->Set( fld );
   return _fld;
}


////////////////////////////////////
// Access - Misc
////////////////////////////////////
RTEDGE::Message *rtEdgeData::msg()
{
   return _msg;
}

rtEdgeField ^rtEdgeData::GetField( int fid )
{
   RTEDGE::Field *fld;
   rtEdgeField   ^rtn;

   // Cached different from volatile (normal)

   rtn = nullptr;
   if ( _cachedFields != nullptr ) {
      if ( _cachedFields->ContainsKey( fid ) )
         rtn = (rtEdgeField ^)_cachedFields[fid];
   }
   else {
      if ( (fld=_msg->GetField( fid )) )
         rtn = _fld->Set( fld );
   }
   return rtn;
}


/////////////////////////////////
//  Operations
/////////////////////////////////
void rtEdgeData::Set( RTEDGE::Message &msg )
{
   Clear();
   _msg  = &msg;
   _data = &msg.data();
   msg.reset();
   _mt     = (rtEdgeType)_msg->mt();
   _NumFld = (u_int)_msg->NumFields();
}

void rtEdgeData::Clear()
{
   _msg  = (RTEDGE::Message *)0;
   _data = (::rtEdgeData *)0;
   _svc  = nullptr;
   _tkr  = nullptr;
   _err  = nullptr;
   _raw  = nullptr;
   _fdb  = nullptr;
}


/////////////////////////////////
//  Helpers
/////////////////////////////////
void rtEdgeData::_CheckHeap( int nf )
{
   int i, nh;

   if ( nf <= (nh=_heap->GetLength( 0 )) )
      return;
   _heap->Resize( _heap, nf );
   for ( i=nh; i<nf; _heap[i++] = gcnew rtEdgeField() );
}

void rtEdgeData::_FreeHeap()
{
   int i, nh;

   if ( _heap != nullptr ) {
      nh = _heap->GetLength( 0 );
      for ( i=0; i<nh; delete _heap[i++] );
      delete _heap; 
   } 
   _heap = nullptr;
}


////////////////////////////////////////////////
//
//       c l a s s   L V C D a t a
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
LVCData::LVCData() :
   _msg( (RTEDGE::Message *)0 ),
   _data( (::LVCData *)0 ),
   _svc( nullptr ),
   _tkr( nullptr ),
   _err( nullptr ),
   _fdb( nullptr ),
   _fld( gcnew rtEdgeField() ),
   _heap( nullptr )
{
//   _InitHeap( 1024 );
   rtEdge::_IncObj();
}

LVCData::~LVCData()
{
   delete _fld;
   _data = nullptr;
   _FreeHeap();
   Clear();
   rtEdge::_DecObj();
}


////////////////////////////////////
// Access - Iterate All Fields
////////////////////////////////////
bool LVCData::forth()
{
   RTEDGE::Field *fld;

   fld = (*_msg)();
   return( fld != 0 );
}

rtEdgeField ^LVCData::field()
{
   RTEDGE::Field *fld;

   _fld->Clear();
   if ( (fld=_msg->field()) )
      _fld->Set( fld );
   return _fld;
}


////////////////////////////////////
// Access - Specific Field in Msg
////////////////////////////////////
RTEDGE::Message *LVCData::msg()
{
   return _msg;
}

rtEdgeField ^LVCData::GetField( int fid )
{
   RTEDGE::Field *fld;

   if ( (fld=_msg->GetField( fid )) )
      return _fld->Set( fld );
   return nullptr;
}


/////////////////////////////////
//  Operations
/////////////////////////////////
void LVCData::Set( RTEDGE::Message &msg )
{
   Clear();
   _msg  = &msg;
   _data = &msg.dataLVC();
   msg.reset();
}

void LVCData::Clear()
{
   _msg  = (RTEDGE::Message *)0;
   _data = (::LVCData *)0;
   _svc  = nullptr;
   _tkr  = nullptr;
   _err  = nullptr;
   _fdb  = nullptr;
}


/////////////////////////////////
//  Helpers
/////////////////////////////////
void LVCData::_InitHeap( int nf )
{
   int i;

   // Once

   if ( _heap == nullptr ) {
      _heap = gcnew array<rtEdgeField ^>( nf );
      for ( i=0; i<nf; _heap[i++] = gcnew rtEdgeField() );
   }
}

void LVCData::_CheckHeap( int nf )
{
   int i, nh;

   _InitHeap( nf );
   if ( nf <= (nh=_heap->GetLength( 0 )) )
      return;
   _heap->Resize( _heap, nf );
   for ( i=nh; i<nf; _heap[i++] = gcnew rtEdgeField() );
}

void LVCData::_FreeHeap()
{
   int i, nh;

   if ( _heap != nullptr ) {
      nh = _heap->GetLength( 0 );
      for ( i=0; i<nh; delete _heap[i++] );
      delete _heap;
   }
   _heap = nullptr;
}



////////////////////////////////////////////////
//
//       c l a s s   L V C D a t a A l l
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
LVCDataAll::LVCDataAll() :
   _all( (RTEDGE::LVCAll *)0 ),
   _data( gcnew LVCData() ),
   _itr( -1 ),
   _fdb( nullptr ),
   _heap( nullptr )
{
//   _InitHeap( 1024 );
   rtEdge::_IncObj();
}

LVCDataAll::~LVCDataAll()
{
   delete _data;
   _FreeHeap();
   Clear();
   rtEdge::_DecObj();
}


////////////////////////////////////
// Access - Specific Record
////////////////////////////////////
LVCData ^LVCDataAll::GetRecord( int idx )
{
   if ( ( 0 <= idx ) && ( idx < _nTkr ) )
      return _tkrs[idx];
   return nullptr;
}


////////////////////////////////////
// Access - Iterate All Rows
////////////////////////////////////
void LVCDataAll::reset()
{
   _itr = -1;
}

bool LVCDataAll::forth()
{
   _itr++;
   return( _itr < _nTkr );
}

LVCData ^LVCDataAll::data()
{
   RTEDGE::Messages &mdb = _all->msgs();
   RTEDGE::Message  *msg;

   _data->Clear();
   if ( _itr >= _nTkr )
      return _data;
   msg = mdb[_itr];
   _data->Set( *msg );
   return _data;
}


////////////////////////////////////
// Access - Column-centric
////////////////////////////////////
array<String ^> ^LVCDataAll::GetColumnAsString( int fid )
{
   RTEDGE::Messages &mdb = _all->msgs();
   RTEDGE::Field    *fld;
   array<String ^>  ^col;
   LVCData           f;
   const char       *val;
   u_int             i, nt;

   // Pre-condition

   if ( !(nt=_nTkr) )
      return nullptr;

   // Rock on ...

   col = gcnew array<String ^>( nt );
   for ( i=0; i<nt; i++ ) {
      fld    = mdb[i]->GetField( fid );
      val    = fld ? fld->GetAsString() : "";
      col[i] = gcnew String( val );
   }
   return col;
}

array<int> ^LVCDataAll::GetColumnAsInt32( int fid )
{
   RTEDGE::Messages &mdb = _all->msgs();
   RTEDGE::Field    *fld;
   array<int>       ^col;
   LVCData           f;
   u_int             i, nt;

   // Pre-condition

   if ( !(nt=_nTkr) )
      return nullptr;

   // Rock on ...

   col = gcnew array<int>( nt );
   for ( i=0; i<nt; i++ ) {
      fld    = mdb[i]->GetField( fid );
      col[i] = fld ? fld->GetAsInt32() : 0;
   }
   return col;
}

array<double> ^LVCDataAll::GetColumnAsDouble( int fid )
{
   RTEDGE::Messages &mdb = _all->msgs();
   RTEDGE::Field    *fld;
   array<double>    ^col;
   LVCData           f;
   u_int             i, nt;

   // Pre-condition

   if ( !(nt=_nTkr) )
      return nullptr;

   // Rock on ...

   col = gcnew array<double>( nt );
   for ( i=0; i<nt; i++ ) {
      fld    = mdb[i]->GetField( fid );
      col[i] = fld ? fld->GetAsDouble() : 0.0;
   }
   return col;
}


/////////////////////////////////
//  Operations
/////////////////////////////////
void LVCDataAll::Set( RTEDGE::LVCAll &all )
{
   Clear();
   _all = &all;
   _data->Clear();
}

void LVCDataAll::Clear()
{
   _all = (RTEDGE::LVCAll *)0;
   _fdb = nullptr;
}


/////////////////////////////////
//  Helpers
/////////////////////////////////
void LVCDataAll::_InitHeap( int nh )
{
   int i;

   // Once

   if ( _heap == nullptr ) {
      _heap = gcnew array<LVCData ^>( nh );
      for ( i=0; i<nh; _heap[i++] = gcnew LVCData() );
   }
}

void LVCDataAll::_CheckHeap( int nf )
{
   int i, nh;

   _InitHeap( nf );
   if ( nf <= (nh=_heap->GetLength( 0 )) )
      return;
   _heap->Resize( _heap, nf );
   for ( i=nh; i<nf; _heap[i++] = gcnew LVCData() );
}

void LVCDataAll::_FreeHeap()
{
   int i, nh;

   if ( _heap != nullptr ) {
      nh = _heap->GetLength( 0 );
      for ( i=0; i<nh; delete _heap[i++] );
      delete _heap; 
   } 
   _heap = nullptr;
}

} // namespace librtEdge
