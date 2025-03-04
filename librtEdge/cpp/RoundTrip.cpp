/******************************************************************************
*
*  RoundTrip.cpp
*
*  REVISION HISTORY:
*     17 FEB 2016 jcs  Created (from Publish.cpp).
*     31 JAN 2025 jcs  Build 75: Binary always; args
*
*  (c) 1994-2025, Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <math.h>
#include <stdarg.h>

#define _TIME_FID  6
#define _MIKE      1000000.0
#define _DFLT_PORT ":9998"

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
      double d0, d1, D0, D1;
      int    fid, sec, uS, age;

      d1  = ClockNs();
      fid = _TIME_FID;
      sec = (f=msg.GetField( fid++ )) ? f->GetAsInt32() : 0;
      uS  = (f=msg.GetField( fid++ )) ? f->GetAsInt32() : 0;
      if ( sec && uS ) {
         d0  = ( uS / _MIKE ) + sec;
         D0  = ::fmod( d0, 60.0 );
         D1  = ::fmod( d1, 60.0 );
         age = (int)( ( d1-d0 ) * _MIKE );
//         _fprintf( "PUB %.6f; Latency(uS) = %d", D0, age );
         _fprintf( "RT,,%.6f,,,%.6f,,%d", D0, D1, age );
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
      double      now = TimeNs();
      va_list     ap;
      std::string s;
      char        obuf[2*K], *cp;

      cp  = obuf;
      cp += sprintf( cp, "[%s] ", pDateTimeUs( s, now ) );
      va_start( ap,fmt );
      cp += vsprintf( cp, fmt, ap );
      cp += sprintf( cp, "\n" );
      va_end( ap );
      *cp = '\0';
      ::fwrite( obuf, cp-obuf, 1, stdout );
      ::fflush( stdout );
   }

}; // class MySub


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

}; // class Watch



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
   MyPub( const char *svrP ) :
      PubChannel( svrP ),
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
      dd  = ClockNs();
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
//      MySub::_fprintf( "PUB %.6f", ::fmod( dd, 60.0 ) );
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

}; // class MyPub

//////////////////////////
// main()
//////////////////////////
static bool _IsTrue( const char *p )
{
   return( !::strcmp( p, "YES" ) || !::strcmp( p, "true" ) );
}

int main( int argc, char **argv )
{
   MySub       sub;
   const char *svrP, *svc, *svrS, *usr, *pf;
   std::string s, ss;
   bool        bCfg, bFast, aOK;
   int         i;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", sub.Version() );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   svrP  = "localhost:9994";
   svrS  = "localhost:9998";
   svc   = "round.trip";
   usr   = argv[0];
   bFast = false;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) { 
      s  = "Usage: %s \\ \n";
      s += "       [ -p  <Pub Server : host:port>> ] \\ \n";
      s += "       [ -h  <Sub Server : host:port>> ] \\ \n";
      s += "       [ -s  <Service Name>> ] \\ \n";
      s += "       [ -u  <Username> ] \\ \n";
      s += "       [ -f  <true for low latency> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -p       : %s\n", svrP );
      printf( "      -h       : %s\n", svrS );
      printf( "      -s       : %s\n", svc );
      printf( "      -u       : %s\n", usr );
      printf( "      -f       : %s\n", bFast ? "true" : "false" );
      return 0;
   }
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-p" ) )
         svrP = argv[++i];
      else if ( !::strcmp( argv[i], "-h" ) ) {
         ss   = argv[++i];
         ss  += !::strstr( ss.data(), ":" ) ? _DFLT_PORT : "";
         svrS = ss.data();
      }
      else if ( !::strcmp( argv[i], "-s" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-u" ) )
         usr = argv[++i];
      else if ( !::strcmp( argv[i], "-f" ) )
         bFast = _IsTrue( argv[++i] );
   }

   ///////////////////////////
   // Rock on
   ///////////////////////////
   MyPub pub( svc );

   pf = bFast ? "" : "NOT ";
   pub.SetBinary( true );
   sub.SetBinary( true );
   sub._fprintf( "%s", pub.Version() );
   sub._fprintf( "BINARY : %s", pub.Start( svrP ) );
   sub._fprintf( "Hit <ENTER> to subscribe ..." ); getchar();
   sub._fprintf( "RT,,Publish,Wire-Pub,Wire-Sub,OnData,,RT," );
   sub._fprintf( "BINARY %sFAST %s", pf, sub.Start( svrS, usr ) );
   sub.SetLowLatency( bFast );
   sub.Subscribe( svc, "LATENCY", NULL );
   sub._fprintf( "Hit <ENTER> to quit ..." ); getchar();

   // Clean up

   sub._fprintf( "Cleaning up ..." ); ::fflush( stdout );
   pub.Stop();
   sub.Stop();
   sub._fprintf( "Done!!" );
   return 1;
}
