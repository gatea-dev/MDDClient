/******************************************************************************
*
*  Pipe.cs
*     librtEdge .NET interface test - Pipe
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*      3 JUL 2011 jcs  Build  3: rtEdgeSubscriber / rtEdgePublisher
*     30 JUL 2011 jcs  Build  6: rtEdgePubUpdate
*     12 SEP 2011 jcs  Build  7: _opn
*     24 JAN 2012 jcs  Build 10: rtEdgeField
*     21 MAR 2012 jcs  Build 11: Conflation
*     10 OCT 2012 jcs  Build 13: PubStart()
*     15 NOV 2012 jcs  Build 15: MsgRate()
*      9 MAY 2013 jcs  Build 17: Native Fields
*     11 JUL 2013 jcs  Build 19a:SetMDDirectMon()
*
*  (c) 1994-2013 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections;
using librtEdge;

class PipeSubscriber : rtEdgeSubscriber  
{
   //////////////
   // Members
   //////////////
   private rtEdgePublisher _pub;
   private string          _pPub;
   private Hashtable       _opn;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public PipeSubscriber( string pSvr, string pUsr, string pPub ) :
      base( pSvr, pUsr )
   {
      _pub  = null;
      _pPub = pPub;
      _opn  = new Hashtable();
   }

   public PipeSubscriber( string pSvr, 
                          string pUsr, 
                          string pPub,
                          double dWait ) :
      base( pSvr, pUsr, 1, dWait )  // Conflation ...
   {
      _pub  = null;
      _pPub = pPub;
      _opn  = new Hashtable();
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void InitPipe( rtEdgePublisher pub )
   {
      _pub = pub;
   }

   ////////////////////////////////
   // rtEdgeSubscriber Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "[{0}] SUB-CONN {1} : {2}", pNow(), pConn, pUp );
   }

   public override void OnService( string pSvc, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      if ( !pSvc.Equals( "__GLOBAL__" ) )
         Console.WriteLine( "[{0}] SUB-SVC  {1} {2}", pNow(), pSvc, pUp );
   }

   public override void OnData( rtEdgeData d )
   {
      rtEdgePubUpdate u;
      rtEdgeField     f;
      string          tkr;
      double          dTrd, dOpn, dNet;
      bool            bAdd;

      // NET

      tkr  = d._pTkr;
      bAdd = false;
      dTrd = 0.0;
      dOpn = 0.0;
      try {
         if ( (f=GetField( "TRDPRC_1" )) != null ) {
            dTrd = f.GetAsDouble();
            bAdd = true;
         }
         if ( (f=GetField( "OPEN_PRC" )) != null ) {
            dOpn = f.GetAsDouble();
            _opn[tkr] = dOpn;
         }
      } catch( Exception ) {
         dOpn = 0.0;
      }
      bAdd &= _opn.ContainsKey( tkr );
      dNet  = bAdd ? ( dTrd - (double)_opn[tkr] ) : 0.0;

      // Convert from Subscriber rtFLD to Publisher FIELD

      OnData_SUMM( d );
      u = new rtEdgePubUpdate( _pub, d );
      if ( bAdd )
         u.AddFieldAsDouble( 11, dNet ); 
      u.Publish();
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   public string pNow()
   {
      DateTime dt;
      string   s;

      // Date

      dt = DateTime.Now;
      s  = dt.Year.ToString("D4") + "-" +
           dt.Month.ToString("D2") + "-" +
           dt.Day.ToString("D2") + " " +
           dt.Hour.ToString("D2") + ":" +
           dt.Minute.ToString("D2") + ":" +
           dt.Second.ToString("D2") + "." +
           dt.Millisecond.ToString("D3");
      return s;
   }

   private void OnData_SUMM( rtEdgeData d )
   {
      string sig;
      uint   nf;
      bool   bImg;

      sig  = "[" + d._pSvc + "," + d._pTkr + "]";
      nf   = d._nFld;
      bImg = ( d._ty == rtEdgeType.edg_image );
      switch( d._ty )
      {
         case rtEdgeType.edg_image:
         case rtEdgeType.edg_update:
            if ( bImg || !MsgRateOn() )
               Console.WriteLine( "[{0}] {1} {2} : {3} fields", pNow(), d._ty, sig, nf );
            _nUpd += bImg ? 0 : 1;
            break;
         case rtEdgeType.edg_recovering:
         case rtEdgeType.edg_stale:
            Console.WriteLine( "[{0}] {1} {2}", pNow(), d._ty, sig );
            break;
         case rtEdgeType.edg_dead:
            Console.WriteLine( "[{0}] {1} {2} : {3}", pNow(), d._ty, sig, d._pErr );
            break;
      }
   }
}

class PipePublisher : rtEdgePublisher  
{
   //////////////
   // Members
   //////////////
   private PipeSubscriber _sub;
   private string         _pSub;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public PipePublisher( string pSvr, string pPub, string pSub ) :
      base( pSvr, pPub, true )
   {
      _sub  = null;
      _pSub = pSub;
   }

   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void InitPipe( PipeSubscriber sub )
   {
      _sub = sub;
   }

   ////////////////////////////////
   // rtEdgePublisher Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "[{0}] PUB-CONN {1} : {2}", _sub.pNow(), pConn, pUp );
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      Console.WriteLine( "[{0}] OPEN  {1}", _sub.pNow(), tkr );
      _sub.Subscribe( _pSub, tkr, 0 );
   }

   public override void OnClose( string tkr )
   {
      Console.WriteLine( "[{0}] CLOSE {1}", _sub.pNow(), tkr );
      _sub.Unsubscribe( _pSub, tkr );
   }

   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args) 
   {
      try {
         PipeSubscriber sub;
         PipePublisher  pub;
         int            i, argc;
         bool           bDotNet, bNative, bReUse, bNoRaw;
         string         pSubHost, pSubUser, pSubSvc, err;
         string         pPubHost, pPubSvc;

         /*
          * Arguments:
          *    <SubHost:port>
          *    <User>
          *    <SubSvc>
          *    <PubHost:port>
          *    <PubSvc> 
          *    [ PARSE_IN_DOTNET | NATIVE_FIELDS | REUSE_FIELDS | NO_RAWDATA ]
          */

         argc = args.Length;
         if ( argc < 5 ) {
            err  = "Usage: <SubHost:port> <User> <SubSvc> ";
            err += "<PubHost:port> <PubSvc> ";
            err += "[ PARSE_IN_DOTNET | NATIVE_FIELDS | REUSE_FIELDS ]";
            Console.WriteLine( err );
            return 0;
         }
