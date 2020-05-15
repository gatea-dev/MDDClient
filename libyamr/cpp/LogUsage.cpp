/******************************************************************************
*
*  LogUsage.cpp
*     libyamr test app
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019, Gatea Ltd.
******************************************************************************/
#include <libyamr.h>
#include <bespoke/Usage.hpp>

using namespace std;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *LogUsageID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)LogUsage Build 3 " );
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
      int         Qsz = SetTxBufSize( 10*K*K );

      ::fprintf( stdout, "CONN %s : %s; Qsz=%d\n", pUp, msg, Qsz );
      ::fflush( stdout );
   }

   virtual void OnStatus( yamrStatus sts )
   {
      const char *dir = ( sts ==  yamr_QloMark ) ? "LO" : "HI";

      ::fprintf( stdout, "STS %s : Qsz=%d\n", dir, GetTxQueueSize() );
      ::fflush( stdout );
   }

}; // class MyYAMR


////////////////////////
//
//     main()
//
////////////////////////
int main( int argc, char **argv )
{
   const char *uTy, *svc, *QoS, *xCol[2], *xVal[2];
   char       *svr, *usr, *cp, tkr[1024];
   MyYAMR      wr;
   int         SessID;
   bool        run;
   int         i, n;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", LogUsageID() );
      printf( "%s\n", wr.Version() );
      return 0;
   }

   // cmd-line args

   if ( argc < 4 ) {
      uTy = "<hosts> <SessID> <User> [<NumLog> <Xtra>]";
      printf( "Usage : %s %s; Exitting ...\n", argv[0], uTy );
      return 0;
   }
   svr     = argv[1];
   SessID  = atoi( argv[2] );
   usr     = argv[3];
   n       = ( argc > 4 ) ? atoi( argv[4] ) : 0;
   xCol[0] = ( argc > 6 ) ? argv[5] : (const char *)0;
   xVal[0] = ( argc > 6 ) ? argv[6] : (const char *)0;
   xCol[1] = (const char *)0;
   xVal[1] = (const char *)0;

   // Supported Protocols

   YAMR::bespoke::Usage  prm( wr );
   YAMR::Data::StringMap sm( wr );

   // Rock on

   uTy     = "OPEN";
   svc     = "ERT";
   QoS     = "real-time";
   ::fprintf( stdout, "%s\n", LogUsageID() );
   ::fprintf( stdout, "%s\n", wr.Version() );
   ::fprintf( stdout, "%s\n", wr.Start( svr, SessID, true ) );
   ::fprintf( stdout, "Enter shit to send; QUIT to terminate...\n" );
   for( run=true; run && ::fgets( tkr, 1024, stdin ); ) {
      if ( (cp=::strstr( tkr, "\n" )) )
         *cp = '\0';
      if ( !strlen(tkr ) )
         continue; // for-run
      run = ::strcmp( tkr, "QUIT" );
      sm.AddMapEntry( "meow", tkr );
      sm.AddMapEntry( "woof", tkr );
      sm.AddMapEntry( "Make America Giggle Again", tkr );
      sm.Send();
      if ( run ) {
         prm.LogGeneric( uTy, usr, svc, tkr, QoS, xCol, xVal );
         for ( i=1; i<n && run; prm.LogGeneric( prm.lgBin() ), i++ );
      }
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   wr.Stop();
   printf( "Done!!\n " );
   return 1;
}
