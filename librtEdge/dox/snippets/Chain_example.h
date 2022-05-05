
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

   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args)
   {
      try {
         rtEdgeSubscriber sub;
         MyChain          chn;

         sub = new rtEdgeSubscriber( "localhost:9998", "ChainTest" );
         chn = new MyChain( "IDN_RDF", "0#AAPL*.U" );
         sub.Subscribe( chn );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         sub = null;
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
    }
}

