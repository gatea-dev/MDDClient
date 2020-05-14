
class MyPublisher : rtEdgePublisher
{
   public override void OnConnect( string msg, rtEdgeState state )
   {
      // Take action for channel connect / disconnect
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      // Start publishing stream for tkr
   }

   public override void OnClose( string tkr )
   {
      // Stop publishing stream for tkr
   }
}
