
//_///////////////////////////////////
// User-Driven Start / Set Protocol
//_///////////////////////////////////
   rtEdgePublisher pub;

   pub = new rtEdgePublisher( hosts, pubName, false, false );
   pub.SetBinary( true );
   pub.PubStart();

