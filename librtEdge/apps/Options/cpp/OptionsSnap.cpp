/******************************************************************************
*
*  OptionsSnap.cpp
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
const char *OptionsSnapID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)OptionsSnap Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

////////////////////////////////////////////////
//
//      c l a s s   G r e e k S e r v e r
//
////////////////////////////////////////////////
class OptionsSnap : public OptionsBase
{
   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsSnap( const char *svr ) :
      OptionsBase( svr )
   { ; }

}; // class OptionsSnap

//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids;
   string      s;
   char        sTkr[4*K], *cp, *rp;
   bool        aOK, bCfg, bHdr, bPut, bCall;
   size_t      nt;
   int         i, fid, ix;
   double      rate;
   const char *svr, *und, *flds, *val;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", OptionsSnapID() );
      return 0;
   }

   // cmd-line args

   svr   = "./cache.lvc";
   und   = "AAPL";
   flds  = "3,6,22,25,32,66,67";
   rate  = 1.0;
   bHdr  = true;
   bPut  = true;
   bCall = true;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -u       <Underlyer> \\ \n";
      s += "       [ -r       <DumpRate> ] \\ \n";
      s += "       [ -f       <CSV Fids> ] \\ \n";
      s += "       [ -h       <Full Header> ] \\ \n";
      s += "       [ -put     <Dump PUT> ] \\ \n";
      s += "       [ -call     <Dump PUT> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -db      : %s\n", svr );
      printf( "      -u       : %s\n", und );
      printf( "      -r       : %.2f\n", rate );
      printf( "      -f       : %s\n", flds );
      printf( "      -h       : %s\n", _pBool( bHdr ) );
      printf( "      -put     : %s\n", _pBool( bPut ) );
      printf( "      -call    : %s\n", _pBool( bCall ) );
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
   }

   /////////////////////
   // Do it
   /////////////////////
   OptionsSnap lvc( svr );
   Message    *msg;
   FieldDef   *fd;
   double      d0, age;
   Ints        puts, calls, both;
   Schema     &sch = lvc.GetSchema();

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
   if ( (nt=fids.size()) ) {
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
   d0    = lvc.TimeNs();
   if ( bPut )
      puts = lvc.GetUnderlyer( und, true ); 
   if ( bCall )
      calls = lvc.GetUnderlyer( und, false ); 
   both  = puts;
   both.insert( both.end(), calls.begin(), calls.end() );
   age   = lvc.TimeNs() - d0;
   /*
    * Rock on
    */
   {
      LVCAll   &all  = lvc.ViewAll();
      Messages &msgs = all.msgs();

      for ( size_t i=0; i<both.size(); i++ ) {
         ix  = both[i];
         msg = msgs[ix];
         s   = lvc.DumpOne( *msg, fids, bHdr ); 
         ::fprintf( stdout, "%d,", ix );
         ::fprintf( stdout, ">>> %ld <<<,", lvc.DaysToExp( *msg ) );
         ::fwrite( s.data(), s.size(), 1, stdout );
         ::fflush( stdout );
      }
      ::fprintf( stdout, "ViewAll() in %.2fs\n", all.dSnap() );
      ::fprintf( stdout, "Build List in %.2fs\n", age );
   }
   ::fprintf( stdout, "Done!!\n " );
   ::fflush( stdout );
   return 1;
}
