/******************************************************************************
*
*  FileSvr.cpp
*     Binary file server - rtFld_bytestream
*
*  REVISION HISTORY:
*     11 JAN 2015 jcs  Created (from FileSvr.cs)
*      4 MAY 2015 jcs  Build 31: Fully-qualified std:: (compiler)
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;

static int _fidPayload = 10001;

class Watch : public ByteStream
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
public:
   PubChannel &_pub;
   void       *_tag;
   u_int       _off;
   int         _nPub;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   Watch( PubChannel &pub,
          const char *tkr,
          void       *tag,
          rtBUF       raw ) :
      ByteStream( pub.pPubName(), tkr ),
      _pub( pub ),
      _tag( tag ),
      _nPub( 0 )
   {
      SetPublishData( raw );
   }

   ~Watch()
   {
      _pub.UnmapFile( pubBuf() );
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
   virtual void OnError( const char *err )
   {
      ::fprintf( stdout, "ERROR( %s ) : %s\n", tkr(), err );
      ::fflush( stdout );
   }

   virtual void OnPublishData( int nByte, int totByte )
   {
      ::fprintf( stdout, "PUB : %d byte chunk; %d total\n", nByte, totByte );
      ::fflush( stdout );
   }

   virtual void OnPublishComplete( int nByte )
   {
      ::fprintf( stdout, "Complete( %s ) : %d bytes\n", tkr(), nByte );
      ::fflush( stdout );
   }
};

typedef std::map<std::string, Watch *> WatchList;

class FileSvr : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   int       _tPub;
   int       _fldSz;
   int       _nFld;
   int       _bytesPerSec;
   WatchList _wl;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   FileSvr( const char *pPub, 
            int         tPub,
            int         fldSz,
            int         nFld,
            int         bps ) :
      PubChannel( pPub ),
      _tPub( tPub ),
      _fldSz( fldSz ),
      _nFld( nFld ),
      _bytesPerSec( bps ),
      _wl()
   {
      SetBinary( true );
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

   void PubDead( const char *tkr, void *arg )
   {
      Update     &u = upd();
      const char *err = "File not found";

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

      ::fprintf( stdout, "CONN %s : %s\n", pUp, msg );
      ::fflush( stdout );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      Watch              *w;
      WatchList::iterator it;
      rtBUF               raw;
      std::string              s( tkr );
      size_t              nt;

      ::fprintf( stdout, "OPEN %s\n", tkr );
      ::fflush( stdout );
      nt  = _wl.size();
      raw = MapFile( tkr );
      if ( !raw._dLen )
         PubDead( tkr, arg );
      else {
         if ( (it=_wl.find( s )) == _wl.end() ) {
            w      = new Watch( *this, tkr, arg, raw );
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

      ::fprintf( stdout, "CLOSE %s\n", tkr );
      ::fflush( stdout );
      if ( (it=_wl.find( s )) != _wl.end() ) {
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
   FileSvr    *pub;
   int         i, tPub, fldSz, nFld, bps;
   const char *pSvr, *pPub, *fmt;

   // [ <host:port> <Svc> <PubIntvl> <FieldSize> <NumFlds> <BytesPerSec>]

   if ( argc > 1 && !::strcmp( argv[1], "-help" ) ) {
      fmt = "%s <host:port> <Svc> <PubIntvl> <FldSz> <nFld> <BytesPerSec>\n";
      printf( fmt, argv[0] );
      return 0;
   }
   pSvr  = "localhost:9995";
   pPub  = "FileSvr";
   tPub  = 1000;
   fldSz = 1024;
   nFld  = 100;
   bps   = 1024*1024;
   for ( i=0; i<argc; i++ ) {
      switch( i ) {
         case 1: pSvr  = argv[i]; break;
         case 2: pPub  = argv[i]; break;
         case 3: tPub  = atoi( argv[i] ); break;
         case 4: fldSz = atoi( argv[i] ); break;
         case 5: nFld  = atoi( argv[i] ); break;
         case 6: bps   = atoi( argv[i] ); break;
      }
   }
   pub = new FileSvr( pPub, tPub, fldSz, nFld, bps );
   ::fprintf( stdout, "%s\n", pub->Version() );
   ::fprintf( stdout, "%s\n", pub->Start( pSvr, true ) );
   ::fprintf( stdout, "Hit <ENTER> to terminate..." );
   ::fflush( stdout );
   getchar();

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub->Stop();
   delete pub;
   ::fprintf( stdout, "Done!!\n" ); ::fflush( stdout );
   return 0;
}
