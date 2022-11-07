/******************************************************************************
*
*  PubSpline.cs
*     Publish SwapSpline from velocity
*
*     We publish 1 ticker specified by the user as a librEdge.Vector
*
*  REVISION HISTORY:
*     24 OCT 2022 jcs  Created (from Pipe.cs)
*     31 OCT 2022 jcs  -fp
*      2 NOV 2022 jcs  -xi
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
//            D T D
//
////////////////////////////////////////
class DTD
{
   ////////////////////////
   // Hard-coded
   ////////////////////////
   public static double _min_dInc = 0.001;
   public static int    _fid_X    = -8001; // UNCLE_VX
   public static int    _fid_Y    = -8002; // UNCLE_VY
   public static int    _fidVal   = 6;

   ////////////////////////
   // Elements
   ////////////////////////
   public static string _elem_sub    = "Subscriber";
   public static string _elem_pub    = "Publisher";
   public static string _elem_knot   = "Knot";
   public static string _elem_spline = "Spline";

   ////////////////////////
   // Attributes
   ////////////////////////
   public static string _attr_svc    = "Service";
   public static string _attr_usr    = "Username";
   public static string _attr_svr    = "Server";
   public static string _attr_tkr    = "Ticker";
   public static string _attr_intvl  = "Interval";
   public static string _attr_name   = "Name";
   public static string _attr_inc    = "Increment";
   public static string _attr_curve  = "Curve";

}; // DTD


////////////////////////////////////////
//
//         K n o t W a t c h
//
////////////////////////////////////////
class KnotWatch
{
   public Knot   _knot;
   public Curve  _curve;
   public double _X;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public KnotWatch( Knot knot, Curve curve, double X )
   {
      _knot  = knot;
      _curve = curve;
      _X     = X;
   }

   ////////////////////////////////
   // Access
   ////////////////////////////////
   public double X() { return _X; }
   public double Y() { return _knot.Y();; }

}; // class KnotWatch


////////////////////////////////////////
//
//            K n o t
//
////////////////////////////////////////
class Knot
{
   public static int _precision = 4;
   //////////////
   // Members
   //////////////
   private List<KnotWatch> _wl;
   private string          _Ticker;
   private double          _Y;
   private int             _StreamID;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public Knot( string tkr )
   {
      _wl       = new List<KnotWatch>();
      _Ticker   = tkr;
      _Y        = 0.0;
      _StreamID = 0;
   }

   ////////////////////////////////
   // Access / Mutator
   ////////////////////////////////
   public double Y()        { return _Y; }
   public int    StreamID{} { return _StreamID; }
   
   public int Subscribe( SwapSubscriber sub, string svc )
   {
      _StreamID = sub.Subscribe( svc, _Ticker, 0 );
      return StreamID();
   }

   public KnotWatch AddWatch( Curve c, double intvl )
   {
      KnotWatch w;

      _curves.Add( (w=new KnotWatch( this, c, intvl )) );
      return w;
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void OnData( SwapSubscriber sub, rtEdgeField f )
   {
      int    i;
      string tm;

      // Pre-condition(s)

      if ( f == null )
         return;

      // Set Value / On-pass to Curve(s)

      _Y = f.GetAsDouble();
      tm = sub.DateTimeMs();
      nw = _wl.Count;
      Console.WriteLine( "[{0}] {1} = {2}", tm, _Ticker, f.Dump() );
      for ( i=0; i<nw; _wl[i]._curve.OnData( this, _wl[i]._X ), i++ );
   }

} // class Knot


////////////////////////////////////////
//
//            C u r v e
//
////////////////////////////////////////
class Curve
{
   //////////////
   // Members
   //////////////
   private SwapSubscriber  _sub;
   private string          _name;
   private double          _Xmax;
   private double[]        _X;
   private double[]        _Y;
   private List<KnotWatch> _kdb;
   private List<Spline>    _splines;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public Curve( SwapSubscriber sub, XmlElem xe )
   {
      XmlElem[] edb = xe.elements();
      double    X;
      string    tkr;
      int       i, nk;

      /*
       * <Curve Name="Swaps">
       *    <Knot Ticker="RATES.SWAP.USD.PAR.3M"  Interval="3" />
       *    <Knot Ticker="RATES.SWAP.USD.PAR.6M"  Interval="6" />
       * </Curve Name="Swaps">
       */
      _sub     = sub;
      _name    = getAttrValue( DTD._attr_name, null );
      _kdb     = new List<KnotWatch>();
      _splines = new List<Spline>();
      _Xmax    = 1.0;
      _X       = null;
      _Y       = null;
      if ( _name == null )
         return;
      for ( i=0; i<edb.Length; i++ ) {
         if ( (tkr=edb[i].getAttrValue( DTD._attr_tkr, null )) == null )
            continue; // for-i
         if ( (X=edb[i].getAttrValue( DTD._attr_intvl, 0.0 )) == 0.0 )
            continue; // for-i
         _kdb.Add( sub.AddWatch( this, tkr, X ) );
         _Xmax = Math.max( X, _Xmax );
      }
      if ( (nw=_kdb.Count) == 0 )
         return;
      _X = new double[nw];
      _Y = new double[nw];
      for ( i=0; i<nw; _X[i] = _kdb[i].X(), i++ );
      for ( i=0; i<nw; _Y[i] = _kdb[i].Y(), i++ );
      for ( i=0; i<nw; _Xmax = Math.Max( _Xmax, _kdb[i].X() ), i++ );
   }

   ////////////////////////////////
   // Access
   ////////////////////////////////
   public bool     IsValid() { return _Y != null; }
   public string   Name()    { return _name; }
   public double[] X()       { return _X; }
   public double[] Y()       { return _Y; }


   ////////////////////////////////
   // Mutator / Operations
   ////////////////////////////////
   public void AddSpline( Spline s )
   {
      _splines.Add( s );
   }

   public void OnData( Knot k, double X )
   {
      int i, nw, ns;

      // 1) Reset Y Values

      nw = _kdb.Count;
      for ( i=0; i<nw; _Y[i] = _kdb[i].Y(), i++ );

      // 2) On-pass to Spines

      ns = _slines.Count;
      for ( i=0; i<nsCount; _splines[i++].CalcSpline( X(), Y(), _Xmax ) );

} // class Curve



