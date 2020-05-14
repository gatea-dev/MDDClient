/******************************************************************************
*
*  RoundTrip
*     librtEdge .NET round-trip publisher - 1 second intervals
*
*  REVISION HISTORY:
*     25 JAN 2012 jcs  Created.
*
*  (c) 1994-2012 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections;
using System.Threading;
using librtEdge;

class TripSubscriber : rtEdgeSubscriber  
{
   //////////////
   // Members
   //////////////
   private string _pSvc;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public TripSubscriber( string pSvr, string pUsr, string pSvc ) :
      base( pSvr, pUsr )
   {
      _pSvc = pSvc;
   }

   ////////////////////////////////
   // rtEdgeSubscriber Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "SUB-CONN {0} : {1}", pConn, pUp );
   }

   public override void OnService( string pSvc, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      if ( pSvc.Equals( "__GLOBAL__" ) )
         return;
      Console.WriteLine( "SUB-SVC  {0} {1}", pSvc, pUp );
      if ( pSvc.Equals( _pSvc ) ) {
         if ( bUp ) Subscribe( _pSvc, "LATENCY", 0 );
         else       Unsubscribe( _pSvc, "LATENCY" );
      }
   }

   public override void OnData( rtEdgeData d )
   {
      rtEdgeField f;
      double      dd, d0, d1;

      // rtEdge.TimeNs() is in TripPublisher._fid

      d1 = rtEdge.TimeNs();
      f  = GetField( TripPublisher._fid );
      d0 = ( f == null ) ? d1 : f.GetAsDouble();
      dd = 1E6 * ( d1-d0 );
      Console.WriteLine( "Round-Trip in {0}uS", dd.ToString("F1") );
   }
}

class TripPublisher : rtEdgePublisher  
{
   static public int _fid = 6;

   //////////////
   // Members
   //////////////
   private Hashtable              _opn;
   private TimerCallback          _cbk;
   private System.Threading.Timer _tmr;


   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public TripPublisher( string pSvr, string pPub ) :
      base( pSvr, pPub, true )
   {
      _opn = new Hashtable();
      _cbk = new TimerCallback( Pump );
      _tmr = new System.Threading.Timer( _cbk, null, 1000, 1000 );
   }


   ////////////////////////////////
   // rtEdgePublisher Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "PUB-CONN {0} : {1}", pConn, pUp );
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      Console.WriteLine( "OPEN {0}", tkr );
      lock( this ) {
         _opn[tkr] = tkr;
      }
   }

   public override void OnClose( string tkr )
   {
      Console.WriteLine( "CLOSE {0}", tkr );
      lock( this ) {
         if ( _opn.ContainsKey( tkr ) )
            _opn.Remove( tkr );
      }
   }

   //////////////////////////////
   // Timer Handler
   //////////////////////////////
   private void Pump( object data )
   {
      rtEdgePubUpdate pub;
      double          dt;

      lock( this ) {
         foreach ( string tkr in _opn.Keys )  {
            pub = new rtEdgePubUpdate( this, tkr, (IntPtr)0, true );
            dt  = rtEdge.TimeNs();
            pub.AddFieldAsDouble( _fid, dt );
            pub.Publish();
         }
      }
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args) 
   {
      try {
         TripSubscriber sub;
         TripPublisher  pub;
         int            i, argc;
         string         pSubHost, pSubUser, err;
         string         pPubHost, pPubSvc;

         // [ <SubHost:port> <User> <PubHost:port> <PubSvc> ]

         argc = args.Length;
         if ( argc < 4 ) {
            err  = "Usage: <SubHost:port> <User> <SubSvc> ";
            err += "<PubHost:port> <PubSvc>";
            Console.WriteLine( err );
            return 0;
         }
         rtEdge.Log( "C:\\TEMP\\Trip.txt", 0 );
         Console.WriteLine( rtEdge.Version() );
         pSubHost = "localhost:9998";
         pSubUser = "I Hate Microsoft";
         pPubHost = "localhost:9995";
         pPubSvc  = "Trip";
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pSubHost = args[i]; break;
               case 1: pSubUser = args[i]; break;
               case 2: pPubHost = args[i]; break;
               case 3: pPubSvc  = args[i]; break;
            }
         }

         // Subscription / Publication Channel

         sub = new TripSubscriber( pSubHost, pSubUser, pPubSvc );
         pub = new TripPublisher( pPubHost, pPubSvc );

         // Do it

         Console.WriteLine( "SUB : {0}", sub.pConn() );
         Console.WriteLine( "PUB : {0}", pub.pConn() );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         sub = null;
         pub = null;
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 
}
