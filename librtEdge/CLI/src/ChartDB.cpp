/******************************************************************************
*
*  ChartDB.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*      2 OCT 2015 jcs  Build 32: CDBTable / ViewTable()
*     10 SEP 2020 jcs  Build 44: MDDResult
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <ChartDB.h>


namespace librtEdge
{

////////////////////////////////////////////////
//
//       c l a s s   C D B D a t a
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
CDBData::CDBData( ChartDB ^cdb ) :
   _cdb( cdb ),
   _data( (RTEDGE::CDBData *)0 ),
   _svc( nullptr ),
   _tkr( nullptr ),
   _err( nullptr ),
   _fdb( nullptr )
{
}

CDBData::~CDBData()
{
   Clear();
}


////////////////////////////////////
// Access
////////////////////////////////////
RTEDGE::CDBData *CDBData::data()
{
   return _data;
}

String ^CDBData::SeriesTime( int n )
{
   std::string s;

   return gcnew String( _data->pSeriesTime( n, s ) );
}

String ^CDBData::DumpNonZero()
{
   return gcnew String( _data->Dump( true ) );
}

String ^CDBData::Dump()
{
   return gcnew String( _data->Dump( false ) );
}


/////////////////////////////////
// Operations
/////////////////////////////////
void CDBData::Set( RTEDGE::CDBData &data )
{
   Clear();
   _data = &data;
}

void CDBData::Clear()
{
   _data = (RTEDGE::CDBData *)0;
   _svc  = nullptr;
   _tkr  = nullptr;
   _fdb  = nullptr;
}



////////////////////////////////////////////////
//
//       c l a s s   C D B T a b l e
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
CDBTable::CDBTable( ChartDB ^cdb ) :
   _cdb( cdb ),
   _tbl( (RTEDGE::CDBTable *)0 ),
   _svc( nullptr ),
   _err( nullptr ),
   _fdb( nullptr )
{
}

CDBTable::~CDBTable()
{
   Clear();
}


/////////////////////////////////
// Access
/////////////////////////////////
String ^CDBTable::pTkr( int nt )
{
   return gcnew String( _tbl->pTkr( nt ) );
}

String ^CDBTable::DumpByTime()
{
   return gcnew String( _tbl->Dump( true ) );
}

String ^CDBTable::DumpByTicker()
{
   return gcnew String( _tbl->Dump( false ) );
}



/////////////////////////////////
// Operations
/////////////////////////////////
void CDBTable::Set( RTEDGE::CDBTable &tbl )
{
   Clear();
   _tbl = &tbl;
}

void CDBTable::Clear()
{
   _tbl = (RTEDGE::CDBTable *)0;
   _svc = nullptr;
   _fdb = nullptr;
}



////////////////////////////////////////////////
//
//       c l a s s   M D D R e c R e f
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
MDDRecDef::MDDRecDef( ::MDDRecDef rec ) :
   _svc( gcnew String( rec._pSvc ) ),
   _tkr( gcnew String( rec._pTkr ) ),
   _Fid( rec._fid ),
   _Interval( rec._interval )
{
}

MDDRecDef::~MDDRecDef()
{
   _svc = nullptr;
   _tkr = nullptr;
}


////////////////////////////////////////////////
//
//       c l a s s   M D D R e s u l t
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
MDDResult::MDDResult( ::MDDResult &qry ) :
   _qry( qry ),
   _rdb( nullptr )
{
}

MDDResult::~MDDResult()
{
   _rdb = nullptr;
}



////////////////////////////////////////////////
//
//        c l a s s   C h a r t D B
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
ChartDB::ChartDB( String ^file, String ^admin ) :
   _cdb( new RTEDGE::ChartDB( _pStr( file ), _pStr( admin ) ) ),
   _qryAll( new ::MDDResult() ),
   _qry( gcnew CDBData( this ) ),
   _tbl( gcnew CDBTable( this ) )
{
}

ChartDB::~ChartDB()
{
   delete _qryAll;
   _qry = nullptr;
   _tbl = nullptr;
   Destroy();
}


////////////////////////////////////
// Cache Query
////////////////////////////////////
CDBData ^ChartDB::View( String ^svc, String ^tkr, int fid )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   Free();
   _qry->Set( _cdb->View( pSvc, pTkr, fid ) );
   return _qry;
}

CDBTable ^ChartDB::ViewTable( String ^svc, array<String ^> ^tkrs, int fid )
{
   const char  *pSvc, *pTkr;
   const char **tdb;
   char        *bp;
   int          i, nt, sz;

   Free();
   pSvc = (const char *)_pStr( svc );
   nt   = tkrs->Length;
   sz   = nt * sizeof( char * );
   bp   = new char[sz];
   tdb  = (const char **)bp;
   for ( i=0; i<nt; i++ )
      tdb[i] = (const char *)_pStr( tkrs[i] );
   _tbl->Set( _cdb->ViewTable( pSvc, tdb, nt, fid ) );
   delete[] bp;
   return _tbl;
}

void ChartDB::Free()
{
   _cdb->Free();
   _qry->Clear();
   _tbl->Clear();
}

MDDResult ^ChartDB::Query()
{
   *_qryAll = _cdb->Query();
   return gcnew MDDResult( *_qryAll );
}

void ChartDB::FreeResult()
{
   _cdb->FreeResult();
}



///////////////////////////////////
// DB Mutator
////////////////////////////////////
void ChartDB::AddTicker( String ^svc, String ^tkr )
{
   AddTicker( svc, tkr, 0 );
}

void ChartDB::AddTicker( String ^svc, String ^tkr, int fid )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _cdb->AddTicker( pSvc, pTkr, fid );
}

void ChartDB::DelTicker( String ^svc, String ^tkr )
{
   DelTicker( svc, tkr, 0 );
}

void ChartDB::DelTicker( String ^svc, String ^tkr, int fid )
{
   const char *pSvc, *pTkr;

   pSvc = (const char *)_pStr( svc );
   pTkr = (const char *)_pStr( tkr );
   _cdb->DelTicker( pSvc, pTkr, fid );
} 



////////////////////////////////////
// Backwards Compatibility
////////////////////////////////////
void ChartDB::Destroy()
{
   if ( _cdb )
      delete _cdb;
   _cdb = (RTEDGE::ChartDB *)0;
   _qry = nullptr; 
}

} // namespace librtEdge
