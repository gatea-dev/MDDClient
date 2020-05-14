/******************************************************************************
*
*  CrossMkt.c
*     Look for Crossed Markets
*
*  REVISION HISTORY:
*     27 SEP 2010 jcs  Created.
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*      4 AUG 2012 jcs  Build 19a:EDGAPI
*     20 OCT 2012 jcs  Build 20: CDB_xxx() - Too lazy to create new app
*
*  (c) 1994-2012 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*************************
 * config.c
 ************************/
extern void  ParseConfigFile( char * );
extern char *GetCfgParam( char * );
extern int   GetCsvList( char *, char ** );

#ifdef FOO
/*************************
 * Data Structure : Cache
 ************************/
typedef struct {
   int    _fidBid;
   int    _fidAsk;
   double _dBid;
   double _dAsk;
} RECORD;

/*************************
 * Callbacks
 ************************/
void EDGAPI ConnHandler( const char *pConn, rtEdgeState s ) 
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "%s %s\n", pConn, ty );
}

void EDGAPI SvcHandler( const char *pSvc, rtEdgeState s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "%s %s\n", pSvc, ty );
}

void EDGAPI DataHandler( rtEdgeData d )
{
   RECORD *rec = (RECORD *)d._arg;
   rtFIELD f;
   int     i;
   char    bQte, buf[K];

   /* Image or Update Events only */

   switch( d._ty ) {
      case edg_image:
      case edg_update:
         break;
      case edg_dead:
      case edg_stale:
      case edg_recovering:
         return;
   }

   /* Walk thru all fields; Find Bid / Ask */

   for ( i=0,bQte=0; i<d._nFld; i++ ) {
      f = d._flds[i];
      if ( f._fid == rec->_fidBid ) {
         rec->_dBid = rtEdge_atof( f );
         bQte = 1;
      }
      else if ( f._fid == rec->_fidAsk ) {
         rec->_dAsk = rtEdge_atof( f );
         bQte = 1;
      }
   }

   /* Print out Quote, if crossed in this update */

   if ( bQte && ( rec->_dBid > rec->_dAsk ) ) {
      printf( "%s [%s,%s] BID=%.6f ASK=%.6f\n",
         rtEdge_pTimeMs( buf ),
         d._pSvc, d._pTkr,
         rec->_dBid,
         rec->_dAsk );
   }
}

/*************************
 * main()
 ************************/
main( int argc, char **argv )
{
   rtEdge_Context cxt;
   rtEdgeAttr     attr;
   RECORD        *pRec, rec;
   char          *pn, *pDbg;
   const char    *pc;
   char          *svcs[K], *tkrs[K], *fids[K];
   char           sTkrs[K], sFids[K];
   int            i, j, nSvc, nTkr, nFid, nOpn, iDbg, fid;

   /* Parse config file */

   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      exit( 0 );
   }
   ParseConfigFile( argv[1] );
   attr._pSvrHosts = GetCfgParam( "rtEdgeCache.Hostname" );
   attr._pUsername = GetCfgParam( "rtEdgeCache.Username" );
   attr._connCbk   = ConnHandler; 
   attr._svcCbk    = SvcHandler; 
   attr._dataCbk   = DataHandler; 
   if ( !(pc=attr._pSvrHosts) ) {
      printf( "Invalid rtEdgeCache.Hostname : %s; Exitting ...\n", pc );
      exit( 0 );
   }

   /* Set up logging; Initialize library and connect */

   pDbg = (pn=GetCfgParam( "Log.Filename" )) ? pn : "stdout";
   iDbg = (pn=GetCfgParam( "Log.Level" )) ? atoi( pn ) : 0;
   rtEdge_Log( pDbg, iDbg );
   if ( !(cxt=rtEdge_Initialize( attr )) ) {
      printf( "Can't initialize rtEdge_Context\n" );
      exit( 0 );
   }
   printf( "%s\n", rtEdge_Version() );
   printf( "%s Connected\n", (pc=rtEdge_Start( cxt )) ? pc : "NOT " );

   /*
    * Configuration:
    *    CrossMkt.Services = BBO,CNX
    *    BBO.Instruments   = EUR/USD,USD/CHF
    *    CNX.Instruments   = EUR/USD,USD/CHF
    *    BBO.Fields        = 4201,4301
    *    CNX.Fields        = 436,441
    */

   /* Services */

   nSvc = GetCsvList( "CrossMkt.Services", svcs );
   for ( i=0,nOpn=0; i<nSvc; i++ ) {
      sprintf( sTkrs, "%s.Instruments", svcs[i] );
      sprintf( sFids, "%s.Fields", svcs[i] );
      nTkr = GetCsvList( sTkrs, tkrs );
      nFid = GetCsvList( sFids, fids );
      memset( &rec, 0, sizeof( rec ) );
      for( j=0; j<nFid; j++ ) {
         fid = atoi( fids[j] );
         switch( j ) {
            case 0: rec._fidBid = fid; break;
            case 1: rec._fidAsk = fid; break;
         }
      }
      for ( j=0; j<nTkr; j++ ) {
         pRec = (RECORD *)malloc( sizeof( rec ) );
         memcpy( pRec, &rec, sizeof( rec ) );
         rtEdge_Subscribe( cxt, svcs[i], tkrs[j], pRec );
         nOpn += 1;
      }
   }

   /* Wait for user input */

   if ( nOpn ) {
      printf( "Hit <ENTER> to stop ...\n" );
      getchar();
   }

   /* Clean up */

   rtEdge_Destroy( cxt );
}
#endif // FOO

