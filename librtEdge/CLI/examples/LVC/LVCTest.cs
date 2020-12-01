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
using System.Collections.Generic;
using System.IO;
using librtEdge;

class LVCTest 
{
   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   static private string[] ReadLines( string pf )
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

   private static int IterateBinFields( LVCData ld )
   {
      rtEdgeField f;
//      DateTime    dtTm;;
      String      s;
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

   private static void DumpTkr( LVCData ld, int[] fids )
   {
      rtEdgeField[] flds;
      rtEdgeField   fld;
      String        svc, tkr, val, csv;
      int           i, nf;

      svc  = ld._pSvc;
      tkr  = ld._pTkr;
      flds = ld._flds;
      nf   = flds.Length;
      if ( fids == null ) {
         Console.WriteLine( "{0},{1} {2} fields", svc, tkr, nf );
         for ( i=0; i<nf; flds[i++].DumpToConsole() );
      }
      else {
         csv = svc + ',' + tkr + ',' +  nf.ToString() + ',';
         for ( i=0; i<fids.Length; i++ ) {
            fld  = ld.GetField( fids[i] );
            val  = ( fld != null ) ? fld.GetAsString( false ) : "-";
            val += ",";
            csv += val;
         }
         Console.WriteLine( csv );
      }
   }


   ////////////////////////////////
   // Main Functions
   ////////////////////////////////
   public static void Dump( LVC      lvc, 
                            String   svc, 
                            String[] tkrs, 
                            String[] flds )
   {
      LVCDataAll   la;
      rtEdgeSchema sch;
      int          i, nf, nt, fid;
      double       d0, dUs;
      String       pFld, pMs, hdr;
      bool         bAll, bCSV;
      int[]        fids;

      /*
       * 1) CSV?  Dump Field Names
       */
      bAll = ( tkrs != null );
      bCSV = ( flds != null );
      sch  = lvc.schema();
      fids = null;
      hdr  = null;
      if ( bCSV ) {
         hdr = "Service,Ticker,NumFld,";
         nf  = flds.Length;
         fids = new int[nf];
         for ( i=0; i<nf; i++ ) {
            if ( !Int32.TryParse( flds[i], out fid ) )
               fid = sch.Fid( flds[i] );
            fids[i] = fid;
            pFld    = sch.Name( fid );
            if ( pFld.Length == 0 )
               pFld = fid.ToString();
            pFld += ",";
            hdr += pFld;
         }
      }
      /*
       * 2) All??
       */
      if ( bAll ) {
         la  = lvc.ViewAll();
         nt  = (int)la._nTkr;
         dUs = la._dSnap * 1000.0;
         pMs = dUs.ToString( "F1" );
         Console.WriteLine( "{0} tkrs snapped in {1}mS", nt, pMs );
         if ( hdr != null )
            Console.WriteLine( hdr );
         for ( i=0; i<nt; DumpTkr( la._tkrs[i], fids ), i++ );
      }
      else {
         List<LVCData> lld = new List<LVCData>();

         nt = tkrs.Length;
         d0 = LVC.TimeNs();
         for ( i=0; i<nt; lld.Add( lvc.View( svc, tkrs[i++] ) ) );
         dUs = ( LVC.TimeNs() - d0 ) * 1000.0;
         pMs = dUs.ToString( "F1" );
         Console.WriteLine( "{0} tkrs snapped in {1}mS", nt, pMs );
         if ( hdr != null )
            Console.WriteLine( hdr );
         for ( i=0; i<nt; DumpTkr( lld[i], fids ), i++ );
      }
   }
 
   public static void PerfTest( LVC lvc, int nSnp )
   {
      LVCDataAll la;
      int        i;
      uint       nt;
      double     d0, dd;
      String     pd;

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
      LVC      lvc;
      String   db, svc, tkr, fld, s;
      String[] tkrs, flds;
      bool     bPrf, aOK;
      int      i, argc;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      db   = "./db/cache.lvc";
      bPrf = false;
      svc  = "ultrafeed";
      tkr  = "IBM";
      fld  = null;
      flds = null;
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -db  <LVC d/b file> ] \\ \n";
         s += "       [ -p   <Performance : Snap All> ] \\ \n";
         s += "       [ -s   <Service> ] \\ \n";
         s += "       [ -t   <Ticker : CSV or Filename or *> ] \\ \n";
         s += "       [ -f   <Fields : CSV or Filename or *> ] \\ \n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -db      : {0}\n", db );
         Console.Write( "      -ty      : {0}\n", bPrf );
         Console.Write( "      -s       : <empty>\n" );
         Console.Write( "      -t       : <empty>\n" );
         Console.Write( "      -f       : <empty>\n" );
         return 0;
      }

      /////////////////////
      // cmd-line args
      /////////////////////
      for ( i=0; i<argc; i++ ) {
         aOK = ( i+1 < argc );
         if ( !aOK )
            break; // for-i
         if ( args[i] == "-db" )
            db = args[++i];
         else if ( args[i] == "-p" )
            bPrf = true;
         else if ( args[i] == "-s" )
            svc = args[++i];
         else if ( args[i] == "-t" )
            tkr = args[++i];
         else if ( args[i] == "-f" )
            fld = args[++i];
      }
      tkrs = ReadLines( tkr );
      if ( tkrs == null )
         tkrs = tkr.Split(',');
      if ( fld != null )
         flds = fld.Split(',');
      Console.WriteLine( rtEdge.Version() );
      lvc = new LVC( db );
      /*
       * By Type
       */
      try {
         if ( bPrf )
            PerfTest( lvc, 2 );
         else
            Dump( lvc, svc, tkrs, flds );
      } catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      Console.WriteLine( "Hit <ENTER> to terminate ..." );
      Console.ReadLine();
      lvc.Destroy();
      Console.WriteLine( "Done!!" );
      return 0;
   }
}
