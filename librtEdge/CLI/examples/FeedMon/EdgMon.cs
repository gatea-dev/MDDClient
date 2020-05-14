   [StructLayout(LayoutKind.Explicit, Size=2918424)]
   public struct EdgStats
   {
       ////////////////////////////////////////////
       // Standard MD-Direct stats header
       ////////////////////////////////////////////
       [FieldOffset(0)]
       public GLmdStatsHdr  _hdr;
       ////////////////////////////////////////////
       // Up to 1024 publishers
       ////////////////////////////////////////////
       [FieldOffset(2072)]
       public GLmdChanStats _pub1;
       [FieldOffset(2784)]
       public GLmdChanStats _pub2;
       . . . 
       [FieldOffset(730448)]
       public GLmdChanStats _pub1024
       ////////////////////////////////////////////
       // Up to 1024 subscribers
       ////////////////////////////////////////////
       [FieldOffset(731160)]
       public GLmdChanStats _sub1;
       [FieldOffset(731872)]
       public GLmdChanStats _sub2;
       . . . 
       [FieldOffset(1459536)]
       public GLmdChanStats _pub1024
       ////////////////////////////////////////////
       // Up to 1024 peer channels
       ////////////////////////////////////////////
       [FieldOffset(1460248)]
       public GLmdChanStats _peer1;
       [FieldOffset(1460960)]
       public GLmdChanStats _peer2;
       . . .
       [FieldOffset(2188624)]
       public GLmdChanStats _peer1024
       ////////////////////////////////////////////
       // Up to 1024 pumps (dispatchers)
       ////////////////////////////////////////////
       [FieldOffset(2189336)]
       public GLmdChanStats _pump1;
       [FieldOffset(2190048)]
       public GLmdChanStats _pump2;
       . . .
       [FieldOffset(2917712)]
       public GLmdChanStats _peer1024
   }
