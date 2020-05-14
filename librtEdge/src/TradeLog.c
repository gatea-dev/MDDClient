/******************************************************************************
*
*  TradeLog.c
*
*  REVISION HISTORY:
*      3 DEC 2009 jcs  Created.
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*      4 AUG 2012 jcs  Build 19a:EDGAPI
*
*  (c) 1994-2012 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/*************************
 * Data Structure : Cache
 ************************/
typedef struct {
   int  _fid;
   char _val[64];
} CACHE_FIELD;

typedef struct {
   CACHE_FIELD _last;    /* Field   6 */
   CACHE_FIELD _cusip;   /* Field  78 */ 
   CACHE_FIELD _coupon;  /* Field  69 */ 
   CACHE_FIELD _matDate; /* Field  68 */
   CACHE_FIELD _trdVol;  /* Field 178 */
   CACHE_FIELD _acVol;   /* Field  32 */
   CACHE_FIELD _yield;   /* Field 362 */
} RECORD;

/*************************
 * config.c
 ************************/
extern void  ParseConfigFile( char * );
extern char *GetCfgParam( char * );
extern int   GetCsvList( char *, char ** );


/*************************
 * Helpers
 ************************/
void CacheField( CACHE_FIELD *c, rtFIELD f )
{
   int sz, cSz;

   cSz = sizeof( c->_val );
   sz  = gmin( f._dLen, cSz-1 );
   memset( c->_val, 0, cSz );
   memcpy( c->_val, f._data, sz );
}

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
   int     i, fid, fSz;
   char    bTrd, buf[K];

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

   /* Walk thru all fields; If trade (FID 6 is populated), then print */

   for ( i=0,bTrd=0; i<d._nFld; i++ ) {
      fid = d._flds[i]._fid;
      if ( fid == rec->_last._fid ) {
         CacheField( &rec->_last, d._flds[i] );
         bTrd = 1;
      }
      else if ( fid == rec->_cusip._fid )
         CacheField( &rec->_cusip, d._flds[i] );
      else if ( fid == rec->_coupon._fid )
         CacheField( &rec->_coupon, d._flds[i] );
      else if ( fid == rec->_matDate._fid )
         CacheField( &rec->_matDate, d._flds[i] );
      else if ( fid == rec->_trdVol._fid )
         CacheField( &rec->_trdVol, d._flds[i] );
      else if ( fid == rec->_acVol._fid )
         CacheField( &rec->_acVol, d._flds[i] );
      else if ( fid == rec->_yield._fid )
         CacheField( &rec->_yield, d._flds[i] );
   }

   /* Print out trade, if RECORD._last._fid was in this update */

   if ( bTrd ) {
      printf( "%s [%s,%s] %s,%s,%s,%s,%s,%s,%s\n",
         rtEdge_pTimeMs( buf ),
         d._pSvc, d._pTkr,
         rec->_last._val,
         rec->_cusip._val,
         rec->_coupon._val,
         rec->_matDate._val,
         rec->_trdVol._val,
         rec->_acVol._val,
         rec->_yield._val );
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
    *    TradeLog.Services     = ESPEED,BROKERTEC
    *    BROKERTEC.Instruments = 2_YEAR,3_YEAR,5_YEAR,10_YEAR,30_YEAR
    *    ESPEED.Instruments    = 3|usg_02Y,5|usg_05Y,4|usg_07Y,4|usg_10Y,1|usg_30Y
    *    BROKERTEC.Fields      = 6,78,69,68,178,32,362
    *    ESPEED.Fields         = 6,78,69,68,178,32,362
    */

   /* Services */

   nSvc = GetCsvList( "TradeLog.Services", svcs );
   for ( i=0,nOpn=0; i<nSvc; i++ ) {
      sprintf( sTkrs, "%s.Instruments", svcs[i] );
      sprintf( sFids, "%s.Fields", svcs[i] );
      nTkr = GetCsvList( sTkrs, tkrs );
      nFid = GetCsvList( sFids, fids );
      memset( &rec, 0, sizeof( rec ) );
      for( j=0; j<nFid; j++ ) {
         fid = atoi( fids[j] );
         switch( j ) {
            case 0: rec._last._fid    = fid; break;
            case 1: rec._cusip._fid   = fid; break;
            case 2: rec._coupon._fid  = fid; break;
            case 3: rec._matDate._fid = fid; break;
            case 4: rec._trdVol._fid  = fid; break;
            case 5: rec._acVol._fid   = fid; break;
            case 6: rec._yield._fid   = fid; break;
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
