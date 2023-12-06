/******************************************************************************
*
*  LVCAddAndDumpCLI.cs
*     AddTicker() in main thread; Snap in backgroun
*
*  REVISION HISTORY:
*      4 DEC 2023 jcs  Created
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;


/////////////////////////////////////
//
//   c l a s s   M y T h r e a d
//
/////////////////////////////////////
public class MyThread
{
   private String _lvcFile;
   private bool   _bRun;
   private Thread _thr;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyThread( String lvcFile )
   {
      ThreadStart beg;

      beg      = new ThreadStart( this.Run );
      _lvcFile = lvcFile;
      _bRun    = true;
      _thr     = new Thread( beg );
   }

   ///////////////////////
   // Thread Shit
   ///////////////////////
   public void Start()
   {
      _thr.Start();
   }

   public void Stop()
   {
      _bRun = false;
      if ( _thr != null )
         _thr.Join();
      _thr = null;
   }

   private void Run()
   {
      int i;

      for ( i=0; _bRun; SnapAll(), i++ );
      Console.WriteLine( "{0} SnapAll()'s", i );
   }

   ///////////////////////
   // Helpers
   ///////////////////////
   private void SnapAll()
   {
      LVC        lvc = new LVC( _lvcFile );
      LVCDataAll all = lvc.ViewAll();
      String     ds  = all._dSnap.ToString( "F3" );

      Console.WriteLine( "SnapAll() : {0} tkrs in {1}s", all._nTkr, ds );
      lvc.FreeAll();
      lvc.Destroy();
   }

};  // class MyThread


/////////////////////////////////////
//
//   c l a s s   M y A d m i n
//
/////////////////////////////////////
public class MyAdmin : LVCAdmin
{
   ///////////////////////////////////
   // Constructor / Destructor
   ///////////////////////////////////
   public MyAdmin( String adm ) :
      base( adm )
   { ; }

   ///////////////////////////////////
   // LVCAdmin Asynchronous Callbacks
   ///////////////////////////////////
   override void OnAdminACK( bool bAdd, String svc, String tkr )
   {
      String ty = bAdd ? "ADD" : "DEL";

      Console.WriteLine( "ACK.{0} : ( {1},{2} )", ty, svc, tkr );
   }

   override void OnAdminNAK( bool bAdd, String svc, String tkr )
   {
      String ty = bAdd ? "ADD" : "DEL";

      Console.WriteLine( "NAK.{0} : ( {1},{2} )", ty, svc, tkr );
   }

}; // class MyAdmin



/////////////////////////////////////
//
//   c l a s s   L V C T e s t
//
/////////////////////////////////////
class LVCAddAndDumpCLI 
{
   static bool _IsTrue( String p )
   {
      return( ( p.ToUpper() == "YES" ) || ( p.ToUpper() == "TRUE" ) );
   }

   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      bool   aOK, run;
      String s, svr, svc, dbFile;
      int    i, argc;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      svr    = "localhost:8775";
      svc    = "bloomberg";
      dbFile = "./cache.lvc";
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -h  <LVC Admin host:port> ] \\ \n";
         s += "       [ -s  <Service> ] \\ \n";
         s += "       [ -db <LVC d/b filename> ] \\ \n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -h  : {0}\n", svr );
         Console.Write( "      -s  : {0}\n", svc );
         Console.Write( "      -db : {0}\n", dbFile );
         return 0;
      }

      /////////////////////
      // cmd-line args
      /////////////////////
      for ( i=0; i<argc; i++ ) {
         aOK = ( i+1 < argc );
         if ( !aOK )
            break; // for-i
         if ( args[i] == "-h" )
            svr = args[++i];
         else if ( args[i] == "-s" )
            svc = args[++i];
         else if ( args[i] == "-db" )
            dbFile = args[++i];
      }

      ////////////////
      // Run until ACK'ed
      ////////////////
      MyThread lvc;
      MyAdmin  adm;

      lvc = new MyThread( dbFile );
      adm = new MyAdmin( svr ); 
      Console.WriteLine( "Enter <Ticker> to ADD; QUIT to stop ..." );
      lvc.Start();
      for ( run=true; run; ) {
         s   = Console.ReadLine();
         run = ( s != "QUIT" );
         if ( run )
            adm.AddTicker( svc, s );
      }
      Console.WriteLine( "Shutting down ..." );
      lvc.Stop();
      Console.WriteLine( "Done!!" );
      return 0;
   } // main()

} // class LVCAddAndDumpCLI
