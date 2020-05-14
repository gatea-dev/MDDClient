/******************************************************************************
*
*  SQLSnap.c
*
*  REVISION HISTORY:
*     28 SEP 2010 jcs  Created
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*
*  (c) 1994-2012 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*************************
 * config.c
 ************************/
extern void  ParseConfigFile( char * );
extern char *GetCfgParam( char * );
extern int   GetCsvList( char *, char ** );

/********************************************************
*   CREATE TABLE  GG.Quotes (
*     QuoteTime datetime NOT NULL,
*     SeqNum    int(4) NOT NULL AUTO_INCREMENT,
*     ECN enum('BLOOMBERG','BLACKBOX') DEFAULT 'BLOOMBERG',
*     Ticker varchar(32) NOT NULL,
*     Bid     float DEFAULT NULL,
*     BidSize int(11) DEFAULT NULL,
*     Ask     float DEFAULT NULL,
*     AskSize int(11) DEFAULT NULL,
*     PRIMARY KEY (SeqNum),
*     KEY QuoteTime (QuoteTime),
*     KEY ECN (ECN),
*     KEY CCY (CCY)
*   );
********************************************************/

/*************************
 * SQLConn.c
 ************************/
extern void SQL_Init( const char *, const char *, const char * );
extern void SQL_Destroy();
extern char SQL_Connect();
extern char SQL_Insert( char *, char **, char ** );


/*************************
 * main()
 ************************/
main( int argc, char **argv )
{
   LVCData     d;
   rtFIELD     f;
   LVC_Context lxt = 0;
   rtEdgeAttr  attr;
   char       *pn, *pDbg;
   const char *pc, *pl, *pa, *ty;
   double      dSlp, dMs;
   char       *svcs[K], *rics[K], *vals[10], sDate[K];
   char       *dsn, *usr, *pwd, *tbl;
   int         i, j, k, l, nf, nSvc, nTkr, nFid, iDbg;
   char       *cols[] = { "QuoteTime", "ECN", "Ticker", 
                          "Bid", "BidSize", "Ask", "AskSize",
                          (char *)0 };


   /* Parse config file */

   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      exit( 0 );
   }
   ParseConfigFile( argv[1] );
   dsn  = GetCfgParam( "DSN.Name" );
   usr  = GetCfgParam( "DSN.Username" );
   pwd  = GetCfgParam( "DSN.Password" );
   tbl  = GetCfgParam( "DSN.Table" );
   pl   = GetCfgParam( "LVC.Filename" );
   pa   = GetCfgParam( "LVC.AdminChannel" );
   dSlp = (pc=GetCfgParam( "LVC.Refresh" )) ? atof( pc ) : 0.0;
   if ( !pl || !pa ) {
      printf( "No LVC.Filename : %s; Exitting ...\n" );
      exit( 0 );
   }

   /* SQL Shit */

   SQL_Init( dsn, usr, pwd );
   if ( !SQL_Connect() ) {
      printf( "SQL_Connect( %s,%s,%s ) failed\n", dsn, usr, pwd );
      exit( 0 );
   }

   /* Set up logging; Initialize library and connect */

   pDbg = (pn=GetCfgParam( "Log.Filename" )) ? pn : "stdout";
   iDbg = (pn=GetCfgParam( "Log.Level" )) ? atoi( pn ) : 0;
   rtEdge_Log( pDbg, iDbg );
   printf( "%s\n", rtEdge_Version() );
   if ( !(lxt=LVC_Initialize( pl, pa )) ) {
      printf( "Can't initialize LVC_Context\n" );
      exit( 0 );
   }

   /* Open Items */

   nSvc = GetCsvList( "LVC.Services", svcs );
   nTkr = GetCsvList( "LVC.Instruments", rics );
   do {
      vals[0] = rtEdge_pDateTimeMs( sDate );
      for ( i=0; i<nSvc; i++ ) {
         for ( j=0; j<nTkr; j++ ) {
            d       = LVC_Snapshot( lxt, svcs[i], rics[j] );
            nf      = d._nFld;
            vals[1] = svcs[i];
            vals[2] = rics[j];
            for ( k=3; k<10; vals[k++] = (char *)"" );
            dMs = d._dAge * 1000.0;
            printf( "[LVC] [%s,%s] %04d flds; Age=%.1fmS\n", 
               d._pSvc, d._pTkr, nf, dMs );
            for ( k=0,l; k<nf; k++ ) {
               f = d._flds[k];
               switch( f._fid ) {
                  case 4201: vals[3] = f._data; break;
                  case 4401: vals[4] = f._data; break;
                  case 4301: vals[5] = f._data; break;
                  case 4501: vals[6] = f._data; break;
               }
            }
            vals[7] = (char *)0;
            SQL_Insert( tbl, cols, vals );
            LVC_Free( &d );
         }
      }
      if ( dSlp )
         rtEdge_Sleep( dSlp );
   } while( dSlp );

   /* Clean up */

   printf( "Cleaning up ...\n" );
   LVC_Destroy( lxt );
   SQL_Destroy();
   printf( "Done!!\n " );
}
