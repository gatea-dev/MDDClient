/******************************************************************************
*
*  SubTest.cs
*     librtEdge .NET interface test - Subscription
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*      3 JUL 2011 jcs  Build  3: rtEdgeSubscriber
*     13 JUL 2011 jcs  Build  4: GetField()
*     20 JUL 2011 jcs  Build  5: HasField()
*     20 JAN 2012 jcs  Build 10: rtEdgeField
*     15 NOV 2012 jcs  Build 15: CPU; Allow tickers from file
*      9 MAY 2013 jcs  Build 17: Native Fields
*     11 JUN 2013 jcs  Build 18: Time in millis
*     23 DEC 2013 jcs  Build 22: BINARY; tApp
*      8 FEB 2017 jcs  Build 32: No mo bDotNet, etc.; Toggle Subscribe
*     17 JUL 2017 jcs  Build 34: Cleaned up
*     12 OCT 2017 jcs  Build 36: Tape
*     29 APR 2020 jcs  Build 43: bds
*
*  (c) 1994-2020 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Threading;
using librtEdge;

class SubTest : rtEdgeSubscriber 
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SubTest( string pSvr, 
                   string pUsr, 
                   bool   bBinary ) :
      base( pSvr, pUsr, bBinary )
   {
   }


   ////////////////////////////////
   // rtEdgeSubscriber Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "CONN {0} : {1}", pConn, pUp );
   }

   public override void OnService( string pSvc, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      if ( !pSvc.Equals( "__GLOBAL__" ) )
         Console.WriteLine( "SVC  {0} {1}", pSvc, pUp );
   }

   public override void OnData( rtEdgeData d )
   {
      OnData_DUMP( d );
   }

   public override void OnDead( rtEdgeData d, string err )
   {
      string sig;

      sig  = "[" + d._pSvc + "," + d._pTkr + "]";
      Console.Write( pNow() );
      Console.WriteLine( "{0} {1} : {2}", d._ty, sig, err );
   }

   public override void OnSymbol( rtEdgeData d, string sym )
   {
      Console.WriteLine( pNow() + " SYM-ADD : " + sym );
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private void OnData_SUMM( rtEdgeData d )
   {
      string sig;
      uint   nf;
      bool   bUpd;

      sig  = "[" + d._pSvc + "," + d._pTkr + "]";
      nf   = d._nFld;
      bUpd = ( d._ty == rtEdgeType.edg_update );
      if ( !bUpd )
         Console.Write( pNow() );
      switch( d._ty ) 
      {
         case rtEdgeType.edg_image:
         case rtEdgeType.edg_update:
            if ( !bUpd )
               Console.Write( "{0} {1} : {2} fields", d._ty, sig, nf );
            _nUpd += bUpd ? 1 : 0;
            break;
         case rtEdgeType.edg_recovering:
         case rtEdgeType.edg_stale:
            Console.Write( "{0} {1}", d._ty, sig );
            break;
         case rtEdgeType.edg_dead:
            Console.Write( "{0} {1} : {2}", d._ty, sig, d._pErr );
            break;
      }
      if ( !bUpd )
         Console.WriteLine( "" );
   }

   private void OnData_DUMP( rtEdgeData d )
   {
       Console.WriteLine( d.Dump() );
   }

   private string pNow()
   {
      DateTime now;
      string   rtn;

      // YYYY-MM-DD HH:MM:SS.mmm

      now  = DateTime.Now;
      rtn  = "[";
      rtn += now.Year.ToString("D4");
      rtn += "-";
      rtn += now.Month.ToString("D2");
      rtn += "-";
      rtn += now.Day.ToString("D2");
      rtn += "] ";
      rtn += now.Hour.ToString("D2");
      rtn += ":";
      rtn += now.Minute.ToString("D2");
      rtn += ":";
      rtn += now.Second.ToString("D2");
      rtn += ".";
      rtn += now.Millisecond.ToString("D3");
      rtn += " ";
      return rtn;
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args) 
   {
      try {
         SubTest  sub;
         int      i, nt, argc, tRun;
         bool     bMF, aOK, bds;
         string[] tkrs;
         string   s, svr, usr, svc, tkr, t0, t1;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         svr  = "localhost:9998";
         usr  = "SubTest";
         svc  = "bloomberg";
         tkr  = null;
         tkrs = null;
         t0   = null;
         t1   = null;
         tRun = 0;
         bMF  = false;
         bds  = false;
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -h   <Source : host:port or TapeFile> ] \\ \n";
            s += "       [ -u   <Username> ] \\ \n";
            s += "       [ -s   <Service> ] \\ \n";
            s += "       [ -t   <Ticker : CSV or Filename> ] \\ \n";
            s += "       [ -t0  <TapeSliceStartTime> ] \\ \n";
            s += "       [ -t1  <TapeSliceEndTime> ] \\ \n";
            s += "       [ -r   <AppRunTime> ] \\ \n";
            s += "       [ -bds <true> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -h     : {0}\n", svr );
            Console.Write( "      -u     : {0}\n", usr );
            Console.Write( "      -s     : {0}\n", svc );
            Console.Write( "      -t     : <empty>\n" );
            Console.Write( "      -t0    : <empty>\n" );
            Console.Write( "      -t1    : <empty>\n" );
            Console.Write( "      -r     : {0}\n", tRun );
            Console.Write( "      -bds   : {0}\n", bds );
            return 0;
         }

         /////////////////////
         // cmd-line args
         /////////////////////
         tRun  = 0;
         for ( i=0; i<argc; i++ ) {
            aOK = ( i+1 < argc );
            if ( !aOK )
               break; // for-i
            if ( args[i] == "-h" )
               svr = args[++i];
            else if ( args[i] == "-u" )
               usr = args[++i];
            else if ( args[i] == "-s" )
               svc = args[++i];
            else if ( args[i] == "-t" ) {
               tkr = args[++i];
               tkrs = ReadLines( tkr );
               if ( tkrs == null )
                  tkrs = tkr.Split(',');
            }
            else if ( args[i] == "-t0" )
               t0  = args[++i];
            else if ( args[i] == "-t1" )
               t1  = args[++i];
            else if ( args[i] == "-r" )
               tRun = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-bds" )
               bds = ( args[++i] == "true" );
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new SubTest( svr, usr, !bMF );
         if ( !bMF )
            Console.WriteLine( "BINARY" );
         Console.WriteLine( sub.Start() );
         nt = ( tkrs != null ) ? tkrs.Length : 0;
         if ( bds )
            for ( i=0; i<nt; sub.OpenBDS( svc, tkrs[i++], 0 ) );
         else
            for ( i=0; i<nt; sub.Subscribe( svc, tkrs[i++], 0 ) );
         if ( sub.IsTape() ) {
            Console.WriteLine( "Pumping tape ..." );
            if ( ( t0 != null ) && ( t1 != null ) )
               sub.StartTapeSlice( t0, t1 );
            else
               sub.StartTape();
         }
         if ( tRun > 0 ) {
            Console.WriteLine( "Running for {0} millisecond ...", tRun );
            Thread.Sleep( tRun );
         }
         else {
            Console.WriteLine( "Hit <ENTER> to terminate..." );
            Console.ReadLine();
         }
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 

   ////////////////////
   // Class-wide
   ////////////////////
   static public string[] ReadLines( string pf )
   {
      TextReader fp;
      string     s, ss;
      string[]   rtn;

      // Pre-condition

      rtn = null;
      if ( !File.Exists( pf ) )
         return rtn;

      // Open / Read

      ss = "";
      fp = File.OpenText( pf );
      while( (s=fp.ReadLine()) != null ) {
         ss += s; ss += "\n";
      }
      fp.Close();
      rtn = ss.Split('\n');
      return rtn;
   }
}
