/******************************************************************************
*
*  Bridge.cpp
*     Single-service Bridge
*
*  REVISION HISTORY:
*      6 JUN 2018 jcs  Created.
*      6 APR 2020 jcs  Build 43: svc@host:port
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#if !defined(WIN32)
#include <SigHandler.h>
#endif // !defined(WIN32)
#include <math.h>
#include <stdarg.h>

#if defined(__LP64__) || defined(_WIN64)
#define GL64 "(64-bit)"
#else
#define GL64 "(32-bit)"
#endif // __LP64__

#define _TIME_FID  6
#define _MIKE      1000000.0

using namespace RTEDGE;
using namespace std;

const char *BridgeID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)Bridge %s Build 43 ", GL64 );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


////////////////////
//
//  C o n n C f g
//
////////////////////
class ConnCfg
{
public:
   string _svc;
   string _svr;

   ///////////////////
   // Constructor
   ///////////////////
public:
   ConnCfg( const char *tkr ) :
      _svc( "undefined" ),
      _svr( "localhost:1234" )
   {
      string s( tkr );
      char  *s1, *s2, *rp;

      /*
       * service@host:port
       * host:port
       */
      s1 = (char *)s.data();
      s1 = ::strtok_r( s1,   "@", &rp );
      s2 = ::strtok_r( NULL, "@", &rp );
      if ( s1 ) {
         _svc = s1;
         _svr = s2;
      }
      else
         _svr = s2;
   }


   ///////////////////
   // Access
   ///////////////////
public:
   const char *Service()
   {
      return _svc.data();
   }

   const char *Server()
   {
      return _svr.data();
   }

}; // class ConnCfg


////////////////////
//
//  W a t c h
//
////////////////////
class Watch : public string
{
public:
   void *_arg;
   int   _rtl;

   ///////////////////
   // Constructor
   ///////////////////
public:
   Watch( const char *tkr, void *arg ) :
      string( tkr ),
      _arg( arg ),
      _rtl( 1 )
   { ; }


   ///////////////////
   // Access
   ///////////////////
public:
   const char *tkr()
   {
      return data();
   }

}; // class Watch

typedef hash_map<string, Watch *> WatchList;


////////////////////////
//
//     B r i d g e
//
////////////////////////
class Bridge
{
private:
   SubChannel &_sub;
   PubChannel &_pub;
   string      _subSvc;
   WatchList   _wl;
   Mutex       _mtx;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   Bridge( SubChannel &sub, PubChannel &pub, ConnCfg &sCfg ) :
      _sub( sub ),
      _pub( pub ),
      _subSvc( sCfg.Service() ),
      _wl(),
      _mtx()
   { ; }


   ////////////////////////////////
   // Access / Operations
   ////////////////////////////////
public:
   PubChannel &pub()
   {
      return _pub;
   }

   const char *SubSvc()
   {
      return _subSvc.data();
   }

   void Stop()
   {
      _sub.Stop();
      _pub.Stop();
   }

   void Open( const char *tkr, void *arg )
   {
      WatchList::iterator it;
      Watch              *w;
      string              s( tkr );
      bool                bNew;

      // 1) Add to Watchlist

      _fprintf( "OPEN( %s )", tkr );
      {
         Locker lck( _mtx );

         it   = _wl.find( s );
         bNew = ( it == _wl.end() );
         if ( bNew ) {
            w      = new Watch( tkr, arg );
            _wl[s] = w;
         }
      }

      // Subscribe if new

      if ( bNew )
         _sub.Subscribe( SubSvc(), tkr, arg );
   }

   void Close( const char *tkr )
   {
      WatchList::iterator it;
      Watch              *w;
      string              s( tkr );
      bool                bDel;

      // 1) Remove from Watchlist
      {
         Locker lck( _mtx );

         it   = _wl.find( s );
         bDel = ( it != _wl.end() );
         if ( bDel ) {
            _fprintf( "CLOSE( %s )", tkr );
            w = (*it).second;
            _wl.erase( it );
            delete w;
         }
      }

      // Unsubscribe if found

      if ( bDel )
         _sub.Unsubscribe( SubSvc(), tkr );
   }


   //////////////////////////
   // Logging
   //////////////////////////
public:
   void _fprintf( const char *msg )
   {
      _fprintf( (char *)msg );
   }

   void _fprintf( char *fmt, ... )
   {
      va_list ap;
      string  s;
      char    obuf[2*K], *cp;

      cp  = obuf;
      cp += sprintf( cp, "[%s] ", _sub.pDateTimeMs( s ) );
      va_start( ap,fmt );
      cp += vsprintf( cp, fmt, ap );
      cp += sprintf( cp, "\n" );
      va_end( ap );
      *cp = '\0';
      ::fwrite( obuf, cp-obuf, 1, stdout );
      ::fflush( stdout );
   }

}; // class Bridge


