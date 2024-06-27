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
using System.IO;
using System.Threading;
using librtEdge;


/////////////////////////////////////
//
//    c l a s s   T e s t S t a t
//
/////////////////////////////////////
public class TestStat
{
   public String Descr    { get; set; }
   public double tLibSnap { get; set; }
   public double tSnap    { get; set; }
   public double tPull    { get; set; }
   public int    NumTkr   { get; set; }
   public int    NumFld   { get; set; }

   ////////////////
   // Constructor
   ////////////////
   public TestStat( TestCfg cfg )
   {
      this.Descr    = cfg.Descr();
      this.tLibSnap = 0.0;
      this.tSnap    = 0.0;
      this.tPull    = 0.0;
      this.NumTkr   = 0.0;
      this.NumFld   = 0.0;
   }

   ////////////////
   // Operations
   ////////////////
   public String Dump()
   {
      String rc;

      rc  = this.Descr;
      rc += ",";
      rc += this.tLibSnap.ToString( "F3" );
      rc += ",";
      rc += this.tSnap.ToString( "F3" );
      rc += ",";
      rc += this.tPull.ToString( "F3" );
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

      // Services,Fields,FldType,

      /*
       * Services
       */
      rc = "";
      if ( this.svcs == null )
         rc = "All ";
      else
         rc = String.join( ";", this.svcs );
      rc += " (";
      if ( this.bSvcFltr == null )
         rc = "Un";
      rc += "Filtered),";
      /*
       * Fields
       */
      if ( this.flds == null )
         rc += "All ";
      else
         rc += this.flds.Replace( ",", ";" );
      rc += " (";
      if ( this.bFldFltr == null )
         rc = "Un";
      rc += "Filtered),";
      /*
       * Field Type
       */
      rc += this.bFldType ? "Native," : "String,";
      return rc;
   }

   /////////////////
   // Field Value
   /////////////////
   public var GetValue( rtEdgeField f )
   {
      var       rc;
      rtFldType ty;

      // Just Dump

      if ( !this.bFldType )
         return f.GetAsString();

      // By Type

      ty = f.TypeFromMsg();
      switch( ty ) {
         case rtFldType.rtFld_int:     rc = f.GetAsInt32(); break;
         case rtFldType.rtFld_double:  rc = f.GetAsDouble(); break;
         case rtFldType.rtFld_date:
         case rtFldType.rtFld_time:
         case rtFldType.rtFld_timeSec: rc  = f.GetAsInt64(); break;
//            case rtFldType.rtFld_timeSec: dtTm = f.GetAsDateTime(); break;
         case rtFldType.rtFld_float:   rc  = f.GetAsFloat(); break;
         case rtFldType.rtFld_int8:    rc  = f.GetAsInt8(); break;
         case rtFldType.rtFld_int16:   rc  = f.GetAsInt16(); break;
         case rtFldType.rtFld_int64:   rc  = f.GetAsInt64(); break;
         case rtFldType.rtFld_string:
         case default:                 rc = f.GetAsString( false ); break;
      }
      return rc;
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
      LVCDataAll      la;
      LVCData         ld;
      rtEdgeField     fld;
      rtEdgeField[]   flds;
      TestStat        st;
      double          d0,  la;
      HashSet<String> svcSet;
      String[]        svcFltr;
      String          fldFltr;
      int             i, j, nf;
      var             val;

      /*
       * 1) Set Filter
       */
      svcSet = new HashSet<String>();
      if ( cfg.svcs != null )
         for ( i=0; i<cfg.svcs.Count; svcSet.Add( cfg.svcs[i] ), i++ );
      svcFltr = ( cfg.bSvcFltr && ( cfg.svcs != null ) ) ? cfg.svcs : null;
      fldFltr = ( cfg.bFldFltr && ( cfg.flds != null ) ) ? cfg.flds : null;
      lvc.SetFilter( svcFltr, fldFltr );
      /*
       * 2) SnapAll()
       */
      st          = new TestStat( cfg );
      d0          = lvc.TimeNs();
      la          = lvc.ViewAll();
      st.tSnap    = ( lvc.TimeNs() - d0 );
      st.tLibSnap = la._dSnap;
      st.NumTkr   = la._nTkr;
      st.NumFld   = 0;
      for ( i=0; i<la._nTkr; st.NumFld += la._tkrs[i++]._nFld );
      /*
       * 3) Walk / Dump
       */
      d0       = lvc.TimeNs();
      for ( i=0; i<la._nTkr; i++ ) {
         ld = la._tkrs[i];
         if ( ( svcFltr == null ) && !svcSet.Contains( ld._pSvc ) )
            continue;
         /*
          * All, else specifics
          */
         if ( (nf=cfg.fids.Count) == 0 ) {
            flds = ld._flds;
            for ( j=0; j<flds.Count; cfg.GetValue( flds[j++] ) );
         }
         else {
            for ( j=0; j<nf; j++ ) {
               if ( (fld=ld.GetField( cfg.fids[j] )) != null )
                  cfg.GetValue( fld );
            }
         }
         
      }
      st.tPull = ( lvc.TimeNs() - d0 );
      return st;
   }

   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      String  svr;
      TestCfg cfg;
      bool    aOK;
      int     i, ns, nf, argc;

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
         flds = cfg.flds.Split( "," );
         for ( i=0; i<flds.Count; i++ ) {
            if ( (def=sch.GetDef( flds[i] )) != null )
               tmp.Add( def.Fid() );
         }
      }
      if ( (nf=tmp.Count) != 0 ) {
         cfg.fids = new int[nf];
         for ( i=0; i<nf; fids[i] = tmp[i], i++ );
      }
      /*
       * 2) Tests : Original, etc.
       */
      Console.WriteLine( "Shutting down ..." );
      sdb.Add( RunIt( lvc, cfg ) );
      cfg.bSvcFltr = true;
      sdb.Add( RunIt( lvc, cfg ) );
      cfg.bSvcFltr = false;
      cfg.bFldFltr = true;
      sdb.Add( RunIt( lvc, cfg ) );
      cfg.bSvcFltr = true;
      sdb.Add( RunIt( lvc, cfg ) );
      /*
       * 3) Clean-up
       */
      Console.Write( "Services,Fields,FldType," );
      Console.WriteLine( "tLibSnap,tSnap,tPull,NumTkr,NumFld" );
      for ( i=0; i<sdb.Count; Console.Write( sdb[i].Dump() ), i++ );
      Console.WriteLine( "Shutting down ..." );
      lvc.Destroy();
      Console.WriteLine( "Done!!" );
      return 0;

   } // main()

} // class LVCPerfCLI
