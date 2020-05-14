/******************************************************************************
*
*  test.c
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*      3 SEP 2010 jcs  Build  6: LVC
*     30 DEC 2010 jcs  Build  9: LVC_SnapAll()
*     13 JAN 2011 jcs  Build 10: LVC_ViewAll()
*     12 JUL 2011 jcs  Build 14: rtEdge_GetField()
*     29 JUL 2011 jcs  Build 15: _schemaCbk
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*      4 AUG 2012 jcs  Build 19a:EDGAPI
*     17 NOV 2012 jcs  Build 20: rtEdge_CPU()
*      9 MAY 2013 jcs  Build 25: _bRate
*     13 SEP 2014 jcs  Build 28: libmddWire; cxt in callbacks
*
*  (c) 1994-2014 Gatea Ltd.
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

void ShowLVC( LVCData d )
{
   const char *ty; 
   double      dMs, dSnp;
   rtFIELD     f;      
   rtBUF       b;      
   int         i;      
 
   dMs  = d._dAge  * 1000.0;
   dSnp = d._dSnap * 1000000.0;
   ty   = pType( d._ty );
   printf( "[LVC-%s] (%.1fuS) [%s,%s] ", ty, dSnp, d._pSvc, d._pTkr );
   printf( "%04d fields; Age=%.1fmS\n", d._nFld, dMs );
   for ( i=0; i<d._nFld; i++ ) {
      f = d._flds[i]; 
      b = f._val._buf;
      printf( "[%s,%s,%04d,%-24s] %s\n", d._pSvc, d._pTkr, f._fid, f._name, b._data );
   }
}

rtEdge_Context cxt = 0;
int            nFid = 0;
char          *eFids[K];

void ShowEdgeFields( rtEdgeData d )
{
   rtFIELD f;
   rtBUF   b;
   char   *pf;
   int     i;

   if ( d._ty == edg_recovering )
      return;
   printf( "%d fields\n", d._nFld );
   for ( i=0; i<nFid; i++ ) {
      pf = eFids[i];
      fprintf( stdout, "   [%-10s] : ", pf );
      f = rtEdge_GetField( cxt, pf );
      b = f._val._buf;
      if ( b._data )
         fwrite( b._data, b._dLen, 1, stdout );
      else
         fprintf( stdout, "Not found" );
      fprintf( stdout, "\n" );
      fflush( stdout );
   }
}

/*************************
 * Callbacks
 ************************/
void EDGAPI ConnHandler( rtEdge_Context cxt, 
                         const char    *pConn, 
                         rtEdgeState    s ) 
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "%s %s\n", pConn, ty );
}

void EDGAPI SvcHandler( rtEdge_Context cxt, 
                        const char    *pSvc,
                        rtEdgeState    s )
{
   const char *ty;

   ty = ( s == edg_up ) ? "UP  " : "DOWN";
   fprintf( stdout, "%s %s\n", pSvc, ty );
}

static int    _nUpd  = 0;
static char   _bRate = 0;
static double _dLife = 0.0;
static char   _bDict = 0;
static FILE  *_byLog = (FILE *)0;

void EDGAPI DataHandler( rtEdge_Context cxt, rtEdgeData d )
{
   const char *ty;
   rtFIELD     f;
   rtVALUE     v;
   rtBUF       b;
   int         i;

   /* 1 line per Image if calculating rates */

   if ( _bRate ) {
      switch( d._ty ) {
         case edg_image:
            ty = pType( d._ty );
            fprintf( stdout, "[%s] [%s,%s]\n", ty, d._pSvc, d._pTkr );
            break;
         case edg_update:
            _nUpd += 1;
            break;
      }
      return;
   }

   /* Else, Dump all */

   ty = pType( d._ty );
   fprintf( stdout, "[%s] [%s,%s] : ", ty, d._pSvc, d._pTkr );
   if ( d._ty == edg_dead )
      printf( "%s\n", d._pErr );
   else if ( nFid )
      ShowEdgeFields( d );
   else {
      printf( "%d fields\n", d._nFld );
      for ( i=0; i<d._nFld; i++ ) {
         f = d._flds[i];
         v = f._val;
         b = v._buf;
         fprintf( stdout, "   [%4d,%-24s] : ", f._fid, f._name );
         switch( f._type ) {
            case rtFld_undef:
               fprintf( stdout, "(undf)  " );
               break;
            case rtFld_string:
               fprintf( stdout, "(str)  " );
               fwrite( b._data, b._dLen, 1, stdout );
               break;
            case rtFld_int:
               fprintf( stdout, "(int)  %d", v._i32 );
               break;
            case rtFld_float:
               fprintf( stdout, "(dbl)  %.3f", v._r32 );
               break;
            case rtFld_double:
               fprintf( stdout, "(dbl)  %.6f", v._r64 );
               break;
            case rtFld_time:
            case rtFld_timeSec:
               fprintf( stdout, "(time) %.3f", v._r64 );
               break;
            case rtFld_date:
               fprintf( stdout, "(date) %.3f", v._r64 );
               break;
            case rtFld_int16:
               fprintf( stdout, "(i16)  %d", v._i16 );
               break;
            case rtFld_int64:
#if !defined(WIN32)
               fprintf( stdout, "(i64)  %lld", v._i64 );
#else
               fprintf( stdout, "(i64)  %I64d", v._i64 );
#endif
               break;
            case rtFld_bytestream:
               fprintf( stdout, "(bstr) %d bytes", b._dLen );
               if ( !_byLog ) 
                  _byLog = fopen( "./byte.log", "wb" );
               fwrite( b._data, b._dLen, 1, _byLog );
               fflush( _byLog );
               break;
         }
         fprintf( stdout, "\n" );
         fflush( stdout );
      } 
   } 
}

