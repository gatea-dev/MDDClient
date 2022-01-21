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

   ////////////////////////////////
   // Main Functions
   ////////////////////////////////
 
   private const int FidLastUpdated = 5;
   private const int FidLastPrice = 6;
   private const int FidBid = 23;
   private const int FidAsk = 26;

   public static Dictionary<string, DataRecord> SnapData(string db, int nDtTm)
   {
       var startTime = DateTime.Now;
       var res = new Dictionary<string, DataRecord>();
       DateTime time1, fTm;
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
               dataRecord.LastUpdated = time1;
               for ( var i=0; i<nDtTm; i++ ) {
                  fTm = Convert.ToDateTime( tickerData.GetField(FidLastUpdated).GetAsDateTime() );
                  dataRecord.LastUpdated = fTm;
               }
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
       Console.Write($"Snapped {res.Count} tickers in {(time1 - startTime).TotalMilliseconds:0.0} ms, processed in {(DateTime.Now - time1).TotalMilliseconds:0.0} ms; ");
       Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
       return res;
   }

   public static void SnapAndCheck(string db, int nDtTm )
   {
       // snap LVC and check value updates

       Console.WriteLine("Snap & Check test : Hit <ENTER> to start ...");
       Console.ReadLine();

       var lastSnap = SnapData( db, nDtTm );
       var i = 0;
       for ( i=1; true; i++ ) {
            Console.WriteLine($"Test # {i} @ {DateTime.Now:yyyy-MM-dd HH:mm:ss.000}; {rtEdge.MemSize()}Kb");
            var newSnap = SnapData(db, nDtTm );
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
      int      i, argc, nDtTm;

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
      nDtTm = 1;
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -db  <LVC d/b file> ] \\ \n";
         s += "       [ -p   <Performance : Snap All> ] \\ \n";
         s += "       [ -s   <Service> ] \\ \n";
         s += "       [ -t   <Ticker : CSV or Filename or *> ] \\ \n";
         s += "       [ -f   <Fields : CSV or Filename or *> ] \\ \n";
         s += "       [ -dt  <Num GetFieldAsDateTime()> ]\n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -db      : {0}\n", db );
         Console.Write( "      -ty      : {0}\n", bPrf );
         Console.Write( "      -s       : <empty>\n" );
         Console.Write( "      -t       : <empty>\n" );
         Console.Write( "      -f       : <empty>\n" );
         Console.Write( "      -dt      : {0}\n", nDtTm );
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
         else if ( args[i] == "-dt" )
            Int32.TryParse( args[++i], out nDtTm );
      }
diff = true;
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
         SnapAndCheck(db, nDtTm );
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

