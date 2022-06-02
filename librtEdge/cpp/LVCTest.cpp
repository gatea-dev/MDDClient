/******************************************************************************
*
*  LVCTest.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created (from Subscribe.cpp)
*     25 SEP 2017 jcs  Build 35: LVCAdmin
*     12 JAN 2018 jcs  Build 39: main_MEM() 
*     27 JUL 2021 jcs  Build 49: LVC Schema bug
*      1 JUN 2022 jcs  Build 54: svc:tkr[:fid]
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <stdio.h>

using namespace RTEDGE;

static int _IterateAllFields( Message &msg, bool bin )
{
   Field      *fld;
   mddField    f;
   mddValue   &v = f._val;
   const char *str;
   int         nf, fid;

fld = msg.GetField( 32 );
   for ( nf=0; (msg)(); nf++ ) {
      fld = msg.field();
      fid = fld->Fid();
      switch( fld->Type() ) {
         case mddFld_undef:
         case mddFld_real:
         case mddFld_bytestream:
            // Not supported
            break;
         case mddFld_string:
            str = fld->GetAsString();
            break;
         case mddFld_int32:
            v._i32 = fld->GetAsInt32();
            break;
         case mddFld_double:
         case mddFld_date:
         case mddFld_time:
         case mddFld_timeSec:
            v._r64 = fld->GetAsDouble();
            break;
         case mddFld_float:
            v._r32 = fld->GetAsFloat();
            break;
         case mddFld_int8:
            v._i8 = fld->GetAsInt8();
            break;
         case mddFld_int16:
            v._i16 = fld->GetAsInt16();
            break;
         case mddFld_int64:
         case mddFld_unixTime:
            v._i64 = fld->GetAsInt64();
            break;
      }
   }
   return nf;
}

static int IterateAllFields( LVCAll &d, bool bin )
{
   Messages &mdb = d.msgs();
   int       i, nf;

   for ( i=0,nf=0; i<d.Size(); nf+=_IterateAllFields( *mdb[i++], bin ) );
   return nf;
}

//////////////////////////
// main()
//////////////////////////
static void _ProgressBar( LVC &lvc, int i, int nd )
{
   if ( i && !( i%nd ) ) {
      printf( "[%06d] : Mem=%dKb\n", i+1, lvc.MemSize() );
//      printf( "." );
   }
   fflush( stdout );
}

int main_MEM( int argc, char **argv )
{
   // cmd-line args

   if ( argc < 3 ) {
      printf( "Usage : %s <LVCfile> <numSnap>; Exitting ...\n", argv[0] );
      return 0;
   }

   int    i, n, nd, nt;
   double d0, dd;

   n  = WithinRange( 1, atoi( argv[2] ), K*K );
   nd = gmin( n / 10, 100 );
/*
   printf( "1st test : Hit <ENTER> to start" );
   fflush( stdout ); getchar();
 */
   d0 = rtEdge::TimeNs();
   {
      LVC lvc( argv[1] );

      for ( i=0; i<n; nt=lvc.ViewAll().Size(), _ProgressBar( lvc, i, nd ), i++ );
   }
   dd = rtEdge::TimeNs() - d0;
   printf( "1st test : %d snaps; %d tkrs in %.1fs\n", i, nt, dd );
/*
   printf( "2nd test : Hit <ENTER> to start" );
   fflush( stdout ); getchar();
 */
   d0 = rtEdge::TimeNs();
   for ( i=0; i<n; i++ ) {
      LVC lvc( argv[1] );

      lvc.GetSchema();
      nt = lvc.ViewAll().Size();
      _ProgressBar( lvc, i, nd );
   }
   dd = rtEdge::TimeNs() - d0;
   printf( "2nd test : %d snaps; %d tkrs in %.1fs\n", i, nt, dd );
/*
   printf( "Hit <ENTER> to terminate" );
   fflush( stdout ); getchar();
 */
   return 1;
}

int main_CACHE( int argc, char **argv )
{
   // cmd-line args

   if ( argc < 2 ) {
      printf( "Usage : %s <LVCfile> [<SVC:TKR[:FID]>]; Exitting ...\n", argv[0] );
      return 0;
   }

   LVC         lvc( argv[1] );
   LVCAll     &all = lvc.ViewAll();
   Schema     &sch = lvc.GetSchema();
   Message    *msg;
   Field      *fld;
   const char *ty;
   double      dMs, d0, dd;
   char       *svc, *tkr, *pFld, *rp;
   bool        bin;
   int         sz, fid;

   svc = (char *)0;
   tkr = (char *)0;
   fid = 0;
   if ( argc > 2 ) {
      svc  = ::strtok_r( argv[2], ":", &rp );
      tkr  = ::strtok_r( NULL,    ":", &rp );
      pFld = ::strtok_r( NULL,    ":", &rp );
      fid  = pFld ? atoi( pFld ) : 0;
   }
   printf( "Schema : %d fields\n", sch.Size() );
/*
   for ( sch.reset(); (sch)(); )
      printf( "[%04d] %s\n", sch.field()->Fid(), sch.field()->Name() );
 */
   if ( svc && tkr && (msg=lvc.Snap( svc, tkr )) ) {
      if ( fid && (fld=msg->GetField( fid )) )
         ::fprintf( stdout, fld->Dump( true ) );
      else
         ::fprintf( stdout, msg->Dump() );
   }
   dMs = 1000.0 * all.dSnap();
   bin = all.IsBinary();
   ty  = bin ? "BIN" : "MF";
   sz  = all.Size();
   d0  = lvc.TimeNs();
   IterateAllFields( all, bin );
   dd  = 1000.0 * ( lvc.TimeNs() - d0 );
   printf( "[%s] SnapAll( %d ) : %.3fmS; Iterate : %.3fmS\n", ty, sz, dMs, dd );
   printf( "Done!!\n " );
   fflush( stdout );
   return 1;
}

int main_ADMIN( int argc, char **argv )
{
   // cmd-line args

   if ( argc < 3 ) {
      printf( "Usage : %s <host:port> <ADD|DEL>; Exitting ...\n", argv[0] );
      return 0;
   }

   const char *ty;
   char        buf[K], *cmd, *svc, *tkr;
   bool        add;

   add = !::strcmp( argv[2], "ADD" );
   ty  = add ? "ADD" : "DEL";
   printf( "Enter <Ticker or File> to %s ...\n", ty );
   while( ::fgets( buf, K, stdin ) ) {
      LVCAdmin    adm( argv[1] );

      ::strtok( buf, "\n" );
      cmd = ::strtok( buf,  " " );
      if ( !(svc=::strtok( NULL, " " )) )
         continue; // while
      if ( !(tkr=::strtok( NULL, " " )) )
         continue; // while
      printf( "%s_Ticker)( %s, %s )\n", cmd, svc, tkr );
      if ( add )
         adm.AddTicker( svc, tkr );
      else
         adm.DelTicker( svc, tkr );
   }
   return 1;
}

int main( int argc, char **argv )
{
//   return main_ADMIN( argc, argv );
//   return main_MEM( argc, argv );
   return main_CACHE( argc, argv );
}

