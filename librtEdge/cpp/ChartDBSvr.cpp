/******************************************************************************
*
*  ChartDbSvr.cpp
*     ChartDB file server - rtFld_bytestream
*
*  REVISION HISTORY:
*     18 MAR 2016 jcs  Created (from FileSvr.cpp)
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

#define _CDB_MAGIC         0x0a0e05   // "aes"
#define _CDB_SVR_SIG_001   "ChartDbSvr 001"

using namespace RTEDGE;

static int _fidPayload = 10001;

// Forwards

class ChartDbSvr;
class Watch;

typedef std::map<std::string, Watch *> WatchList;

/////////////////////////
// ChartDB Data on Wire ...
/////////////////////////
class WireMsg
{
public:
   int  _magic;          // 0x0a0e05 = aes
   char _signature[20];  // _CDB_SVR_SIG_001
   int  _interval;
   int  _curTick;
   int  _nPts;
//   float _pts[_nPts];
};

/////////////////////////
// Watched ByteStream
/////////////////////////
class Watch : public ByteStream
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
public:
   PubChannel &_pub;
   void       *_tag;
   rtBUF       _raw;
   u_int       _off;
   int         _nPub;
   std::string _tm;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   Watch( PubChannel      &pub,
          const char      *tkr,
          void            *tag,
          RTEDGE::CDBData &cdb ) :
      ByteStream( pub.pPubName(), tkr ),
      _pub( pub ),
      _tag( tag ),
      _nPub( 0 ),
      _tm()
   {
      ::memset( &_raw, 0, sizeof( _raw ) );
      _SetPublishData( cdb );
   }

   ~Watch()
   {
      if ( _raw._data )
         delete[] _raw._data;
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
   virtual void OnError( const char *err )
   {
      const char *fmt = "[%s] ERROR( %s ) : %s\n";

      ::fprintf( stdout, fmt, tm(), tkr(), err );
      ::fflush( stdout );
   }

   virtual void OnPublishData( int nByte, int totByte )
   {
      const char *fmt = "[%s] PUB : %d byte chunk; %d total\n";

      ::fprintf( stdout, fmt, tm(), nByte, totByte );
      ::fflush( stdout );
   }

   virtual void OnPublishComplete( int nByte )
   {
      const char *fmt = "[%s] Complete( %s ) : %d bytes\n";

      ::fprintf( stdout, fmt, tm(), tkr(), nByte );
      ::fflush( stdout );
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
private:
   void _SetPublishData( RTEDGE::CDBData &cdb )
   {
      ::CDBData &d = cdb.data();
      WireMsg   *w;
      char      *cp;
      float     *src, *dst;
      int        i, np, sz;
      static int _wSz = sizeof( WireMsg );

      // Allocate Buffer

      src        = cdb.flds();
      np         = cdb.Size();
      sz         = _wSz + ( np * sizeof( float ) );
      _raw._data = new char[sz+4];
      _raw._dLen = sz;

      // WireMsg Header

      cp  = _raw._data;
      w   = (WireMsg *)cp;
      cp += _wSz;
      ::memset( w, 0, _wSz );
      w->_magic    = _CDB_MAGIC;
      w->_interval = d._interval;
      w->_curTick  = d._curTick;
      w->_nPts     = np;
      strcpy( w->_signature, _CDB_SVR_SIG_001 );

      // Payload ...

      dst = (float *)cp;
      for ( i=0; i<np; dst[i]=src[i],i++ );
      SetPublishData( _raw );
   }


   ////////////////////////////////////
   // Helpers
   ////////////////////////////////////
private:
   const char *tm()
   {
      return _pub.pDateTimeMs( _tm );
   }
};


class ChartDbSvr : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   RTEDGE::ChartDB &_chartDB;
   int              _fldSz;
   int              _nFld;
   int              _bytesPerSec;
   WatchList        _wl;
   std::string      _tm;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   ChartDbSvr( RTEDGE::ChartDB &chartDB, const char *pPub ) :
      PubChannel( pPub ),
      _chartDB( chartDB ),
      _fldSz( K ),
      _nFld( K ),
      _bytesPerSec( K*K ),
      _wl(),
      _tm()
   {
      SetBinary( true );
   }


   ////////////////////////////////////
   // Access
   ////////////////////////////////////
public:
   const char *tm()
   {
      return pDateTimeMs( _tm );
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
public:
   void PubTkr( Watch &w )
   {
      Update &u = upd();

      u.Init( w.tkr(), w._tag, true );
      u.Publish( w, 
                 _fidPayload,       // Put payload into this field ... 
                 _fldSz,            // ... up to this size per field ...
                 _nFld,             // ... and this many fields per msg ...
                 _bytesPerSec  );   // ... and this many bytes / sec if 
                                    //     multiple messages required.
      w._nPub += 1;
   }

   void PubDead( const char *tkr, const char *err, void *arg )
   {
      Update &u = upd();

      u.Init( tkr, arg, false );
      u.PubError( err );
      ::fprintf( stdout, "DEAD %s : %s\n", tkr, err );
      ::fflush( stdout );
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      ::fprintf( stdout, "[%s] CONN %s : %s\n", tm(), pUp, msg );
      ::fflush( stdout );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      Watch              *w;
      WatchList::iterator it;
      const char         *err;
      std::string         s( tkr ), tmp( tkr );
      char               *qry, *svc, *ric, *fld, *rp;
      size_t              nt;
      int                 fid;

      // Svc|Ticker|FID

      ::fprintf( stdout, "[%s] OPEN %s\n", tm(), tkr );
      ::fflush( stdout );
      qry = (char *)tmp.data();
      svc = ::strtok_r( qry,  "|", &rp );
      ric = ::strtok_r( NULL, "|", &rp );
      fld = ::strtok_r( NULL, "|", &rp );
      fid = fld ? atoi( fld ) : 0;
      err = !fid ? "Wrong format : Svc|Ticker|FID" : NULL;
      if ( err ) {
         PubDead( tkr, err, arg );
         return;
      }

      // Process : Pull from ChartDB

      RTEDGE::CDBData &cdb = _chartDB.View( svc, ric, fid );

      err = cdb.pErr();
      nt  = _wl.size();
      if ( err && strlen( err ) )
         PubDead( tkr, err, arg );
      else {
         if ( (it=_wl.find( s )) == _wl.end() ) {
            w      = new Watch( *this, tkr, arg, cdb );
            _wl[s] = w;
         }
         else
            w = (*it).second;
         PubTkr( *w );
      }
   }

   virtual void OnPubClose( const char *tkr )
   {
      WatchList::iterator it;
      std::string         s( tkr );

      if ( (it=_wl.find( s )) != _wl.end() ) {
         ::fprintf( stdout, "[%s] CLOSE %s\n", tm(), tkr );
         ::fflush( stdout );
         _wl.erase( it );
         delete (*it).second;
      }
   }


   ////////////////////////////////////
   // PubChannel Interface
   ////////////////////////////////////
protected:
   virtual Update *CreateUpdate()
   {
      return new Update( *this );
   }
};


////////////////////////////////
// main()
////////////////////////////////
int main( int argc, char **argv )
{
   RTEDGE::ChartDB *cdb;
   ChartDbSvr      *pub;
   double           dSlp;
   char            *ps;
   int              i;
   const char      *pSvr, *pPub, *pLVC;

   // [ <host:port> <Svc> <LVCfilename>]

   if ( argc > 1 && !::strcmp( argv[1], "-help" ) ) {
      printf( "%s <host:port> <Svc> <LVCfilename>\n", argv[0] );
      return 0;
   }
   pSvr  = "localhost:9995";
   pPub  = "ChartDbSvr";
   pLVC  = "./chart.db";
   for ( i=0; i<argc; i++ ) {
      switch( i ) {
         case 1: pSvr = argv[i]; break;
         case 2: pPub = argv[i]; break;
         case 3: pLVC = argv[i]; break;
      }
   }
   cdb = new ChartDB( pLVC );
   if ( !cdb->IsValid() ) {
      ::fprintf( stdout, "Invalid LVC file %s; Exitting ...\n", pLVC );
      return 0;
   }
   pub = new ChartDbSvr( *cdb, pPub );
   ::fprintf( stdout, "%s\n", pub->Version() );
   ::fprintf( stdout, "%s\n", pub->Start( pSvr, true ) );
   ps   = ::getenv( "__JD_SLEEP" );
   dSlp = ps ? atof( ps ) : 0.0;
   if ( dSlp == 0.0 ) {
      ::fprintf( stdout, "Hit <ENTER> to terminate..." );
      ::fflush( stdout );
      getchar();
   }
   else {
      ::fprintf( stdout, "Sleeping for %.1fs...\n", dSlp );
      pub->Sleep( dSlp );
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub->Stop();
   delete pub;
   delete cdb;
   ::fprintf( stdout, "Done!!\n" ); ::fflush( stdout );
   return 0;
}
