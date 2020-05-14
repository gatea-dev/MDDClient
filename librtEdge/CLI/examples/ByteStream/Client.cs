/******************************************************************************
*
*  Client.cs
*     Binary file client - rtFld_bytestream
*
*  REVISION HISTORY:
*     14 DEC 2014 jcs  Created.
*
*  (c) 1994-2014 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Media;
using librtEdge;


class MyStream : ByteStream
{
   private static string _wav = "./tmp.wav";

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyStream( string svc, string tkr ) :
      base( svc, tkr )
   { ; }


   ////////////////////////////////
   // ByteStream Interface
   ////////////////////////////////
   public override void OnData( byte[] buf )
   {
      Console.WriteLine( "OnData() : {0} bytes", buf.Length );
   }

   public override void OnError( string err )
   {
      Console.WriteLine( "ERROR {0}", err );
   }

   public override void OnSubscribeComplete()
   {
      SoundPlayer play;
      bool        wav;

      // Log complete; If *.wav, play it ...

      Console.WriteLine( "COMPLETE : {0} bytes", subBufLen() );
      wav = tkr().EndsWith( ".wav" ) || tkr().EndsWith( ".WAV" );
      if ( wav ) {
Console.WriteLine( "Playing {0} ... ", tkr() );
         File.WriteAllBytes( _wav, subBuf() );
         using( play = new SoundPlayer( _wav ) ) {
            play.PlaySync();
         }
/*
         play = new SoundPlayer( _wav );
         play.Stream = new MemoryStream();
         play.Play();
 */
Console.WriteLine( "Done playing {0} ... ", tkr() );
         Console.WriteLine( "Hit <ENTER> when done listening to {0}", tkr() );
         Console.ReadLine();
      }
   }
}

class Client : rtEdgeSubscriber
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public Client( string pSvr, string pUsr ) :
      base( pSvr, pUsr )
   {
      SetBinary( true );
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
         Client   sub;
         MyStream str;
         int      i, argc;
         string   pEdg, pSvc, pTkr, pUsr;

         // [ <host:port> <Svc> <Tkr> <User> ]

         argc  = args.Length;
         pEdg  = "localhost:9998";
         pSvc  = "FileSvr";
         pTkr  = "./a.out";
         pUsr  = "Client";
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pEdg = args[i]; break;
               case 1: pSvc = args[i]; break;
               case 2: pTkr = args[i]; break;
               case 3: pUsr = args[i]; break;
            }
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new Client( pEdg, pUsr );
         str = new MyStream( pSvc, pTkr );
         Console.WriteLine( sub.Start() );
         sub.Subscribe( str );
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
