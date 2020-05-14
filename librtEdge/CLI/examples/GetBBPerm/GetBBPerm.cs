/******************************************************************************
*
*  GetBBPerm.cs
*     Query bbPortal3 for EID's
*
*  REVISION HISTORY:
*     10 DEC 2018 jcs  Created.
*
*  (c) 1994-2018, Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using librtEdge;


class MyStream : ByteStream
{
   static public bool _bDone = false;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyStream( string svc, string tkr ) :
      base( svc, tkr )
   {
   }


   ////////////////////////////////
   // ByteStream Interface
   ////////////////////////////////
   public override void OnData( byte[] buf )
   {
      int  off, eid, i, n;

      // Run-length Encoded; 1st int is Expiration Time

      Console.WriteLine( "OnData() : {0} bytes", buf.Length );
      for ( off=0; off<buf.Length; off+=8 ) {
         eid = BitConverter.ToInt32( buf, off );
         n   = BitConverter.ToInt32( buf, off+4 );
         if ( off == 0 )
            Console.WriteLine( "Expire {0}", rtEdge.DateTimeMs( eid ) );
         else
            for ( i=0; i<=n; Console.WriteLine( "   EID {0}", eid+i ), i++ );
      }
   }

   public override void OnError( string err )
   {
      Console.WriteLine( "ERROR {0}", err );
      _bDone = true;
   }

   public override void OnSubscribeComplete()
   {
      Console.WriteLine( "COMPLETE : {0} bytes", subBufLen() );
      _bDone = true;
   }

}  // class MyStream


class GetBBPerm : rtEdgeSubscriber
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public GetBBPerm( string pSvr, string pUsr ) :
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

   public override void OnService( string svc, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      if ( !svc.Equals( "__GLOBAL__" ) )
         Console.WriteLine( "SVC  {0} {1}", svc, pUp );
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args)
   {
      try {
         GetBBPerm sub;
         MyStream  str;
         bool      aOK;
         int       i, argc;
         string    s, svr, svc, usr, uuid, addr;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         svr  = "localhost:9998";
         usr  = "GetBBPerm";
         svc  = "bloomberg";
         uuid = "shaffer";
         addr = "127.0.0.1";
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -h  <Source : host:port or TapeFile> ] \\ \n";
            s += "       [ -u  <Username> ] \\ \n";
            s += "       [ -s  <Service> ] \\ \n";
            s += "       [ -uuid  <BBG UUID> ] \\ \n";
            s += "       [ -addr  <BBG IP Adress> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -h     : {0}\n", svr );
            Console.Write( "      -u     : {0}\n", usr );
            Console.Write( "      -s     : {0}\n", svc );
            Console.Write( "      -uuid  : {0}\n", uuid );
            Console.Write( "      -addr  : {0}\n", addr );
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
            else if ( args[i] == "-u" )
               usr = args[++i];
            else if ( args[i] == "-s" )
               svc = args[++i];
            else if ( args[i] == "-uuid" )
               uuid = args[++i];
            else if ( args[i] == "-addr" )
               addr = args[++i];
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new GetBBPerm( svr, usr );
         str = new MyStream( svc, "PERM," + uuid + "," + addr );
         Console.WriteLine( sub.Start() );
         rtEdge.Sleep( 1.0 ); // Wait for protocol negotiation to finish
         sub.Subscribe( str );
         for ( i=0; i<10 && !MyStream._bDone; rtEdge.Sleep( 1.0 ), i++ );
//         Console.WriteLine( "Hit <ENTER> to terminate..." );
//         Console.ReadLine();
         sub = null; // sub.Destroy();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
    }

}  // class GetBBPerm 
