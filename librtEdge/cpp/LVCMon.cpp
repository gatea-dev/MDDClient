/******************************************************************************
*
*  LVCMon.cpp
*     LVC Stats Monitor
*
*  REVISION HISTORY:
*      9 SEP 2024 jcs  Created
*
*  (c) 1994-2024, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;

typedef std::vector<int> Ints;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *LVCMonID()
{
   static std::string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)LVCMon Build %s ", _MDD_LIB_BLD );
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
   const char *file, *upd;
   double      tSlp;
   std::string s;
   char        bp[4*K], *cp;
   int         i, num;
   bool        aOK, bCfg;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", LVCMonID() );
      return 0;
   }
   file = "./MDDirectMon.stats";
   tSlp = 1.0;
   num  = 15;
   upd  = "15,30,60,300,900";
   bCfg = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -f <LVC stats file> ] \\ \n";
      s += "       [ -r <Stat Sample Rate> ] \\ \n";
      s += "       [ -n <Num Snaps> ] \\ \n";
      s += "       [ -u <CSV UpdRates> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -f  : %s\n", file );
      printf( "      -r  : %.2f\n", tSlp );
      printf( "      -n  : %d\n", num );
      printf( "      -u  : %s\n", upd );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-f" ) )
         file = argv[++i];
      else if ( !::strcmp( argv[i], "-r" ) )
         tSlp = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-n" ) )
         num = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-u" ) )
         upd = argv[++i];
   }
   tSlp = WithinRange( 0.1, tSlp, 86400.0 );
   num = WithinRange( 1, num, 3600 );

   ////////////////
   // Rock on 
   ////////////////
   LVCStatMon      mon( file );
   Ints            idb;
   rtEdgeChanStats st;
   std::string     su( upd );
   size_t          ii;

   cp = ::strtok( (char *)su.data(), "," );
   for ( ; cp; cp=::strtok( NULL, "," ) )
      idb.push_back( atoi( cp ) );
   printf( "BUILD : %s\n", mon.BuildNum() );
   printf( "EXE   : %s\n", mon.ExeName() );
   cp  = bp;
   cp += sprintf( cp, "NumMsg,NumByte," );
   for ( ii=0; ii<idb.size(); ii++ )
      cp += sprintf( cp, "%02d:%02d,", idb[ii]/60, idb[ii]%60 );
   printf( "%s\n", bp );
   for ( i=0; i<num; i++ ) {
      mon.Snap( st );
      cp  = bp;
      cp += sprintf( cp, "%ld,%ld,", st._nMsg, st._nByte );
      for ( ii=0; ii<idb.size(); ii++ )
         cp += sprintf( cp, "%d,", mon.UpdPerSec( idb[ii] ) );
      printf( "%s\n", bp );
      mon.Sleep( tSlp );
   }
   ::fprintf( stdout, "Done!!\n" );
   ::fflush( stdout );
   return 1;
}

