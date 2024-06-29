/******************************************************************************
*
*  LVCPerfCLI.cs
*     C# LVC Performance Test
*
*  REVISION HISTORY:
*     27 JUN 2024 jcs  Created.
*
*  (c) 1994-2024, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using librtEdge;


/////////////////////////////////////
//
//    c l a s s   T e s t S t a t
//
/////////////////////////////////////
public class TestStat
{
   public String Descr    { get; set; }
   public uint   tLibSnap { get; set; }
   public uint   tSnap    { get; set; }
   public uint   tPull    { get; set; }
   public uint   tAll     { get; set; }
   public uint   NumTkr   { get; set; }
   public uint   NumFld   { get; set; }

   ////////////////
   // Constructor
   ////////////////
   public TestStat( TestCfg cfg )
   {
      this.Descr    = cfg.Descr();
      this.tLibSnap = 0;
      this.tSnap    = 0;
      this.tPull    = 0;
      this.tAll    = 0;
      this.NumTkr   = 0;
      this.NumFld   = 0;
   }

   ////////////////
   // Operations
   ////////////////
   public String Dump()
   {
      String rc;

      rc  = this.Descr;
      rc += this.tLibSnap.ToString();
      rc += ",";
      rc += this.tSnap.ToString();
      rc += ",";
      rc += this.tPull.ToString();
      rc += ",";
      rc += this.tAll.ToString();
      rc += ",";
      rc += this.NumTkr.ToString();
      rc += ",";
      rc += this.NumFld.ToString();
      rc += "\n";
      return rc;
   }

} // class TestStat


/////////////////////////////////////
//
//    c l a s s   T e s t C f g
//
/////////////////////////////////////
public class TestCfg
{
   public String[] svcs     { get; set; }
   public String   flds     { get; set; }
   public int[]    fids     { get; set; }
   public bool     bSvcFltr { get; set; }
   public bool     bFldFltr { get; set; }
   public bool     bFldType { get; set; }

   /////////////////
   // Constructor
   /////////////////
   public TestCfg()
   {
      svcs     = null;
      flds     = null;
      fids     = null;
      bSvcFltr = false;
      bFldFltr = false;
      bFldType = false;
   }

   /////////////////
   // Access
   /////////////////
   public String Descr()
   {
      String rc;

      // Services,Fields,Filter,FldType,

      /*
       * Services
       */
      rc = "";
      if ( this.svcs == null ) {
         rc           += "All";
         this.bSvcFltr = false;
      }
      else
         rc = String.Join( ";", this.svcs );
      rc += ",";
      /*
       * Fields
       */
      if ( this.flds == null ) {
         rc           += "All";
         this.bFldFltr = false;
      }
      else
         rc += this.flds.Replace( ",", ";" );
      rc += ",";
      /*
       * Filter
       */
      if ( this.bSvcFltr || this.bFldFltr ) {
         rc += this.bSvcFltr ? "SVC " : "";
         rc += this.bFldFltr ? "FLD" : "";
      }
      else
         rc += "None";
      rc += ",";
      /*
       * Field Type
       */
      rc += this.bFldType ? "Native," : "String,";
      return rc;
   }

   /////////////////
   // Field Value
   /////////////////
   public object GetValue( rtEdgeField f )
   {
      rtFldType ty;

      // Just Dump

      if ( !this.bFldType )
         return f.GetAsString( false );

      // By Type

      ty = f.TypeFromMsg();
      switch( ty ) {
         case rtFldType.rtFld_int:     return f.GetAsInt32();
         case rtFldType.rtFld_double:  return f.GetAsDouble();
         case rtFldType.rtFld_date:
         case rtFldType.rtFld_time:
         case rtFldType.rtFld_timeSec: return f.GetAsInt64();
//            case rtFldType.rtFld_timeSec: dtTm = f.GetAsDateTime();
         case rtFldType.rtFld_float:   return f.GetAsFloat();
         case rtFldType.rtFld_int8:    return f.GetAsInt8();
         case rtFldType.rtFld_int16:   return f.GetAsInt16();
         case rtFldType.rtFld_int64:   return f.GetAsInt64();
         case rtFldType.rtFld_string:
         default:                      break;
      }
      return f.GetAsString( false );
   }

} // class TestCfg


