/******************************************************************************
*
*  LVCDump.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created (from Subscribe.cpp)
*     . . .
*     14 JUN 2022 jcs  Build 55: LVCDump.cpp
*     12 DEC 2022 jcs  Build 61: Show Snap Time
*      8 MAR 2023 jcs  Build 62: MEM; -threads; No <ENTER>
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;

typedef std::vector<int> Ints;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *LVCDumpID()
{
   static std::string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)LVCDump Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

static void _DumpOne( Message *msg, Ints &fids )
{
   LVCData    &ld  = msg->dataLVC();
   const char *act = ld._bActive ? "ACTIVE" : "DEAD";
   const char *svc = msg->Service();
   const char *tkr = msg->Ticker();
   const char *pt;
   Field      *fld;
   std::string tm, sm;
   char        hdr[4*K], *cp;
   size_t      i, nf;

   /*
    * 1) Header
    */
   pt = msg->pDateTimeMs( tm, msg->MsgTime() );
   cp = hdr;
   if ( (nf=fids.size()) ) {
      cp += sprintf( cp, "%s,%s,%s,%s,", pt, svc, tkr, act );
      cp += sprintf( cp, "%.2f,%d,%d,", ld._dAge, ld._nUpd, ld._nFld );
   }
   else {
      cp += sprintf( cp, "[%s] (%s,%s) %s; ", pt, svc, tkr, act );
      cp += sprintf( cp, "%d flds; %d upds; ", ld._nFld, ld._nUpd );
      cp += sprintf( cp, "Age=%.2fs\n", ld._dAge );
   }
   sm = hdr;
   /*
    * 2) Fields
    */
   if ( nf ) {
      for ( i=0; i<nf; i++ ) {
         fld = msg->GetField( fids[i] );
         sm += fld ? fld->GetAsString() : "-";
         sm += ",";
      }
   }
   else {
      for ( msg->reset(); (*msg)(); ) {
         fld = msg->field();
         sm += fld->Dump( true );
         sm += "\n";
      }
   }
   /*
    * 3) Dump
    */
   sm += nf ? "\n" : "";
   ::fwrite( sm.data(), sm.size(), 1, stdout );
   ::fflush( stdout );
}


/////////////////////////////////////
//
//   c l a s s   M y T h r e a d
//
/////////////////////////////////////
class MyThread : public SubChannel
{
private:
   string    _lvcFile;
   u_int64_t _tid;
   u_int64_t _num;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyThread( const char *lvcFile ) :
      _lvcFile( lvcFile ),
      _tid( 0 ),
      _num( 0 )
   { ; }

   ~MyThread()
   {
      ::fprintf( stdout, "[0x%lx] : %ld SnapAll()'s\n", _tid, _num );
   }

   ////////////////////////////////
   // Operations
   ////////////////////////////////
public:
   void SnapAll( bool bLog )
   {
      LVC     lvc( _lvcFile.data() );
      LVCAll &all = lvc.ViewAll();
      int     nt  = all.Size();
      char    buf[K];

      _tid  = GetThreadID();
      _num += 1;
      if ( bLog ) {
         sprintf( buf, "[0x%lx] %d tkrs; MEM=%d (Kb)", _tid, nt, MemSize() );
         ::fprintf( stdout, buf );
         ::fflush( stdout );
      }
   }

   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
protected:
   virtual void OnWorkerThread()
   {
      SnapAll( false );
   }

}; // class MyThread