//         rtEdge.Log( "C:\\TEMP\\Pipe.txt", 0 );
         SetMDDirectMon("MDDirectMon.stats", "Pipe Build 19a", "Pipe.exe");
         Console.WriteLine( rtEdge.Version() );
         pSubHost = "localhost:9998";
         pSubUser = "I Hate Microsoft";
         pSubSvc  = "IDN_RDF";
         pPubHost = "localhost:9995";
         pPubSvc  = "PIPE";
         bDotNet  = false;
         bNative  = false;
         bReUse   = false;
         bNoRaw   = false;
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pSubHost = args[i]; break;
               case 1: pSubUser = args[i]; break;
               case 2: pSubSvc  = args[i]; break;
               case 3: pPubHost = args[i]; break;
               case 4: pPubSvc  = args[i]; break;
               default:
                  bDotNet |= args[i].Equals( "PARSE_IN_DOTNET" );
                  bNative |= args[i].Equals( "NATIVE_FIELDS" );
                  bReUse  |= args[i].Equals( "REUSE_FIELDS" );
                  bNoRaw  |= args[i].Equals( "NO_RAWDATA" );
                  bNative |= args[i].Equals( "BEST" );
                  bReUse  |= args[i].Equals( "BEST" );
                  bNoRaw  |= args[i].Equals( "BEST" );
                  break;
            }
         }
         bDotNet &= !bNative; // No parsing in .NET if Native Fields
         bDotNet &= !bNoRaw;  // No parsing in .NET if No Raw Data

         // Subscription / Publication Channel

         sub = new PipeSubscriber( pSubHost, pSubUser, pPubSvc );
         pub = new PipePublisher( pPubHost, pPubSvc, pSubSvc );
         if ( bDotNet ) {
            Console.WriteLine( "PARSE IN .NET" );
            sub.SetParseInDotNet( bDotNet );
         }
         if ( bReUse ) {
            Console.WriteLine( "RE-USABLE FIELDS" );
            sub.SetReusableFields();
         }
         if ( bNative ) {
            Console.WriteLine( "BEST FIELDS" );
            sub.SetNativeFields( bNative );
            pub.SetNativeFields( bNative );
         }
         if ( bNoRaw ) {
            Console.WriteLine( "NO_RAWDATA" );
            sub.SetRawData( !bNoRaw );
         }
         sub.InitPipe( pub );
         pub.InitPipe( sub );
         pub.PubStart();
         sub.DumpMsgRate( 15 );

         // Do it

         Console.WriteLine( "SUB : {0}", sub.pConn() );
         Console.WriteLine( "PUB : {0}", pub.pConn() );
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
}
