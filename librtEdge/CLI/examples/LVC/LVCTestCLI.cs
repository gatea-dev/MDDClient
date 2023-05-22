/******************************************************************************
*
*  LVCTestCLI.cs
*     librtEdge .NET interface test - Last Value Cache(LVC)
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*     . . .
*     20 NOV 2020 jcs  Build 46: Beefed up : arg switches
*      2 JUN 2022 jcs  Build 55: Single-field dump
*      9 MAR 2023 jcs  Build 62: MEM; -threads; No <ENTER>
*     18 MAY 2023 jcs  Build 63: -schema
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
//    c l a s s   F i e l d L
//
/////////////////////////////////////
public class FieldL
{
    public string    Name { get; set; }
    public int       Fid  { get; set; }
    public rtFldType Type { get; set; }
}

/////////////////////////////////////
//
//    c l a s s   T e s t C f g
//
/////////////////////////////////////
public class TestCfg
{
    public bool bSafe   { get; set; }
    public bool bShare  { get; set; }
    public bool bSchema { get; set; }
    public bool bQuery  { get; set; }
    public bool bLiam   { get; set; }
}


/////////////////////////////////////
//
// c l a s s   D a t a R e c o r d
//
/////////////////////////////////////
public class DataRecord
{
    public string Ticker { get; set; }
    public double Mid { get; set; }
    public DateTime LastUpdated { get; set; }

}

/////////////////////////////////////
//
//   c l a s s   M y T h r e a d
//
/////////////////////////////////////
class MyThread
{
   private string  _lvcFile;
   private int     _tid;
   private LVC     _lvc;
   private TestCfg _cfg;
   private int     _num;
   private long    _nTkr;
   private bool    _bRun;
   private Thread  _thr;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyThread( string lvcFile, int tid, LVC lvc, TestCfg cfg )
   {
      ThreadStart beg;

      beg      = new ThreadStart( this.Run );
      _lvcFile = lvcFile;
      _tid     = tid;
      _lvc     = lvc;
      _cfg     = cfg;
      _num     = 0;
      _nTkr    = 0;
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
      for ( _num=0; _bRun; SnapAll(), _num++ );
      Console.Write( "[{0}] : {1} SnapAll()'s", _tid, _num );
      if ( _tid == 0 )
         Console.Write( "; MEM = {0} (Kb)", rtEdge.MemSize() );
      Console.WriteLine( "" );
   }


   ///////////////////////
   // Helpers
   ///////////////////////
   private void SnapAll()
   {
      LVC          lvc = _cfg.bShare ? _lvc : new LVC( _lvcFile );
      LVCDataAll   la;
      List<FieldL> lst;
      rtEdgeSchema schema;
      rtEdgeField  fld;
      FieldL       fl;

      if ( _cfg.bSchema ) {
         schema = lvc.GetSchema( _cfg.bQuery );
         if ( _cfg.bLiam ) {
            lst = new List<FieldL>();
            for ( schema.reset(); schema.forth(); ) {
               fld     = schema.field();
               fl      = new FieldL();
               fl.Fid  = fld.Fid();
               fl.Name = fld.Name();
               fl.Type = fld.Type();
               lst.Add( fl );
            }
         }
      }
      if ( _cfg.bSafe ) {
         la = new LVCDataAll();
         lvc.ViewAll_safe( la );
      }
      else
         la = lvc.ViewAll();
      _nTkr += la._nTkr;
      la.Clear();
      if ( !_cfg.bShare )
         lvc.Destroy();
   }

};  // class MyThread


/////////////////////////////////////
//
//   c l a s s   L V C T e s t
//
/////////////////////////////////////
class LVCTestCLI 
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
   public static void MemTest1( LVC lvc, TestCfg cfg )
   {
      LVCDataAll la;

      Console.WriteLine( "MEM1 = {0} (Kb)", rtEdge.MemSize() );
      la = lvc.ViewAll();
      Console.WriteLine( "MEM2 = {0} (Kb); {1} tkrs", rtEdge.MemSize(), la._nTkr );
      la.Clear();
      Console.WriteLine( "MEM3 = {0} (Kb)", rtEdge.MemSize() );
   }

   public static void MemTestN( string lvcFile, int nThr, TestCfg cfg )
   {
      List<MyThread> tdb;
      LVC            lvc;
      int            i;

      lvc = cfg.bShare ? new LVC( lvcFile ) : null;
      Console.Write( "MEM1 = {0} (Kb)", rtEdge.MemSize() );
      Console.Write( "; Share={0}", cfg.bShare );
      Console.Write( "; Safe={0}", cfg.bSafe );
      Console.Write( "; Schema={0}", cfg.bSchema );
      Console.Write( "; SchemaQ={0}", cfg.bQuery );
      Console.WriteLine( "; SchemaL={0}", cfg.bLiam );
      tdb = new List<MyThread> ();
      for ( i=0; i<nThr; i++ )
         tdb.Add( new MyThread( lvcFile, i, lvc, cfg ) );
      for ( i=0; i<nThr; tdb[i++].Start() );
      Console.WriteLine( "Hit <ENTER> to terminate ..." );
      Console.ReadLine();
      for ( i=0; i<nThr; tdb[i++].Stop() );
      if ( cfg.bShare )
         lvc.Destroy();
      Console.WriteLine( "MEM2 = {0} (Kb)", rtEdge.MemSize() );
   }

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
      bAll = ( tkrs.Length == 1 ) && ( tkrs[0] == "*" );
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

   } // PerfTest()

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
       Console.Write($"Snapped {res.Count} tickers in {(time1 - startTime).TotalMilliseconds:0.0} ms, processed in {(DateTime.Now - time1).TotalMilliseconds:0.0} ms; ");
       Console.WriteLine( "{0} CLI objects", rtEdge.NumObj() );
       return res;

   } // SnapData()


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
/*
                    if (kv.Value.Mid == null && lastRecord.Mid != null)
                        nGoneBad++;
                    else if (Math.Abs((kv.Value.Mid ?? 0) - (lastRecord.Mid ?? 0)) > 1e-8)
                        nChangedValue++;
 */
                }
            }
            Console.WriteLine($"{nNewTicker} new tickers, {nGoneBad} gone bad, {nUpdated} updated, {nChangedValue} changed value.");

            lastSnap = newSnap;
            Thread.Sleep( tSleepMs );
       }

   } // SnapAndCheck()

   static bool _IsTrue( string p )
   {
      return( ( p.ToUpper() == "YES" ) || ( p.ToUpper() == "TRUE" ) );
   }


    ////////////////////////////////
    // main()
    ////////////////////////////////
    public static int Main( String[] args ) 
   {
      LVC      lvc;
      String   cmd, svr, svc, tkr, fld, s;
      String[] tkrs, flds;
      TestCfg  cfg;
      bool     aOK;
      int      i, argc, slpMs, nThr;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      cfg          = new TestCfg();
      svr          = "./cache.lvc";
      cmd          = "DUMP";
      svc          = "*";
      tkr          = "*";
      fld          = null;
      flds         = null;
      slpMs        = 1000;
      nThr         = 1;
      cfg.bSafe    = true;
      cfg.bShare   = false;
      cfg.bSchema  = false;
      cfg.bQuery   = false;
      cfg.bLiam    = false;
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -db      <LVC d/b file> ] \\ \n";
         s += "       [ -ty      <DUMP | PERF | DIFF | MEM> ] \\ \n";
         s += "       [ -p       <Performance : Snap All> ] \\ \n";
         s += "       [ -s       <Service> ] \\ \n";
         s += "       [ -t       <Ticker : CSV or Filename or *> ] \\ \n";
         s += "       [ -f       <Fields : CSV or Filename or *> ] \\ \n";
         s += "       [ -z       <tSleepMs> ] \\ \n";
         s += "       [ -threads <NumThreads; Implies MEM> ] \\ \n";
         s += "       [ -shared  <1 LVC if -threads> ] \\ \n";
         s += "       [ -safe    <ViewAll_safe() if -threads> ] \\ \n";
         s += "       [ -schema  <GetSchema reference before ViewAll> ] \\ \n";
         s += "       [ -schemaQ <GetSchema query before ViewAll> ] \\ \n";
         s += "       [ -schemaL <GetSchema query and List before ViewAll> ] \\ \n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -db      : {0}\n", svr );
         Console.Write( "      -ty      : {0}\n", cmd );
         Console.Write( "      -s       : {0}\n", svc );
         Console.Write( "      -t       : {0}\n", tkr );
         Console.Write( "      -f       : <empty>\n" );
         Console.Write( "      -z       : {0}\n", slpMs );
         Console.Write( "      -threads : {0}\n", nThr );
         Console.Write( "      -shared  : {0}\n", cfg.bShare );
         Console.Write( "      -safe    : {0}\n", cfg.bSafe );
         Console.Write( "      -schema  : {0}\n", cfg.bSchema );
         Console.Write( "      -schemaQ : {0}\n", cfg.bQuery );
         Console.Write( "      -schemaL : {0}\n", cfg.bLiam );
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
            svr = args[++i];
         else if ( args[i] == "-ty" )
            cmd = args[++i];
         else if ( args[i] == "-s" )
            svc = args[++i];
         else if ( args[i] == "-t" )
            tkr = args[++i];
         else if ( args[i] == "-f" )
            fld = args[++i];
         else if ( args[i] == "-z" )
            Int32.TryParse( args[++i], out slpMs );
         else if ( args[i] == "-threads" )
            Int32.TryParse( args[++i], out nThr );
         else if ( args[i] == "-shared" )
            cfg.bShare = _IsTrue( args[++i] );
         else if ( args[i] == "-safe" )
            cfg.bSafe = _IsTrue( args[++i] );
         else if ( args[i] == "-schema" )
            cfg.bSchema = _IsTrue( args[++i] );
         else if ( args[i] == "-schemaQ" ) {
            cfg.bQuery   = _IsTrue( args[++i] );
            cfg.bSchema |= cfg.bQuery;
         }
         else if ( args[i] == "-schemaL" ) {
            cfg.bLiam    = _IsTrue( args[++i] );
            cfg.bQuery  |= cfg.bLiam;
            cfg.bSchema |= cfg.bLiam;
         }
      }
      tkrs = ReadLines( tkr );
      if ( tkrs == null )
         tkrs = tkr.Split(',');
      if ( fld != null )
         flds = fld.Split(',');
      Console.WriteLine( rtEdge.Version() );
      lvc = null;
      /*
       * By Type
       */
      try {
         if ( cmd == "DIFF" ) 
            SnapAndCheck( svr, 0, slpMs );
         else if ( cmd == "PERF" )
            PerfTest( (lvc=new LVC( svr )), 2 );
         else if ( cmd == "MEM" ) {
            if ( nThr == 1 )
               MemTest1( (lvc=new LVC( svr )), cfg );
            else
               MemTestN( svr, nThr, cfg );
         }
         else
            Dump( (lvc=new LVC( svr )), svc, tkrs, flds);
      } catch (Exception e) {
         Console.WriteLine( e.ToString() );
         Console.WriteLine( e.StackTrace );
      } finally {
         Console.WriteLine( "Shutting down ..." );
         if ( lvc != null )
             lvc.Destroy();
      }
      Console.WriteLine( "Done!!" );
      return 0;

   } // main()

} // class LVCTestCLI