/////////////////////////////////////
//
//  c l a s s   L V C P e r f C L I
//
/////////////////////////////////////
class LVCPerfCLI 
{
   ////////////////////////////////
   // Main Functions
   ////////////////////////////////
   public static TestStat RunIt( LVC lvc, TestCfg cfg )
   {
      LVCDataAll              la;
      LVCData                 ld;
      rtEdgeField             fld;
      rtEdgeField[]           flds;
      TestStat                st;
      double                  d0, d1, d2;
      HashSet<String>         svcSet;
      Dictionary<int, object> fdb;
      String[]                svcFltr;
      String                  fldFltr;
      int                     i, j, nf;
      int[]                   fids;

      /*
       * 1) Set Filter
       */
      fids   = cfg.fids;
      svcSet = new HashSet<String>();
      if ( cfg.svcs != null )
         for ( i=0; i<cfg.svcs.Length; svcSet.Add( cfg.svcs[i] ), i++ );
      svcFltr = ( cfg.bSvcFltr && ( cfg.svcs != null ) ) ? cfg.svcs : null;
      fldFltr = ( cfg.bFldFltr && ( cfg.flds != null ) ) ? cfg.flds : null;
      lvc.SetFilter( fldFltr, svcFltr );
      /*
       * 2) SnapAll()
       */
      st          = new TestStat( cfg );
      d0          = rtEdge.TimeNs();
      la          = lvc.ViewAll();
      d1          = rtEdge.TimeNs();
      st.tSnap    = (uint)( 1000.0 * ( d1 - d0 ) );
      st.tLibSnap = (uint)( 1000.0 * la._dSnap );
      st.NumTkr   = la._nTkr;
      st.NumFld   = 0;
      for ( i=0; i<(int)la._nTkr; st.NumFld += la._tkrs[i++]._nFld );
      /*
       * 3) Walk / Dump
       */
      d1       = rtEdge.TimeNs();
      for ( i=0; i<la._nTkr; i++ ) {
         ld = la._tkrs[i];
         if ( ( svcFltr == null ) && !svcSet.Contains( ld._pSvc ) )
            continue;
         /*
          * All, else specifics
          */
         fdb = new Dictionary<int, object>();
         nf  = ( fids != null ) ? fids.Length : 0;
         if ( nf != 0 ) {
            flds = ld._flds;
            for ( j=0; j<flds.Length; j++ ) {
               if ( (fld=flds[j]) != null )
                  fdb.Add( fld.Fid(), cfg.GetValue( fld ) );
            }
         }
         else {
            for ( j=0; j<nf; j++ ) {
               if ( (fld=ld.GetField( cfg.fids[j] )) != null )
                  fdb.Add( fld.Fid(), cfg.GetValue( fld ) );
            }
         }
      }
      d2       = rtEdge.TimeNs();
      st.tPull = (uint)( 1000.0 * ( d2 - d1 ) );
      st.tAll  = (uint)( 1000.0 * ( d2 - d0 ) );
      return st;
   }

   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      String  svr, s;
      TestCfg cfg;
      bool    aOK;
      int     i, j, k, nf, argc;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      cfg  = new TestCfg();
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      svr = "./cache.lvc";
      if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
         s  = "Usage: %s \\ \n";
         s += "       [ -db <LVC d/b file> ] \\ \n";
         s += "       [ -s  <CSV Service List> ] \\ \n";
         s += "       [ -f  <CSV Field ID List> ] \\ \n";
         Console.WriteLine( s );
         Console.Write( "   Defaults:\n" );
         Console.Write( "      -db : {0}\n", svr );
         Console.Write( "      -s  : <empty>\n" );
         Console.Write( "      -f  : <empty>\n" );
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
         else if ( args[i] == "-s" )
            cfg.svcs = args[++i].Split('.');
         else if ( args[i] == "-f" )
            cfg.flds = args[++i];
      }
      /*
       * 1) Create LVC; Get Field List from Schema
       */
      LVC            lvc;
      rtEdgeSchema   sch;
      rtEdgeField    def;
      List<int>      tmp;
      List<TestStat> sdb;
      String[]       flds;

      tmp = new List<int>();
      sdb = new List<TestStat>();
      lvc = new LVC( svr );
      if ( (sch=lvc.GetSchema()) ==  null ) {
         Console.WriteLine( "Can not query schema from " + svr );
         return 0;
      }
      if ( cfg.flds != null ) {
         flds = cfg.flds.Split( ',' );
         for ( i=0; i<flds.Length; i++ ) {
            if ( (def=sch.GetDef( flds[i] )) != null )
               tmp.Add( def.Fid() );
         }
      }
      if ( (nf=tmp.Count) != 0 ) {
         cfg.fids = new int[nf];
         for ( i=0; i<nf; cfg.fids[i] = tmp[i], i++ );
      }
      /*
       * 2) Tests : Original, etc.
       */
      Console.Write( "Services,Fields,Filter,FldType," );
      Console.WriteLine( "tSnap-C,tSnap-C#,tPull,tAll,NumTkr,NumFld" );
      RunIt( lvc, cfg ); // 'Warp up' LVC datafile
      for ( i=0; i<2; i++ ) {
         cfg.bSvcFltr = ( i != 0 );
         if ( cfg.bSvcFltr && ( cfg.svcs == null ) )
            continue; // for-i
         for ( j=0; j<2; j++ ) {
            cfg.bFldFltr = ( j != 0 );
            if ( cfg.bFldFltr && ( cfg.flds == null ) )
               continue; // for-i
            for ( k=0; k<2; k++ ) {
               cfg.bFldType = ( k != 0 );
               sdb.Add( RunIt( lvc, cfg ) );
            }
         }
      }
      /*
       * 3) Clean-up
       */
      for ( i=0; i<sdb.Count; Console.Write( sdb[i].Dump() ), i++ );
      Console.WriteLine( "Shutting down ..." );
      lvc.Destroy();
      Console.WriteLine( "Done!!" );
      return 0;

   } // main()

} // class LVCPerfCLI
