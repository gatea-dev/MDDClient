/******************************************************************************
*
*  Ysend.cpp
*     libyamr test app
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019, Gatea Ltd.
******************************************************************************/
#include <libyamr.h>
#include <string>
#include <vector>

using namespace std;

typedef vector<string> Strings;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *YsendID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)Ysend Build 3 " );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


////////////////////////
//
//    M y Y A M R
//
////////////////////////
class MyYAMR : public YAMR::Writer
{
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


   ////////////////////////////////
   // Access
   ////////////////////////////////
   size_t GetFile( Strings &sdb, const char *pfile )
   {
      yamrBuf yb;
      char    *cp, *rp;
      int      rqSz, txSz;

      sdb.clear();
      if ( (yb=MapFile( pfile ))._dLen ) {
         string      s( yb._data, yb._dLen );
         const char *_CR = "\n";

         cp = ::strtok_r( (char *)s.data(), _CR, &rp );
         for ( ; cp; cp = ::strtok_r( NULL, _CR, &rp ) )
            sdb.push_back( string( cp ) );
         rqSz = gmax( GetTxBufSize(), (int)yb._dLen );
         txSz = SetTxBufSize( rqSz );
         ::fprintf( stdout, "SetTxBufSize( %d ) = %d\n", rqSz, txSz );
         ::fflush( stdout );
      }
      UnmapFile( yb );
      return sdb.size();
   }

}; // class MyYAMR


////////////////////////
//
//     main()
//
////////////////////////
int main( int argc, char **argv )
{
   char   *svr, *cp, buf[1024];
   MyYAMR  wr;
   Strings sdb;
   size_t  i, n;
   int     SessID, proto;
   bool    run;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", YsendID() );
      printf( "%s\n", wr.Version() );
      return 0;
   }

   // cmd-line args

   if ( argc < 4 ) {
      printf( "Usage: %s <hosts> <SessID> <Proto>; Exitting ...\n", argv[0] );
      return 0;
   }
   svr    = argv[1];
   SessID = atoi( argv[2] );
   proto  = atoi( argv[3] );
   ::fprintf( stdout, "%s\n", YsendID() );
   ::fprintf( stdout, "%s\n", wr.Version() );
   ::fprintf( stdout, "%s\n", wr.Start( svr, SessID, true ) );
   ::fprintf( stdout, "Enter shit to send; QUIT to terminate...\n" );
   for( run=true; run && ::fgets( buf, 1024, stdin ); ) {
      if ( (cp=::strstr( buf, "\n" )) )
         *cp = '\0';
      run = ::strcmp( buf, "QUIT" );
      if ( run ) {
         if ( strlen( buf ) ) {
            n = wr.GetFile( sdb, buf );
            for ( i=0; i<n; wr.Send( sdb[i++], proto ) );
            if ( !n )
               wr.Send( buf, strlen( buf ), proto );
            sdb.clear();
         }
      }
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   wr.Stop();
   printf( "Done!!\n " );
   return 1;
}
