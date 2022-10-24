/******************************************************************************
*
*  SubscribeCLI.cs
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
*     11 SEP 2020 jcs  Build 44: -tapeDir; -query
*      1 DEC 2020 jcs  Build 47: -ti, -s0, -sn
*     23 SEP 2022 jcs  Build 56: -csvF; No mo -tf; Always binary
*     22 OCT 2022 jcs  Build 58: MyVector
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;

/////////////////////////////////////
//
//   c l a s s   S u b s c r i b e C L I
//
/////////////////////////////////////
class SubscribeCLI : rtEdgeSubscriber 
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private int[] _csvFids;
   
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SubscribeCLI( string pSvr, string pUsr ) :
      base( pSvr, pUsr, true )
   {
      _csvFids = null;
   }


   ////////////////////////////////
   // Mutator
   ////////////////////////////////
   public void LoadCSVFids( string csvF )
   {
      string[]  fids = csvF.Split(',');
      List<int> fdb = new List<int>();
      int       fid, i, n;

      // 1) From CSV, to List<int> ...

      for ( i=n=0; i<fids.Length; i++ ) {
         try {
            fid = Convert.ToInt32( fids[i], 10 );
            fdb.Add( fid );
         } catch( Exception ) {
            Console.WriteLine( "Invalid CSV FID " + fids[i] );
         }
      }

      // 2) ... to int[] array

      if ( (n=fdb.Count) > 0 ) {
         _csvFids = new int[n];
         for ( i=0; i<n; _csvFids[i]=fdb[i], i++ );
      }
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
      if ( _csvFids != null )
         OnData_CSV( d );
      else
         OnData_DUMP( d );
   }

   public override void OnRecovering( rtEdgeData d )
   {
      OnDead( d, d._pErr );
   }

   public override void OnStreamDone( rtEdgeData d )
   {
      OnDead( d, d._pErr );
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


   public override void OnSchema( rtEdgeSchema db )
   {
      string      rc;
      int         i, fid;
      rtEdgeField def;

      // CSV Only

      if ( _csvFids == null )
         return;
      rc = "Date,Type,Service,Ticker,";
      for ( i=0; i<_csvFids.Length; i++ ) {
         fid = _csvFids[i];
         if ( (def=db.GetDef( fid )) != null )
            rc += def.Name();
         else
            rc += fid.ToString();
         rc += ",";
      }
      Console.WriteLine( rc );
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

   private void OnData_CSV( rtEdgeData d )
   {
      string      sig;
      int         fid;
      rtEdgeField fld;

      sig  = pTime( (DateTime)d._MsgTime ) + ",";
      sig += d.MsgType() + ",";
      sig += d._pSvc + ",";
      sig += d._pTkr + ",";
      sig += d._nFld.ToString() + ",";
      for ( uint i=0; i<_csvFids.Length; i++ ) {
         fid = _csvFids[i];
         if ( (fld=d.GetField( fid )) != null )
            sig += fld.GetAsString( false );
         else
            sig += "-";
         sig += ",";
      }
      Console.WriteLine( sig );
   }

   private void OnData_DUMP( rtEdgeData d )
   {
       Console.WriteLine( d.Dump() );
   }

   private string pNow()
   {
      return pTime( DateTime.Now );
   }

   private string pTime( DateTime dtTm )
   {
      string   rtn;

      // YYYY-MM-DD HH:MM:SS.mmm

      rtn  = "[";
      rtn += dtTm.Year.ToString("D4");
      rtn += "-";
      rtn += dtTm.Month.ToString("D2");
      rtn += "-";
      rtn += dtTm.Day.ToString("D2");
      rtn += "] ";
      rtn += dtTm.Hour.ToString("D2");
      rtn += ":";
      rtn += dtTm.Minute.ToString("D2");
      rtn += ":";
      rtn += dtTm.Second.ToString("D2");
      rtn += ".";
      rtn += dtTm.Millisecond.ToString("D3");
      rtn += " ";
      return rtn;
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args) 
   {
      try {
         SubscribeCLI sub;
         int          i, nt, argc, tRun, ti, sn;
         bool         aOK, bds, bVec, bTape, bQry;
         string[]     tkrs;
         MDDRecDef[]  dbTkrs;
         MyVector[]   vdb;
         string       s, svr, usr, svc, tkr, csvF;
         DateTime     t0, t1;
         long         s0;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         svr   = "localhost:9998";
         usr   = "SubscribeCLI";
         svc   = "bloomberg";
         tkr   = null;
         tkrs  = null;
         t0    = DateTime.MinValue;
         t1    = DateTime.MinValue;
         csvF  = null;
         ti    = 0;
         s0    = 0;
         sn    = 0;
         tRun  = 0;
         bds   = false;
         bVec  = false;
         bTape = true;
         bQry  = false;
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -h    <Source : host:port or TapeFile> ] \\ \n";
            s += "       [ -u    <Username> ] \\ \n";
            s += "       [ -s    <Service> ] \\ \n";
            s += "       [ -t    <Ticker : CSV or Filename> ] \\ \n";
            s += "       [ -csvF <fid1,fid2,...> \\ \n";
            s += "       [ -t0   <TapeSliceStartTime> ] \\ \n";
            s += "       [ -t1   <TapeSliceEndTime> ] \\ \n";
            s += "       [ -ti   <TapeSlice Sample Interval> ] \\ \n";
            s += "       [ -s0   <TapeSlice Start Offset> ] \\ \n";
            s += "       [ -sn   <TapeSlice NumMsg> ] \\ \n";
            s += "       [ -r    <AppRunTime> ] \\ \n";
            s += "       [ -vector <If included, bytestream> ] \\ \n";
            s += "       [ -bds  <true> ] \\ \n";
            s += "       [ -tapeDir <true to pump in tape (reverse) dir> ] \\ \n";
            s += "       [ -query <true to dump d/b directory> ]  \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -h       : {0}\n", svr );
            Console.Write( "      -u       : {0}\n", usr );
            Console.Write( "      -s       : {0}\n", svc );
            Console.Write( "      -t       : <empty>\n" );
            Console.Write( "      -t0      : <empty>\n" );
            Console.Write( "      -t1      : <empty>\n" );
            Console.Write( "      -csvF    : <empty>\n" );
            Console.Write( "      -ti      : ${0}\n", ti );
            Console.Write( "      -s0      : ${0}\n", s0 );
            Console.Write( "      -sn      : ${0}\n", sn );
            Console.Write( "      -r       : {0}\n", tRun );
            Console.Write( "      -vector  : {0}\n", bVec );
            Console.Write( "      -bds     : {0}\n", bds );
            Console.Write( "      -tapeDir : {0}\n", bTape );
            Console.Write( "      -query   : {0}\n", bQry );
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
               t0  = rtEdge.StringToTapeTime( args[++i] );
            else if ( args[i] == "-t1" )
               t1  = rtEdge.StringToTapeTime( args[++i] );
            else if ( args[i] == "-csvF" )
               csvF = args[++i];
            else if ( args[i] == "-ti" )
               ti = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-s0" )
               s0 = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-sn" )
               sn = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-r" )
               tRun = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-vector" )
               bVec = _IsTrue( args[++i] );
            else if ( args[i] == "-bds" )
               bds = _IsTrue( args[++i] );
            else if ( args[i] == "-tapeDir" )
               bTape = _IsTrue( args[++i] );
            else if ( args[i] == "-query" )
               bQry = _IsTrue( args[++i] );
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new SubscribeCLI( svr, usr );
         if ( csvF != null )
            sub.LoadCSVFids( csvF );
         sub.SetTapeDirection( bTape );
         Console.WriteLine( "BINARY" );
         Console.WriteLine( sub.Start() );
         /*
          * Tape Query??
          */
         if ( bQry ) {
            dbTkrs = sub.Query()._recs;
            Console.WriteLine( "Service,Ticker,NumMsg," );
            for ( i=0; i<dbTkrs.Length; i++ ) {
               Console.Write( dbTkrs[i]._pSvc + "," );
               Console.Write( dbTkrs[i]._pTkr + "," );
               Console.WriteLine( dbTkrs[i]._interval );
            }
            sub.FreeResult();
         }
         /*
          * Subscribe onward ...
          */
         nt = ( tkrs != null ) ? tkrs.Length : 0;
         if ( bds )
            for ( i=0; i<nt; sub.OpenBDS( svc, tkrs[i++], 0 ) );
         else if ( bVec ) {
            vdb = new MyVector[nt];
            for ( i=0; i<nt; vdb[i] = new MyVector( svc, tkrs[i] ), i++ );
            for ( i=0; i<nt; vdb[i].Subscribe( sub ), i++ );
         }
         else
            for ( i=0; i<nt; sub.Subscribe( svc, tkrs[i++], 0 ) );
         if ( sub.IsTape() ) {
            Console.WriteLine( "Pumping tape ..." );
            if ( ( t0 != DateTime.MinValue ) && ( t1 != DateTime.MinValue ) )
               sub.PumpTapeSlice( t0, t1 );
            else if ( ( sn != 0 ) )
               sub.PumpFullTape( s0, sn );
            else
               sub.PumpTape();
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

   static bool _IsTrue( string p )
   {
      return( ( p == "YES" ) || ( p == "true" ) );
   }

} // class SubscribeCLI


/////////////////////////////////////
//
//   c l a s s   M y V e c t o r
//
/////////////////////////////////////
class MyVector : Vector
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyVector( string svc, string tkr ) :
      base( svc, tkr, 0 )
   { ; }

   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
   public override void OnData( double[] img )
   {
      Console.WriteLine( Dump( true ) );
   }

   public override void OnData( VectorValue[] upd )
   {
      Console.WriteLine( Dump( upd, true ) );
   }

   public override void OnError( string err )
   {
      Console.WriteLine( "ERR ({0},{1}) : {2}", Service(), Ticker(), err );
   }

}; // class MyVector

