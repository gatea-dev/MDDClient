/******************************************************************************
*
*  RoundTrip.c
*     Simple pipe
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

typedef struct {
   char  _tkr[K];
   void *_tag;
} Open;

static Open _opens[K];
static int  _nOpen = 0;

/* Globals */

rtEdge_Context cxt     = 0;
rtEdge_Context pxt     = 0;
const char    *pSubSvc = 0;
char           _bDone  = 0;
int            iPubBin = 0;
int            iSubBin = 0;

/************************
 * config.c
 ************************/
extern void  ParseConfigFile( char * );
extern char *GetCfgParam( char * );
extern int   GetCsvList( char *, char ** );


/************************
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

static void Pump( Open o )
{
   rtEdgeData d;
   rtFIELD    flds[K], f1, f2, f3, f4;
   char       buf[K];
   double     dt, dUs;
   u_int      i1, i2;
   int        i;

   d._pSvc = pSubSvc;
   d._pTkr = o._tkr;
   d._pErr = (char *)0;
   d._ty   = edg_image;
   d._flds = flds;
   d._nFld = 4;
   d._arg  = o._tag;
   f1._fid = 6;
   f2._fid = 7;
   f3._fid = 8;
   f4._fid = 9;
   f1._type = rtFld_int;
   f2._type = rtFld_int;
   f3._type = rtFld_float;
   f4._type = rtFld_double;
   dt           = rtEdge_TimeNs();
   i1           = (u_int)dt;
   dUs          = ( dt - i1 ) * 1000000.0;
   i2           = (u_int)dUs;
   f1._val._i32 = i1;
   f2._val._i32 = i2;
   f3._val._r32 = -3.14159265358979323846;
   f4._val._r64 = -2.71828182845904523536;
   flds[0]      = f1;
   flds[1]      = f2;
   flds[2]      = f3;
   flds[3]      = f4;
   rtEdge_Publish( pxt, d );
}

/************************
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
   Open o;
   int  i, j;

   fprintf( stdout, "OnPubOpen( %s ) : %d\n", pTkr, (long)tag );
   strcpy( o._tkr, pTkr );
   o._tag         = tag;
   _opens[_nOpen] = o;
   _nOpen        += 1;
#ifdef FOO
   for ( i=0; !_bDone && i<10; i++ ) {
      for ( j=0; j<_nOpen; Pump( _opens[j++] ) );
      rtEdge_Sleep( 1.0 );
   }
#endif /* FOO */
}

