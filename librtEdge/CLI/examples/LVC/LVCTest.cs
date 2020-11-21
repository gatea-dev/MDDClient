/******************************************************************************
*
*  LVCTest.cs
*     librtEdge .NET interface test - Last Value Cache(LVC)
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*     31 JUL 2011 jcs  Build  6: GetSchema() / LVCSnap
*     17 JAN 2012 jcs  Build  9: SnapAll() / ViewAll() on 64-bit 
*     20 JAN 2012 jcs  Build 10: rtEdgeField
*     12 JUN 2013 jcs  Build 18: GetAsString()
*     10 FEB 2016 jcs  Build 32: Binary / Performance
*     12 JAN 2018 jcs  Build 39: main_MEM()
*     20 NOV 2020 jcs  Build 46: Beefed up : arg switches
*
*  (c) 1994-2020 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using librtEdge;

class LVCTest 
{
   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private static int IterateBinFields( LVCData ld )
   {
      rtEdgeField f;
//      DateTime    dtTm;;
      string      s;
      double      r64;
      float       r32;
      int         i32;
      long        i64;
      int         i, nf, fid;

      nf = ld._flds.Length;
      for ( i=0; i<nf; i++ ) {
         f   = ld._flds[i];
         fid = f.Fid();
         switch( f.Type() ) {
            case rtFldType.rtFld_string:  s   = f.GetAsString( false ); break;
            case rtFldType.rtFld_int:     i32 = f.GetAsInt32(); break;
            case rtFldType.rtFld_double:  r64 = f.GetAsDouble(); break;
            case rtFldType.rtFld_date:
            case rtFldType.rtFld_time:
            case rtFldType.rtFld_timeSec: i64  = f.GetAsInt64(); break;
//            case rtFldType.rtFld_timeSec: dtTm = f.GetAsDateTime(); break;
            case rtFldType.rtFld_float:   r32  = f.GetAsFloat(); break;
            case rtFldType.rtFld_int8:    i32  = f.GetAsInt8(); break;
            case rtFldType.rtFld_int16:   i32  = f.GetAsInt16(); break;
            case rtFldType.rtFld_int64:   i64  = f.GetAsInt64(); break;
         }
      }
      return nf;
   }

   private static int IterateMFFields( LVCData ld )
   {
      rtEdgeField f;
//      DateTime    dtTm;
      string      s;
      double      r64;
      float       r32;
      int         i32;
      long        i64;
      int         i, nf, fid;

      nf = ld._flds.Length;
      for ( i=0; i<nf; i++ ) {
         f   = ld._flds[i];
         fid = f.Fid();
         s   = f.GetAsString( false );
         switch( f.Type() ) {
            case rtFldType.rtFld_string: break;
            case rtFldType.rtFld_int:    Int32.TryParse( s, out i32 ); break;
            case rtFldType.rtFld_double: Double.TryParse( s, out r64 ); break;
            case rtFldType.rtFld_date:
            case rtFldType.rtFld_time:
            case rtFldType.rtFld_timeSec: Int64.TryParse( s, out i64 ); break;
//            case rtFldType.rtFld_timeSec: dtTm = f.GetAsDateTime(); break;
            case rtFldType.rtFld_float:   Single.TryParse( s, out r32 ); break;
            case rtFldType.rtFld_int8:    Int32.TryParse( s, out i32 ); break;
            case rtFldType.rtFld_int16:   Int32.TryParse( s, out i32 ); break;
            case rtFldType.rtFld_int64:   Int64.TryParse( s, out i64 ); break;
         }
      }
      return nf;
   }

   private static void Dump( LVCData ld )
   {
      int i;

      Console.WriteLine( "{0},{1}", ld._pSvc, ld._pTkr );
      for ( i=0; i<ld._flds.Length; ld._flds[i++].DumpToConsole() );
   }


   ////////////////////////////////
   // Main Functions
   ////////////////////////////////
   public static int Main_ADMIN(String[] args) 
   {
      try {
         LVCAdmin adm = new LVCAdmin( "localhost:8775" );

         Console.WriteLine( rtEdge.Version() );
         adm.AddTicker( "a", "b" );
         return 0;
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   }
 
   public static void Dump( LVC lvc, String svc, String[] tkrs )
   {
      LVCData    ld;
      LVCDataAll la;
      int        i, nf, argc;
      double     dUs, d0, dd;
      string     pLVC, pMs, ty, dmpTkr;

      // LVC - All Tickers; Dump BID if found ...

      la  = lvc.ViewAll();
      ty  = la.IsBinary ? "BIN" : "MF";
      dUs = la._dSnap * 1000.0;
      pMs = dUs.ToString( "F1" );
      Console.WriteLine( "{0}-SNAP {1} tkrs in {2}mS", ty, la._nTkr, pMs );
      d0 = LVC.TimeNs();
      nf = 0;
      for ( i=0; i<la._nTkr; i++ ) {
         ld  = la._tkrs[i];
         if ( la.IsBinary )
            nf += IterateBinFields( ld );
         else
            nf += IterateMFFields( ld );
         if ( ld._pTkr == dmpTkr )
            Dump( ld );
      }
      dd  = 1000.0 * ( LVC.TimeNs() - d0 );
      pMs = dd.ToString( "F1" );
      Console.WriteLine( "{0}-ITER {1} tkrs in {2}mS", ty, la._nTkr, pMs );
   }
 
   public static void Snap( LVC lvc, int nSnp )
   {
      LVCDataAll la;
      int        i, argc;
      uint       nt;
      double     d0, dd;
      string     pLVC, pd;

      /*
       * Test 1 : 1 LVC(); Multiple ViewAll()'s
       */
      Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
      Console.WriteLine( "1st test : Hit <ENTER> to start ..." );
      Console.ReadLine();
      d0 = rtEdge.TimeNs();
      nt = 0;
      for ( i=0; i<nSnp; i++ ) {
         la = lvc.ViewAll();
         nt = la._nTkr;
      }
      dd = rtEdge.TimeNs() - d0;
      pd = dd.ToString( "F1" );
      Console.WriteLine( "1st : {0} snaps; {1} tkrs in {2}s", i, nt, pd );
      /*
       * Test 2 : new LVC() each ViewAll()
       */
      Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
      Console.WriteLine( "2nd test : Hit <ENTER> to start ..." );
      Console.ReadLine();
      d0  = rtEdge.TimeNs();
      for ( i=0; i<nSnp; i++ ) {
         la  = lvc.ViewAll();
         nt  = la._nTkr;
      }
      dd = rtEdge.TimeNs() - d0;
      pd = dd.ToString( "F1" );
      Console.WriteLine( "2nd : {0} snaps; {1} tkrs in {2}s", i, nt, pd );
      Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      LVC    lvc;
      String db, ty, svc, tkr;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      db    = "./db/cache.lvc";
      ty    =  "DUMP";
      nSnp  = 0;
      svc   = null;
      tkr   = null;
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -db  <LVC d/b file> ] \\ \n";
         s += "       [ -ty  <Type : DUMP | SNAP | ... > ] \\ \n";
         s += "       [ -s   <Service> ] \\ \n";
         s += "       [ -t   <Ticker : CSV or Filename> ] \\ \n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -db      : {0}\n", db );
         Console.Write( "      -ty      : {0}\n", ty );
         Console.Write( "      -s       : <empty>\n" );
         Console.Write( "      -t       : <empty>\n" );
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
         if ( args[i] == "-db" )
            db = args[++i];
         else if ( args[i] == "-ty" )
            ty = args[++i];
         else if ( args[i] == "-s" )
            svc = args[++i];
         else if ( args[i] == "-t" ) {
            tkr = args[++i];
            tkrs = ReadLines( tkr );
            if ( tkrs == null )
               tkrs = tkr.Split(',');
         }
      }
      Console.WriteLine( rtEdge.Version() );
      lvc = new LVC( db );
      /*
       * By Type
       */
      try {
         if ( ( ty == "SNAP" ) && ( nSnp > 0 ) )
            Snap( lvc, nSnp );
         else
            Dump( lvc );
      } catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      Console.WriteLine( "Hit <ENTER> to terminate ..." );
      Console.ReadLine();
      lvc.Destroy();
      Console.WriteLine( "Done!!" );
   }

}
