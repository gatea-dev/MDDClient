/******************************************************************************
*
*  LVC.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*     25 SEP 2017 jcs  Build 35: LVCAdmin
*     11 JAN 2018 jcs  Build 39: Leak : FreeAll() in Destroy() 
*     16 MAR 2022 jcs  Build 51: LVCAdmin.AddTickers(); OnAcminXX()
*     26 APR 2022 jcs  Build 53: LVCAdmin.AddBDS()
*     17 MAY 2022 jcs  Build 54: LVCAdmin.RefreshTickers()
*     23 OCT 2022 jcs  Build 58: cli::array<>
*      8 MAR 2023 jcs  Build 62: XxxxAll_safe()
*     18 MAY 2023 jcs  Build 63: cpp().GetSchema( false )
*     22 AUG 2023 jcs  Build 64: Named Schema
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <LVC.h>


namespace librtEdgePRIVATE
{  
   
////////////////////////////////////////////////
// 
//      c l a s s   L V C A d m i n C P P
// 
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
LVCAdminCPP::LVCAdminCPP( ILVCAdmin ^cli, const char *admin ) :
   RTEDGE::LVCAdmin( admin ),
   _cli( cli )
{   
}   
    
LVCAdminCPP::~LVCAdminCPP()
{   
}   
 
  
//////////////////////////
// Asynchronous Callbacks
//////////////////////////
bool LVCAdminCPP::OnAdminACK( bool bAdd, const char *svc, const char *tkr )
{
   _cli->OnAdminACK( bAdd, gcnew String( svc ), gcnew String( tkr ) );
   return true;
}

bool LVCAdminCPP::OnAdminNAK( bool bAdd, const char *svc, const char *tkr )
{
   _cli->OnAdminNAK( bAdd, gcnew String( svc ), gcnew String( tkr ) );
   return true;
}

} // namespace librtEdgePRIVATE


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
   return GetSchema( false );
}

rtEdgeSchema ^LVC::GetSchema( bool bQry )
{
   _schema->Set( cpp().GetSchema( bQry ) );
   return schema();
}

LVCData ^LVC::Snap( String ^svc, String ^tkr )
{
   const char      *pSvc, *pTkr;
   RTEDGE::Message *msg;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   Free();
   if ( (msg=cpp().Snap( pSvc, pTkr )) )
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
   if ( (msg=cpp().View( pSvc, pTkr )) )
      _qry->Set( *msg );
   return _qry;
}

void LVC::Free()
{
   cpp().Free();
   _qry->Clear();
}

LVCDataAll ^LVC::SnapAll()
{
   FreeAll();
   _qryAll->Set( cpp().SnapAll(), true );
   return _qryAll;
}

LVCDataAll ^LVC::ViewAll()
{
   FreeAll();
   _qryAll->Set( cpp().ViewAll(), true );
   return _qryAll;
}

LVCDataAll ^LVC::SnapAll_safe( LVCDataAll ^dst )
{
   RTEDGE::LVCAll *all;

   all = new RTEDGE::LVCAll( cpp(), cpp().GetSchema( false ) );
   dst->Set( cpp().SnapAll_safe( *all ), false );
   return dst;
}

LVCDataAll ^LVC::ViewAll_safe( LVCDataAll ^dst )
{
   RTEDGE::LVCAll *all;

   all = new RTEDGE::LVCAll( cpp(), cpp().GetSchema( false ) );
   dst->Set( cpp().ViewAll_safe( *all ), false );
   return dst;
}

void LVC::FreeAll()
{
   cpp().FreeAll();
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
LVCAdmin::LVCAdmin() :
   _lvc( (librtEdgePRIVATE::LVCAdminCPP *)0 )
{
}

LVCAdmin::LVCAdmin( String ^admin ) :
   _lvc( (librtEdgePRIVATE::LVCAdminCPP *)0 )
{
   Start( admin );
}

LVCAdmin::~LVCAdmin()
{
   if ( _lvc )
      delete _lvc;
   _lvc = (librtEdgePRIVATE::LVCAdminCPP *)0;
}


/////////////////////////////////
// Operations
/////////////////////////////////
void LVCAdmin::Start( String ^admin )
{
   // Once

   if ( !_lvc )
      _lvc = new librtEdgePRIVATE::LVCAdminCPP( this, _pStr( admin ) );
}


////////////////////////////////////
// Admin
////////////////////////////////////
String ^LVCAdmin::Admin()
{
   return gcnew String( _lvc->pAdmin() );
}

void LVCAdmin::AddBDS( String ^svc, String ^bds )
{
   const char *pSvc, *pBDS;

   pSvc = (const char *)_pStr( svc );
   pBDS = (const char *)_pStr( bds );
   _lvc->AddBDS( pSvc, pBDS );
}

void LVCAdmin::AddTicker( String ^svc, String ^tkr )
{
   AddTickerToSchema( svc, tkr, "" );
}

void LVCAdmin::AddTickerToSchema( String ^svc, String ^tkr, String ^schema )
{
   const char *pSvc, *pTkr, *pSch;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   pSch = (const char *)_pStr( schema );
   _lvc->AddTicker( pSvc, pTkr, pSch );
}

void LVCAdmin::AddTickers( String ^svc, cli::array<String ^> ^tkrs )
{
   AddTickersToSchema( svc, tkrs, "" );
}

void LVCAdmin::AddTickersToSchema( String               ^svc, 
                                   cli::array<String ^> ^tkrs,
                                   String               ^schema )
{
   const char  *pSvc, *pSch;
   const char **pTkrs;
   char        *bp;
   size_t       sz;
   int          i, nl;

   // Pre-condition

   if ( !(nl=tkrs->Length) )
      return;

   // Safe to continue

   pSvc  = (const char *)_pStr( svc );
   pSch  = (const char *)_pStr( schema );
   sz    = ( nl+4 ) * sizeof( const char * );
   bp    = new char[sz];
   pTkrs = (const char **)bp;
   for ( i=0; i<nl; pTkrs[i] = (const char *)_pStr( tkrs[i] ), i++ );
   pTkrs[i] = (const char *)0; 
   _lvc->AddTickers( pSvc, pTkrs, pSch );
   delete[] bp;
}

void LVCAdmin::DelTicker( String ^svc, String ^tkr )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _lvc->DelTicker( pSvc, pTkr );
}

void LVCAdmin::RefreshTickers( String ^svc, cli::array<String ^> ^tkrs )
{
   const char  *pSvc;
   const char **pTkrs;
   char        *bp;
   size_t       sz;
   int          i, nl;

   // Pre-condition

   if ( !(nl=tkrs->Length) )
      return;

   // Safe to continue

   pSvc  = (const char *)_pStr( svc );
   sz    = ( nl+4 ) * sizeof( const char * );
   bp    = new char[sz];
   pTkrs = (const char **)bp;
   for ( i=0; i<nl; pTkrs[i] = (const char *)_pStr( tkrs[i] ), i++ );
   pTkrs[i] = (const char *)0; 
   _lvc->RefreshTickers( pSvc, pTkrs );
   delete[] bp;
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
