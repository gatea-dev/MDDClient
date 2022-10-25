/******************************************************************************
*
*  SubCurve.cs
*     librtEdge .NET Subscribe to Vector
*
*  REVISION HISTORY:
*     25 OCT 2022 jcs  Created (from SubCurve)
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
using System;
using System.Collections;
using System.IO;
using System.Threading;
using librtEdge;

/////////////////////////////////////
//
//   c l a s s   S u b C u r v e
//
/////////////////////////////////////
class SubCurve : rtEdgeSubscriber 
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public SubCurve( string svr, string usr ) :
      base( svr, usr, true )
   { ; }


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
         SubCurve   sub;
         int        i, nt, argc;
         bool       aOK;
         string[]   tkrs;
         MyVector[] vdb;
         string     s, svr, usr, svc, tkr;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         svr   = "localhost:9998";
         usr   = "SubCurve";
         svc   = "bloomberg";
         tkr   = null;
         tkrs  = null;
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: %s \\ \n";
            s += "       [ -h    <Source : host:port or TapeFile> ] \\ \n";
            s += "       [ -u    <Username> ] \\ \n";
            s += "       [ -s    <Service> ] \\ \n";
            s += "       [ -t    <Ticker : CSV or Filename> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "   Defaults:\n" );
            Console.Write( "      -h       : {0}\n", svr );
            Console.Write( "      -u       : {0}\n", usr );
            Console.Write( "      -s       : {0}\n", svc );
            Console.Write( "      -t       : <empty>\n" );
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
            else if ( args[i] == "-t" ) {
               tkr = args[++i];
               tkrs = ReadLines( tkr );
               if ( tkrs == null )
                  tkrs = tkr.Split(',');
            }
         }
         Console.WriteLine( rtEdge.Version() );
         sub = new SubCurve( svr, usr );
         sub.SetBinary( true );
         Console.WriteLine( "BINARY" );
         Console.WriteLine( sub.Start() );
         /*
          * Subscribe onward ...
          */
         rtEdge.Sleep( 2.5 ); // 2.5 seconds to ensure BINARY
         nt  = tkrs.Length;
         vdb = new MyVector[nt];
         for ( i=0; i<nt; vdb[i] = new MyVector( svc, tkrs[i] ), i++ );
         for ( i=0; i<nt; vdb[i].Subscribe( sub ), i++ );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 

   ////////////////////
   // Class-wide
   ////////////////////
   static public string[] ReadLines( string pf )
   {
      TextReader fp;
      string     s, ss;
      string[]   rtn;

      // Pre-condition

      rtn = null;
      if ( !File.Exists( pf ) )
         return rtn;

      // Open / Read

      ss = "";
      fp = File.OpenText( pf );
      while( (s=fp.ReadLine()) != null ) {
         ss += s; ss += "\n";
      }
      fp.Close();
      rtn = ss.Split('\n');
      return rtn;
   }

} // class SubCurve


/////////////////////////////////////
//
//   c l a s s   M y V e c t o r
//
/////////////////////////////////////
class MyVector : Vector
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyVector( string svc, string tkr ) :
      base( svc, tkr, 0 )
   { ; }

   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
   public override void OnData( double[] img )
   {
      Console.Write( Dump( true ) );
   }

   public override void OnData( VectorValue[] upd )
   {
      Console.Write( Dump( upd, true ) );
   }

   public override void OnError( string err )
   {
      Console.WriteLine( "ERR ({0},{1}) : {2}", Service(), Ticker(), err );
   }

}; // class MyVector
