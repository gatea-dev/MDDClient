/******************************************************************************
*
*  PubCurve.cs
*     Publish SwapCurve from velocity
*
*     We publish 1 ticker specified by the user as a librEdge.Vector
*
*  REVISION HISTORY:
*     24 OCT 2022 jcs  Created (from Pipe.cs)
*     31 OCT 2022 jcs  -fp
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using librtEdge;
using QUANT;


////////////////////////////////////////
//
//       S w a p T i c k e r
//
////////////////////////////////////////
class SwapTicker
{
   public static int _fidVal    = 6;
   public static int _precision = 4;
   //////////////
   // Members
   //////////////
   public SwapPublisher _pub;
   public string        _Ticker;
   public double        _X;
   public double        _Y;
   public int           _StreamID;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SwapTicker( string tkrDef )
   {
      string[] kv = tkrDef.Split(':');
      int      nk = kv.Length;

      _pub    = null;
      _Ticker = kv[0];
      _X      = ( nk > 1 ) ? Convert.ToDouble( kv[1] ) : 0;
      _Y      = 0.0;
      _StreamID = 0;
   }

} // class SwapTicker


////////////////////////////////////////
//
//    S w a p S u b s c r i b e r
//
////////////////////////////////////////
class SwapSubscriber : rtEdgeSubscriber  
{
   //////////////
   // Members
   //////////////
   private string                      _svc;
   private Dictionary<int, SwapTicker> _wl;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SwapSubscriber( string svr, string usr, string svc ) :
      base( svr, usr)
   {
      _svc = svc;
      _wl  = new Dictionary<int, SwapTicker>();
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void OpenKnot( SwapPublisher pub, SwapTicker t )
   {
      // 1) Open Stream; Add to WatchList

      t._pub      = pub;
      t._StreamID = Subscribe( _svc, t._Ticker, 0 );
      _wl.Add( t._StreamID, t );
   }

   ////////////////////////////////
   // rtEdgeSubscriber Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "[{0}] SUB-CONN {1} : {2}", DateTimeMs(), pConn, pUp );
   }

   public override void OnService( string pSvc, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      if ( !pSvc.Equals( "__GLOBAL__" ) )
         Console.WriteLine( "[{0}] SUB-SVC  {1} {2}", DateTimeMs(), pSvc, pUp );
   }

   public override void OnData( rtEdgeData d )
   {
      rtEdgeField f;
      SwapTicker  t;
      double[]    X, Y;
      int         i, n, xn;

      // Pre-condition(s)

      if ( !_wl.TryGetValue( d._StreamID, out t ) )
         return;
      if ( (f=d.GetField( SwapTicker._fidVal )) == null )
         return;
      /*
       * 1) Set Value
       */
      t._Y = f.GetAsDouble();
      Console.WriteLine( "[{0}] {1} = {2}", DateTimeMs(), t._Ticker, f.Dump() );
      /*
       * 2) Calc and Publish Vector, always (LVC)
       *    - This is stupid brute force; Better performance awaits recoding ...
       */
      n  = _wl.Count;
      X  = new double[n];
      Y  = new double[n];
      i  = 0;
      xn = 1;
      foreach( var kv in _wl ) {
         t    = (SwapTicker)kv.Value;
         X[i] = t._X;
         Y[i] = t._Y;
         xn   = (int)Math.Max( t._X, xn );
         i++;
      }
      t._pub.CalcCurve( X, Y, xn );
   }

} // class SwapSubscriber


////////////////////////////////////////
//
//    S w a p P u b l i s h e r
//
////////////////////////////////////////
class SwapPublisher : rtEdgePublisher  
{
   //////////////
   // Members
   //////////////
   private string _tkr;
   private bool   _bPubFld;
   private Vector _vec;
   private int    _StreamID; // Non-zero means it is watched

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SwapPublisher( string svr, string svc, string tkr, bool bPubFld ) :
      base( svr, svc, true, false ) // binary, bStart
   {
      _tkr      = tkr;
      _bPubFld  = bPubFld;
      _vec      = new Vector( svc, tkr, SwapTicker._precision );
      _StreamID = 0;
   }

   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void CalcCurve( double[] X, double[] Y, int xn )
   {
      CubicSpline cs;
      double[]    Z;
      int         x;

      /*
       * Use Vector class as:
       *  1) !bPubFld : Container and Publisher
       *  2) bPubFld  : Container only
       */
      cs = new CubicSpline( X, Y );
      Z  = new double[xn];
      for ( x=0; x<xn; Z[x]=cs.Spline( x ), x++ );
      _vec.Update( Z );
      if ( _StreamID != 0 )
         PubCurve();
   }

