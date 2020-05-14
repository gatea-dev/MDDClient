/******************************************************************************
*
*  topCPU.cpp
*     top space 1
*
*  REVISION HISTORY:
*     18 JUL 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;

////////////////////////////////////////////
// Do it baby!!
////////////////////////////////////////////
int main( int argc, char **argv )
{
   CPUStats  st;
   OSCpuStat cpu;
   double    tSlp;
   char      bp[K], *cp, *pt;
   bool      bT;
   string    s, t;
   int       i, n, c0, c1;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }

   // cmd-line args : [<tSnap> <MinCPU> <MaxCPU> <bInlineTime>]

   tSlp = ( argc > 1 ) ? WithinRange( 1.0, atof( argv[1] ), 900.0 ) : 1.0;
   c0   = ( argc > 2 ) ? atoi( argv[2] ) : 0;
   c1   = ( argc > 3 ) ? atoi( argv[3] ) : K;
   bT   = ( argc > 4 );
   for ( ; true; rtEdge::Sleep( tSlp ) ) {
      n     = st.Snap();
      pt    = (char *)rtEdge::pTimeMs( t );
      s     = "";
      s    += bT ? "" : pt;
      s    += bT ? "" : "\n";
      pt[8] = '\0';
      for ( i=0; i<n; i++ ) {
         if ( !InRange( c0, i, c1 ) )
            continue; // for-i
         cpu = st.Get( i );
         cp  = bp;
         cp += bT ? sprintf( cp, "%s ", pt ) : 0;
         cp += sprintf( cp, "Cpu%-2d : ", i );
         cp += sprintf( cp, "%5.1f us,", cpu._us );
         cp += sprintf( cp, "%5.1f sy,", cpu._sy );
         cp += sprintf( cp, "%5.1f ni,", cpu._ni );
         cp += sprintf( cp, "%5.1f id,", cpu._id );
         cp += sprintf( cp, "%5.1f wa,", cpu._wa );
         cp += sprintf( cp, "%5.1f si,", cpu._si );
         cp += sprintf( cp, "\n" );
         s  += bp;
      }
      printf( s.data() );
      fflush( stdout );
   }
   return 1;
}
