/******************************************************************************
*
*  RoundTrip.cpp
*
*  REVISION HISTORY:
*     17 FEB 2016 jcs  Created (from Publish.cpp).
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <math.h>
#include <stdarg.h>

#define _TIME_FID  6
#define _MIKE      1000000.0

using namespace RTEDGE;


////////////////////////
//
//     M y S u b
//
////////////////////////
class MySub : public SubChannel
{
   ////////////////////////////////
   // Asynchronous Notifications
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      _fprintf( "SUB-CONN %s : %s", pUp, msg );
   }

   virtual void OnService( const char *msg, bool bOK )
   {
      const char *ty = bOK ? "" : "NOT ";

      _fprintf( "PUB-SVC( %s ) %sOK", msg, ty );
   }

   virtual void OnData( Message &msg )
   {
      Field *f;
      double d0, d1;
      int    fid, sec, uS;

      fid = _TIME_FID;
      sec = (f=msg.GetField( fid++ )) ? f->GetAsInt32() : 0;
      uS  = (f=msg.GetField( fid++ )) ? f->GetAsInt32() : 0;
      if ( sec && uS ) {
         d0 = ( uS / _MIKE ) + sec;
         d1 = TimeNs();
         _fprintf( "%.1fuS", ( d1-d0 ) * _MIKE );
      }
   }

   virtual void OnDead( Message &msg, const char *err )
   {
      const char *svc, *tkr, *mt;

      mt  = msg.MsgType();
      svc = msg.Service();
      tkr = msg.Ticker();
      _fprintf( "%s ( %s,%s ) :%s", mt, svc, tkr, err );
   }

   virtual void OnSchema( Schema &sch )
   {
      _fprintf( "SUB-OnSchema() nFld=%d", sch.Size() );
   }

   //////////////////////////
   // Class-wide
   //////////////////////////
public:
   static void _fprintf( char *fmt, ... )
   {
      va_list     ap;
      std::string s;
      char        obuf[2*K], *cp;

      cp  = obuf;
      cp += sprintf( cp, "[%s] ", pDateTimeMs( s ) );
      va_start( ap,fmt );
      cp += vsprintf( cp, fmt, ap );
      cp += sprintf( cp, "\n" );
      va_end( ap );
      *cp = '\0';
      ::fwrite( obuf, cp-obuf, 1, stdout );
      ::fflush( stdout );
   }
};


////////////////////
//
//  W a t c h
//
////////////////////
class Watch
{
public:
   std::string _tkr;
   void       *_arg;
   int         _rtl;

   // Constructor
public:
   Watch( const char *tkr, void *arg ) :
      _tkr( tkr ),
      _arg( arg ),
      _rtl( 1 )
   { ; }

   // Access
public:
   const char *tkr()
   {
      return _tkr.c_str();
   }
};



////////////////////////
//
//     M y P u b
//
////////////////////////

typedef std::map<std::string, Watch *> WatchList;

class MyPub : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   WatchList _wl;
   Mutex     _mtx;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyPub( const char *pPub ) :
      PubChannel( pPub ),
      _wl(),
      _mtx()
   {
      SetUserPubMsgTy( true );
      SetIdleCallback( true );
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
public:
   int PublishAll()
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      Watch              *w;

      // Not synchronized; Quick Hack

      for ( it=_wl.begin(); it!=_wl.end(); it++ ) {
         w = (*it).second;
         PubTkr( *w );
      }
      return _wl.size();
   }

   void PubTkr( Watch &w )
   {
      Locker  lck( _mtx );
      Update &u = upd();
      double  dd, dr;
      int     fid, sec, uS;

      fid = _TIME_FID;
      u.Init( w.tkr(), w._arg, true );
      dd  = TimeNs();
      sec = (int)dd;
      dr  = _MIKE * ( dd - sec );
      uS  = (int)dr;
      u.AddField( fid++, sec );
      u.AddField( fid++, uS );
      u.AddField( fid++, w._rtl );
      u.AddField( fid++, (double)-3.14159265358979323846 );
      u.AddField( fid++, (double)-2.71828182845904523536 );
      u.Publish();
      w._rtl += 1;
      MySub::_fprintf( "PUB %.6f", ::fmod( dd, 60.0 ) );
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      MySub::_fprintf( "PUB-CONN %s : %s", pUp, msg );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      Watch              *w;
      std::string         s( tkr ), tmp( tkr );

      MySub::_fprintf( "PUB-OPEN %s", tkr );
      if ( (it=_wl.find( s )) == _wl.end() ) {
         w      = new Watch( tkr, arg );
         _wl[s] = w;
      }
      else
         w = (*it).second;
      PubTkr( *w );
   }

   virtual void OnPubClose( const char *tkr )
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      std::string         s( tkr );

      MySub::_fprintf( "CLOSE %s", tkr );
      if ( (it=_wl.find( s )) != _wl.end() ) {
         _wl.erase( it );
         delete (*it).second;
      }
   }

   virtual void OnIdle()
   {
      PublishAll();
   }

   ////////////////////////////////////
   // PubChannel Interface
   ////////////////////////////////////
protected:
   virtual Update *CreateUpdate()
   {
      return new Update( *this );
   }
};

int main( int argc, char **argv )
{
   const char *pPub, *pSvc, *pSub, *pUsr, *pc;
   bool        bBin;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }

   // cmd-line args

   if ( argc < 5 ) {
      pc = "Usage: %s <PubHost> <PubSvc> <SubHost> <user> [<bMF>]";
      printf( pc, argv[0] );
      printf( "; Exitting ...\n" );
      return 0;
   }
   pPub = argv[1];
   pSvc = argv[2];
   pSub = argv[3];
   pUsr = argv[4];
   bBin = ( argc < 6 );
   pc   = bBin ? "BIN-" : "MF-";

   MyPub pub( pSvc );
   MySub sub;

   pub.SetBinary( bBin );
   sub.SetBinary( bBin );
   sub._fprintf( "%s", pub.Version() );
   sub._fprintf( "%s%s", pc, pub.Start( pPub ) );
   sub._fprintf( "Hit <ENTER> to subscribe ..." ); getchar();
   sub._fprintf( "%s%s", pc, sub.Start( pSub, pUsr ) );
   sub.Subscribe( pSvc, "LATENCY", NULL );
   sub._fprintf( "Hit <ENTER> to quit ..." ); getchar();

   // Clean up

   sub._fprintf( "Cleaning up ..." ); ::fflush( stdout );
   pub.Stop();
   sub.Stop();
   sub._fprintf( "Done!!" );
   return 1;
}
