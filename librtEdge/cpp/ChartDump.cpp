/******************************************************************************
*
*  ChartDump.cpp
*     librtEdge interface test - ChartDB
*
*  REVISION HISTORY:
*     12 OCT 2015 jcs  Created
*     14 JAN 2024 jcs  Build 67: Renamed to ChartDump
*     24 JAN 2025 jcs  Build 75: swig
*
*  (c) 1994-2025, Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *ChartDumpID()
{
   static std::string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)ChartDump Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   bool        aOK, bCfg;
   int         i, fid;
   std::string s;
   const char *svr, *svc, *tkr;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", ChartDumpID() );
      return 0;
   }

   // cmd-line args

   svr  = "./chart.db";
   svc  = "*";
   tkr  = "*";
   fid  = 6;
   bCfg = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db <Chart d/b filename> ] \\ \n";
      s += "       [ -s  <Service> ] \\ \n";
      s += "       [ -t  <Ticker : CSV or * for all> ] \\ \n";
      s += "       [ -f  <Field ID> ] \\ \n";
      s += "       [ -r  Dump all Records ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -db : %s\n", svr );
      printf( "      -s  : %s\n", svc );
      printf( "      -t  : %s\n", tkr );
      printf( "      -f  : %d\n", fid );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-db" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-s" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-t" ) )
         tkr = argv[++i];
      else if ( !::strcmp( argv[i], "-f" ) )
         fid = atoi( argv[++i] );
   }

   ChartDB cdb( svr );
   const char *tkrs[64*K];
   char       *str, *rp;
   int         nt;

   s   = tkr;
   str = (char *)s.data();
   for ( nt=0; ; nt++ ) {
      tkrs[nt] = ::strtok_r( str, ",", &rp );
      if ( !tkrs[nt] )
         break; // for-n
      str = NULL;
   }

   // Dump

   if ( nt > 1 ) {
      CDBTable &t = cdb.ViewTable( svc, tkrs, nt, fid );

      printf( t.Dump() );
   }
   else {
      RTEDGE::CDBData &d = cdb.View( svc, tkrs[0], fid );

      printf( d.Dump() );
   }
   printf( "Done!!\n " );
   return 1;
}
