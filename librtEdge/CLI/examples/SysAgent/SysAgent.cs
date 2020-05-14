/******************************************************************************
*
*  SysAgent.cs
*     Connectionless publisher agent to monitor CPU and Disk stats
*
*  REVISION HISTORY:
*     13 JUL 2017 jcs  Created.
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections;
using System.Net;
using System.Threading;
using System.Runtime.InteropServices;
using librtEdge;

////////////////////////
// System CPU / Disk Monitor
////////////////////////
class SysAgent : rtEdgePublisher
{
   ////////////////////////////////
   // Class-wide Members
   ////////////////////////////////
   static int _nameFid =    3;
   static int _cpuFid  =    6;
   static int _seqFid  = 1021; // SEQNUM

   //////////////
   // Members
   //////////////
   private CPUStats               _cpu;
   private DiskStats              _disk;
   private String                 _host;
   private int                    _RTL;
   private int                    _iTmr;
   private TimerCallback          _cbk;
   private System.Threading.Timer _tmr;
 
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SysAgent( string pSvr, 
                    string pPub, 
                    double tSnap,
                    bool   bDisk ) :
      base( pSvr, pPub )
   {
      String fmt;

      // 1) CPU / Disk Stats

      _cpu  = bDisk ? null : new CPUStats();
      _disk = bDisk ? new DiskStats() : null;
      _host = Dns.GetHostName();
      _RTL  = 1;

      // 2) Connectionless Publisher

      PubStartConnectionless();

      // 3) Snapshot Timer

      _iTmr = Convert.ToInt32( 1000.0 * tSnap );
      _cbk  = new TimerCallback( Snap );
      _tmr  = new System.Threading.Timer( _cbk, null, _iTmr, _iTmr );
      fmt   = "{0} to udp:{1} every {2}ms for {3}";
      Console.WriteLine( fmt, pPub, pSvr, _iTmr, _host );
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
   }

   public override void OnClose( string tkr )
   {
      Console.WriteLine( "CLOSE {0}", tkr );
   }

   public override void OnSymListQuery( int nSym )
   {
      Console.WriteLine( "OnSymListQuery( {0} )", nSym );
   }

   public override void OnRefreshImage( String tkr, IntPtr StreamID )
   {
      Console.WriteLine( "OnRefreshImage( {0} )", tkr );
   }


   #region Timer Handler
   //////////////////////////////
   // Timer Handler
   //////////////////////////////
   private void Snap( object data )
   {
      if ( _cpu != null )
         _Publish( _cpu );
      if ( _disk != null )
         _Publish( _disk );
      _RTL += 1;
   }

   private void _Publish( CPUStats cs )
   {
      rtEdgePubUpdate u;
      String          tkr;
      OSCpuStat       c;
      bool            bImg;
      int             i, id, n, fid;

      n    = cs.Snap();
      bImg = ( _RTL == 1 );
      for ( i=0; i<n; i++ ) {
         id  = i+1;
         tkr = _host + "." + id.ToString();
         u   = new rtEdgePubUpdate( this, tkr, (IntPtr)i, bImg );
         c   = cs.Get( i );
         fid = _cpuFid;
         u.AddFieldAsDouble( fid++, c._us );
         u.AddFieldAsDouble( fid++, c._sy );
         u.AddFieldAsDouble( fid++, c._ni );
         u.AddFieldAsDouble( fid++, c._id );
         u.AddFieldAsDouble( fid++, c._wa );
         u.AddFieldAsDouble( fid++, c._si );
         u.AddFieldAsDouble( fid++, c._st );
         u.AddFieldAsInt32( _seqFid, _RTL );
         u.Publish();
      }
   }

   private void _Publish( DiskStats ds )
   {
      rtEdgePubUpdate u;
      String          tkr;
      OSDiskStat      d;
      bool            bImg;
      int             i, id, n, fid;

      n    = ds.Snap();
      bImg = ( _RTL == 1 );
      for ( i=0; i<n; i++ ) {
         id  = i+1;
         tkr = _host + "." + id.ToString();
         u   = new rtEdgePubUpdate( this, tkr, (IntPtr)i, bImg );
         d   = ds.Get( i );
         fid = _cpuFid;
         u.AddFieldAsString( _nameFid, d.Name() );
         u.AddFieldAsDouble( fid++, d._nRdSec );
         u.AddFieldAsDouble( fid++, d._nWrSec );
         u.AddFieldAsDouble( fid++, d._nIoMs );
         u.AddFieldAsInt32( _seqFid, _RTL );
         u.Publish();
      }
   }

   #endregion
 

   //////////////////////////////
   // main()
   //////////////////////////////
   static int Main( string[] args )
   {
      try {
         SysAgent app;
         String   pSvr, pPub;
         double   tTmr;
         int      argc;
         bool     bDsk;

         // Quickie Check

         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         if ( ( argc > 0 && ( args[0] == "--config" ) ) || ( argc < 3 ) ) {
            pSvr = "<host:port> <Svc> <pubTmr> [<bDisk>]";
            Console.WriteLine( pSvr );
            return 0;
         }

         // [ <hosts> <Svc> <pubTmr> [<bDisk>]

         pSvr = args[0];
         pPub = args[1];
         tTmr = Convert.ToDouble( args[2] );
         bDsk = ( argc > 3 );
         app  = new SysAgent( pSvr, pPub, tTmr, bDsk );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
      } catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   }

}  // class SysAgent
