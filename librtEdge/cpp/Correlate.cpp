/******************************************************************************
*
*  Correlate.cpp
*     General Purpose Test Utility
*
*  REVISION HISTORY:
*      2 AUG 2013 jcs  Created
*     15 FEB 2023 jcs  Made general purpose
*
*  (c) 1994-2023, Gatea Ltd.
*******************************************************************************/
#include <librtEdge.h>

using namespace std;
using namespace RTEDGE;
using namespace QUANT;

///////////////////////
//
//  main()
//
///////////////////////
int main( int argc, char **argv )
{
   const char *inp = ( argc > 1 ) ? argv[1] : "-1,1.5|1,-1";
   DoubleGrid  A;
   DoubleList  row;
   Strings     rows;
   string      s( inp /* argv[1] */ );
   const char *SEP;
   char       *cp;

   cp  = (char *)s.data();
   SEP = "|";
   for ( cp=::strtok( cp, SEP ); cp; cp=::strtok( NULL, SEP ) )
      rows.push_back( string( cp ) );
   SEP = ",";
   for ( size_t i=0; i<rows.size(); i++ ) {
      cp = (char *)rows[i].data();
      row.clear();
      for ( cp=::strtok( cp, SEP ); cp; cp=::strtok( NULL, SEP ) )
         row.push_back( atof( cp ) );
      A.push_back( row );
   }

   LU          lu( A );
   DoubleGrid  inv = lu.Invert();
   const char *op;
   string      obuf;

   op = rtEdge::Dump( A, obuf );
   printf( "============= A =============\n" );
   ::fwrite( op, obuf.size(), 1, stdout );
   ::fflush( stdout );
   op = rtEdge::Dump( inv, obuf );
   printf( "============= A (-1) =============\n" );
   ::fwrite( op, obuf.size(), 1, stdout );
   ::fflush( stdout );
   printf( "\nShould be [ 2,3 ] [ 2,2 ]\n" );
   return 0;
}