char *pTm( int i, int intvl, char *buf )
{
   int h,m,s, tm;

   tm = i * intvl;
   h  = tm / 3600;
   m  = ( tm %3600 ) / 60;
   s  = tm % 60;
   sprintf( buf, "[%02d:%02d:%02d]", h, m, s );
   return buf;
}

main( int argc, char **argv )
{
   CDBData     d;
   CDBQuery    q;
   CDBRecDef   rr;
   CDB_Context cxt;
   const char *pc, *pa;
   double      dMs, dSnp;
   char       *svcs[K], *rics[K], *fids[K], tm[K];
   char       *ps, *pt;
   float       dv;
   int         i, j, k, l, nSvc, nTkr, nFid, nTck, invl, fid;

   /* Parse config file */

   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      exit( 0 );
   }
   ParseConfigFile( argv[1] );
   pc   = GetCfgParam( "ChartDB.Filename" );
   pa   = GetCfgParam( "ChartDB.AdminChannel" );
   nSvc = GetCsvList( "ChartDB.Services", svcs );
   nTkr = GetCsvList( "ChartDB.Instruments", rics );
   nFid = GetCsvList( "ChartDB.Fields", fids );
   if ( !pc ) {
      printf( "No ChartDB.Filename : %s; Exitting ...\n" );
      exit( 0 );
   }
   printf( "%s\n", rtEdge_Version() );
   if ( !(cxt=CDB_Initialize( pc, pa )) ) {
      printf( "Can't initialize from ChartDB file %s\n", pc );
      exit( 0 );
   }

   /* Dump OUR ticker */

   for ( i=0; i<nSvc; i++ ) {
      ps = svcs[i];
      for ( j=0; j<nTkr; j++ ) {
         pt = rics[j];
         for ( k=0; k<nFid; k++ ) {
            fid = atoi( fids[k] );
            d    = CDB_View( cxt, ps, pt, fid );
            dMs  = d._dAge  * 1000.0;
            dSnp = d._dSnap * 1000000.0;
            nTck = d._curTick;
            invl = d._interval;
            printf( "[CDB] (%.1fuS) [%s,%s,%d] ", dSnp, ps, pt, fid );
            printf( "%d of %d ticks; Age=%.1fmS\n", nTck, d._numTick, dMs );
            for ( l=0; l<nTck; l++ ) {
               if ( !(dv=d._flds[l]) )
                  continue;  /* Non-zero values only */
               printf( "   %s : %.4f\n", pTm( l, invl, tm ), dv );
            }
            CDB_Free( &d );
         }
      }
   }

   /* Dump entire DB */
/*
   printf( "Hit <ENTER> to dump ALL tickers..." ); getchar();
   q = CDB_Query( cxt );
   for ( i=0; i<q._nRec; i++ ) {
      rr = q._recs[i];
      printf( "[%s,%s,%d]\n", rr._pSvc, rr._pTkr, rr._fid );
   }
 */

   /* Clean up */

   printf( "Cleaning up ...\n" );
   CDB_Destroy( cxt );
   printf( "Done!!\n " );
}
