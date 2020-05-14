
class MySubscriber : rtEdgeSubscriber
{
   public override void OnConnect( string msg, rtEdgeState state )
   {
      // Take action for channel connect / disconnect
   }

   public override void OnService( string svc, rtEdgeState state )
   {
      // Take action for service UP / DOWN
   }

   public override void OnData( rtEdgeData msg )
   {
      // Process incoming real-time market data message
   }
}
