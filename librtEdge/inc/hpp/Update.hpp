/******************************************************************************
*
*  Update.hpp
*     Publish one update to rtEdgeCache3
*
*  REVISION HISTORY:
*      3 SEP 2014 jcs  Created.
*     23 JAN 2015 jcs  Build 29: ByteStreamFld; AddChainLink()
*     20 JUN 2015 jcs  Build 31: Fully-qualified std:: (compiler)
*     20 MAR 2016 jcs  Build 32: AddField( int, const char *, ... )
*     25 APR 2016 jcs  Build 33: AddField( int, rtDate / rtTime );
*     24 OCT 2017 jcs  Build 36: _MAX_DBL; Non-zero FID only
*      4 DEC 2017 jcs  Build 39: _MAX_FLOAT
*      6 MAR 2018 jcs  Build 40: Reset()
*      6 DEC 2018 jcs  Build 41: VOID_PTR
*
*  (c) 1994-2018 Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Update_H
#define __RTEDGE_Update_H
#include <hpp/rtEdge.hpp>

// TODO : New libmddWire bumps this to 1750

static double _MAX_DBL   =   879.0; // 879.6093022207 = 0x7ffffffffff 
static double _MAX_FLOAT = 53000.0; // 53687.0911 = 0x1fffffff 

namespace RTEDGE
{

////////////////////////////////////////////////
//
//        c l a s s   U p d a t e
//
////////////////////////////////////////////////

/**
 * \class Update
 * \brief This reusable class is an update published to rtEdgeCache3.
 *
 * You build an update with this reusable class by calling Init()
 * then adding fields from the AddField() and friends.
 * You then call Publish() to do the deed.
 */