   public void PubCurve()
   {
      rtEdgePubUpdate u;
      double[]        Z;

      /*
       * Use Vector class as:
       *  1) !bPubFld : Container and Publisher
       *  2) bPubFld  : Container only
       */
      if ( _bPubFld ) {
         Z = _vec.Get();
         u = new rtEdgePubUpdate( this, _tkr, (IntPtr)_StreamID, true );
         u.AddFieldAsVector( "UNCLE_V", Z );
         u.Publish();
      }
      else
         _vec.Publish( this, _StreamID, true );
   }


   ////////////////////////////////
   // rtEdgePublisher Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "[{0}] PUB-CONN {1} : {2}", DateTimeMs(), pConn, pUp );
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      rtEdgePubUpdate u;

      Console.WriteLine( "[{0}] OPEN  {1}", DateTimeMs(), tkr );
      if ( tkr == _vec.Ticker() ) {
         _StreamID = (int)arg;
         PubCurve();
      }
      else {
         u = new rtEdgePubUpdate( this, tkr, arg, false );
         u.PubError( "non-existent ticker" );
      }
   }

   public override void OnClose( string tkr )
   {
      Console.WriteLine( "[{0}] CLOSE {1}", DateTimeMs(), tkr );
      if ( tkr == _vec.Ticker() )
         _StreamID = 0;
   }

   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      try {
         SwapSubscriber   sub;
         SwapPublisher    pub;
         int              i, argc, nt;
         bool             aOK, bPubFld;
         string           s, pubSvr, pubSvc, pubTkr, subSvr, subUsr, subSvc;
         SwapTicker       tkr;
         List<SwapTicker> tkrs;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         subSvr  = "localhost:9998";
         subSvc  = "velocity";
         subUsr  = "SwapCurve";
         pubSvr  = "localhost:9995";
         pubSvc  = subUsr;
         pubTkr  = subUsr;
         bPubFld = false;
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -hp  <Pub : host:port> ] \\ \n";
            s += "       [ -sp  <Pub Service> ] \\ \n";
            s += "       [ -tp  <Pub Ticker> ] \\ \n";
            s += "       [ -fp  <Pub es rtFld_vector Field> ] \\ \n";
            s += "       [ -hs  <Sub : host:port> ] \\ \n";
            s += "       [ -ss  <Sub Service> ] \\ \n";
            s += "       [ -ts  <Sub Ticker:Length ] \\ \n";
            s += "       [ -u   <Sub Username> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -hp  : {0}\n", pubSvr );
            Console.Write( "      -sp  : {0}\n", pubSvc );
            Console.Write( "      -tp  : {0}\n", pubTkr );
            Console.Write( "      -fp  : {0}\n", bPubFld );
            Console.Write( "      -hs  : {0}\n", subSvr );
            Console.Write( "      -ss  : {0}\n", subSvc );
            Console.Write( "      -ts  : <empty>\n" );
            Console.Write( "      -u   : {0}\n", subUsr );
            return 0;
         }

         /////////////////////
         // cmd-line args
         /////////////////////
         tkrs = new List<SwapTicker>();
         for ( i=0; i<argc; i++ ) {
            aOK = ( i+1 < argc );
            if ( !aOK )
               break; // for-i
            if ( args[i] == "-hp" )
               pubSvr = args[++i];
            else if ( args[i] == "-sp" )
               pubSvc = args[++i];
            else if ( args[i] == "-tp" )
               pubTkr = args[++i];
            else if ( args[i] == "-fp" )
               bPubFld = _IsTrue( args[++i] );
            else if ( args[i] == "-hs" )
               subSvr = args[++i];
            else if ( args[i] == "-ss" )
               subSvc = args[++i];
            else if ( args[i] == "-ts" ) {
               tkr = new SwapTicker( args[++i] );
               if ( tkr._X != 0 )
                  tkrs.Add( tkr );
            }
            else if ( args[i] == "-u" )
               subUsr = args[++i];
         }
         if ( (nt=tkrs.Count) == 0 ) {
            Console.WriteLine( "No tickers specified; Exitting ..." );;
            return 0;
         }

         /////////////////////////////////////
         // Pub / Sub Channels
         /////////////////////////////////////
         Console.WriteLine( rtEdge.Version() );
         sub = new SwapSubscriber( subSvr, subUsr, subSvc );
         pub = new SwapPublisher( pubSvr, pubSvc, pubTkr, bPubFld );
         sub.Start();
         for ( i=0; i<nt; sub.OpenKnot( pub, tkrs[i++] ) );
         pub.PubStart();

         // Do it

         Console.WriteLine( "SUB : {0}", sub.pConn() );
         Console.WriteLine( "PUB : {0}", pub.pConn() );
         Console.WriteLine( "Publishing Curve as {0}", pubTkr );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         sub.Stop();
         sub = null;
         pub = null;
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 

   static bool _IsTrue( string p )
   {
      return( ( p == "YES" ) || ( p == "true" ) );
   }

} // SwapPublisher
