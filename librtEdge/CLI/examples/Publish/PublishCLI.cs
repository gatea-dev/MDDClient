/******************************************************************************
*
*  PublishCLI.cs
*     librtEdge .NET interface test - Publication
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*     . . .
*     23 JAN 2015 jcs  Chains
*      5 MAY 2022 jcs  Build 53: New constructor; SetMDDirectMon()
*     22 OCT 2022 jcs  Build 58: -s service -t ticker
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;

///////////////////////////
//
//  P u b l i s h C L I
//
///////////////////////////
class PublishCLI : rtEdgePublisher
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private int                          _vecSz;
   private int                          _vPrec;
   private Dictionary<string, IntPtr>   _wl;
   private Dictionary<string, MyVector> _wlV;
   private Timer                        _tmr;
   private TimerCallback                _cbk;
   private int                          _rtl;
   private string[]                     _chn;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public PublishCLI( string svr, 
                      string svc, 
                      int    tTmr, 
                      int    vecSz,
                      int    vPrec ) :
      base( svr, svc, true, false ) // binary, bStart 
   {
      // Fields / Watchlist

      _vecSz = vecSz;
      _vPrec = vPrec;
      _wl    = new Dictionary<string, IntPtr>();
      _wlV   = new Dictionary<string, MyVector>();
      _rtl   = 0;
      _chn   = null;

      // Timer Callback

      _cbk = new TimerCallback( PublishAll );
      _tmr = new Timer( _cbk, this, tTmr, tTmr );
   }


   ////////////////////////////////
   // Timer Handler
   ////////////////////////////////
   public void PublishAll( object data )
   {
      MyVector vec;

      lock( _wl ) {
         foreach ( var kv in _wl ) {
            PubTkr( (string)kv.Key, (IntPtr)kv.Value, false );
         }
      }
      lock( _wlV ) {
         foreach ( var kv in _wlV ) {
            vec = (MyVector)kv.Value;
            vec.PubVector();
         }
      }
   }

   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void PubTkr( string tkr, IntPtr arg, bool bImg )
   {
      rtEdgePubUpdate u;
      double          r64;
      long            i64;
      int             fid;

      // RTL; Time (int); Time (double); DateTime, String

      u   = new rtEdgePubUpdate( this, tkr, arg, bImg );
      fid = 6;
      u.AddFieldAsInt32( fid++, _rtl );
      u.AddFieldAsInt32( fid++, (int)rtEdge.TimeSec() );
      u.AddFieldAsDouble( fid++, rtEdge.TimeNs() );
      u.AddFieldAsDateTime( fid++, DateTime.Now );
      i64        = 7723845300000;
      u.AddFieldAsInt64(  fid++, i64 );
      i64        = 4503595332403200;
      u.AddFieldAsInt64(  fid++, i64 );
      r64        = 123456789.987654321 /* + w._rtl */;
      u.AddFieldAsDouble(  fid++, r64 );
      r64        = 6120.987654321;
      u.AddFieldAsDouble(  fid++, r64 );
      r64        = 3.14159265358979323846;
      u.AddFieldAsDouble(  fid++, r64 );
      u.AddFieldAsUnixTime(  fid++, DateTime.Now );
      u.AddFieldAsString(  2147483647, "2147483647" );
      u.AddFieldAsString( -2147483647, "-2147483647" );
      u.AddFieldAsString( 16260000, "16260000" );
      u.Publish();
      _rtl += 1;
   }

   public void PubChainLink( int lnk, string chain, IntPtr arg )
   {
      rtEdgePubUpdate u;
      string[]        lnks;
      bool            bFinal;
      int             _NUM_LINK = 14;

      // Up to _NUM_LINK per link

      lnks   = GetSlice( lnk, _NUM_LINK );
      bFinal = ( _chn.Length <= ( (lnk+1)*_NUM_LINK ) );
      u = new rtEdgePubUpdate( this, chain, arg, true );
      u.PubChainLink( chain, arg, lnk, bFinal, lnks );
      Console.WriteLine( "CHAIN {0}", chain );
   }


   ////////////////////////////////
   // rtEdgePublisher Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "[{0}] CONN {1} : {2}", DateTimeMs(), pConn, pUp );
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      string[] kv = tkr.Split('#');
      bool     bChn;
      IntPtr   w;
      MyVector vec;
      int      lnk;

      Console.WriteLine( "[{0}] OPEN {1}", DateTimeMs(), tkr );
      bChn = ( _chn != null ) && ( kv.Length == 2 );
      lnk  = -1;
      if ( bChn ) {
         try {
            lnk  = Convert.ToInt32( kv[0], 10 );
            bChn = true;
         }
         catch( Exception e ) {
            Console.WriteLine( "Exception: " + e.Message );
         }
      }
      if ( _vecSz > 0 ) {
         lock( _wlV ) {
            if ( !_wlV.TryGetValue( tkr, out vec ) ) {
               vec = new MyVector( this, tkr, _vecSz, _vPrec, arg );
               _wlV.Add( tkr, vec );
            }
            vec.PubVector();
         }
      }
      else {
         lock( _wl ) {
            if ( bChn )
               PubChainLink( lnk, kv[1], arg );
            else {
               PubTkr( tkr, arg, true );
               if ( !_wl.TryGetValue( tkr, out w ) )
                  _wl.Add( tkr, arg );
            }
         }
      }
   }

   public override void OnClose( string tkr )
   {
      Console.WriteLine( "[{0}] CLOSE {1}", DateTimeMs(), tkr );
      lock( _wl ) {
         _wl.Remove( tkr );
      }
      lock( _wlV ) {
         _wlV.Remove( tkr );
      }
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private string[] GetSlice( int nLnk, int _NUM_LINK )
   {
      string[] rtn;
      int      i, off, sz, nc;

      // Can get _chn.Skip().Take() from System.Linq to work

      rtn = null;
      off = nLnk * _NUM_LINK;
      nc  = _chn.Length;
      sz  = Math.Min( _NUM_LINK, nc-off );
      if ( sz > 0 ) {
         rtn = new string[sz];
         for ( i=0; i<sz; rtn[i]=_chn[off+i], i++ );
      }
      return rtn;
   }

   static bool _IsTrue( string p )
   {
      return( ( p == "YES" ) || ( p == "true" ) );
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args )
   {
      try {
         PublishCLI pub;
         string     s, svr, svc;
         int        i, argc,  tPub, hbeat, vecSz, vPrec;
         double     tRun;
         bool       aOK, bPack;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         svr   = "localhost:9995";
         svc   = "my_publisher";
         hbeat = 15;
         tRun  = 60.0;
         tPub  = 1;
         vecSz = 0;
         vPrec = 2;
         bPack = true;
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -h       <Source : host:port> ] \\ \n";
            s += "       [ -s       <Service> ] \\ \n";
            s += "       [ -pub     <Publication Interval> ] \\ \n";
            s += "       [ -run     <App Run Time> ] \\ \n";
            s += "       [ -vector  <Non-zero for vector; 0 for Field List> ] \\ \n";
            s += "       [ -vecPrec <Vector Precision> ] \\ \n";
            s += "       [ -packed  <true for packed; false for UnPacked> ] \\ \n";
            s += "       [ -hbeat   <Heartbeat> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -h       : {0}\n", svr );
            Console.Write( "      -s       : {0}\n", svc );
            Console.Write( "      -pub     : {0}\n", tPub );
            Console.Write( "      -run     : {0}\n", tRun );
            Console.Write( "      -vector  : {0}\n", vecSz );
            Console.Write( "      -vecPrec : {0}\n", vPrec );
            Console.Write( "      -packed  : {0}\n", bPack );
            Console.Write( "      -hbeat   : {0}\n", hbeat );
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
            else if ( args[i] == "-s" )
               svc = args[++i];
            else if ( args[i] == "-pub" )
               tPub = Convert.ToInt32( args[++i] );
            else if ( args[i] == "-run" )
               tRun = Convert.ToDouble( args[++i] );
            else if ( args[i] == "-vector" )
               vecSz = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-vecPrec" )
               vPrec = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-packed" )
               bPack = _IsTrue( args[++i] );
            else if ( args[i] == "-hbeat" )
               hbeat = Convert.ToInt32( args[++i], 10 );
         }

         // Rock on

         Console.WriteLine( rtEdge.Version() );
         pub = new PublishCLI( svr, svc, tPub, vecSz, vPrec );
         pub.PubStart();
