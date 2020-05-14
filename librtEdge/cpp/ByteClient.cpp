/******************************************************************************
*
*  ByteClient.cpp
*     ByteStream client - From FileSvr
*
*  REVISION HISTORY:
*     11 JAN 2015 jcs  Created (from Client.cs).
*
*  (c) 1994-2015 Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;


class MyStream : public ByteStream
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyStream( const char *svc, const char *tkr ) :
      ByteStream( svc, tkr )
   { ; }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnData( rtBUF buf )
   {
      ::fprintf( stdout, "OnData() : %d bytes\n", buf._dLen );
      ::fflush( stdout );
   }

   virtual void OnError( const char *err )
   {
      ::fprintf( stdout, "OnError() : %s\n", err );
      ::fflush( stdout );
   }

   virtual void OnSubscribeComplete()
   {
      ::fprintf( stdout, "COMPLETE : %d bytes\n", subBufLen() );
      ::fflush( stdout );
   }
};

class ByteClient : public SubChannel
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   ByteClient()
   {
      SetBinary( true );
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

   virtual void OnService( const char *svc, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      ::fprintf( stdout, "Service %s : %s\n", svc, pUp );
      ::fflush( stdout );
   }
};


////////////////////////////////
// main()
////////////////////////////////
int main( int argc, char **argv )
{
   ByteClient  sub;
   MyStream   *str;
   int         i;
   const char *pEdg, *pSvc, *pTkr, *pUsr;

   // [ <host:port> <Svc> <Tkr> <User> ]

   if ( argc > 1 && !::strcmp( argv[1], "-help" ) ) {
      printf( "%s <host:port> <Svc> <Tkr> <User>\n", argv[0] );
      return 0;
   }
   pEdg  = "localhost:9998";
   pSvc  = "FileSvr";
   pTkr  = "./a.out";
   pUsr  = "Client";
   for ( i=1; i<argc; i++ ) {
      switch( i ) {
         case 1: pEdg = argv[i]; break;
         case 2: pSvc = argv[i]; break;
         case 3: pTkr = argv[i]; break;
         case 4: pUsr = argv[i]; break;
      }
   }
   ::fprintf( stdout, "%s\n", sub.Version() );
   str = new MyStream( pSvc, pTkr );
   ::fprintf( stdout, "%s\n", sub.Start( pEdg, pUsr ) );
   sub.Sleep( 1.0 ); // Wait for protocol negotiation to finish
   sub.Subscribe( *str );
   ::fprintf( stdout, "Hit <ENTER> to terminate..." );
   ::fflush( stdout );
   getchar();

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   sub.Stop();
   ::fprintf( stdout, "Done!!\n" ); ::fflush( stdout );
   return 0;
}
