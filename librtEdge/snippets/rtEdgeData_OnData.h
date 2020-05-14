
class MySubscriber : rtEdgeSubscriber
{
   public override OnData( rtEdgeData msg )
   {
      Field fld;

      // Reusable rtEdgeData object

      for ( msg.reset(); msg.forth(); ) {
         fld = msg.field();

         // Reusable Field object

      }
   }
}
