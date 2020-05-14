/******************************************************************************
*
*  Pipe.c
*     Simple pipe
*
*  REVISION HISTORY:
*     26 SEP 2010 jcs  Created.
*      4 AUG 2012 jcs  Build 19a:EDGAPI
*     17 NOV 2012 jcs  Build 20: rtEdge_CPU()
*
*  (c) 1994-2010 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Globals */

rtEdge_Context cxt     = 0;
rtEdge_Context pxt     = 0;
const char    *pSubSvc = 0;

/*************************
 * config.c
 ************************/
extern void  ParseConfigFile( char * );
extern char *GetCfgParam( char * );
extern int   GetCsvList( char *, char ** );


/*************************
 * Helpers
 ************************/
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
void EDGAPI PubConnHandler( rtEdge_Context cxt,
                            const char    *pConn,
                            rtEdgeState    s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "[PUB] %s %s\n", pConn, ty );
}

void EDGAPI OnPubOpen( rtEdge_Context cxt, const char *pTkr, void *tag )
{
   fprintf( stdout, "OnPubOpen( %s ) : %d\n", pTkr, (long)tag );
   rtEdge_Subscribe( cxt, pSubSvc, pTkr, tag );
}

void EDGAPI OnPubClose( rtEdge_Context cxt, const char *pTkr )
{
   fprintf( stdout, "OnPubClose( %s )\n", pTkr );
   rtEdge_Unsubscribe( cxt, pSubSvc, pTkr );
}


/*************************
 * Subscription Callbacks
 ************************/
void EDGAPI ConnHandler( rtEdge_Context cxt,
                         const char    *pConn,
                         rtEdgeState    s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "[SUB] %s %s\n", pConn, ty );
}

void EDGAPI SvcHandler( rtEdge_Context cxt,
                        const char    *pSvc,
                        rtEdgeState    s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "%s %s\n", pSvc, ty );
}

static int _nUpd = 0;

void EDGAPI DataHandler( rtEdge_Context cxt, rtEdgeData d )
{
   const char *ty = pType( d._ty );
   int         i;

#ifdef FOO
   fprintf( stdout, "[%s] [%s,%s] : ", ty, d._pSvc, d._pTkr );
   if ( d._ty == edg_dead )
      printf( "%s\n", d._pErr );
   else {
      printf( "%d fields\n", d._nFld );
      rtEdge_Publish( pxt, d );
   }
#endif // FOO
   switch( d._ty ) {
      case edg_image:
         ty = pType( d._ty );
         fprintf( stdout, "[%s] [%s,%s]\n", ty, d._pSvc, d._pTkr );
         rtEdge_Publish( pxt, d );
         break;
      case edg_update:
         _nUpd += 1;
         rtEdge_Publish( pxt, d );
         break;
   }
}

/*************************
 * main()
 ************************/
main( int argc, char **argv )
{
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
   pSubSvc        = GetCfgParam( "rtEdgeCache.Service" );
   sub._connCbk   = ConnHandler; 
   sub._svcCbk    = SvcHandler; 
   sub._dataCbk   = DataHandler;
   pub._pSvrHosts = GetCfgParam( "rtEdgeCachePub.Hostname" );
   pub._pPubName  = GetCfgParam( "rtEdgeCachePub.Service" );
   pub._connCbk   = PubConnHandler;
   pub._openCbk   = OnPubOpen;
   pub._closeCbk  = OnPubClose;
   pub._bInteractive = 1;
   if ( (pn=GetCfgParam( "rtEdgeCachePub.Interactive" )) )
      pub._bInteractive = atoi( pn );
   if ( !sub._pSvrHosts || !pSubSvc ) {
      printf( "No rtEdgeCache.Hostname or Service; Exitting ...\n" );
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

   /* Wait for user input */

#ifdef FOO
   while( 1 ) {
      fprintf( stdout, "%d,%.3f\n", _nUpd, rtEdge_CPU() );
      fflush( stdout );
      rtEdge_Sleep( 15.0 );
   }
#else
   printf( "Hit <ENTER> to stop ...\n" ); getchar();
#endif /* FOO */

   /* Clean up */

   printf( "Cleaning up ...\n" );
   rtEdge_Destroy( cxt );
   rtEdge_PubDestroy( pxt );
   printf( "Done!!\n " );
}

