/******************************************************************************
*
*  FileSvr.cs
*     Binary file server - rtFld_bytestream
*
*  REVISION HISTORY:
*     19 JUN 2014 jcs  Created.
*     13 DEC 2014 jcs  ByteStream object
*     12 JAN 2015 jcs  1 thread per Watch
*
*  (c) 1994-2015 Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;

class Watch : ByteStream
{
   ////////////////////////////////
   // Class-wide Members
   ////////////////////////////////
   static public int _tPub        = 1000;
   static public int _fldSz       = 1024;
   static public int _nFld        = 100;
   static public int _bytesPerSec = 1024*1024;
   static public int _fidPayload  = 10001;

   ////////////////////////////////
   // Members
   ////////////////////////////////
   private FileSvr         _pub;
   private IntPtr          _tag;
   private Thread          _thr;
   private rtEdgePubUpdate _upd;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public Watch( FileSvr pub, 
                 string  tkr, 
                 IntPtr  tag, 
                 byte[]  raw ) :
      base( pub.pPubName(), tkr )
   {
      _pub = pub;
      _tag = tag;
      _thr = null;
      _upd = null;
      SetPublishData( raw );
   }

   ~Watch()
   {
      _thr.Join();
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void PubFile()
   {
      ThreadStart beg;

      beg  = new ThreadStart( this.Run );
      _thr = new Thread( beg );
      _thr.Start();
   }

   public void Stop()
   {
      lock( this ) {
         if ( _upd != null )
            _upd.Stop( this );
      }
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private void Run()
   {
      lock( this ) {
         _upd = new rtEdgePubUpdate( _pub, tkr(), _tag, true );
      }
      _upd.Publish( this,
                   _fidPayload,       // Put payload into this field ...
                   _fldSz,            // ... up to this size per field ...
                   _nFld,             // ... and this many fields per msg ...
                   _bytesPerSec  );   // ... and this many bytes / sec if
                                      //     multiple messages required.
      lock( this ) {
         _upd = null;
      }
   }

};

class FileSvr : rtEdgePublisher
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private Dictionary<string, Watch> _wl;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public FileSvr( string pSvr, string pPub ) :
      base( pSvr, pPub, true, false, true ) /* Interactive; No Schema; BINARY */
   {
      _wl = new Dictionary<string, Watch>();
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void PubDead( string tkr, IntPtr arg )
   {
      rtEdgePubUpdate u;
      string          err = "File not found";

      u = new rtEdgePubUpdate( this, tkr, arg, err );
      u.Publish();
      Console.WriteLine( "DEAD {0} : {1}", tkr, err );
   }


   ////////////////////////////////
   // rtEdgePublisher Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "CONN {0} : {1}", pConn, pUp );
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      Watch  w;
      byte[] raw;
      int    nt;

      Console.WriteLine( "OPEN {0}", tkr );
      lock( _wl ) {
         nt = _wl.Count;
      }
      raw = rtEdge.ReadFile( tkr );
      if ( raw == null )
         PubDead( tkr, arg );
      else {
         w = null;
         lock( _wl ) {
            if ( !_wl.TryGetValue( tkr, out w ) ) {
               w = new Watch( this, tkr, arg, raw );
               _wl.Add( tkr, w );
            }
         }
         w.PubFile();
      }
   }

   public override void OnClose( string tkr )
   {
      Watch w;

      Console.WriteLine( "CLOSE {0}", tkr );
      lock( _wl ) {
         if ( _wl.TryGetValue( tkr, out w ) )
            w.Stop();
         _wl.Remove( tkr );
      }
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args)
   {
      try {
         FileSvr pub;
         int     i, argc, tPub, fldSz, nFld, bps;
         string  pSvr, pPub;

         // [ <host:port> <Svc> <PubIntvl> <FieldSize> <NumFlds> <BytesPerSec>]

         argc  = args.Length;
         pSvr  = "localhost:9995";
         pPub  = "FileSvr";
         tPub  = 1000;
         fldSz = 1024;
         nFld  = 100;
         bps   = 1024*1024;
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pSvr  = args[i]; break;
               case 1: pPub  = args[i]; break;
               case 2: tPub  = Convert.ToInt32( args[i], 10 ); break;
               case 3: fldSz = Convert.ToInt32( args[i], 10 ); break;
               case 4: nFld  = Convert.ToInt32( args[i], 10 ); break;
               case 5: bps   = Convert.ToInt32( args[i], 10 ); break;
            }
         }
         Console.WriteLine( rtEdge.Version() );
         pub = new FileSvr( pSvr, pPub );
         pub.PubStart();
         Console.WriteLine( pub.pConn() );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         pub = null; // pub.Destroy();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
    }
}
