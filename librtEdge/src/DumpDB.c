/******************************************************************************
*
*  Dump ChartDB
*
*  REVISION HISTORY:
*      8 JAN 2014 jcs  Created
*
*  (c) 1994-2014 Gatea Ltd.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <librtEdge.h>


#ifdef WIN32
#define LOCALTIME(a,b) localtime( (a) )
#else
#define LOCALTIME(a,b) localtime_r( (a), (b) )
#endif /* WIN32 */

/************
 * Helpers
 ************/
char *DateTime( time_t tMidNt, int tInt, int j, char *buf )
{
   struct tm *tm, lt;
   time_t     tv;

   tv  = tMidNt;
   tv += ( tInt * j );
   tm  = LOCALTIME( &tv, &lt );
   sprintf( buf, "%04d-%02d-%02d %02d:%02d:%02d",
      tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
      tm->tm_hour, tm->tm_min, tm->tm_sec );
   return buf;
}


/********************
 *
 *  main()
 *
 ********************/
int main( int argc, char **argv )
{
   CDB_Context cxt;
   CDBData     d;
   CDBQuery    q;
   CDBRecDef   rec;
   float       dv;
   time_t      t0, t12;
   struct tm  *tm,  lt;
   int         i, j, nr, nj, fid, tInt;
   const char *pSvc, *pTkr;
   char        buf[1024], *dt, *sql;

   /* Pre-condition */

   if ( argc < 2 ) {
      printf( "Usage : %s <file> [<SQLtable>]\n", argv[0] );
      return 0;
   }
   sql = ( argc > 2 ) ? argv[2] : (char *)0;

   /* Rock and Roll */

   printf( "%s\n", rtEdge_Version() );
   if ( !(cxt=CDB_Initialize( argv[1], NULL )) ) {
      printf( "ERROR opening ChartDB file %s\n", argv[1] );
      return 0;
   }
   t0  = rtEdge_TimeSec();
   t12 = t0;
   tm  = LOCALTIME( &t0, &lt );
   t12 -= ( tm->tm_hour * 3600 );
   t12 -= ( tm->tm_min * 60 );
   t12 -= tm->tm_sec;
   q    = CDB_Query( cxt );
   nr   = q._nRec;
   for ( i=0; i<nr; i++ ) {
      rec  = q._recs[i];
      pSvc = rec._pSvc;
      pTkr = rec._pTkr;
      fid  = rec._fid;
      tInt = rec._interval;
      d    = CDB_View( cxt, pSvc, pTkr, fid );
      nj   = gmin( d._curTick, d._numTick );
      for ( j=0; j<nj; j++ ) {
         dt = DateTime( t12, tInt, j, buf );
         dv = d._flds[j];
         if ( sql ) {
            printf( "INSERT INTO %s ", sql ); 
            printf( "( QuoteTime,Service,Ticker,FieldID,FieldValue ) " );
            printf( "VALUES( \"%s\",", dt );
            printf( "\"%s\",\"%s\",", pSvc, pTkr );
            printf( "\"%d\",\"%.4f\" );\n", fid, dv );
         }
         else
            printf( "%s,%s,%s,%d,%.4f\n", dt, pSvc, pTkr, fid, dv );
      }
      CDB_Free( &d );
   }

   /* Done */

   CDB_FreeQry( &q );
   CDB_Destroy( cxt );
   return 0;
}
