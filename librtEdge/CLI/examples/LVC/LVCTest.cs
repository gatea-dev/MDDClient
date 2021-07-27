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
using System.Threading;
using librtEdge;

public class DataRecord
{
    public string Ticker { get; set; }
    public double? Mid { get; set; }
    public DateTime LastUpdated { get; set; }

}

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
      var startTime = DateTime.Now;
        nt = 0;
      for ( i=0; i<nSnp; i++ ) {
         la = lvc.ViewAll();
         nt = la._nTkr;
      }
      Console.WriteLine($"1st : {i} snaps; {nt} tkrs in {(DateTime.Now - startTime).TotalMilliseconds:0.0} ms");
        /*
         * Test 2 : new LVC() each ViewAll()
         */
        Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
      Console.WriteLine( "2nd test : Hit <ENTER> to start ..." );
      Console.ReadLine();
      startTime = DateTime.Now;
      for ( i=0; i<nSnp; i++ ) {
         la  = lvc.ViewAll();
         nt  = la._nTkr;
      }
      Console.WriteLine($"2nd : {i} snaps; {nt} tkrs in {(DateTime.Now - startTime).TotalMilliseconds:0.0} ms");
        Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
   }

   private const int FidLastUpdated = 5;
   private const int FidLastPrice = 6;
   private const int FidBid = 23;
   private const int FidAsk = 26;


   public static Dictionary<string, DataRecord> SnapData(string db)
   {
       var startTime = DateTime.Now;
       var res = new Dictionary<string, DataRecord>();
       DateTime time1;
       using (var lvc = new LVC(db))
       {
           var la = lvc.ViewAll();
           time1 = DateTime.Now;
           foreach (var tickerData in la._tkrs)
           {
               if (!res.TryGetValue(tickerData._pTkr, out var dataRecord))
               {
                   dataRecord = new DataRecord { Ticker = tickerData._pTkr };
                   res.Add(dataRecord.Ticker, dataRecord);
               }

               dataRecord.LastUpdated = Convert.ToDateTime(tickerData.GetField(FidLastUpdated).GetAsDateTime());
               var bidField = tickerData.GetField(FidBid);
               var askField = tickerData.GetField(FidAsk);
               if (bidField != null && askField != null)
                   dataRecord.Mid = (bidField.GetAsDouble() + askField.GetAsDouble()) / 2;
               else if (bidField != null)
                   dataRecord.Mid = bidField.GetAsDouble();
               else if (askField != null)
                   dataRecord.Mid = askField.GetAsDouble();
               else
               {
                   var lastPxField = tickerData.GetField(FidLastPrice);
                   if (lastPxField != null)
                       dataRecord.Mid = lastPxField.GetAsDouble();
               }
           }
       }
       Console.WriteLine($"Snapped {res.Count} tickers in {(time1 - startTime).TotalMilliseconds:0.0} ms, processed in {(DateTime.Now - time1).TotalMilliseconds:0.0} ms");
       return res;
   }

   public static void SnapAndCheck(string db, int nSnp, int tSleepMs )
   {
       // snap LVC and check value updates

       Console.WriteLine("Snap & Check test : Hit <ENTER> to start ...");
       Console.ReadLine();

       var lastSnap = SnapData(db);
       var i = 0;
       while (nSnp <= 0 || i < nSnp)
       {
            Console.WriteLine($"Test # {++i} @ {DateTime.Now:yyyy-MM-dd HH:mm:ss.000}");
            var newSnap = SnapData(db);
            var testTicker = "ESU1 Index";
            var record = newSnap.TryGetValue(testTicker, out var v) ? v : null;
            if (record != null)
                Console.WriteLine($"{testTicker}: Updated @ {record.LastUpdated:yyyy-MM-dd HH:mm:ss.000} Mid={record.Mid}");
            var nNewTicker = 0;
            var nGoneBad = 0;
            var nUpdated = 0;
            var nChangedValue = 0;
            foreach (var kv in newSnap)
            {
                if (!lastSnap.TryGetValue(kv.Key, out var lastRecord))
                    nNewTicker++;
                else
                {
                    if (kv.Value.LastUpdated > lastRecord.LastUpdated)
                        nUpdated++;

                    if (kv.Value.Mid == null && lastRecord.Mid != null)
                        nGoneBad++;
                    else if (Math.Abs((kv.Value.Mid ?? 0) - (lastRecord.Mid ?? 0)) > 1e-8)
                        nChangedValue++;
                }
            }
            Console.WriteLine($"{nNewTicker} new tickers, {nGoneBad} gone bad, {nUpdated} updated, {nChangedValue} changed value.");

            lastSnap = newSnap;
            Thread.Sleep( tSleepMs );
       }
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
      int      i, argc, slpMs;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      db    = "./db/cache.lvc";
      bPrf  = false;
      var diff = false;
      svc   = "ultrafeed";
      tkr   = "IBM";
      fld   = null;
      flds  = null;
      slpMs = 1000;
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -db  <LVC d/b file> ] \\ \n";
         s += "       [ -p   <Performance : Snap All> ] \\ \n";
         s += "       [ -s   <Service> ] \\ \n";
         s += "       [ -t   <Ticker : CSV or Filename or *> ] \\ \n";
         s += "       [ -f   <Fields : CSV or Filename or *> ] \\ \n";
         s += "       [ -z   <tSleepMs> ] \\ \n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -db      : {0}\n", db );
         Console.Write( "      -ty      : {0}\n", bPrf );
         Console.Write( "      -s       : <empty>\n" );
         Console.Write( "      -t       : <empty>\n" );
         Console.Write( "      -f       : <empty>\n" );
         Console.Write( "      -z       : {0}\n", slpMs );
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
         else if ( args[i] == "-diff" )
            diff = true;
         else if ( args[i] == "-p" )
            bPrf = true;
         else if ( args[i] == "-s" )
            svc = args[++i];
         else if ( args[i] == "-t" )
            tkr = args[++i];
         else if ( args[i] == "-f" )
            fld = args[++i];
         else if ( args[i] == "-z" )
            Int32.TryParse( args[++i], out slpMs );
      }
      tkrs = ReadLines( tkr );
      if ( tkrs == null )
         tkrs = tkr.Split(',');
      if ( fld != null )
         flds = fld.Split(',');
      Console.WriteLine( rtEdge.Version() );
      lvc = diff ? null : new LVC( db );
      /*
       * By Type
       */
      try
      {
          if (diff)
              SnapAndCheck(db, 0, slpMs);
          else if (bPrf)
              PerfTest(lvc, 2);
          else
              Dump(lvc, svc, tkrs, flds);
      }
      catch (Exception e)
      {
          Console.WriteLine(e.ToString());
          Console.WriteLine(e.StackTrace);
      }
      finally
      {
          lvc?.Destroy();
      }
      Console.WriteLine( "Hit <ENTER> to terminate ..." );
      Console.ReadLine();
      Console.WriteLine( "Done!!" );
      return 0;
   }
}