class Update : public rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.  This update is associated with a single PubChannel
	 *
	 * \param pub - PubChannel that this update is associated with
	 */
	Update( PubChannel &pub ) :
	   _pub( pub ),
	   _svc( pub.pPubName() ),
	   _tkr(),
	   _flds( new rtFIELD[K] ),
	   _nAlloc( K ),
	   _bPubByteStr( false )
	{
	}

	~Update()
	{
	   delete[] _flds;
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/**
	 * \brief Returns published service name (i.e., PubChannel::pPubName())
	 *
	 * \return Published Service name (i.e., PubChannel::pPubName())
	 */
	const char *pSvc()
	{
	   return _svc.data();
	}

	/**
	 * \brief Returns published ticker name
	 *
	 * The ticker name may change on each update and is set in Init().
	 * \return Published ticker name
	 */
	const char *pTkr()
	{
	   return _tkr.data();
	}

	/**
	 * \brief Returns data to publish
	 *
	 * This is initialized in Init() and set via AddField() and friends. 
	 * \return Data to publish
	 */
	rtEdgeData data()
	{
	   return _d;
	}

	/**
	 * \brief Returns number of fields to publish
	 *
	 * \return Number of fields to publish
	 */
	int NumField()
	{
	   return _d._nFld;
	}


	////////////////////////////////////
	// Mutator
	////////////////////////////////////
public:
	/**
	 * \brief Resets the internals of this update so you can call
	 * AddField() and friends BEFORE calling Init()
	 */
	void Reset()
	{
	   _Clear();
	   _d._flds = _flds;
	}

	/**
	 * \brief Initialize for publication of tkr
	 * 
	 * \param tkr - Ticker name
	 * \param arg - User-defined argument
	 * \param bImg - true if Image; false if update
	 * \param bReset - true to call Reset()
	 */
	void Init( const char *tkr, 
	           void       *arg, 
	           bool        bImg   = false, 
	           bool        bReset = true  )
	{
	   if ( bReset )
	      Reset();
	   _tkr     = tkr;
	   _d._pSvc = pSvc();
	   _d._pTkr = pTkr();
	   _d._arg  = arg;
	   _d._ty   = bImg ? edg_image : edg_update;
	   _d._flds = _flds;
	}

	/**
	 * \brief Initialize for publication of tkr
	 * 
	 * \param tkr - Ticker name
	 * \param PubID - User-defined argument
	 * \param bImg - true if Image; false if update
	 * \param bReset - true to call Reset()
	 */
	void Init( const char *tkr, 
	           int         PubID,
	           bool        bImg   = false, 
	           bool        bReset = true  )
	{
	   return Init( tkr, (VOID_PTR)PubID, bImg, bReset );
	}

	/**
	 * \brief Add field list to udpate
	 * 
	 * The caller is responsible for ensuring the integrity of the 
	 * data in the field list until Publish() is called.
	 *
	 * \param fl - mddFieldList
	 */
	void AddFieldList( mddFieldList &fl )
	{
	   int      i;
	   mddField f;
	   rtFIELD  rf;
	   rtVALUE &v   = rf._val;
	   double  &r64 = f._val._r64;
	   bool     bDbl;

	   // bbPortal3 uses this API

	   for ( i=0; i<fl._nFld; i++ ) {
	      f        = fl._flds[i];
	      rf._fid  = f._fid; 
	      rf._name = f._name;
	      bDbl     = ( f._type == mddFld_double );
	      if ( bDbl && !InRange( -_MAX_DBL, r64, _MAX_DBL ) ) {
	         if ( InRange( -_MAX_FLOAT, r64, _MAX_FLOAT ) ) {
	            v._r32   = (float)r64;
	            rf._type = rtFld_float;
	         }
	         else {
	            v._i64   = (u_int64_t)r64;
	            rf._type = rtFld_int64;
	         }
	      }
	      else {
	         rf._type = (rtFldType)f._type;
	         rf._val  = f._val;
	      }
	      _Add( rf );
	   }
	}

	/**
	 * \brief Add string field to update
	 * 
	 * The caller is responsible for ensuring the integrity of the 
	 * data in pFld until Publish() is called. 
	 *
	 * \param fid - Field ID
	 * \param pFld - Field value as string
	 * \param fLen - Field length; 0 implies NULL-terminated
	 */
	void AddField( int fid, char *pFld, int fLen=0 )
	{
	   rtFIELD f;
	   rtBUF  &b = f._val._buf;

	   f._type = rtFld_string;
	   f._fid  = fid;
	   b._data = pFld;
	   b._dLen = fLen ? fLen : strlen( pFld );
	   _Add( f );
	}

	/**
	 * \brief Add string field to update
	 * 
	 * The caller is responsible for ensuring the integrity of the 
	 * data in pFld until Publish() is called. 
	 *
	 * \param fid - Field ID
	 * \param pFld - Field value as string
	 * \param fLen - Field length; 0 implies NULL-terminated
	 */
	void AddField( int fid, const char *pFld, int fLen=0 )
	{
	   AddField( fid, (char *)pFld, fLen );
	}

	/**
	 * \brief Add 8-bit int field to update
	 * 
	 * \param fid - Field ID
	 * \param i8 - Field value as 8-bit int
	 */
	void AddField( int fid, u_char i8 )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;

	   f._type = rtFld_int8;
	   f._fid  = fid;
	   v._i8   = i8;
	   _Add( f );
	}

	/**
	 * \brief Add 16-bit int field to update
	 * 
	 * \param fid - Field ID
	 * \param i16 - Field value as 16-bit int
	 */
	void AddField( int fid, u_short i16 )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;

	   f._type = rtFld_int16;
	   f._fid  = fid;
	   v._i16  = i16;
	   _Add( f );
	}

	/**
	 * \brief Add 32-bit int field to update
	 * 
	 * \param fid - Field ID
	 * \param i32 - Field value as 32-bit int
	 */
	void AddField( int fid, int i32 )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;

	   f._type = rtFld_int;
	   f._fid  = fid;
	   v._i32  = i32;
	   _Add( f );
	}

	/**
	 * \brief Add 64-bit int field to update
	 * 
	 * \param fid - Field ID
	 * \param i64 - Field value as 64-bit int
	 */
	void AddField( int fid, u_int64_t i64 )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;

	   f._type = rtFld_int64;
	   f._fid  = fid;
	   v._i64  = i64;
	   _Add( f );
	}


	/**
	 * \brief Add float field to update
	 * 
	 * \param fid - Field ID
	 * \param r32 - Field value as float
	 */
	void AddField( int fid, float r32 )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;

	   f._type = rtFld_float;
	   f._fid  = fid;
	   v._r32  = r32;
	   _Add( f );
	}

	/**
	 * \brief Add double field to update
	 * 
	 * \param fid - Field ID
	 * \param r64 - Field value as double
	 */
	void AddField( int fid, double r64 )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;

	   if ( !InRange( -_MAX_DBL, r64, _MAX_DBL ) ) {
	      if ( InRange( -_MAX_FLOAT, r64, _MAX_FLOAT ) )
	         AddField( fid, (float)r64 );
	      else
	         AddField( fid, (u_int64_t)r64 );
	   }
	   else {
	      f._type = rtFld_double;
	      f._fid  = fid;
	      v._r64  = r64;
	      _Add( f );
	   }
	}

	/**
	 * \brief Add rtdate field to update
	 * 
	 * \param fid - Field ID
	 * \param dt - Field value as rtDate
	 */
	void AddField( int fid, rtDate dt )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;
	   int      ymd;

	   // YMD

	   f._type = rtFld_date;
	   f._fid  = fid;
	   ymd     = ( dt._year*10000 ) + ( dt._month*100 ) + dt._mday;
	   v._r64  = ymd;
	   v._r64 *= 1000000.0;
	   _Add( f );
	}

	/**
	 * \brief Add rtTime field to update
	 * 
	 * \param fid - Field ID
	 * \param tm - Field value as rtTime
	 */
	void AddField( int fid, rtTime tm )
	{
	   rtFIELD  f;
	   rtVALUE &v = f._val;
	   int      hms;

	   f._type = rtFld_time;
	   f._fid  = fid;
	   hms     = ( tm._hour*10000 ) + ( tm._minute*100 ) + tm._second;
	   v._r64  = hms;
	   v._r64 += ( 0.000001 * tm._micros );;
	   _Add( f );
	}

	/**
	 * \brief Add rtDateTime field to update
	 * 
	 * \param fid - Field ID
	 * \param dtTm - Field value as rtDateTime
	 */
	void AddField( int fid, rtDateTime dtTm )
	{
	   rtFIELD  f;
	   rtVALUE &v  = f._val;
	   rtDate  &dt = dtTm._date;
	   rtTime  &tm = dtTm._time;
	   int      ymd, hms;
	   bool     bDt, bTm;

	   // Support for rtDate-only or rtTime-only

	   bDt = dt._year || dt._month || dt._mday;
	   bTm = tm._hour || tm._minute || tm._second;
	   if ( bDt && !bTm ) {
	      AddField( fid, dt );
	      return;
	   }
	   else if ( !bDt && bTm ) {
	      AddField( fid, tm );
	      return;
	   }
	   else if ( !bDt && !bTm )
	      return;

	   // OK - It's a true date / time

	   f._type = rtFld_time;
	   f._fid  = fid;
	   ymd     = ( dt._year*10000 ) + ( dt._month*100 ) + dt._mday;
	   hms     = ( tm._hour*10000 ) + ( tm._minute*100 ) + tm._second;
	   v._r64  = ymd;
	   v._r64 *= 1000000.0;
	   v._r64 += hms;
	   v._r64 += ( 0.000001 * tm._micros );;
	   _Add( f );
	}

	/**
	 * \brief Add ByteStream field to update
	 * 
	 * \param fid - Field ID
	 * \param bStr - Field value as ByteStreamFld
	 */
	void AddField( int fid, ByteStreamFld bStr )
	{
	   rtFIELD f;
	   rtBUF  &dst = f._val._buf;
	   rtBUF   src = bStr.buf();

	   f._type   = rtFld_bytestream;
	   f._fid    = fid;
	   dst._data = src._data;
	   dst._dLen = src._dLen;
	   _Add( f );
	}

	////////////////////////////////////
	// Mutator - Chain 
	////////////////////////////////////
	/**
	 * \brief Builds and publishes chain link.
	 *
	 * This method adds the chain prefix - 0#, 1#, etc. - to the name.
	 * 
	 * \param chainName - Un-prefixed Chain Name
	 * \param arg - User-defined argument
	 * \param linkNum - Link Number
	 * \param bFinal - true if this is final link in chain
	 * \param links - Link names
	 * \param nLink - Number of links
	 * \param dpyTpl - Display template number
	 */
	int PubChainLink( const char  *chainName,
	                  void        *arg,
	                  int          linkNum, 
	                  bool         bFinal, 
	                  const char **links,
	                  int          nLink,
	                  int          dpyTpl=999 )
	{
	   std::string s, s1, s2;
	   char        pfx[10];
	   int         i, fid;

	   // Initialize

	   sprintf( pfx, "%d#", linkNum );
	   s  = pfx;
	   s += chainName;
	   Init( s.data(), arg );

	   // Display Template, Display Name, Num Links

	   nLink = gmin( nLink, _NUM_LINK );
	   AddField( RDNDISPLAY, dpyTpl );
	   AddField( DSPLY_NAME, (char *)chainName );
	   AddField( REF_COUNT, nLink );
	   AddField( RECORDTYPE, _CHAIN_REC );

	   // Previous Link?

	   if ( linkNum > 0 ) {
	      sprintf( pfx, "%d#", linkNum-1 );
	      s1  = pfx;
	      s1 += chainName;
	      AddField( LONGPREVLR, (char *)s1.data() );
	   }

	   // Links

	   fid = LONGLINK1;
	   for ( i=0; i<nLink; AddField( fid++, (char *)links[i++] ) );
	   for ( ; i<_NUM_LINK; AddField( fid++, (char *)"" ), i++ );

	   // Next link

	   if ( !bFinal ) {
	      sprintf( pfx, "%d#", linkNum+1 );
	      s2  = pfx;
	      s2 += chainName;
	      AddField( LONGNEXTLR, (char *)s2.data() );
	   }

	   return Publish();
	}

 
	//////////////////////////////////// 
	// Operations 
	////////////////////////////////////