////////////////////////
//
//     M y P u b
//
////////////////////////
class MyPub : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   ConnCfg &_cfg;
   Bridge  *_br;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyPub( ConnCfg &cfg ) :
      PubChannel( cfg.Service() ),
      _cfg( cfg ),
      _br( (Bridge *)0 )
   {
      SetBinary( true );
      SetUserPubMsgTy( true );
      SetIdleCallback( true );
   }

   void SetBridge( Bridge &br )
   {
      _br = &br;
   }


   ////////////////////////////////
   // Access / Operations
   ////////////////////////////////
public:
   Bridge &br()
   {
      return *_br;
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      br()._fprintf( "PUB.%s CONN %s : %s", pPubName(), pUp, msg );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      br().Open( tkr, arg );
   }

   virtual void OnPubClose( const char *tkr )
   {
      br().Close( tkr );
   }

   virtual void OnIdle()
   {
      // Might find something useful to do with this at some point 

      breakpoint();
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



////////////////////////
//
//     M y S u b
//
////////////////////////
class MySub : public SubChannel
{
private:
   ConnCfg &_cfg;
   Bridge  *_br;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MySub( ConnCfg &cfg ) :
      SubChannel(),
      _cfg( cfg ),
      _br( (Bridge *)0 )
   {
      SetBinary( true );
   }

   void SetBridge( Bridge &br )
   {
      _br = &br;
   }


   ////////////////////////////////
   // Access / Operations
   ////////////////////////////////
   Bridge &br()
   {
      return *_br;
   }

   const char *subSvc()
   {
      return _cfg.Service();
   }


   ////////////////////////////////
   // Asynchronous Notifications
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      br()._fprintf( "SUB.%s CONN %s : %s", subSvc(), pUp, msg );
   }

   virtual void OnService( const char *msg, bool bOK )
   {
      const char *pUp = bOK ? "UP" : "DOWN";

      if ( !::strcmp( msg, subSvc() ) )
         br()._fprintf( "SUB.%s SVC %s", subSvc(), pUp );
   }

   virtual void OnData( Message &msg )
   {
      RTEDGE::Update &u    = br().pub().upd();
      const char     *tkr  = msg.Ticker();
      void           *arg  = msg.arg();
      bool            bImg = msg.IsImage();
      rtFIELD        *fdb  = msg.Fields();
      mddFieldList    fl;

      // 1) Copy mddFieldList wire-to-wire

      fl._flds   = (mddField *)fdb;
      fl._nFld   = msg.NumFields();
      fl._nAlloc = 0;

      // 2) Bitch Ass-ness

      if ( bImg )
         br()._fprintf( "IMG.%s %s", subSvc(), tkr );
      u.Init( tkr, arg, bImg );
      u.AddFieldList( fl );
      u.Publish();
   }

   virtual void OnDead( Message &msg, const char *err )
   {
      RTEDGE::Update &u   = br().pub().upd();
      const char     *tkr = msg.Ticker();
      void           *arg = msg.arg();

      u.Init( tkr, arg, true );
      u.PubError( err );
      br()._fprintf( "DEAD.%s %s : %s", subSvc(), tkr, err );
      br().Close( tkr );
   }

   virtual void OnSchema( Schema &sch )
   {
      br()._fprintf( "SUB.%s OnSchema() nFld=%d", subSvc(), sch.Size() );
   }

}; // class MySub


/////////////////////////
//
//     main()
//
/////////////////////////
int main( int argc, char **argv )
{
   string      pc;
   const char *pUsr;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", BridgeID() );
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }

   // cmd-line args

   if ( argc < 4 ) {
      pc  = "Usage: %s <pubSvc@PubHost> <SubSvc@SubHost> <SubUser> ";
      pc += "; Exitting ...\n";
      printf( pc.data(), argv[0] );
      return 0;
   }
   pUsr = argv[3];

   ConnCfg pCfg( argv[1] ), sCfg( argv[2] );
   MySub   sub( sCfg );
   MyPub   pub( pCfg );
   Bridge  br( sub, pub, sCfg );

   pub.SetBridge( br );
   sub.SetBridge( br );
   br._fprintf( BridgeID() );
   br._fprintf( pub.Version() );
   br._fprintf( pub.Start( pCfg.Server() ) );
   br._fprintf( sub.Start( sCfg.Server(), pUsr ) );
#if !defined(WIN32)
   br._fprintf( "Hit <CTRL>-C to quit" );
   forkAndSig( false );
   for ( ; !_pSigLog; pub.Sleep( 0.5 ) );
#else
   const char *_pSigLog = "SIGUSR";

   br._fprintf( "Hit <ENTER>- to quit" ); getchar();
#endif // !defined(WIN32)
   br._fprintf( "%s caught; Shutting down ...", _pSigLog );

   // Clean up

   br._fprintf( "Cleaning up ..." );
   pub.Stop();
   br._fprintf( "Done!! " );
   return 1;
}
