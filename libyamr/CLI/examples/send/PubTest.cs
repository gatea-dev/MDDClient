/******************************************************************************
*
*  PubTest.cs
*     librtEdge .NET interface test - Publication
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*     29 JUN 2011 jcs  Build  3: rtEdgePublisher
*     30 JUL 2011 jcs  Build  6: rtEdgePubUpdate
*     23 SEP 2011 jcs  Build 12: 3rd arg = timer
*     10 OCT 2012 jcs  Build 13: PubStart()
*     25 MAY 2013 jcs  Build 17: lock( _wl )
*     11 JUL 2013 jcs  Build 19: "BID"
*     23 DEC 2013 jcs  Build 22: tApp
*     23 JAN 2015 jcs  Chains
*
*  (c) 1994-2013 Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections;
using System.IO;
using System.Threading;
using librtEdge;

class PubTest : rtEdgePublisher
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private Hashtable     _wl;
   private Timer         _tmr;
   private TimerCallback _cbk;
   private int           _rtl;
   private string[]      _chn;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public PubTest( string pSvr, string pPub, int tTmr, string[] chn ) :
      base( pSvr, pPub /* , true */ )
   {
      // Fields / Watchlist

      _wl  = new Hashtable();
      _rtl = 0;
      _chn = chn;

      // Timer Callback

      _cbk = new TimerCallback( PublishAll );
      _tmr = new Timer( _cbk, this, tTmr, tTmr );
   }


   ////////////////////////////////
   // Timer Handler
   ////////////////////////////////
   public void PublishAll( object data )
   {
      lock( _wl ) {
         foreach ( DictionaryEntry  kv in _wl ) {
            PubTkr( (string)kv.Key, (IntPtr)kv.Value, false );
         }
      }
   }

   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void PubTkr( string tkr, IntPtr arg, bool bImg )
   {
      rtEdgePubUpdate u;
      int             fid;

      // RTL; Time (int); Time (double); DateTime, String

      u   = new rtEdgePubUpdate( this, tkr, arg, bImg );
      fid = 6;
      u.AddFieldAsInt32( fid++, _rtl );
      u.AddFieldAsInt32( fid++, (int)rtEdge.TimeSec() );
      u.AddFieldAsDouble( fid++, rtEdge.TimeNs() );
      u.AddFieldAsDateTime( fid++, DateTime.Now );
      u.AddFieldAsString( 70, "+10 43/64" );
      u.AddFieldAsString( 9151, "3001.95000019077" );
      u.AddFieldAsString( 9152, "0.0237629979273164" );
      u.AddFieldAsString( 9153, "0.0301808869670407" );
      u.AddFieldAsString( 9154, "-0.999999986920079" );
      u.AddFieldAsString( 9155, "0.0504438875743321" );
      u.AddFieldAsString( 9156, "0.642828433854239" );
      u.AddFieldAsString( 9157, "-0.0769193799974062" );
      u.AddFieldAsString( 9158, "0.154162692851771" );
      u.AddFieldAsString( 9159, "0" );
      u.AddFieldAsString( 9160, "0.145033631216388" );
      u.AddFieldAsString( 9161, "9.48514228079972E-10" );
      u.AddFieldAsString( 9140, "0.000895748729725115" );
      u.AddFieldAsString( 9141, "0.000282777396460158" );
      u.AddFieldAsString( 9142, "0.000131151388637757" );
      u.AddFieldAsString( 9143, "0.999951726026228" );
      u.AddFieldAsString( 9144, "0.999945779518907" );
      u.AddFieldAsString( 9145, "0" );
      u.AddFieldAsString( 9146, "0" );
      u.AddFieldAsString( 9147, "14" );
      u.AddFieldAsString( 9148, "2400" );
      u.AddFieldAsString( 9149, "4000" );
      u.AddFieldAsString( 9009, "0" );
      u.AddFieldAsString( 9532, "0" );
      u.AddFieldAsString( 9034, "20171215" );
      u.AddFieldAsString( 9501, "15 DEC 2017 00:00:00.000" );
      u.AddFieldAsString( 9502, "2.4027397260274" );
      u.AddFieldAsString( 9500, "3238.26" );
      u.AddFieldAsString( 9503, "3001.95000019077" );
      u.AddFieldAsString( 9504, "0.211533009563856" );
      u.AddFieldAsString( 9531, "0" );
      u.AddFieldAsString( 9528, "0.999999902167299" );
      u.AddFieldAsString( 9534, "0" );
      u.AddFieldAsString( 9533, "0" );
      u.AddFieldAsString( 9553, "0" );
      u.AddFieldAsString( 9529, "BLOOMBERG|VGH5 INDEX|LAST_PRICE" );
      u.AddFieldAsString( 9530, "3236" );
      u.AddFieldAsString( 9505, "NULL" );
      u.AddFieldAsString( 9506, "0" );
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
      lock( _wl ) {
         if ( bChn )
            PubChainLink( lnk, kv[1], arg );
         else {
            PubTkr( tkr, arg, true );
            if ( !_wl.ContainsKey( tkr ) )
               _wl.Add( tkr, arg );
         }
      }
   }

   public override void OnClose( string tkr )
   {
      Console.WriteLine( "[{0}] CLOSE {1}", DateTimeMs(), tkr );
      lock( _wl ) {
         _wl.Remove( tkr );
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


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args)
   {
      try {
         PubTest pub;
         int      i, tHb, tTmr, argc;
         string   pSvr, pPub;
         string[] chn;

         // Quickie Check

         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         if ( argc > 0 && ( args[0] == "--config" ) ) {
            pSvr = "<host:port> <Svc> <pubTmr> <ChainFile> <tHbeat>";
            Console.WriteLine( pSvr );
            return 0;
         }

         // [ <host:port> <Svc> <pubTmr> <ChainFile> <tHbeat> ]

         pSvr = "localhost:9995";
         pPub = "MySource";
         tTmr = 1000;
         chn  = null;
         tHb  = 3600;
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0:
                  pSvr = args[i];
                  break;
               case 1:
                  pPub = args[i];
                  break;
               case 2:
                  tTmr = Convert.ToInt32( args[i], 10 );
                  break;
               case 3:
                  chn = File.ReadAllLines( args[i] );
                  if ( chn.Length == 0 )
                     chn = null;
                  break;
               case 4:
                  tHb = Convert.ToInt32( args[i], 10 );
                  break;
            }
         }
         Console.WriteLine( rtEdge.Version() );
         pub = new PubTest( pSvr, pPub, tTmr, chn );
         pub.SetHeartbeat( tHb );
         pub.PubStart();
         Console.WriteLine( pub.pConn() );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         pub.Stop();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   }
}