public:
	/**
	 * \brief Publish fields added by AddField() and friends
	 *
	 * \return Length of published update in bytes
	 */
	int Publish()
	{
	   int rtn;

	   rtn = NumField() ? _pub.Publish( _d ) : 0;
	   _Clear();
	   return rtn;
	}

	/**
	 * \brief Publish pre-built field list
	 *
	 * \param d - Pre-built rtEdgeData struct
	 * \return Length of published update in bytes
	 */
	int Publish( rtEdgeData d )
	{
	   return _pub.Publish( d );
	}

	/**
	 * \brief Publish pre-formatted data payload
	 *
	 * \param b - Pre-formatted data payload
	 * \param dt - Payload data type
	 * \return Length of published update in bytes
	 */
	int Publish( rtBUF b, mddDataType dt=mddDt_FieldList )
	{
	   int rtn;

	   rtn = _pub.Publish( _d, b, dt );
	   _Clear();
	   return rtn;
	}

	/**
	 * \brief Publish an error
	 *
	 * \return Length of published msg in bytes
	 */
	int PubError( const char *err )
	{
	   int rtn;

	   _d._pErr = err;
	   rtn      = _pub.PubError( _d, err );
	   _Clear();
	   return rtn;
	}


	////////////////////////
	// Operaions - ByteStream
	////////////////////////
	/**
	 * \brief Publish a ByteStream
	 *
	 * \param bStr - ByteStream to publish
	 * \param fidData - Field ID where payload data will be put in message.
	 * \param maxFldSiz - Max bytes in each field; Default 1024
	 * \param maxFld - Max fields per message; Default 1024
	 * \param bytesPerSec - Max bytes / sec to publish if multi-message stream;
	 * Default 1048576 (1 MB / sec )
	 * \return  Number of bytes published
	 */
	int Publish( ByteStream &bStr, 
	             int         fidData,
	             int         maxFldSiz   = K, 
	             int         maxFld      = K,
	             int         bytesPerSec = K*K )
	{
	   rtBUF         pBuf = bStr.pubBuf();
	   void         *arg  = _d._arg;
	   ByteStreamFld f;
	   const char   *fmt;
	   char         *cp, buf[K];
	   int           i, nf, np, off, len, fid, fSz, byMsg;
	   double        dSlp;

	   // Quick Check : Channel must be binary

	   if ( !_pub.IsBinary() ) {
	      PubError( "Publication Channel must be binary" ); 
	      bStr.OnError( "Publication Channel must be binary" );
	      return 0;
	   }

	   // Bytes / message; Sleep interval

	   if ( !InRange( K, bytesPerSec, 100*K*K ) )
	      bytesPerSec = K*K;
	   byMsg = maxFldSiz * maxFld;
	   dSlp  = (double)byMsg / (double)bytesPerSec;

	   // Do it ...

	   cp           = pBuf._data;
	   len          = pBuf._dLen;
	   _bPubByteStr = true;
	   for ( i=0,off=0; off<len && _bPubByteStr; i++ ) {
	      Init( bStr.tkr(), arg, true );
	      AddField( bStr.fidOff(), off );
	      AddField( bStr.fidLen(), len );
	      AddField( bStr.fidPayload(), fidData );
	      fid = fidData;
	      for ( nf=0,np=0; off<len && nf<maxFld && _bPubByteStr; nf++ ) {
	         fSz = gmin( len-off, maxFldSiz );
	         AddField( fid++, f.Set( cp, fSz ) );
	         cp  += fSz;
	         off += fSz;
	         np  += fSz;
	      }
	      if ( _bPubByteStr ) {
	         AddField( bStr.fidNumFld(), nf );
	         Publish();
	         bStr.OnPublishData( np, off );
	         if ( off<len )
	            _pub.Sleep( dSlp );
	      }
	      else {
	         fmt = "ByteStream terminated by user; %d of %d bytes sent";
	         sprintf( buf, fmt, off, len );
	         bStr.OnError( buf );
	         PubError( buf );
	      }
	   }
	   if ( _bPubByteStr )
	      bStr.OnPublishComplete( off );
	   return off;
	}

	/**
	 * \brief Terminate ByteStream Publishing
	 *
	 * This method allows a different thread to terminate the publication 
	 * of a ByteStream.  This may be useful when publishing a large 
	 * ByteStream.
	 *
	 * \param bStr - ByteStream to publish
	 */
	void Stop( ByteStream &bStr )
	{
	   _bPubByteStr = false;
	} 

	////////////////////////
	// Helpers
	////////////////////////
private:
	void _Clear()
	{
	   ::memset( &_d, 0, sizeof( _d ) );
	}

	void _Add( rtFIELD f )
	{
	   int      i, nf;
	   rtFIELD *fdb;

	   // Non-zero FID only

	   if ( !f._fid )
	      return;

	   // Grow, if necessary

	   nf = _d._nFld;
	   if ( (nf+1) >= _nAlloc ) {
	      fdb      = _flds;
	      _nAlloc += _nAlloc;
	      _flds    = new rtFIELD[_nAlloc];
	      _d._flds = _flds;
	      for ( i=0; i<nf; _flds[i] = fdb[i], i++ );
	      delete[] fdb;
	   }

	   // Add back

	   _flds[nf] = f;
	   _d._nFld += 1;
	}

	////////////////////////
	// Private Members
	////////////////////////
private:
	PubChannel  &_pub;
	std::string  _svc;
	std::string  _tkr;
	rtEdgeData   _d;
	rtFIELD     *_flds;
	int          _nAlloc;
	bool         _bPubByteStr;

};  // class Update

} // namespace RTEDGE

#endif // __RTEDGE_Update_H 