void EDGAPI SchemaHandler( rtEdge_Context cxt, rtEdgeData d )
{
   const char *ty = pType( d._ty );
   rtFIELD     f;
   rtVALUE     v;
   rtBUF       b;
   int         i;

   fprintf( stdout, "DATA DICT : %d fields\n", d._nFld );
   for ( i=0; _bDict && !_bRate && i<d._nFld; i++ ) {
      f = d._flds[i];
      v = f._val;
      b = v._buf;
      fprintf( stdout, "   [%4d] : ", f._fid );
      fwrite( b._data, b._dLen, 1, stdout );
      fprintf( stdout, "\n" );
      fflush( stdout );
   }
}


/*************************
 * main()
 ************************/
main( int argc, char **argv )
{
   LVCData         d;
   LVCDataAll      da;
   rtFIELD         f;
   LVC_Context     lxt = 0;
   rtEdgeAttr      attr;
   rtEdgeChanStats st;
   char           *pn, *pN, *pDbg, bFull;
   const char     *pc, *pl, *pa, *pr, *pA, *pb, *ty, *pd, *ll;
   double          dSlp, dAll;
   char           *svcs[K], *rics[K], *pFids;
   char            sTkr[K], *cp;
   char            bEdg, bBin, bLVC, bLVCAll;
   int             i, j, k, l, nSvc, nTkr, iDbg, iNative, iBinary, nu;
   FILE           *fp;

   /* Parse config file */

   if ( argc > 1 && !strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", rtEdge_Version() );
      return 0;
   }
   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      exit( 0 );
   }
   ParseConfigFile( argv[1] );
   attr._pSvrHosts = GetCfgParam( "rtEdgeCache.Hostname" );
   attr._pUsername = GetCfgParam( "rtEdgeCache.Username" );
   nFid            = GetCsvList( "rtEdgeCache.Fields", eFids );
   pN              = GetCfgParam( "rtEdgeCache.Native" );
   pr              = GetCfgParam( "rtEdgeCache.CalcRate" );
   ll              = GetCfgParam( "rtEdgeCache.Lifetime" );
   pb              = GetCfgParam( "rtEdgeCache.Protocol" );
   pd              = GetCfgParam( "libmddWire.Driver" );
   pl              = GetCfgParam( "LVC.Filename" );
   pa              = GetCfgParam( "LVC.AdminChannel" );
   pA              = GetCfgParam( "LVC.SnapAll" );
   dSlp            = (pc=GetCfgParam( "LVC.Refresh" )) ? atof( pc ) : 0.0;
   pFids           = GetCfgParam( "LVC.Fields" );
   attr._connCbk   = ConnHandler; 
   attr._svcCbk    = SvcHandler; 
   attr._dataCbk   = DataHandler; 
   attr._schemaCbk = SchemaHandler;
   bEdg            = ( attr._pSvrHosts != (char *)0 );
   bLVC            = ( pl != (char *)0 );
   bLVCAll         = bLVC && pA && !strcmp( pA, "YES" );
   iNative         = pN && !strcmp( pN, "YES" ) ? 1 : 0;
   iBinary         = pb && !strcmp( pb, "BINARY" ) ? 1 : 0;
   _bRate          = pr && !strcmp( pr, "YES" ) ? 1 : 0;
   _dLife          = ll ? atof( ll ) : 0.0;
   if ( !bEdg && !bLVC ) {
      printf( "No rtEdgeCache.Hostname or LVC.Filename : %s; Exitting ...\n" );
      exit( 0 );
   }

   /* Set up logging; Initialize library and connect */

   pDbg = (pn=GetCfgParam( "Log.Filename" )) ? pn : "stdout";
   iDbg = (pn=GetCfgParam( "Log.Level" )) ? atoi( pn ) : 0;
   rtEdge_Log( pDbg, iDbg );
   printf( "%s\n", rtEdge_Version() );
   if ( bLVC ) {
      if ( !(lxt=LVC_Initialize( pl, pa )) ) {
         printf( "Can't initialize LVC_Context\n" );
         exit( 0 );
      }
      LVC_SetFilter( lxt, pFids );
   }
   if ( bEdg ) {
      if ( !(cxt=rtEdge_Initialize( attr )) ) {
         printf( "Can't initialize rtEdge_Context\n" );
         exit( 0 );
      }
      if ( iNative )
         rtEdge_ioctl( cxt, ioctl_nativeField, &iNative );
      if ( iBinary )
         rtEdge_ioctl( cxt, ioctl_binary, &iBinary );
      if ( pd )
         rtEdge_ioctl( cxt, ioctl_fixedLibrary, (void *)pd );
      printf( "%s Connected\n", (pc=rtEdge_Start( cxt )) ? pc : "NOT " );
   }

   /* Open Items - Allow for read from file ... */

   nSvc = GetCsvList( "Test.Services", svcs );
   nTkr = GetCsvList( "Test.Instruments", rics );
   if ( bEdg ) {
      if ( nTkr == 1 && (fp=fopen( rics[0], "r" )) ) {
         for ( i=0; i<nSvc; i++ ) {
            while( fgets( (cp=sTkr), K, fp ) ) {
               cp += ( strlen( sTkr ) - 1 );
               if ( ( *cp == 0x0d ) || ( *cp == 0x0a ) )
                  *cp = '\0';
               if ( (cp=strchr( sTkr, '#' )) )
                  *cp = '\0';
               if ( strlen( sTkr ) )
                  rtEdge_Subscribe( cxt, svcs[i], sTkr, (void *)0 );
            }
         }
         fclose( fp );
      }
      for ( i=0; i<nSvc; i++ ) {
         for ( j=0; j<nTkr; j++ )
            rtEdge_Subscribe( cxt, svcs[i], rics[j], (void *)0 );
      }
   }
   bFull = 0;
   do {
      if ( bLVC ) {
         if ( !(lxt=LVC_Initialize( pl, pa )) ) {
            printf( "Can't initialize LVC_Context\n" );
            exit( 0 );
         }
         LVC_SetFilter( lxt, pFids );
      }
      if ( bLVCAll ) {
#ifdef OBSOLETE
         da   = LVC_SnapAll( lxt );
         dAll = da._dSnap * 1000000.0;
/*
         for ( i=0; i<da._nTkr; ShowLVC( da._tkrs[i++] ) );
 */
         printf( "[%d] LVC_SnapAll( %d tkrs ): (%.1fuS)\n", bFull, da._nTkr, dAll );
         LVC_FreeAll( &da );
#endif // OBSOLETE
         da   = LVC_ViewAll( lxt );
         dAll = da._dSnap * 1000000.0;
         for ( i=0,nu=0; i<da._nTkr; nu+=da._tkrs[i++]._nUpd );
         printf( "[%d] LVC_ViewAll( %d tkrs ): nUpd=%d (%.1fuS)\n", 
               bFull, da._nTkr, nu, dAll );
         LVC_FreeAll( &da );
         bFull = !bFull ? 1 : 0;
      }
      else if ( bLVC ) {
         for ( i=0; i<nSvc; i++ ) {
            for ( j=0; j<nTkr; j++ ) {
               d = LVC_Snapshot( lxt, svcs[i], rics[j] );
               ShowLVC( d );
               LVC_Free( &d );
            }
         }
      }
      LVC_Destroy( lxt );
      if ( dSlp )
         rtEdge_Sleep( dSlp );
   } while( bLVC && dSlp );

   /* Wait for user input */

   if ( bEdg && !bLVC ) {
      while( _bRate ) {
         fprintf( stdout, "%d,%.3f\n", _nUpd, rtEdge_CPU() );
         fflush( stdout );
         rtEdge_Sleep( 15.0 );
      }
      if ( !_bRate ) {
         if ( _dLife ) {
            printf( "Running for %.3fs ...", _dLife );
            rtEdge_Sleep( _dLife );
         }
         else {
            printf( "Hit <ENTER> to stop ...\n" );
            getchar();
         }
      }
   }

   /* Clean up */

   printf( "Cleaning up ...\n" );
   rtEdge_Destroy( cxt );
   printf( "Done!!\n " );
   return 0;
}
