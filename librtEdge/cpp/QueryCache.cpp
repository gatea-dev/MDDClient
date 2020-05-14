/******************************************************************************
*
*  QueryCache.cpp
*
*  REVISION HISTORY:
*     13 NOV 2014 jcs  Created (from Subscribe.cpp)
*
*  (c) 1994-2014 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <stdio.h>

static void *arg_ = (void *)"User-supplied argument";

class MyChannel : public RTEDGE::SubChannel
{
private:
   int _nUpd;

   //////////////////////////
   // Constructor
   //////////////////////////
public:
   MyChannel() :
      _nUpd( 0 )
   { ; }
   
   //////////////////////////
   // Asynchronous Notifications
   //////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bOK )
   {
      ::fprintf( stdout, "OnConnect( %s,%sOK )\n", msg, bOK ? "" : "NOT " );
      _flush();
   }

   virtual void OnService( const char *msg, bool bOK )
   {
      ::fprintf( stdout, "OnService( %s,%sOK )\n", msg, bOK ? "" : "NOT " );
      _flush();
   }

   virtual void OnData( RTEDGE::Message & )
   {
      _nUpd += 1;
   }


   //////////////////////////
   // Operations
   //////////////////////////
   void Snap( const char *svc, const char *tkr )
   {
      RTEDGE::Message *msg;

      if ( !(msg=QueryCache( svc, tkr )) )
         ::fprintf( stdout, "Not enabled ...\n" );
      else
         Dump( *msg );
   }

   //////////////////////////
   // Private helpers
   //////////////////////////
private:
   void Dump( RTEDGE::Message &msg )
   {
      std::string    s;
      const char    *tm, *svc, *tkr;
      RTEDGE::Field *f;
      rtFIELD        fld;
      rtVALUE       &v = fld._val;
      rtBUF         &b = v._buf;
      int            nf;

      tm  = pDateTimeMs( s );
      svc = msg.Service();
      tkr = msg.Ticker();
      nf  = msg.NumFields();
      ::fprintf( stdout, "[%s] Dump( %s,%s ) nUpd=%d\n", tm, svc, tkr, _nUpd );
      for ( msg.reset(); (f=(msg)()); ) {
         fld = f->field();
         ::fprintf( stdout, "   [%04d] : ", f->Fid() );
         ::fwrite( b._data, b._dLen, 1, stdout );
         _flush( "\n" );
      }
      _flush();
   }

   void _flush( char *msg="" )
   {
      fprintf( stdout, msg );
      fflush( stdout );
   }
};

//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   MyChannel   ch;
   const char *pc;
   int         i;

   // cmd-line args

   if ( argc < 5 ) {
      pc = "Usage: %s <hosts> <user> <svc> <tkr1> [tkr2,...]; Exitting ...\n";
      printf( pc, argv[0] );
      return 0;
   }
   ch.EnableCache();
   printf( "%s\n", ch.Version() );
   pc = ch.Start( argv[1], argv[2] );
   printf( "%s\n", pc ? pc : "" );
   if ( !ch.IsValid() )
      return 0;

   // Open Items; Snap 3 times

   for ( i=4; i<argc; ch.Subscribe( argv[3], argv[i++], arg_ ) );
   printf( "Hit <ENTER> to Query Cache ...\n" ); getchar();
   for ( i=4; i<argc; ch.Snap( argv[3], argv[i++] ) );
   printf( "Hit <ENTER> to Query Cache ...\n" ); getchar();
   for ( i=4; i<argc; ch.Snap( argv[3], argv[i++] ) );
   printf( "Hit <ENTER> to Query and exit ...\n" ); getchar();
   for ( i=4; i<argc; ch.Snap( argv[3], argv[i++] ) );

   // Clean up

   printf( "Cleaning up ...\n" );
   ch.Stop();
   printf( "Done!!\n " );
   return 1;
}