////////////////////////////////////////
//
//            S p l i n e
//
////////////////////////////////////////
class Spline
{
   //////////////
   // Members
   //////////////
   private SwapPublisher _pub;
   private string        _name;
   private IntPtr        _StreamID;
   private double[]      _X;
   private double[]      _Z;
   private Curve         _curve;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public Spline( SwapPublisher pub, string tkr, Curve curve )
   {
      _pub      = pub;
      _name     = tkr;
      _StreamID = (IntPtr)0;
      _X        = null;
      _Z        = null;
      _curve    = curve;
      _curve.AddSpline( this );
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void AddWatch( IntPtr StreamID )
   {
      _StreamID = StreamID;
   }

   public void ClearWatch()
   {
      _StreamID = (IntPtr)0;
   }

   public void CalcSpline( double[] X, double[] Y, double xn )
   {
      CubicSpline cs;
      double      x;
      int         i, nx;

      /*
       * Use Vector class as:
       *  1) !bPubFld : Container and Publisher
       *  2) bPubFld  : Container only
       */
      cs = new CubicSpline( X, Y );
      nx = (int)( xn / _xInc );
      _X = new double[nx];
      _Z = new double[nx];
      for ( i=0,x=0.0; i<nx && x<xn; i++, x++ ) {
         _X[i] = x;
         _Z[i] = cs.Spline( _X[i] );
      }
      if ( _StreamID != 0 )
         Publish();
   }

   public void Publish()
   {
      rtEdgePubUpdate u;

      u = new rtEdgePubUpdate( this, _name, (IntPtr)_StreamID, true );
      u.AddFieldAsVector( DTD._fid_X, _X );
      u.AddFieldAsVector( DTD._fid_Y, _Z );
      u.Publish();
   }

} // class Spline



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
   private string                    _svc;
   private Dictionary<int, Knot>     _byId;
   private Dictionary<string, Knot>  _byName;
   private Dictionary<string, Curve> _curves;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SwapSubscriber( XmlElem xe ) :
      base( xe.getAttrValue( DTD._attr_svr, "localhost:9998" ), 
            xe.getAttrValue( DTD._attr_usr, "SwapSpline" ) )
   {
      XmlElem[] edb = xe.elements();
      string    k;
      Curve     crv;

      /*
       * 1) Collections
       */
      _svc    = xe.getAttrValue( DTD._attr_svc, "velocity" ) )
      _byId   = new Dictionary<int, Knot>();
      _byName = new Dictionary<string, Knot>();
      _curves = new Dictionary<string, Curve>();
      /*
       * 2) LoadCurves()
       */
      for ( int i=0; i<edb.Length; i++ ) {
         crv = new Curve( this, edb[i] );
         k   = crv.Name();
         if ( crv.IsValid() )
            _curves[k] = crv;
      }
   }

   ////////////////////////////////
   // Access / /Operations
   ////////////////////////////////
   public bool IsValid()
   {
      return( ( _curves.Length > 0 ) && ( _byName.Length > 0 ) );
   }

   public Curve GetCurve( string k )
   {
      Curve rc;

      if ( ( k != null ) && _curves.TryGetValue( k, out rc ) )
         return rc;
      return null;
   }

   public KnotWatch AddWatch( Curve crv, string k, double X )
   {
      Knot rc;

      // Create if necessary

      if ( !_byName.TryGetValue( k, out rc ) ) {
         rc         = new Knot( k );
         _byName[k] = rc;
      }
      return k.AddWatch( crv, X );
   }

   public int Size()
   {
      return _byName.Length;
   }

   public void OpenAll()
   {
      string tkr;
      Knot   knot;
      int    sid;

      foreach( var kv in _byName ) {
         tkr        = (string)kv.Key;
         knot       = (Knot)kv.Value;
         sid        = knot.Subscribe( this, _svc );
         _byId[sid] = knot;
      }
   }


   ////////////////////////////////
   // rtEdgeSubscriber Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );

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
      Knot k;

      // On-pass

      if ( _wl.TryGetValue( d._StreamID, out k ) )
         k.OnData( d.GetField( DTD._fidVal ) );
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
   private Dictionary<string, Spline> _splines;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SwapPublisher( XmlElem xp ) :
      base( xp.getAttrValue( DTD._attr_svr, "localhost:9995" ),
            xp.getAttrValue( DTD._attr_svc, "SwapSpline" ),
            true,     // bBinary
            false )   // bStart
   {
      _splines = new Dictionary<string, Spline>();
   }

   ////////////////////////////////
   // Access / Operations
   ////////////////////////////////
   public int Size()
   {
      return _splines.Count;
   }

   public int OpenSplines( SwapPublisher, XmlElem xp )
   {
      XmlElem[] edb = xe.elements();
      string    k, tkr;
      Curve     crv;

      for ( int i=0; i<edb.Length; i++ ) {
         if ( (tkr=edb[i].getAttrValue( DTD._attr_name, null )) == null )
            continue; // for-k
         if ( (k=edb[i].getAttrValue( DTD._attr_curve, null )) == null )
            continue; // for-k
         if ( (crv=sub.GetCurve( k )) == null )
            continue; // for-k
         _splines[k] = new Spline( this, tkr, crv );
      }
      return Size();
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
      Spline          s;

      Console.WriteLine( "[{0}] OPEN  {1}", DateTimeMs(), tkr );
      if ( _splines.TryGetValue( tkr, out s ) ) {
         s.AddWatch( arg );
         s.Publish();
      }
      else {
         u = new rtEdgePubUpdate( this, tkr, arg, false );
         u.PubError( "non-existent ticker" );
      }
   }

   public override void OnClose( string tkr )
   {
      rtEdgePubUpdate u;
      Spline          s;

      Console.WriteLine( "[{0}] CLOSE {1}", DateTimeMs(), tkr );
      if ( _splines.TryGetValue( tkr, out s ) )
         s.ClearWatch();
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      try {
         SwapSubscriber sub;
         SwapPublisher  pub;
         XmlParser      cfg;
         int            i, argc, nc, ns;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         if ( argc == 0 ) {
            Console.Write( "Usage: PubSpline.exe <XML_cfgFile>; Exitting ...\n" );
            return 0;
         }

         /////////////////////
         // XML Config File
         /////////////////////
         XmlElem   xs, xp;
         XmlElem[] sdb, pdb;
         string    ps, pp;

         cfg = new XmlParser();
         if ( !cfg.Load( args[0] ) ) {
            Console.Write( "Invalid XML cfg file {0}; Exitting ...\n", args[0] );
            return 0;
         }
         if ( (xs=cfg.getElem( (ps=DTD._elem_sub) )) == null ) {
            Console.Write( "<{0}> not found; Exitting ...\n", ps );
            return 0;
         }
         if ( (xp=cfg.getElem( (pp=DTD._elem_pub) )) == null ) {
            Console.Write( "<{0}> not found; Exitting ...\n", pp );
            return 0;
         }
         if ( ( (sdb=xs.elements()) == null )) || ( sdb.Length == 0 ) ) {
            Console.Write( "No Curves in <{0}> not found; Exitting ...\n", ps );
            return 0;
         }

         if ( ( (pdb=xp.elements()) == null )) || ( pdb.Length == 0 ) ) {
            Console.Write( "No Splines in <{0}> not found; Exitting ...\n", pp );
            return 0;
         }

         /////////////////////////////////////
         // Pub / Sub Channels
         /////////////////////////////////////
         Console.WriteLine( rtEdge.Version() );
         sub = new SwapSubscriber( xs );
         pub = new SwapPublisher( xp );
         if ( (nc=sub.Size()) == 0 ) {
            Console.Write( "No Curves found in {0}; Exitting ...\n", ps );
            return 0;
         }
         if ( (ns=pub.OpenSplines( sub )) == 0 ) {
            Console.Write( "No Splines found in {0}; Exitting ...\n", pp );
            return 0;
         }

         /////////////////////////////////////
         // Pub / Sub Channels
         /////////////////////////////////////
         Console.WriteLine( rtEdge.Version() );
         pub = new SwapPublisher( pubSvr, pubSvc, pubTkr, xInc, bPubFld );
         sub.Start();
         sub.OpenAll();
         pub.PubStart();

         // Do it

         Console.WriteLine( "SUB : {0} : {1} Curves", sub.pConn(), nc );
         Console.WriteLine( "PUB : {0} : {1} Splines", pub.pConn(), ns );
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
