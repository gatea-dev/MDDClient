/******************************************************************************
*
*  SnapOptions.cpp
*
*  REVISION HISTORY:
*     13 SEP 2023 jcs  Created (from LVCDump.cpp)
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <OptionsBase.cpp>

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *SnapOptionsID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)SnapOptions Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

////////////////////////////////////////////////
//
//     c l a s s   S n a p O p t i o n s
//
////////////////////////////////////////////////
class SnapOptions : public OptionsBase
{
   /////////////////////////
   // Constructor
   /////////////////////////
public:
   SnapOptions( const char *svr ) :
      OptionsBase( svr )
   { ; }

}; // class SnapOptions

//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids;
   string      s;
   char        sTkr[4*K], *cp, *rp, *exp;
   bool        aOK, bCfg, bHdr, bPut, bCall, bDmpExp;
   size_t      nt;
   int         i, fid, ix;
   double      rate;
   const char *svr, *und, *flds, *val;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", SnapOptionsID() );
      return 0;
   }

   // cmd-line args

   svr     = "./cache.lvc";
   und     = "AAPL";
   flds    = "3,6,22,25,32,66,67";
   rate    = 1.0;
   bHdr    = true;
   bPut    = true;
   bCall   = true;
   bDmpExp = false;
   exp     = NULL;
   bCfg    = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -u       <Underlyer> \\ \n";
      s += "       [ -r       <DumpRate> ] \\ \n";
      s += "       [ -f       <CSV Fids> ] \\ \n";
      s += "       [ -h       <Full Header> ] \\ \n";
      s += "       [ -put     <Dump PUT> ] \\ \n";
      s += "       [ -call    <Dump PUT> ] \\ \n";
      s += "       [ -exp     <Expiration Date> ] \\ \n";
      s += "       [ -dumpExp <true to dump ExpDate> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -db      : %s\n", svr );
      printf( "      -u       : %s\n", und );
      printf( "      -r       : %.2f\n", rate );
      printf( "      -f       : %s\n", flds );
      printf( "      -h       : %s\n", _pBool( bHdr ) );
      printf( "      -put     : %s\n", _pBool( bPut ) );
      printf( "      -call    : %s\n", _pBool( bCall ) );
      printf( "      -exp     : <empty>\n" );
      printf( "      -dumpExp : %s\n", _pBool( bDmpExp ) );
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
      else if ( !::strcmp( argv[i], "-u" ) )
         und = argv[++i];
      else if ( !::strcmp( argv[i], "-f" ) )
         flds = argv[++i];
      else if ( !::strcmp( argv[i], "-r" ) )
         rate = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-h" ) )
         bHdr = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-put" ) )
         bPut = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-call" ) )
         bCall = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-exp" ) )
         exp = argv[++i];
      else if ( !::strcmp( argv[i], "-dumpExp" ) )
         bDmpExp = _IsTrue( argv[++i] );
   }

   /////////////////////
   // Do it
   /////////////////////
   SnapOptions lvc( svr );
   Message    *msg;
   FieldDef   *fd;
   double      d0, age;
   Ints        puts, calls, both;
   Schema     &sch  = lvc.GetSchema();
   u_int64_t   tExp = exp ? lvc.ParseDate( exp, true ) : 0; 

   /*
    * CSV : Field Name or FID
    */
   s   = flds;
   val = ::strtok_r( (char *)s.data(), ",", &rp );
   for ( ; val && strlen( val ); val=::strtok_r( NULL, ",", &rp ) ) {
      if ( (fid=atoi( val )) )
         fids.push_back( fid );
      else if ( (fd=sch.GetDef( val )) )
         fids.push_back( fd->Fid() );
   }
   if ( !bDmpExp && (nt=fids.size()) ) {
      cp  = sTkr;
      val = bHdr ?  "Index,Time,Ticker,Active,Age,NumUpd," : "Ticker,";
      cp += sprintf( cp, val );
      for ( size_t i=0; i<nt; i++ ) {
         if ( (fd=sch.GetDef( fids[i] )) )
            cp += sprintf( cp, "%s,", fd->pName() );
         else
            cp += sprintf( cp, "%d,", fids[i] );
      }
      cp += sprintf( cp, "\n" );
      ::fprintf( stdout, sTkr );
      ::fflush( stdout );
   }
   /*
    * First kiss : Walk all; build sorted list of indices by ( Expire, Strike )
    */
   LVCAll &all  = lvc.ViewAll();

   d0    = lvc.TimeNs();
   bPut  |= bDmpExp;
   bCall |= bDmpExp;
   if ( bPut )
      puts = lvc.GetUnderlyer( all, und, true ); 
   if ( bCall )
      calls = lvc.GetUnderlyer( all, und, false ); 
   both  = puts;
   both.insert( both.end(), calls.begin(), calls.end() );
   age   = lvc.TimeNs() - d0;
   /*
    * Rock on
    */
   {
      Messages      &msgs = all.msgs();
      SortedInt64Set exps;
      u_int64_t      jExp, jNow;

      jNow = lvc.TimeSec() / 86400;
      for ( size_t i=0; i<both.size(); i++ ) {
         ix   = both[i];
         msg  = msgs[ix];
         if ( bDmpExp ) {
            exps.insert( lvc.Expiration( *msg, false ) );
            continue; // for-i
         }
         jExp = lvc.Expiration( *msg );
         if ( tExp && ( jExp != tExp ) )
            continue; // for-i
         s    = lvc.DumpOne( *msg, fids, bHdr ); 
         ::fprintf( stdout, "%d,", ix );
         ::fprintf( stdout, ">>> %ld <<<,", jExp-jNow );
         ::fwrite( s.data(), s.size(), 1, stdout );
         ::fflush( stdout );
      }
      SortedInt64Set::iterator et;

      for ( et=exps.begin(); et!=exps.end(); et++ )
         ::fprintf( stdout, "Expire : %ld\n", (*et) );
      ::fprintf( stdout, "ViewAll() in %.2fs\n", all.dSnap() );
      ::fprintf( stdout, "Build List in %.2fs\n", age );
   }
   ::fprintf( stdout, "Done!!\n " );
   ::fflush( stdout );
   return 1;
}
