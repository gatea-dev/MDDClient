/******************************************************************************
*
*  Snap.cs
*     librtEdge .NET interface test - Subscription Snapshot
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*      . . .
*     14 FEB 2020 jcs  Build 42: rtEdgeData copy constructor
*
*  (c) 1994-2020 Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;

class Snap : rtEdgeSubscriber 
{
   private static bool _bBinary  = true;
   bool         _bWait;
   rtEdgeData[] _res;
   int          _nRsp;
   int[]        _fids;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public Snap( string pSvr, string pUsr, bool bWait, int[] fids ) :
      base( pSvr, pUsr, _bBinary )
   {
      _bWait = bWait;
      _res   = null;
      _nRsp  = 0;
      _fids  = fids;
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void SnapTickers( string svc, string[] tkrs, int tWaitMs )
   {
      int i, nt;

      nt   = tkrs.Length;
      _res = new rtEdgeData[nt];
      for ( i=0; i<nt; _res[i++] = null );
      for ( i=0; i<nt; Subscribe( svc, tkrs[i], i ), i++ );
      Wait( tWaitMs );
   }

   public void Wait( int waitMs )
   {
      int    i;
      string s;

      lock( this ) {
         Monitor.Wait( this, waitMs );
      }
      s = "";
      if ( _fids != null )
         for ( i=0; i<_nRsp; s+=OnData_CSV( _res[i], i ), i++ );
      else
         for ( i=0; i<_nRsp; s+=OnData_DUMP( _res[i] ), i++ );
      Console.WriteLine( s );
   }

   private void Notify()
   {
      lock( this ) {
         Monitor.Pulse( this );
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
      if ( _nRsp < _res.Length ) {
         _res[_nRsp] = new rtEdgeData( d );
         _nRsp++;
      }
      Unsubscribe( d._pSvc, d._pTkr );
      if ( ( _nRsp >= _res.Length ) && !_bWait )
         Notify();
   }

   public override void OnDead( rtEdgeData d, string err )
   {
      string sig;

      sig  = "[" + d._pSvc + "," + d._pTkr + "]";
      Console.Write( pNow() );
      Console.WriteLine( "{0} {1} : {2}", d._ty, sig, err );
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private string OnData_SUMM( rtEdgeData d )
   {
      string s, sig;
      uint   nf;

      // Pre-condition

      if ( d._ty == rtEdgeType.edg_update ) {
         _nUpd += 1;
         return null;
      }

      // OK to rock

      s   = pNow();
      sig = "[" + d._pSvc + "," + d._pTkr + "] ";
      nf  = d._nFld;
      switch( d._ty ) 
      {
         case rtEdgeType.edg_image:
         case rtEdgeType.edg_update:
            s += d.MsgType() + " " + sig + nf.ToString() + " fields";
            break;
         case rtEdgeType.edg_recovering:
         case rtEdgeType.edg_stale:
            s += d.MsgType() + " " + sig;
            break;
         case rtEdgeType.edg_dead:
            s += d.MsgType() + " " + sig + " : " + d._pErr;
            break;
      }
      return s;
   }

   private string OnData_DUMP( rtEdgeData d )
   {
      string      s, pf, fmt;
      rtEdgeField f;
      int         i, fid;

      // Pre-condition

      if ( (s=OnData_SUMM( d )) == null )
         return "";

      // Fields ...

      s  += "\n";
      fmt = "   [{0,4}] {1,-12} : {2}\n";
      for ( i=0; i<d._nFld; i++ ) {
         f   = d._flds[i];
         fid = f.Fid();
         pf  = f.Name();
         s  += string.Format( fmt, fid, pf, f.Dump() );
      }
/*
 * Doesn't work w/ rtEdgeData copy constructor
 *
      s = d.Dump();
 */
      return s;
   }

   private string OnData_CSV( rtEdgeData d, int num )
   {
      rtEdgeSchema sch;
      string       s;
      rtEdgeField  f;
      int          i, nf;

      // Pre-condition

      if ( (nf=_fids.Length) == 0 )
         return "";
      if ( d._ty == rtEdgeType.edg_update ) {
         _nUpd += 1;
         return "";
      }

      // 1st time : Field Names

      sch = schema();
      s   = "";
      if ( num == 0 ) {
         s += "Time,Ticker,";
         for ( i=0; i<nf; i++ ) {
            if ( (f=sch.GetDef( _fids[i] )) != null )
               s += f.Name();
            else
               s += _fids[i].ToString();
            s += ',';
         }
         s += '\n';
      }

      // Fields ...

      s += string.Format( "{0},{1},", pNow().Trim( ' ' ), d._pTkr );
      for ( i=0; i<nf; i++ ) {
         if ( (f=d.GetField( _fids[i] )) != null )
            s += f.Dump();
         else
            s += '-';
         s += ',';
      }
      s  += "\n";
      return s;
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
         Snap     sub;
         int      i, j, nt, nf, argc, tRun, fid;
         bool     aOK, bWait;
         string[] tkrs, kv;
         int[]    fids;
         string   s, svr, usr, svc, tkr;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         svr   = "bluestone-ims.com:9998";
         usr   = "Snap";
         svc   = "ultrafeed";
         tkr   = "AAPL,IBM";
         tkrs  = tkr.Split(',');
         fids  = null;
         tRun  = 2500;
         bWait = false;
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -h  <Source : host:port or TapeFile> ] \\ \n";
            s += "       [ -u  <Username> ] \\ \n";
            s += "       [ -s  <Service> ] \\ \n";
            s += "       [ -t  <Ticker : CSV or Filename> ] \\ \n";
            s += "       [ -f  <CSV list of FIDs to dump> ] \\ \n";  
            s += "       [ -w  <Wait : true or false> ] \\ \n";  
            s += "       [ -r  <AppRunTime> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -h     : {0}\n", svr );
            Console.Write( "      -u     : {0}\n", usr );
            Console.Write( "      -s     : {0}\n", svc );
            Console.Write( "      -t     : <empty>\n" );
            Console.Write( "      -f     : <empty>\n" );
            Console.Write( "      -w     : false\n" );
            Console.Write( "      -r     : {0}\n", tRun );
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
            else if ( args[i] == "-u" )
               usr = args[++i];
            else if ( args[i] == "-s" )
               svc = args[++i];
            else if ( args[i] == "-t" ) {
               tkr  = args[++i];
               tkrs = ReadLines( tkr );
               if ( tkrs == null )
                  tkrs = tkr.Split(',');
            }
            else if ( args[i] == "-f" ) {
               List<int> tmp = new List<int>();

               tkr = args[++i];
               kv  = tkr.Split(',');
               for ( j=0; j<kv.Length; j++ ) {
                  if ( Int32.TryParse( kv[j], out fid ) )
                     tmp.Add( fid );
               }
               nf = (int)tmp.Count;
               if ( nf > 0 ) {
                  fids = new int[nf];
                  for ( j=0; j<nf; fids[j]=tmp[j],j++ );
               }
            }
            else if ( args[i] == "-r" )
               tRun = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-w" )
               bWait = ( args[++i] == "true" );
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new Snap( svr, usr, bWait, fids );
         Console.WriteLine( sub.Start() );
         nt = ( tkrs != null ) ? tkrs.Length : 0;
         if ( nt > 0 )
            sub.SnapTickers( svc, tkrs, tRun );
         else
            Console.WriteLine( "No tickers; Exitting ..." );
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

