/******************************************************************************
*
*  LVCAdmin.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created (from Subscribe.cpp)
*     . . .
*     14 JUN 2022 jcs  Build 55: LVCAdmin.cpp
*     24 AUG 2023 jcs  Build 64: Named Schemas
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *LVCAdminID()
{
   static std::string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)LVCAdmin Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

/////////////////////////////////////
//
//   c l a s s   M y A d m i n
//
/////////////////////////////////////
class MyAdmin : public LVCAdmin
{
public:
   size_t _nAck;
   bool   _bUP;

   ///////////////////////////////////
   // Constructor / Destructor
   ///////////////////////////////////
public:
   MyAdmin( const char *adm ) :
      LVCAdmin( adm ),
      _nAck( 0 ),
      _bUP( 0 )
   { ; }

   ///////////////////////////////////
   // RTEDGE::LVCAdmin Notifications
   ///////////////////////////////////
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *ty = bUP ? "UP" : "DOWN";

      _bUP = bUP;
      ::fprintf( stdout, "CONN.%s : %s\n", ty, msg );
      ::fflush( stdout );
   }

   virtual bool OnAdminACK( bool bAdd, const char *svc, const char *tkr )
   {
      const char *ty = bAdd ? "ADD" : "DEL";

      ::fprintf( stdout, "ACK.%s : ( %s,%s )\n", ty, svc, tkr );
      ::fflush( stdout );
      _nAck += 1;
      return true;
   }

   virtual bool OnAdminNAK( bool bAdd, const char *svc, const char *tkr )
   {
      const char *ty = bAdd ? "ADD" : "DEL";

      ::fprintf( stdout, "NAK.%s : ( %s,%s )\n", ty, svc, tkr );
      ::fflush( stdout );
      _nAck += 1;
      return true;
   }

}; // class MyAdmin


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   std::string s;
   char        sTkr[K], *cp, *rp;
   bool        aOK, bCfg;
   size_t      n, nt;
   FILE       *fp;
   const char *cmd, *svr, *svc, *tkr, *schema;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", LVCAdminID() );
      return 0;
   }
   cmd    = "ADD";
   svr    = "localhost:8775";
   svc    = "bloomberg"; 
   tkr    = "";
   schema = "";
   bCfg   = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -c  <ADD | DEL> ] \\ \n";
      s += "       [ -h  <LVC Admin host:port> ] \\ \n";
      s += "       [ -s  <Service> ] \\ \n";
      s += "       [ -t  <Ticker : CSV or Filename flat ASCII> ] \\ \n";
      s += "       [ -x  <Schema Name> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -c       : %s\n", cmd );
      printf( "      -h       : %s\n", svr );
      printf( "      -s       : %s\n", svc );
      printf( "      -t       : <empty>\n" );
      printf( "      -x       : <empty>\n" );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( int i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-c" ) )
         cmd = argv[++i];
      else if ( !::strcmp( argv[i], "-h" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-s" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-t" ) )
         tkr = argv[++i];
      else if ( !::strcmp( argv[i], "-x" ) )
         schema = argv[++i];
   }

   ////////////////
   // Tickers 
   ////////////////
   if ( (fp=::fopen( tkr, "r" )) ) {
      while( ::fgets( (cp=sTkr), K, fp ) ) {
         ::strtok( sTkr, "#" );
         cp += ( strlen( sTkr ) - 1 );
         for ( ; cp > sTkr; ) {
            if ( ( *cp == '\r' ) || 
                 ( *cp == '\n' ) ||
                 ( *cp == '\t' ) || 
                 ( *cp == ' ' ) )
            {
               cp--;
               continue; // for-i
            }
            break; // for-cp
         }
         cp[1] = '\0';
         if ( strlen( sTkr ) )
            tkrs.push_back( std::string( sTkr ) );
      }
      ::fclose( fp );
   }
   else {
      s   = tkr;
      tkr = ::strtok_r( (char *)s.data(), ",", &rp );
      for ( ; tkr && strlen( tkr ); tkr=::strtok_r( NULL, ",", &rp ) )
         tkrs.push_back( std::string( tkr ) );
   }
   if ( !(nt=tkrs.size()) ) {
      ::fprintf( stdout, "No ticker specified; Exitting ...\n" );
      ::fflush( stdout );
   }

   ////////////////
   // Run until ACK'ed
   ////////////////
   MyAdmin lvc( svr );
   bool    bAdd = ( ::strcmp( cmd, "DEL" ) != 0 );

   for ( n=0; n<nt; n++ ) {
      tkr = tkrs[n].data();
      if ( bAdd )
         lvc.AddTicker( svc, tkr, schema );
      else
         lvc.DelTicker( svc, tkr );
   }
   /*
    * Wait up to 5 secs for all ACK's
    */
   for ( n=0; ( lvc._nAck < nt ) && n<50; rtEdge::Sleep( 0.100 ), n++ );
   ::fprintf( stdout, "%ld of %ld ACK's received\n", lvc._nAck, nt );
   lvc.Stop();
   ::fprintf( stdout, "Done!!\n" );
   ::fflush( stdout );
   return 1;
}

