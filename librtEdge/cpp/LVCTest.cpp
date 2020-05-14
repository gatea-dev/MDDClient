/******************************************************************************
*
*  LVCTest.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created (from Subscribe.cpp)
*     25 SEP 2017 jcs  Build 35: LVCAdmin
*     12 JAN 2018 jcs  Build 39: main_MEM() 
*     11 FEB 2020 jcs  Build 42: LVC Schema bug
*
*  (c) 1994-2020 Gatea, Ltd.
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
static void _ProgressBar( int i, int nd )
{
   if ( i && !( i%nd ) )
      printf( "." );
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

   n = WithinRange( 1, atoi( argv[2] ), K*K );
   printf( "1st test : Hit <ENTER> to start" );
   fflush( stdout ); getchar();
   d0 = rtEdge::TimeNs();
   {
      LVC lvc( argv[1] );

      nd = n / 10;
      for ( i=0; i<n; nt=lvc.ViewAll().Size(), _ProgressBar( i, nd ), i++ );
   }
   dd = rtEdge::TimeNs() - d0;
   printf( "1st test : %d snaps; %d tkrs in %.1fs\n", i, nt, dd );
   printf( "2nd test : Hit <ENTER> to start" );
   fflush( stdout ); getchar();
   d0 = rtEdge::TimeNs();
   for ( i=0; i<n; i++ ) {
      LVC lvc( argv[1] );

      lvc.GetSchema();
      nt = lvc.ViewAll().Size();
      _ProgressBar( i, nd );
   }
   dd = rtEdge::TimeNs() - d0;
   printf( "2nd test : %d snaps; %d tkrs in %.1fs\n", i, nt, dd );
   printf( "Hit <ENTER> to terminate" );
   fflush( stdout ); getchar();
   return 1;
}

int main_CACHE( int argc, char **argv )
{
   // cmd-line args

   if ( argc < 2 ) {
      printf( "Usage : %s <LVCfile> [<SVC:TKR>]; Exitting ...\n", argv[0] );
      return 0;
   }

   LVC         lvc( argv[1] );
   LVCAll     &all = lvc.ViewAll();
   Schema     &sch = lvc.GetSchema();
   Message    *msg;
   const char *ty;
   double      dMs, d0, dd;
   char       *svc, *tkr, *rp;
   bool        bin;
   int         sz;

   svc = (char *)0;
   tkr = (char *)0;
   if ( argc > 2 ) {
      svc = ::strtok_r( argv[2], ":", &rp );
      tkr = ::strtok_r( NULL,    ":", &rp );
   }
   printf( "Schema : %d fields\n", sch.Size() );
   for ( sch.reset(); (sch)(); )
      printf( "[%04d] %s\n", sch.field()->Fid(), sch.field()->Name() );
   if ( svc && tkr && (msg=lvc.Snap( svc, tkr )) )
      ::fprintf( stdout, msg->Dump() );
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

   if ( argc < 2 ) {
      printf( "Usage : %s <host:port>; Exitting ...\n", argv[0] );
      return 0;
   }

   LVCAdmin adm( argv[1] );
   char     buf[K], *cmd, *svc, *tkr;

   printf( "Enter <ADD Service Ticker> or <DEL Service Ticker>" );
   while( ::fgets( buf, K, stdin ) ) {
      ::strtok( buf, "\n" );
      cmd = ::strtok( buf,  " " );
      if ( !(svc=::strtok( NULL, " " )) )
         continue; // while
      if ( !(tkr=::strtok( NULL, " " )) )
         continue; // while
      printf( "%s_Ticker)( %s, %s )\n", cmd, svc, tkr );
      if ( !::strcmp( cmd, "ADD" ) )
         adm.AddTicker( svc, tkr );
      else
         adm.DelTicker( svc, tkr );
   }
   return 1;
}

int main( int argc, char **argv )
{
//   return main_MEM( argc, argv );
   return main_CACHE( argc, argv );
}