typedef vector<MyThread *> MyThreads;


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids;
   std::string s;
   char        sTkr[4*K], *cp, *rp;
   bool        aOK, bCfg, bAllS, bAllT;
   size_t      nt;
   int         i, fid, nThr;
   FILE       *fp;
   const char *cmd, *svr, *svc, *tkr, *flds;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", LVCDumpID() );
      return 0;
   }

   // cmd-line args

   cmd  = "DUMP";
   svr  = "./cache.lvc";
   svc  = "*";
   tkr  = "*";
   flds = "";
   nThr = 1;
   bCfg = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -ty      <DUMP | DICT | MEM> ] \\ \n";
      s += "       [ -h       <LVC d/b filename> ] \\ \n";
      s += "       [ -s       <Service> ] \\ \n";
      s += "       [ -t       <Ticker : CSV, filename or * for all> ] \\ \n";
      s += "       [ -f       <CSV Fids or empty for all> ] \\ \\n";
      s += "       [ -threads <NumThreads; Implies MEM> ] \\ \\n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -ty      : %s\n", cmd );
      printf( "      -h       : %s\n", svr );
      printf( "      -s       : %s\n", svc );
      printf( "      -t       : %s\n", tkr );
      printf( "      -f       : <empty>\n" );
      printf( "      -threads : %d\n", nThr );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-ty" ) )
         cmd = argv[++i];
      else if ( !::strcmp( argv[i], "-h" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-s" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-t" ) )
         tkr = argv[++i];
      else if ( !::strcmp( argv[i], "-f" ) )
         flds = argv[++i];
      else if ( !::strcmp( argv[i], "-threads" ) )
         nThr = atoi( argv[++i] );
   }
   nThr = WithinRange( 1, nThr, K );
   cmd  = ( nThr > 1 ) ? "MEM" : cmd;

   /////////////////////
   // Tickers
   /////////////////////
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
   bAllS = !::strcmp( svc, "*" );
   bAllT = !::strcmp( tkrs[0].data(), "*" );

   /////////////////////
   // Do it
   /////////////////////
   LVC       lvc( svr );
   Message  *msg;
   FieldDef *fd;
   Schema   &sch = lvc.GetSchema();
   MyThreads thrs;

   /*
    * CSV : Field Name or FID
    */
   s   = flds;
   tkr = ::strtok_r( (char *)s.data(), ",", &rp );
   for ( ; tkr && strlen( tkr ); tkr=::strtok_r( NULL, ",", &rp ) ) {
      if ( (fid=atoi( tkr )) )
         fids.push_back( fid );
      else if ( (fd=sch.GetDef( tkr )) )
         fids.push_back( fd->Fid() );
   }
   if ( (nt=fids.size()) ) {
      cp  = sTkr;
      cp += sprintf( cp, "Time,Service,Ticker,Active,Age,NumFld,NumUpd," );
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
    * 1) Dict only??
    * 2) Else Dump All
    * 3) Else Mem
    * 4) Else Pick 'em
    */
   if ( !::strcmp( cmd, "DICT" ) ) {
      ::fprintf( stdout, "Schema : %d fields\n", sch.Size() );
      for ( sch.reset(); (sch)(); )
         printf( "[%04d] %s\n", sch.field()->Fid(), sch.field()->Name() );
      ::fflush( stdout );
   }
   else if ( !::strcmp( cmd, "MEM" ) ) {
      if ( nThr > 1 ) {
         ::fprintf( stdout, "MEM1 = %d (Kb)\n", lvc.MemSize() );
         for ( i=0; i<nThr; thrs.push_back( new MyThread( svr ) ), i++ );
         for ( i=0; i<nThr; thrs[i]->StartThread(), i++ );
         ::fprintf( stdout, "Hit <ENTER> to terminate ..." ); getchar();
         for ( i=0; i<nThr; thrs[i]->StopThread(), i++ );
         ::fprintf( stdout, "MEM2 = %d (Kb)\n", lvc.MemSize() );
         for ( i=0; i<nThr; delete thrs[i], i++ );
         ::fprintf( stdout, "MEM3 = %d (Kb)\n", lvc.MemSize() );
         thrs.clear();
      }
      else {
         MyThread thr( svr );

         ::fprintf( stdout, "QUIT to terminate; <ENTER> to iterate ..." );
         while( ::fgets( sTkr, K, stdin ) ) {
            if ( ::strstr( sTkr, "QUIT" ) )
               break; // while-fgets
            thr.SnapAll( true );
         }
      }
   }
   else if ( bAllS || bAllT ) {
      LVCAll   &all = lvc.ViewAll();
      Messages &mdb = all.msgs();
      int       nb  = all.Size();
      size_t    nt  = mdb.size();
      double    tMs = 1000.0 * all.dSnap();

      printf( "[%d bytes] : %ld tickers in %.2fmS\n", nb, nt, tMs );
      if ( bAllS && bAllT )
         for ( size_t i=0; i<nt; _DumpOne( mdb[i], fids ), i++ );
      else if ( bAllS ) {
         for ( size_t i=0; i<nt; i++ ) {
            if ( !::strcmp( tkr, mdb[i]->Ticker() ) )
               _DumpOne( mdb[i], fids );
         }
      }
      else if ( bAllT ) {
         for ( size_t i=0; i<nt; i++ ) {
            if ( !::strcmp( svc, mdb[i]->Service() ) )
               _DumpOne( mdb[i], fids );
         }
      }
   }
   else { 
      for ( size_t i=0; i<tkrs.size(); i++ ) {
         tkr = tkrs[i].data();
         if ( (msg=lvc.View( svc, tkr )) )
            _DumpOne( msg, fids );
         else
            ::fprintf( stdout, "(%s,%s) NOT FOUND\n", svc, tkr );
      }
   }
   ::fprintf( stdout, "Done!!\n " );
   ::fflush( stdout );
   return 1;
}