void EDGAPI OnPubClose( rtEdge_Context cxt, const char *pTkr )
{
   fprintf( stdout, "OnPubClose( %s )\n", pTkr );
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

void EDGAPI DataHandler( rtEdge_Context cxt, rtEdgeData d )
{
   rtFIELD f1, f2, f3, f4;
   u_int   i1, i2;
   float   r3;
   double  d0, d1, dd, r4; 

   /* Pre-condition */

   if ( !d._nFld )
      return ;
 
   f1 = d._flds[0];
   f2 = d._flds[1];
   f3 = d._flds[2];
   f4 = d._flds[3];
   i1 = f1._val._i32;
   i2 = f2._val._i32;
   r3 = f3._val._r32;
   r4 = f4._val._r64;
   d1 = rtEdge_TimeNs();
   d0 = ( 1.0 * i1 ) + ( i2 / 1000000.0 );
   dd = 1E6 * ( d1-d0 );
   fprintf( stdout, "%.1fuS; PE=%.6f; e=%.6f\n", dd, r3, r4 );
}

void EDGAPI DataHandler_SLOW( rtEdgeData d )
{
   rtFIELD f;
   double  d0, d1, dd;

   if ( !rtEdge_HasFieldByID( cxt, 6 ) )
      return;
   d1 = rtEdge_TimeNs();
   d0 = rtEdge_atof( rtEdge_GetFieldByID( cxt, 6 ) );
   dd = 1E6 * ( d1-d0 );
   fprintf( stdout, "%.1fuS; ", dd );
}

/*************************
 * main()
 ************************/
int main( int argc, char **argv )
{
   rtEdgeAttr     sub;
   rtEdgePubAttr  pub;
   rtEdgeData     d;
   char          *pn, *pDbg, *sb, *pb, *srw, *prw, buf[K];
   double         dSlp;
   int            i, j, nPub, iDbg, iSubRW, iPubRW;

   /* Parse config file */

   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      return 0;
   }
   memset( &sub, 0, sizeof( sub ) );
   memset( &pub, 0, sizeof( pub ) );
   ParseConfigFile( argv[1] );
   sub._pSvrHosts = GetCfgParam( "rtEdgeCache.Hostname" );
   sub._pUsername = GetCfgParam( "rtEdgeCache.Username" );
   sb             = GetCfgParam( "rtEdgeCache.Protocol" );
   srw            = GetCfgParam( "rtEdgeCache.Measure_Latency" );
   sub._connCbk   = ConnHandler; 
   sub._svcCbk    = SvcHandler; 
   sub._dataCbk   = DataHandler;
   pub._pSvrHosts = GetCfgParam( "rtEdgeCachePub.Hostname" );
   pub._pPubName  = GetCfgParam( "rtEdgeCachePub.Service" );
   pb             = GetCfgParam( "rtEdgeCachePub.Protocol" );
   prw            = GetCfgParam( "rtEdgeCachePub.Measure_Latency" );
   pSubSvc        = pub._pPubName;
   pub._connCbk   = PubConnHandler;
   pub._openCbk   = OnPubOpen;
   pub._closeCbk  = OnPubClose;
   pub._bInteractive = 1;
   iSubBin        = sb  && !strcmp( sb,  "BINARY" ) ? 1 : 0;
   iPubBin        = pb  && !strcmp( pb,  "BINARY" ) ? 1 : 0;
   iSubRW         = srw && !strcmp( srw, "YES" ) ? 1 : 0;
   iPubRW         = prw && !strcmp( prw, "YES" ) ? 1 : 0;
   if ( !sub._pSvrHosts || !pSubSvc ) {
      printf( "No rtEdgeCache.Hostname or Service; Exitting ...\n" );
      return 0;
   }
   if ( !pub._pSvrHosts || !pub._pPubName ) {
      printf( "No rtEdgeCachePub.Hostname or Service; Exitting ...\n" );
      return 0;
   }
   if ( !sub._pSvrHosts ) {
      printf( "No rtEdgeCache.Hostname; Exitting ...\n" );
      return 0;
   }
   if ( !pub._pSvrHosts || !pub._pPubName ) {
      printf( "No rtEdgeCachePub.Hostname or Service; Exitting ...\n" );
      return 0;
   }

   /* Set up logging; Initialize library and connect */

   pDbg = (pn=GetCfgParam( "Log.Filename" )) ? pn : "stdout";
   iDbg = (pn=GetCfgParam( "Log.Level" )) ? atoi( pn ) : 0;
   rtEdge_Log( pDbg, iDbg );
   printf( "%s\n", rtEdge_Version() );
   if ( !(cxt=rtEdge_Initialize( sub )) ) {
      printf( "rtEdge_Initialize() failed\n" );
      return 0;
   }
   if ( !(pxt=rtEdge_PubInit( pub )) ) {
      printf( "rtEdge_PubInit() failed\n" );
      return 0;
   }
   if ( iSubBin )
      rtEdge_ioctl( cxt, ioctl_binary, &iSubBin );
   if ( iPubBin )
      rtEdge_ioctl( pxt, ioctl_binary, &iPubBin );
   if ( iSubRW )
      rtEdge_ioctl( cxt, ioctl_measureLatency, &iSubRW );
   if ( iPubRW )
      rtEdge_ioctl( pxt, ioctl_measureLatency, &iPubRW );
   rtEdge_Start( cxt );
   rtEdge_PubStart( pxt );

   /* Wait for user input */

   printf( "Hit <ENTER> to subscribe ...\n" ); getchar();
   rtEdge_Subscribe( cxt, pSubSvc, "LATENCY", 0 );
#if !defined(FOO)
   while( 1 ) {
      for ( j=0; j<_nOpen; Pump( _opens[j++] ) );
      rtEdge_Sleep( 1.0 );
   }
#endif /* !defined(FOO) */
   printf( "Hit <ENTER> to stop ...\n" ); getchar();

   /* Clean up */

   _bDone = 1;
   printf( "Cleaning up ...\n" );
   rtEdge_Destroy( cxt );
   rtEdge_PubDestroy( pxt );
   printf( "Done!!\n " );
   return 0;
}

