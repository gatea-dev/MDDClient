/******************************************************************************
*
*  LVC.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*     25 SEP 2017 jcs  Build 35: LVCAdmin
*     11 JAN 2018 jcs  Build 39: Leak : FreeAll() in Destroy() 
*
*  (c) 1994-2018 Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <LVC.h>


namespace librtEdge
{

////////////////////////////////////////////////
//
//        c l a s s   L V C
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
LVC::LVC( String ^file ) :
   _lvc( new RTEDGE::LVC( _pStr( file ) ) ),
   _qry( gcnew LVCData() ),
   _qryAll( gcnew LVCDataAll() ),
   _schema( gcnew rtEdgeSchema() )
{
   GetSchema();
   rtEdge::_IncObj();
}

LVC::~LVC()
{
   Destroy();
}


////////////////////////////////////
// Access
////////////////////////////////////
rtEdgeSchema ^LVC::schema()
{
   return _schema;
}


////////////////////////////////////
// Cache Query
////////////////////////////////////
rtEdgeSchema ^LVC::GetSchema()
{
   _schema->Set( _lvc->GetSchema() );
   return schema();
}

LVCData ^LVC::Snap( String ^svc, String ^tkr )
{
   const char      *pSvc, *pTkr;
   RTEDGE::Message *msg;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   Free();
   if ( (msg=_lvc->Snap( pSvc, pTkr )) )
      _qry->Set( *msg );
   return _qry;
}

LVCData ^LVC::View( String ^svc, String ^tkr )
{
   const char      *pSvc, *pTkr;
   RTEDGE::Message *msg;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   Free();
   if ( (msg=_lvc->View( pSvc, pTkr )) )
      _qry->Set( *msg );
   return _qry;
}

void LVC::Free()
{
   _lvc->Free();
   _qry->Clear();
}

LVCDataAll ^LVC::SnapAll()
{
   FreeAll();
   _qryAll->Set( _lvc->SnapAll() );
   return _qryAll;
}

LVCDataAll ^LVC::ViewAll()
{
   FreeAll();
   _qryAll->Set( _lvc->ViewAll() );
   return _qryAll;
}

void LVC::FreeAll()
{
   _lvc->FreeAll();
   _qryAll->Clear();
}


////////////////////////////////////
// Backwards Compatibility
////////////////////////////////////
void LVC::Destroy()
{
   // Once

   if ( !_lvc )
      return;

   // OK to clean up

   FreeAll();
   delete _lvc;
   delete _qry;
   delete _qryAll;
   delete _schema;
   _lvc    = (RTEDGE::LVC *)0;
   _qry    = nullptr;
   _qryAll = nullptr;
   _schema = nullptr;
   rtEdge::_DecObj();
_heapmin();
}



////////////////////////////////////////////////
//
//       c l a s s   L V C A d m i n
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
LVCAdmin::LVCAdmin( String ^admin ) :
   _lvc( new RTEDGE::LVCAdmin( _pStr( admin ) ) )
{
}

LVCAdmin::~LVCAdmin()
{
   if ( _lvc )
      delete _lvc;
   _lvc = (RTEDGE::LVCAdmin *)0;
}


////////////////////////////////////
// Admin
////////////////////////////////////
String ^LVCAdmin::Admin()
{
   return gcnew String( _lvc->pAdmin() );
}

void LVCAdmin::AddTicker( String ^svc, String ^tkr )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _lvc->AddTicker( pSvc, pTkr );
}

void LVCAdmin::DelTicker( String ^svc, String ^tkr )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _lvc->DelTicker( pSvc, pTkr );
}



////////////////////////////////////////////////
//
//        c l a s s   L V C S n a p
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
LVCSnap::LVCSnap( LVC ^lvc, LVCData ^data ) :
   _lvc( lvc ),
   _data( data ),
   _nullFld( gcnew rtEdgeField() ),
   _fidIdx( gcnew Hashtable() )
{
   _Parse();
}

LVCSnap::~LVCSnap()
{
   delete _fidIdx;
   delete _nullFld;
}


////////////////////////////////////
// Operations
////////////////////////////////////
bool LVCSnap::HasField( String ^fld )
{
   return HasField( _Fid( fld ) );
}

bool LVCSnap::HasField( int fid )
{
   return _fidIdx->ContainsKey( fid );
}

String ^LVCSnap::GetField( String ^fld )
{
   return GetField( _Fid( fld ) );
}

String ^LVCSnap::GetField( int fid )
{
   rtEdgeField ^f;

   f = _Field( fid );
   return f->GetAsString( false );
}


////////////////////////////////////
// Helpers
////////////////////////////////////
rtEdgeField ^LVCSnap::_Field( int fid )
{
   rtEdgeField ^fld;
   int          idx;

   if ( HasField( fid ) ) {
      idx = (int)_fidIdx[fid];
      fld = _data->_flds[idx];
   }
   else
      fld = _nullFld;
   return fld;
}

int LVCSnap::_Fid( String ^fld )
{
   return _lvc->schema()->Fid( fld );
}

void LVCSnap::_Parse()
{
   int idx, fid;

   // Create FID-to-index dictionary once per LVCSnap

   for ( idx=0; idx<_data->_nFld; idx++ ) {
      fid          = (int)_data->_flds[idx]->Fid();
      _fidIdx[fid] = idx;
   }
}



////////////////////////////////////////////////
//
//      c l a s s   L V C V o l a t i l e
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
LVCVolatile::LVCVolatile( String ^file ) :
   _idxs( nullptr ),
   LVC( file )
{
   ViewAll();
}

LVCVolatile::~LVCVolatile()
{
   if ( _idxs != nullptr )
      delete _idxs;
}


////////////////////////////////////
// Access
////////////////////////////////////
int LVCVolatile::GetIdx( String ^svc, String ^tkr )
{
   String ^key;
   int     idx;

   // Build once

   if ( _idxs == nullptr )
      BuildIdxDb();
   key = gcnew String( svc + "|" + tkr );
   idx = _idxs->ContainsKey( key ) ? _idxs[key] : -1;
   return idx;
}

LVCData ^LVCVolatile::ViewByIdx( int idx )
{
   return _qryAll->GetRecord( idx );
}


////////////////////////////////////
// Helpers
////////////////////////////////////
void LVCVolatile::BuildIdxDb()
{
   LVCDataAll ^la;
   LVCData    ^ld;
   String     ^key;
   int         idx;

   _idxs = gcnew Dictionary<String ^, int>();
   la    = _qryAll;
   for ( idx=0,la->reset(); la->forth(); idx++ ) {
      ld         = la->data();
      key        = ld->_pSvc + "|" + ld->_pTkr;
      _idxs[key] = idx;
   }
}

} // namespace librtEdge
