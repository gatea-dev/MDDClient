/******************************************************************************
*
*  Update.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created.
*     23 JAN 2015 jcs  Build 29: ByteStreamFld; PubChainLink()
*      7 JUL 2015 jcs  Build 31: Publish( cli::array<Byte> ^ )
*     28 MAR 2022 jcs  Build 52: AddFieldAsDateTime() filled in
*     23 MAY 2022 jcs  Build 55: AddFieldAsUnixTime()
*     30 OCT 2022 jcs  Build 60: rtFld_vector
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Update.h>

using namespace librtEdgePRIVATE;

namespace librtEdge
{

////////////////////////////////////////////////
//
//  c l a s s   r t E d g e P u b U p d a t e
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
rtEdgePubUpdate::rtEdgePubUpdate( IrtEdgePublisher ^pub,
                                  String           ^tkr,
                                  IntPtr            arg,
                                  bool             bImg ) :
   _pub( pub ),
   _upd( pub->upd() ),
   _err( nullptr )
{
   Init( tkr, arg, bImg );
}

rtEdgePubUpdate::rtEdgePubUpdate( IrtEdgePublisher ^pub,
                                  String           ^tkr,
                                  IntPtr            arg,
                                  String           ^err ) :
   _pub( pub ), 
   _upd( pub->upd() ),
   _err( err )
{
   Init( tkr, arg, false );
}

rtEdgePubUpdate::rtEdgePubUpdate( IrtEdgePublisher ^pub,
                                  rtEdgeData       ^d ) :
   _pub( pub ), 
   _upd( pub->upd() ),
   _err( nullptr )
{
   int i;

   Init( d->_pTkr, IntPtr( (long)d->_arg ), false );
   for ( i=0; i<d->_nFld; AddField( d->_flds[i++] ) );
}

rtEdgePubUpdate::~rtEdgePubUpdate()
{
}


/////////////////////////////////
// Operations - Reusability
/////////////////////////////////
void rtEdgePubUpdate::Init( String ^tkr, IntPtr arg, bool bImg )
{
   _upd.Init( rtEdge::_pStr( tkr ), (void *)arg, bImg );
}


/////////////////////////////////
// Publish
/////////////////////////////////
int rtEdgePubUpdate::Publish()
{
   if ( _err != nullptr )
      return PubError( _err );
   return _upd.Publish();
}

int rtEdgePubUpdate::Publish( cli::array<Byte> ^buf, bool bFieldList )
{
   ::rtBUF       r;
   ::mddDataType dt;

   r  = rtEdge::_memcpy( buf );
   dt = bFieldList ? mddDt_FieldList : mddDt_FixedMsg;
   return _upd.Publish( r, dt );
}

int rtEdgePubUpdate::PubError( String ^err )
{
   int rtn;

   rtn  = _upd.PubError( rtEdge::_pStr( err ) );
   _err = nullptr;
   return rtn;
}


/////////////////////////////////
// ByteStream
/////////////////////////////////
int rtEdgePubUpdate::Publish( ByteStream ^bStr, 
                              int         fidData )
{
   return _upd.Publish( bStr->bStr(), fidData );
}

int rtEdgePubUpdate::Publish( ByteStream ^bStr, 
                              int         fidData,
                              int         maxFldSiz,
                              int         maxFld,
                              int         bytesPerSec )
{
   return _upd.Publish( bStr->bStr(), fidData, maxFldSiz, maxFld, bytesPerSec );
}

void rtEdgePubUpdate::Stop( ByteStream ^bStr )
{
   _upd.Stop( bStr->bStr() );
}


/////////////////////////////////
// Fields
/////////////////////////////////
void rtEdgePubUpdate::AddField( rtEdgeField ^fld )
{
   switch( fld->Type() ) {
      case rtFldType::rtFld_undef:
         break;
      case rtFldType::rtFld_string:
         AddFieldAsString( fld->Fid(), fld->GetAsString( false ) );
         break;
      case rtFldType::rtFld_int:
         AddFieldAsInt32( fld->Fid(), fld->GetAsInt32() );
         break;
      case rtFldType::rtFld_double:
         AddFieldAsDouble( fld->Fid(), fld->GetAsDouble() );
         break;
      case rtFldType::rtFld_date:
      case rtFldType::rtFld_time:
      case rtFldType::rtFld_timeSec:
         // TODO
         break;
      case rtFldType::rtFld_float:
         AddFieldAsFloat( fld->Fid(), fld->GetAsFloat() );
         break;
      case rtFldType::rtFld_int8:
         AddFieldAsInt8( fld->Fid(), fld->GetAsInt8() );
         break;
      case rtFldType::rtFld_int16:
         AddFieldAsInt16( fld->Fid(), fld->GetAsInt16() );
         break;
      case rtFldType::rtFld_int64:
         AddFieldAsInt64( fld->Fid(), fld->GetAsInt64() );
         break;
      case rtFldType::rtFld_real:
         // TODO
         break;
      case rtFldType::rtFld_bytestream:
         AddFieldAsByteStream( fld->Fid(), fld->GetAsByteStream() );
         break;
      case rtFldType::rtFld_unixTime:
         AddFieldAsUnixTime( fld->Fid(), fld->GetAsDateTime() );
         break;
      case rtFldType::rtFld_vector:
         AddFieldAsVector( fld->Fid(), fld->GetAsVector() );
         break;
   }
}

void rtEdgePubUpdate::AddFieldAsString( int fid, String ^str )
{
   _upd.AddField( fid, (char *)rtEdge::_pStr( str ) );
}

void rtEdgePubUpdate::AddFieldAsInt8( int fid, u_char i8 )
{
   _upd.AddField( fid, i8 );
}

void rtEdgePubUpdate::AddFieldAsInt16( int fid, u_short i16 )
{
   _upd.AddField( fid, i16 );
}

void rtEdgePubUpdate::AddFieldAsInt32( int fid, int i32 )
{
   _upd.AddField( fid, i32 );
}

void rtEdgePubUpdate::AddFieldAsInt64( int fid, long long i64 )
{
   _upd.AddField( fid, i64 );
}

void rtEdgePubUpdate::AddFieldAsFloat( int fid, float r32 )
{
   _upd.AddField( fid, r32 );
}

void rtEdgePubUpdate::AddFieldAsDouble( int fid, double r64 )
{
   _upd.AddField( fid, r64 );
}

void rtEdgePubUpdate::AddFieldAsByteStream( int fid, ByteStreamFld ^bStr )
{
   RTEDGE::ByteStreamFld *bs;

   bs = bStr->bStr();
   _upd.AddField( fid, *bs );
}

void rtEdgePubUpdate::AddFieldAsVector( int fid, cli::array<double> ^vec )
{
   RTEDGE::Doubles vdb;

   for ( int i=0; i<vec->Length; vdb.push_back( vec[i] ), i++ );
   _upd.AddVector( fid, vdb );
}

void rtEdgePubUpdate::AddFieldAsVector( int                 fid, 
                                        cli::array<double> ^vec,
                                        int                 precision )
{
   RTEDGE::Doubles vdb;

   for ( int i=0; i<vec->Length; vdb.push_back( vec[i] ), i++ );
   _upd.AddVector( fid, vdb, precision );
}

void rtEdgePubUpdate::AddFieldAsDateTime( int fid, DateTime ^dt )
{
   _upd.AddField( fid, _ConvertDateTime( dt ) );
}

void rtEdgePubUpdate::AddFieldAsUnixTime( int fid, DateTime ^dt )
{
   _upd.AddFieldAsUnixTime( fid, _ConvertDateTime( dt ) );
}

void rtEdgePubUpdate::AddFieldAsDate( int fid, DateTime ^dt )
{
   _upd.AddField( fid, _ConvertDateTime( dt )._date );
}

void rtEdgePubUpdate::AddFieldAsTime( int fid, DateTime ^dt )
{
   _upd.AddField( fid, _ConvertDateTime( dt )._time );
}

void rtEdgePubUpdate::AddFieldAsString( String ^pFld, String ^str )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsString( fid, str );
}

void rtEdgePubUpdate::AddFieldAsInt8( String ^pFld, u_char i8 )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsInt8( fid, i8 );
}

void rtEdgePubUpdate::AddFieldAsInt16( String ^pFld, u_short i16 )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsInt16( fid, i16 );
}

void rtEdgePubUpdate::AddFieldAsInt32( String ^pFld, int i32 )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsInt32( fid, i32 );
}

void rtEdgePubUpdate::AddFieldAsInt64( String ^pFld, long long i64 )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsInt64( fid, i64 );
}

void rtEdgePubUpdate::AddFieldAsFloat( String ^pFld, float r32 )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsFloat( fid, r32 );
}

void rtEdgePubUpdate::AddFieldAsDouble( String ^pFld, double r64 )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsDouble( fid, r64 );
}

void rtEdgePubUpdate::AddFieldAsByteStream( String ^pFld, ByteStreamFld ^bStr )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsByteStream( fid, bStr );
}

void rtEdgePubUpdate::AddFieldAsVector( String ^pFld, cli::array<double> ^vec )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsVector( fid, vec );
}

void rtEdgePubUpdate::AddFieldAsVector( String             ^pFld,
                                        cli::array<double> ^vec,
                                        int                 precision )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsVector( fid, vec, precision );
}

void rtEdgePubUpdate::AddFieldAsDateTime( String ^pFld, DateTime ^dt )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsDateTime( fid, dt );
}

void rtEdgePubUpdate::AddFieldAsDate( String ^pFld, DateTime ^dt )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsDate( fid, dt );
}

void rtEdgePubUpdate::AddFieldAsTime( String ^pFld, DateTime ^dt )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsTime( fid, dt );
}


/////////////////////////////////
// Chain
/////////////////////////////////
int rtEdgePubUpdate::PubChainLink( String               ^chainName,
                                   IntPtr                arg,
                                   int                   linkNum,
                                   bool                  bFinal,
                                   cli::array<String ^> ^links,
                                   int                   dpyTpl )
{
   const char *ldb[K], *pChn;
   void       *vArg;
   int         i, nl;

   pChn = rtEdge::_pStr( chainName );
   nl   = gmin( links->Length, K-1 );
   vArg = (void *)arg;
   for ( i=0; i<nl; ldb[i]=rtEdge::_pStr( links[i++] ) );
   return _upd.PubChainLink( pChn, vArg, linkNum, bFinal, ldb, nl, dpyTpl );
}

int rtEdgePubUpdate::PubChainLink( String               ^chainName,
                                   IntPtr                arg,
                                   int                   linkNum,
                                   bool                  bFinal,
                                   cli::array<String ^> ^links ) 
{
   return PubChainLink( chainName, arg, linkNum, bFinal, links, 999 );
}

} // namespace librtEdge
