/******************************************************************************
*
*  ChainTest.cs
*     Chain subscriber
*
*  REVISION HISTORY:
*     14 DEC 2014 jcs  Created.
*
*  (c) 1994-2014 Gatea, Ltd.
******************************************************************************/
using System;
using librtEdge;


class MyChain : Chain
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyChain( string svc, string tkr ) :
      base( svc, tkr )
   { ; }


   ////////////////////////////////
   // Chain Interface
   ////////////////////////////////
   public override void OnLink( string name, int nLnk, rtEdgeData data )
   {
      Console.WriteLine( "OnLink( {0}, {1} )", nLnk, name );
   }

   public override void OnData( string name, int pos, int nUpd, rtEdgeData data )
   {
      string mt = data.MsgType();

      Console.WriteLine( "{0}( {1}, {2} ) : nUpd={3}", mt, pos, name, nUpd );
   }

}

class ChainTest : rtEdgeSubscriber
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public ChainTest( string pSvr, string pUsr ) :
      base( pSvr, pUsr )
   {
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


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args)
   {
      try {
         ChainTest sub;
         MyChain   chn;
         int       i, argc;
         string    pEdg, pSvc, pTkr, pUsr;

         // [ <host:port> <Svc> <Tkr> <User> ]

         argc  = args.Length;
         pEdg  = "localhost:9998";
         pSvc  = "B-PIPE";
         pTkr  = "0#INDEX";
         pUsr  = "ChainTest";
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pEdg = args[i]; break;
               case 1: pSvc = args[i]; break;
               case 2: pTkr = args[i]; break;
               case 3: pUsr = args[i]; break;
            }
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new ChainTest( pEdg, pUsr );
         chn = new MyChain( pSvc, pTkr );
         Console.WriteLine( sub.Start() );
         sub.Subscribe( chn );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         sub = null; // sub.Destroy();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
    }
}
