/******************************************************************************
*
*  test.c
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     12 NOV 2014 jcs  Build  8: -Wall
*     23 MAY 2022 jcs  Build 14: mddFld_unixTime
*     28 OCT 2022 jcs  Build 16: mddFld_vector / mddFld_surface
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <libmddWire.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/*************************
 * config.c
 ************************/
extern void  ParseConfigFile( char * );
extern char *GetCfgParam( char * );
extern int   GetCsvList( char *, char ** );


/*************************
 * Helpers
 ************************/
const char *pType( mddMsgType ty )
{
   switch( ty ) {
      case mddMt_image:      return "IMG ";
      case mddMt_update:     return "UPD ";
      case mddMt_dead:       return "DEAD";
      case mddMt_stale:      return "STALE";
      case mddMt_recovering: return "RECOVERING";
      case mddMt_undef:
      case mddMt_mount:
      case mddMt_ping:
      case mddMt_ctl:
      case mddMt_open:
      case mddMt_close:
      case mddMt_query:
      case mddMt_insert:
      case mddMt_insAck:
      case mddMt_gblStatus:
      case mddMt_history:
      case mddMt_dbQry:
      case mddMt_dbTable:
         break;
   }
   return "UNKNOWN";
}

mddWire_Context cxt = 0;

void ShowFieldList( mddFieldList d )
{
   mddField f;
   mddValue v;
   mddBuf   b;
   int      i, nf;

   nf = d._nFld;
   printf( "%d fields\n", nf );
   for ( i=0; i<nf; i++ ) {
      f = d._flds[i];
      v = f._val;
      b = v._buf;
      fprintf( stdout, "   [%4d,%-24s] : ", f._fid, f._name );
      switch( f._type ) {
         case mddFld_undef:
         case mddFld_string:
         case mddFld_date:
            fprintf( stdout, "(str) " );
            fwrite( b._data, b._dLen, 1, stdout );
            break;
         case mddFld_int32:
            fprintf( stdout, "(int) %d", v._i32 );
            break;
         case mddFld_float:
            fprintf( stdout, "(d32) %.3f", v._r32 );
            break;
         case mddFld_double:
            fprintf( stdout, "(dbl) %.6f", v._r64 );
            break;
         case mddFld_time:
         case mddFld_timeSec:
            fprintf( stdout, "(dbl) %.3f", v._r64 );
            break;
         case mddFld_int8:
         case mddFld_int16:
         case mddFld_int64:
         case mddFld_real:
         case mddFld_bytestream:
         case mddFld_vector:
         case mddFld_unixTime:
            break;
      }
      fprintf( stdout, "\n" );
      fflush( stdout );
   } 
}


/*************************
 * main()
 ************************/
int main_OBSOLETE( int argc, char **argv )
{
   char *pn, *pDbg;
   int   iDbg;

   /* Parse config file */

   if ( argc < 2 ) {
      printf( "Usage: %s <config_file>; Exitting ...\n", argv[0] );
      exit( 0 );
   }
   ParseConfigFile( argv[1] );

   /* Set up logging; Initialize library and connect */

   pDbg = (pn=GetCfgParam( "Log.Filename" )) ? pn : "stdout";
   iDbg = (pn=GetCfgParam( "Log.Level" )) ? atoi( pn ) : 0;
   mddWire_Log( pDbg, iDbg );
   printf( "%s\n", mddWire_Version() );
   if ( !(cxt=mddSub_Initialize()) ) {
      printf( "Can't initialize mddWire_Context\n" );
      exit( 0 );
   }

   /* Wait for user input */

   printf( "Hit <ENTER> to stop ...\n" );
   getchar();

   /* Clean up */

   printf( "Cleaning up ...\n" );
   mddSub_Destroy( cxt );
   printf( "Done!!\n " );
   return 0;
}

int main( int argc, char **argv )
{
   mddWire_Context mdd;
   mddMsgHdr   h;
   mddMsgBuf   b;
   char       *bp, *cp;
   const char *ty;
   FILE       *fp;
   struct stat st;
   int         i, fSz, rSz, nL, nb;

   if ( argc < 2 ) {
      printf( "Usage: %s <filename>; Exitting ...\n", argv[0] );
      return 0;
   }
   if ( stat( argv[1], &st ) != 0 ) {
      printf( "Can't stat( %s )\n", argv[1] );
      return 0;
   }
   if ( !(fp=fopen( argv[1], "rb" )) ) {
      printf( "Can't fopen( %s )\n", argv[1] );
      return 0;
   }
   fSz = st.st_size;
   bp  = malloc( fSz+100 );
   memset( bp, 0, fSz+100 );
   rSz = fread( bp, 1, fSz, fp );
   printf( "%d of %d read in\n", rSz, fSz );
   mdd = mddSub_Initialize();
   mddWire_SetProtocol( mdd, mddProto_Binary );
   cp  = bp;
   nL  = rSz - ( cp-bp );
   nb  = 1;
   for ( i=0,cp=bp; nL && nb; i++ ) {
      memset( &h, 0, sizeof( h ) );
      b._data = cp;
      b._dLen = nL;
      b._hdr  = (mddMsgHdr *)0;
      nb      = mddSub_ParseHdr( mdd, b, &h );
      cp     += nb;
      nL     -= nb;
      switch( h._mt ) {
         case mddMt_image:      ty = "image     "; break;
         case mddMt_update:     ty = "update    "; break;
         case mddMt_stale:      ty = "stale     "; break;
         case mddMt_recovering: ty = "recovering"; break;
         case mddMt_dead:       ty = "dead      "; break;
         case mddMt_mount:      ty = "mount     "; break;
         case mddMt_ping:       ty = "ping      "; break;
         case mddMt_ctl:        ty = "ctl       "; break;
         case mddMt_open:       ty = "open      "; break;
         case mddMt_close:      ty = "close     "; break;
         case mddMt_query:      ty = "query     "; break;
         case mddMt_insert:     ty = "insert    "; break;
         case mddMt_insAck:     ty = "insAck    "; break;
         case mddMt_gblStatus:  ty = "gblStatus "; break;
         case mddMt_history:    ty = "historyi  "; break;
         case mddMt_dbQry:      ty = "dbQry     "; break;
         case mddMt_dbTable:    ty = "dbTable   "; break;
         default:               ty = "Undefined "; break;
      }
      printf( "[%06d] mSz=%03d nL=%09ld : %s\n", i, nb, cp-bp, ty );
   }
   free( bp );
   fclose( fp );
   mddSub_Destroy( mdd );
   return 0;
}