//         pub.SetMDDirectMon( mdd, "PublishCLI", "PublishCLI" );
         pub.SetUnPacked( !bPack );
         pub.SetHeartbeat( hbeat );
         Console.WriteLine( pub.pConn() );
         Console.WriteLine( pub.IsUnPacked() ? "UNPACKED" : "PACKED" );
//         Console.WriteLine( "Stats in " + mdd );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         pub.Stop();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;

   } // Main()

}  // class PublishCLI


////////////////////
//
//  M y V e c t o r
//
////////////////////
class MyVector : Vector
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private rtEdgePublisher _pub;
   private int             _Size;
   private IntPtr          _StreamID;
   private int             _RTL;

   ///////////////////
   // Constructor
   ///////////////////
   public MyVector( rtEdgePublisher pub,
                    string          tkr, 
                    int             vecSz, 
                    int             vecPrec, 
                    IntPtr          StreamID ) :
      base( pub.pPubName(), tkr, vecPrec )
   {
      _pub      = pub;
      _Size     = vecSz;
      _StreamID = StreamID;
      _RTL      = 1;
      for ( int i=0; i<vecSz; UpdateAt( i, Math.PI * i ), i++ );
   }


   ///////////////////
   // Operations
   ///////////////////
   public void PubVector()
   {
      int ix = ( _RTL % _Size );

      // Every 5th time

      _RTL += 1;
      Publish( _pub, (int)_StreamID, true );
      if ( ( _RTL % 5 ) == 0 )
         UpdateAt( ix, Math.E );
      else
         ShiftRight( 1, true );
   }

}; // class MyVector

