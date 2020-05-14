/******************************************************************************
*
*  PubSub.c
*
*  REVISION HISTORY:
*     26 SEP 2010 jcs  Created.
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*      4 AUG 2012 jcs  Build 19a:EDGAPI
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


/*************************
 * Helpers
 ************************/
char ShowFid( int fid, int *fids, int nFid )
{
   int i;

   for ( i=0; i<nFid; i++ )
      if ( fid == fids[i] ) return 1;
   return !nFid;
}

const char *pType( rtEdgeType ty )
{
   switch( ty ) {
      case edg_image:      return "IMG ";
      case edg_update:     return "UPD ";
      case edg_dead:       return "DEAD";
      case edg_stale:      return "STALE";
      case edg_recovering: return "RECOVERING";
   }
   return "UNKNOWN";
}

/*************************
 * Publication Callbacks
 ************************/
void EDGAPI PubConnHandler( const char *pConn, rtEdgeState s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "[PUB] %s %s\n", pConn, ty );
}

void EDGAPI OnPubOpen( const char *pTkr, void *tag )
{
   fprintf( stdout, "OnPubOpen( %s ) : %d\n", pTkr, (int)tag );
}

void EDGAPI OnPubClose( const char *pTkr )
{
   fprintf( stdout, "OnPubClose( %s )\n", pTkr );
}


/*************************
 * Subscription Callbacks
 ************************/
void EDGAPI ConnHandler( const char *pConn, rtEdgeState s ) 
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "[SUB] %s %s\n", pConn, ty );
}

void EDGAPI SvcHandler( const char *pSvc, rtEdgeState s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "%s %s\n", pSvc, ty );
}

void EDGAPI DataHandler( rtEdgeData d )
{
   const char *ty = pType( d._ty );
   int         i;

   fprintf( stdout, "[%s] [%s,%s] : ", ty, d._pSvc, d._pTkr );
   if ( d._ty == edg_dead )
      printf( "%s\n", d._pErr );
   else {
      printf( "%d fields\n", d._nFld );
      for ( i=0; i<d._nFld; i++ ) {
         fprintf( stdout, "   [%4d] : ", d._flds[i]._fid );
         fwrite( d._flds[i]._data, d._flds[i]._dLen, 1, stdout );
         fprintf( stdout, "\n" );
         fflush( stdout );
      } 
   }
}

/*************************
 * main()
 ************************/
main( int argc, char **argv )
{
   rtEdge_Context cxt, pxt;
   rtFIELD        f, flds[K];
   rtEdgeAttr     sub;
   rtEdgePubAttr  pub;
   rtEdgeData     d;
   char          *pn, *pDbg, buf[K];
   double         dSlp;
   int            i, j, nPub, iDbg;

   /* Parse config file */

   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      exit( 0 );
   }
   memset( &sub, 0, sizeof( sub ) );
   memset( &pub, 0, sizeof( pub ) );
   ParseConfigFile( argv[1] );
   sub._pSvrHosts = GetCfgParam( "rtEdgeCache.Hostname" );
   sub._pUsername = GetCfgParam( "rtEdgeCache.Username" );
   sub._connCbk   = ConnHandler; 
   sub._svcCbk    = SvcHandler; 
   sub._dataCbk   = DataHandler;
   pub._pSvrHosts = GetCfgParam( "rtEdgeCachePub.Hostname" );
   pub._pPubName  = GetCfgParam( "rtEdgeCachePub.Service" );
   pub._connCbk   = PubConnHandler;
   pub._openCbk   = OnPubOpen;
   pub._closeCbk  = OnPubClose;
   if ( (pn=GetCfgParam( "rtEdgeCachePub.Interactive" )) )
      pub._bInteractive = atoi( pn );
   if ( !sub._pSvrHosts ) {
      printf( "No rtEdgeCache.Hostname; Exitting ...\n" );
      exit( 0 );
   }
   if ( !pub._pSvrHosts || !pub._pPubName ) {
      printf( "No rtEdgeCachePub.Hostname or Service; Exitting ...\n" );
      exit( 0 );
   }
   if ( !sub._pSvrHosts ) {
      printf( "No rtEdgeCache.Hostname; Exitting ...\n" );
      exit( 0 );
   }
   if ( !pub._pSvrHosts || !pub._pPubName ) {
      printf( "No rtEdgeCachePub.Hostname or Service; Exitting ...\n" );
      exit( 0 );
   }
   dSlp = 0.0;
   if ( (pn=GetCfgParam( "Publish.Interval" )) )
      dSlp = atof( pn );
   dSlp = !dSlp ? 1.0 : dSlp;
   nPub = 100;
   if ( (pn=GetCfgParam( "Publish.Iterations" )) )
      nPub = atoi( pn );
   nPub = !nPub ? 100 : nPub;

   /* Set up logging; Initialize library and connect */

   pDbg = (pn=GetCfgParam( "Log.Filename" )) ? pn : "stdout";
   iDbg = (pn=GetCfgParam( "Log.Level" )) ? atoi( pn ) : 0;
   rtEdge_Log( pDbg, iDbg );
   printf( "%s\n", rtEdge_Version() );
   if ( !(cxt=rtEdge_Initialize( sub )) ) {
      printf( "rtEdge_Initialize() failed\n" );
      exit( 0 );
   }
   if ( !(pxt=rtEdge_PubInit( pub )) ) {
      printf( "rtEdge_PubInit() failed\n" );
      exit( 0 );
   }
   rtEdge_Start( cxt );
   rtEdge_PubStart( pxt );

   /* Publish 1 item every dSlp seconds */

   memset( &d, 0, sizeof( d ) );
   d._pSvc = pub._pPubName;
   d._pTkr = "MYITEM";
   d._ty   = edg_image;
   d._nFld = 2;
   d._flds = flds;
   for ( j=0; j<2; j++ ) {
      f._fid  = 6+j;
      f._data = (char *)"0";
      f._dLen = strlen( f._data );
      flds[j] = f;
   }
   rtEdge_Publish( pxt, d );
   rtEdge_Sleep( 1.0 ); /* Give it time to get into rtEdgeCache */
/*
   rtEdge_Subscribe( cxt, d._pSvc, d._pTkr, (void *)0 );
 */
   for ( i=0; i<nPub; i++ ) {
      sprintf( buf, "%d", i );
      for ( j=0; j<2; j++ ) {
         f._fid  = 6+j;
         f._data = buf;
         f._dLen = strlen( f._data );
         flds[j] = f;
      }
      rtEdge_Publish( pxt, d );
      rtEdge_Sleep( dSlp );
   }

   /* Wait for user input */

   printf( "Hit <ENTER> to stop ...\n" );
   getchar();

   /* Clean up */

   printf( "Cleaning up ...\n" );
   rtEdge_Destroy( cxt );
   rtEdge_PubDestroy( pxt );
   printf( "Done!!\n " );
}

