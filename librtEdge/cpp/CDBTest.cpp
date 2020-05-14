/******************************************************************************
*
*  CDBTest.cpp
*     librtEdge interface test - ChartDB
*
*  REVISION HISTORY:
*     12 OCT 2015 jcs  Created
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <stdio.h>


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   // cmd-line args

   if ( argc < 5 ) {
      printf( "Usage : %s <ChartDbFile> <Svc> <Tickers>, <FID>; ", argv[0] );
      printf( "Exitting ...\n" );
      return 0;
   }

   RTEDGE::ChartDB cdb( argv[1] );
   const char     *tkrs[K];
   char           *svc, *str, *rp;
   int             nt, fid;

   // Parse 'em up

   svc = argv[2];
   str = argv[3];
   fid = atoi( argv[4] );
   for ( nt=0; ; nt++ ) {
      tkrs[nt] = ::strtok_r( str, ",", &rp );
      if ( !tkrs[nt] )
         break; // for-n
      str = NULL;
   }

   // Dump

   if ( nt > 1 ) {
      RTEDGE::CDBTable &t = cdb.ViewTable( svc, tkrs, nt, fid );

      printf( t.Dump() );
   }
   else {
      RTEDGE::CDBData &d = cdb.View( svc, tkrs[0], fid );

      printf( d.Dump() );
   }
   printf( "Done!!\n " );
   return 1;
}
