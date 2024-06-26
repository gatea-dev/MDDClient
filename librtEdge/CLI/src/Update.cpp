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
*     10 NOV 2022 jcs  Build 61: AddFieldAsVector( DateTime )
*     30 JUN 2023 jcs  Build 63: StringDoor
*     24 OCT 2023 jcs  Build 66: AddEmptyField()
*     18 MAR 2024 jcs  Build 70: AddFieldAsDouble( ..., int precision )
*     28 JUN 2024 jcs  Build 72: Nullable GetAsXxx()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Update.h>

// 10 sig Fig for unpacked double 

#define _DFLT_DBL_PRECISION 10

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
   _upd.Init( _pStr( tkr ), (void *)arg, bImg );
}


/////////////////////////////////
// Publish
/////////////////////////////////
int rtEdgePubUpdate::Publish()
{
   int rc;

   if ( _err != nullptr )
      rc = PubError( _err );
   else
      rc = _upd.Publish();
   _Free_strGC();
   return rc;
}

int rtEdgePubUpdate::Publish( cli::array<Byte> ^buf, bool bFieldList )
{
   ::rtBUF       r;
   ::mddDataType dt;
   int           rc;

   r  = rtEdge::_memcpy( buf );
   dt = bFieldList ? mddDt_FieldList : mddDt_FixedMsg;
   rc = _upd.Publish( r, dt );
   _Free_strGC();
   return rc;
}

int rtEdgePubUpdate::PubError( String ^err )
{
   int rtn;

   rtn  = _upd.PubError( _pStr( err ) );
   _err = nullptr;
   return rtn;
}


/////////////////////////////////
// ByteStream
/////////////////////////////////
int rtEdgePubUpdate::Publish( ByteStream ^bStr, 
                              int         fidData )
{
   int rc;

   rc = _upd.Publish( bStr->bStr(), fidData );
   _Free_strGC();
   return rc;
}

int rtEdgePubUpdate::Publish( ByteStream ^bStr, 
                              int         fidData,
                              int         maxFldSiz,
                              int         maxFld,
                              int         bytesPerSec )
{
   int rc;

   rc = _upd.Publish( bStr->bStr(), fidData, maxFldSiz, maxFld, bytesPerSec );
   _Free_strGC();
   return rc;
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
         AddEmptyField( fld->Fid() );
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

void rtEdgePubUpdate::AddEmptyField( int fid )
{
   _upd.AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsString( int fid, String ^str )
{
   if ( str != nullptr )
      _upd.AddField( fid, (char *)_pStr( str ) );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsInt8( int fid, Nullable<u_char> i8 )
{
   if ( i8.HasValue )
      _upd.AddField( fid, i8.Value );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsInt16( int fid, Nullable<u_short> i16 )
{
   if ( i16.HasValue )
      _upd.AddField( fid, i16.Value );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsInt32( int fid, Nullable<int> i32 )
{
   if ( i32.HasValue )
      _upd.AddField( fid, i32.Value );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsInt64( int fid, Nullable<long long> i64 )
{
   if ( i64.HasValue )
      _upd.AddField( fid, i64.Value );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsFloat( int fid, Nullable<float> r32 )
{
   if ( r32.HasValue )
      _upd.AddField( fid, r32.Value );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsDouble( int fid, Nullable<double> r64 )
{
   AddFieldAsDouble( fid, r64, _DFLT_DBL_PRECISION );
}

void rtEdgePubUpdate::AddFieldAsDouble( int              fid, 
                                        Nullable<double> r64, 
                                        int              precision )
{
   if ( r64.HasValue )
      _upd.AddField( fid, r64.Value, precision );
   else
      AddEmptyField( fid );
}

void rtEdgePubUpdate::AddFieldAsByteStream( int fid, ByteStreamFld ^bStr )
{
   RTEDGE::ByteStreamFld *bs;

   bs = bStr->bStr();
   _upd.AddField( fid, *bs );
}

void rtEdgePubUpdate::AddFieldAsVector( int fid, cli::array<double> ^vec )
{
   RTEDGE::DoubleList vdb;

   for ( int i=0; i<vec->Length; vdb.push_back( vec[i] ), i++ );
   _upd.AddVector( fid, vdb );
}

void rtEdgePubUpdate::AddFieldAsVector( int                 fid, 
                                        cli::array<double> ^src,
                                        int                 precision )
{
   RTEDGE::DoubleList dst;

   for ( int i=0; i<src->Length; dst.push_back( src[i++] ) );
   _upd.AddVector( fid, dst, precision );
}

void rtEdgePubUpdate::AddFieldAsVector( int                     fid, 
                                        cli::array<DateTime ^> ^src )
{
   RTEDGE::DateTimeList dst;
   int                  i, n;

   n = src->Length;
   for ( i=0; i<n; dst.push_back( _ConvertDateTime( src[i++] ) ) );
   _upd.AddVector( fid, dst );
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
   AddFieldAsDouble( pFld, r64, _DFLT_DBL_PRECISION );
}

void rtEdgePubUpdate::AddFieldAsDouble( String ^pFld, double r64, int precision )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsDouble( fid, r64, precision );
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

void rtEdgePubUpdate::AddFieldAsVector( String                 ^pFld,
                                        cli::array<DateTime ^> ^vec )
{
   int fid;

   if ( (fid=_pub->GetFid( pFld )) )
      AddFieldAsVector( fid, vec );
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

   pChn = _pStr( chainName );
   nl   = gmin( links->Length, K-1 );
   vArg = (void *)arg;
   for ( i=0; i<nl; ldb[i]=_pStr( links[i++] ) );
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
