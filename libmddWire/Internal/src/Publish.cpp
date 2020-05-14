/******************************************************************************
*
*  Publish.cpp
*     MD-Direct publication channel data
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*      4 JUN 2014 jcs  Build  7: mddFld_real / mddFld_bytestream; _estFldSz
*     12 NOV 2014 jcs  Build  8: -Wall
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*     16 APR 2016 jcs  Build 11: mddMt_query is XML in Publish.BuildMsg()
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#include <MDW_Internal.h>
#include <GLedgDTD.h>

using namespace MDDWIRE_PRIVATE;

static char *_sigr32 = "%.4f";    //  4 sigFig max - float
static char *_sigr64 = "%.10f";   // 10 sigFig max - dbl
static char *_fldLen = "<%-12s "; // Field Names : 12 chars

/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      P u b l i s h
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Publish::Publish() :
   Data( true ),
   _upds(),
   _estFldSz( 0 ),
   _xTrans( (char *)0 )
{
}

Publish::~Publish()
{
   if ( _xTrans )
      delete[] _xTrans;
   if ( _log )
      _log->logT( 3, "~Publish()\n" );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
int Publish::nFld()
{
   return _upds.size();
}


////////////////////////////////////////////
// API
////////////////////////////////////////////
int Publish::AddFieldList( mddFieldList fl )
{
   mddField f;
   mddBuf  &b = f._val._buf;
   int      i, fSz;

   _estFldSz = 0;
   for ( i=0; i<fl._nFld; i++ ) {
      f = fl._flds[i];
if ( f._type == mddFld_string )
assert( b._dLen != (u_int)-1 );
      _upds.Insert( f );
      switch( f._type ) {
         case mddFld_string:
         case mddFld_bytestream:
            fSz = gmax( K, b._dLen+K );
            break;
         default:
            fSz = K;
            break;
      }
      _estFldSz += fSz;
   }
   return nFld();
}

mddBuf Publish::BuildMsg( mddMsgHdr   h, 
                          mddProtocol pro, 
                          mddBldBuf  &bld,
                          bool        bFldNm )
{
   mddBuf z;
   bool   bUndef;

   // Special Cases

   bUndef = ( pro == mddProto_Undef );
   switch( h._mt ) {
      case mddMt_open:
      case mddMt_close:
      case mddMt_insert:
      case mddMt_mount:
      case mddMt_query:
         pro    = bUndef ? mddProto_XML : pro;
         bUndef = false;
         break;
      case mddMt_ping:
         return Ping();
      case mddMt_image:
      case mddMt_update:     
      case mddMt_dead:       
      case mddMt_stale:      
      case mddMt_recovering: 
      case mddMt_undef:
      case mddMt_ctl:
      case mddMt_insAck:
      case mddMt_gblStatus:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }

   // Allow caller to specify protocol; Else use ours

   pro     = bUndef ? _proto : pro;
   z._data = 0;
   z._dLen = 0;
   switch( pro ) {
      case mddProto_Undef:  return z;
      case mddProto_XML:    return _XML_Build( h, z, bld, bFldNm );
      case mddProto_MF:     return _MF_Build( h, z, bld );
      case mddProto_Binary: return _Binary_Build( h, z, bld );
   }
   return z;
}

mddBuf Publish::BuildRawMsg( mddMsgHdr  h,
                             mddBuf     payload,
                             mddBldBuf &bld )
{  
   mddBuf z;

   // Allow caller to specify protocol; Else use ours

   z._data = 0;
   z._dLen = 0;
   switch( _proto ) {
      case mddProto_Undef:  return z;
      case mddProto_XML:    return _XML_Build( h, payload, bld, false );
      case mddProto_MF:     return _MF_Build( h, payload, bld );
      case mddProto_Binary: return _Binary_Build( h, payload, bld );
   }
   return z;
}  

mddBuf Publish::SetHdrTag( u_int iTag, mddBuf &bld )
{
   mddBuf z;

   // Only Binary supported

   z._data = 0;
   z._dLen = 0;
   if ( _proto == mddProto_Binary )
       return _Binary_SetHdrTag( iTag, bld );
   return z;
}

mddBuf Publish::ConvertFieldList( mddWireMsg  w, 
                                  mddProtocol pro, 
                                  mddBldBuf  &bld,
                                  bool        bFldNm )
{
   mddMsgHdr h;
   int       nf;

   // FieldList only

   if ( w._dt == mddDt_FieldList ) {
      ::memset( &h, 0, sizeof( h ) );
      nf = AddFieldList( w._flds );
      h._mt  = w._mt;
      h._dt  = w._dt;
      h._svc = w._svc;
      h._tkr = w._tkr;
      return BuildMsg( h, pro, bld, bFldNm );
   }
   return _bz;
}


////////////////////////////////////////////
// API - Data Interface
////////////////////////////////////////////
int Publish::Parse( mddMsgBuf b, mddWireMsg &r )
{
   int sz;

   // Incoming = XML Only

   sz = 0;
   if ( _proto == mddProto_XML )
      sz = _XML_Parse( b, r );
   r._proto = _proto;
   return sz;
}
 
int Publish::ParseHdr( mddMsgBuf b, mddMsgHdr &r )
{
   // Incoming = XML Only

   if ( _proto == mddProto_XML )
      return _XML_ParseHdr( b, r );
   return 0;
}



////////////////////////////////////////////
// XML
////////////////////////////////////////////
mddBuf Publish::_XML_Build( mddMsgHdr  m, 
                            mddBuf     payload, 
                            mddBldBuf &bld,
                            bool       bFldNm )
{
   mddFieldList &adb = m._attrs;
   mddField     *fdb, tmp[K];
   mddBuf        r;
   GLxmlMsgType  xMt;
   GLxml         x;
   mddField      f;
   mddBuf       &b = f._val._buf;
   GLxmlElem    *xe;
   char         *pMsg, *cp, *px, *ps;
   bool          bHdr;
   int           i, na, nf, hSz, estSz;
   string        hdr;

   // Message Type

   bHdr    = bld._bNoHdr ? false : true;
   r._data = (char *)0;
   r._dLen = 0;
   pMsg    = (char *)0;
   na      = adb._nFld;
   fdb     = adb._flds ? adb._flds : tmp;
   switch( m._mt ) {
      case mddMt_image:
      case mddMt_update:
         xMt       = ( m._mt == mddMt_image ) ? xm_image : xm_update;
         pMsg      = _mdd_pXmlMsg[xMt];
         f._type   = mddFld_string;
         f._name   = _mdd_pAttrSvc;
         b         = m._svc;
         fdb[na++] = f;
         f._name   = _mdd_pAttrName;
         b         = m._tkr;
         fdb[na++] = f;
         break;
      case mddMt_stale:
      case mddMt_recovering:
      case mddMt_dead:
         pMsg      = _mdd_pXmlMsg[xm_status];
         f._name   = _mdd_pType;
         b._data   = _mdd_pItem;
         fdb[na++] = f;
         f._type   = mddFld_string;
         f._name   = _mdd_pAttrSvc;
         b         = m._svc;
         fdb[na++] = f;
         f._name   = _mdd_pAttrName;
         b         = m._tkr;
         fdb[na++] = f;
         f._name   = _mdd_pCode;
         b._data   = m._tag;
         fdb[na++] = f;
         break;
      case mddMt_mount:
         pMsg = _mdd_pXmlMsg[xm_mount];
         break;
      case mddMt_ping:
assert( 0 );  // Handled in BuildMsg()
      case mddMt_ctl:
         pMsg = _mdd_pXmlMsg[xm_ioctl];
         break;
      case mddMt_open:
         pMsg = _mdd_pXmlMsg[xm_open];
         break;
      case mddMt_close:
         pMsg = _mdd_pXmlMsg[xm_close];
         break;
      case mddMt_gblStatus:
         pMsg = _mdd_pXmlMsg[xm_status];
         break;
      case mddMt_query:
         pMsg = _mdd_pXmlMsg[xm_query];
         break;
      case mddMt_undef:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }
   adb._nFld = na;
   if ( !pMsg )
      return r;

   // Build away w/ attributes
// TODO : !bBldHdr
   xe      = x.addElement( pMsg );
   _xTrans = !_xTrans ? new char[_MAX_FLD_LEN] : _xTrans;
   for ( i=0; i<na; i++ ) {
      f = fdb[i];
      if ( f._type == mddFld_string ) {
         string s( b._data, b._dLen );

         ps = (char  *)s.c_str();
         px = x.xTranslate( _xTrans, ps );
         xe->addAttr( (char *)f._name, px );
      }
   }
   hdr     = x.build();
   hSz     = hdr.size();
   nf      = nFld();
   estSz   = hSz + ( nf * K ); // ASSUME : Fields < K
   r._data = _InitBuf( bld, estSz );
   ::memcpy( r._data, hdr.c_str(), hSz );
   cp      = r._data;
   cp     += hSz;

   // Fields To Publish??

   if ( nf ) {
      cp -= 3; // strlen( "/>\n" );
      cp += sprintf( cp, ">\n" );
      for ( i=0; i<nf; cp+=_XML_BuildFld( cp, _upds[i++], bFldNm ) );
      cp += sprintf( cp, "</%s>\n", pMsg );
   }
   r._dLen      = cp - r._data;
   bld._payload = bld._data;
   bld._dLen    = r._dLen;
   _upds.clear();
   return r;
}

int Publish::_XML_BuildFld( char *bp, mddField f, bool bFldNm )
{
   mddField *def;
   char     *cp, *xt;
   int       sz;

   def = bFldNm ? GetDef( f._fid ) : (mddField *)0;
   cp  = bp;
   if ( def )
      cp += sprintf( cp, _fldLen, def->_name );
   else
      cp += sprintf( cp, "<_%d ", f._fid );
   cp += sprintf( cp, " %s=\"", _mdd_pVal );
   sz  = _ASCII_BuildFld( _xTrans, f );
   xt  = GLxml::xTranslate( cp, _xTrans );
   cp += strlen( cp );
   cp += sprintf( cp, "\"/>\n" );
   return( cp-bp );
}


////////////////////////////////////////////
// MarketFeed
////////////////////////////////////////////
mddBuf Publish::_MF_Build( mddMsgHdr  m,
                           mddBuf     payload,
                           mddBldBuf &bld )
{
   mddBuf r;
   bool   bHdr;
   char  *bp, *cp;
   int    i, nf, rSz, bSz;

   // MF Header, Fields, Trailer

   bHdr = bld._bNoHdr ? false : true;
   rSz  = payload._dLen;
   bSz  = rSz  + K; // Room for header ...
   bp   = rSz ? _InitBuf( bld, bSz ) : _InitFieldListBuf( bld );
   cp   = bp;
   cp  += bHdr ? _MF_Header( cp, m ) : 0;
   bld._payload = cp;
   if ( rSz ) {
      ::memcpy( cp, payload._data, rSz );
      cp += rSz;
   }
   else {
      switch( m._mt ) {
         case mddMt_update:
         case mddMt_image:
            nf  = nFld();
            for ( i=0; i<nf; cp += _MF_BuildFld( cp, _upds[i++] ) );
            break;
         case mddMt_dead:       
         case mddMt_stale:      
         case mddMt_recovering: 
         case mddMt_undef:
         case mddMt_mount:
         case mddMt_ping:
         case mddMt_ctl:
         case mddMt_open:
         case mddMt_close:
         case mddMt_query:
         case mddMt_insert:
         case mddMt_insAck:
         case mddMt_gblStatus:
         case mddMt_history:
         case mddMt_dbQry:
         case mddMt_dbTable:
            break;
      }
   }
   if ( bHdr ) {
      *cp++ = FS;
      *cp   = '\0';
   }
   r._data   = bp;
   r._dLen   = cp - bp;
   bld._dLen = r._dLen;
   _upds.clear();
   return r;
}

int Publish::_MF_Header( char *bp, mddMsgHdr m )
{
   mddBuf &svc = m._svc;
   mddBuf &tkr = m._tkr;
   bool    bImg;
   char   *cp;
   int     mt;

   // IMAGE or UPDATE  only (for now)

   cp = bp;
   switch( m._mt ) {
      case mddMt_update:
      case mddMt_image:
      {
         bImg  = ( m._mt == mddMt_image );
         mt    = bImg ? MT_REC_RES : MT_UPDATE;
         cp    = bp;
         *cp++ = FS;
         cp   += _int2str( cp, mt );
         *cp++ = US;
         cp   += _bufcpy( cp, svc );
         *cp++ = GS;
         cp   += _bufcpy( cp, tkr );
         *cp++ = US;
         if ( bImg )
            *cp++ = US;
         if ( m._iTag )
            cp += sprintf( cp, "%d", m._iTag );
         break;
      }
      case mddMt_ping:
assert( 0 ); // Handled in BuildMsg()
      case mddMt_dead:
         cp   += sprintf( cp, "%c%d", FS, MT_STAT_RES );
         *cp++ = US;
         cp   += _bufcpy( cp, svc );
         *cp++ = GS;
         cp   += _bufcpy( cp, tkr );
         cp   += sprintf( cp, "%c%s", RS, m._tag );
         break;
      case mddMt_gblStatus:
         cp += sprintf( cp, "%c%d", FS, MT_STATUS_MESS );
         cp += sprintf( cp, "%c%s%c%s", GS, _mdd_pGblSts, RS, m._tag );
         break;
      case mddMt_ctl:
         cp += sprintf( cp, "%c%d", FS, MT_COM_RESP );
         cp += sprintf( cp, "%c%s%c%c", GS, svc._data, US, US );
         cp += sprintf( cp, "%c%s", RS, m._tag );
         break;
      case mddMt_stale:      
      case mddMt_recovering: 
      case mddMt_undef:
      case mddMt_mount:
      case mddMt_open:
      case mddMt_close:
      case mddMt_query:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }
   return( cp-bp );
}

int Publish::_MF_BuildFld( char *bp, mddField f )
{
   char *cp;

   cp    = bp;
   *cp++ = RS;
   cp   += _int2str( cp, f._fid );
   *cp++ = US;
   cp   += _ASCII_BuildFld( cp, f );
   return( cp-bp );
}


////////////////////////////////////////////
// Binary
////////////////////////////////////////////
mddBuf Publish::_Binary_Build( mddMsgHdr  m,
                               mddBuf     payload,
                               mddBldBuf &bld ) 
{
   mddBuf    r;
   mddBinHdr h;
   mddField  f;
   mddBuf   &b = f._val._buf;
   Binary    bin;
   bool      bHdr;
   char     *bp;
   u_char   *cp;
   int       i, nf, rSz, bSz;

   // mddBinHdr - Time set in Binary._TimeNow()

   ::memset( &h, 0, sizeof( h ) );
   h._dt       = m._dt;
   h._mt       = m._mt;
   h._protocol = mddProto_Binary;
   if ( m._bPack )
      h._protocol = (mddProtocol)( h._protocol | PACKED_BINARY );
   h._tag      = m._iTag;
   h._RTL      = m._RTL;

   // Support for specific mddMsgType's

   switch( m._mt ) {
      case mddMt_dead:
      case mddMt_gblStatus:
         f._fid  = -1;
         f._type = mddFld_string;
         b._data = m._tag;
         b._dLen = strlen( b._data );
         _upds.Insert( f );
         break;
      case mddMt_image:
      case mddMt_update:     
      case mddMt_stale:      
      case mddMt_recovering: 
      case mddMt_undef:
      case mddMt_mount:
      case mddMt_ping:
      case mddMt_ctl:
      case mddMt_open:
      case mddMt_close:
      case mddMt_query:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }

   // Pack Header, Fields, Header Len

   bHdr = bld._bNoHdr ? false : true;
   rSz  = payload._dLen;
   bSz  = rSz  + K; // Room for header ...
   bp   = rSz ? _InitBuf( bld, bSz ) : _InitFieldListBuf( bld );
   cp   = (u_char *)bp;
   cp  += bHdr ? bin.Set( cp, h ) : 0;
   bld._payload = (char *)cp;
   if ( payload._dLen ) {
      ::memcpy( cp, payload._data, payload._dLen );
      cp += payload._dLen;
   }
   else {
      nf  = nFld();
      for ( i=0; i<nf; cp += bin.Set( cp, _upds[i++] ) );
   }
   h._len = ( cp - (u_char *)bp );
   bin.Set( (u_char *)bp, h, true );
   r._data   = bp;
   r._dLen   = h._len;
   bld._dLen = r._dLen;
   _upds.clear();
   return r;
}

mddBuf Publish::_Binary_SetHdrTag( u_int iTag, mddBuf &bld )
{
   Binary    bin;
   mddBinHdr h;
   u_char   *cp;

   h._len = 0;
   cp     = (u_char *)bld._data;
   cp    += sizeof( h._len );
   bin.Set( cp, iTag, true );
   return bld;
}


////////////////////////////////////////////
// Helpers
////////////////////////////////////////////
int Publish::_ASCII_BuildFld( char *bp, mddField f ) 
{
   mddValue &v = f._val;
   mddBuf   &b = v._buf;
   char     *cp, *fmt;
   char      buf[K];
   double    ymd;
   u_int64_t hms64;
   int       ymd32, Y, M, D, hms32, h, m, s, ms; 

   cp = bp;
   switch( f._type ) { 
      case mddFld_string:
         cp += _bufcpy( cp, b );
         // Fall-through
      case mddFld_undef:
         *cp = '\0';
         break;
      case mddFld_int8:
         cp += sprintf( cp, "%d", v._i8 );
         break;
      case mddFld_int32:
         cp += sprintf( cp, "%d", v._i32 );
         break;
      case mddFld_int16:
         cp += sprintf( cp, "%d", v._i16 );
         break;
      case mddFld_int64:
#if !defined(WIN32)
         cp += sprintf( cp, "%ld", v._i64 );
#else
         cp += sprintf( cp, "%I64d", v._i64 );
#endif
         break;
      case mddFld_double:
         sprintf( buf, _sigr64, v._r64 );
         cp += sprintf( cp, _TrimTrailingZero( buf ) );
         break;
      case mddFld_float:
         sprintf( buf, _sigr32, v._r32 );
         cp += sprintf( cp, _TrimTrailingZero( buf ) );
         break;
      case mddFld_date:
         ymd   = v._r64 / 1000000.0;
         ymd32 = (int)ymd;
         Y     = ymd32 / 10000;
         M     = ( ymd32 / 100 ) % 100;
         M     = WithinRange( 1, M, 12 );
         D     = ymd32 % 100;
         cp   += sprintf( cp, "%02d %s %04d ", D, _pMons[M-1], Y );
         // Fall-through
      case mddFld_time:
      case mddFld_timeSec:
         hms64 = (u_int64_t)v._r64;
         hms32 = hms64 % 1000000;
         h     = hms32 / 10000;
         m     = ( hms32 / 100 ) % 100;
         s     = hms32 % 100;
         ms    = (int)( ( v._r64 - ( 1.0 * hms64 ) ) * 1000.0 );
         cp   += sprintf( cp, "%02d:%02d:%02d", h, m, s );
         if ( ms && ( f._type == mddFld_timeSec ) ) 
            cp += sprintf( cp, ".%03d", ms );
         break;
      case mddFld_real:
         fmt = "ERROR : FID %d; BINARY channel only for mddFld_real";
         cp += sprintf( cp, fmt, f._fid );
         break;
      case mddFld_bytestream:
         fmt = "ERROR : FID %d; BINARY channel only for mddFld_bytestream";
         cp += sprintf( cp, fmt, f._fid );
         break;
   }
   return( cp-bp );
}

char *Publish::_InitFieldListBuf( mddBldBuf &bld )
{
   int fSz;

   // Estimated size : K byte / field

   fSz = _estFldSz ? _estFldSz : gmax( nFld()*K, K );
   return _InitBuf( bld, fSz );
}
