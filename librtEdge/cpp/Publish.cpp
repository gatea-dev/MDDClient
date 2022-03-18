/******************************************************************************
*
*  Publish.cpp
*
*  REVISION HISTORY:
*     20 JAN 2015 jcs  Created.
*     28 FEB 2015 jcs  Build 30: SetHeartbeat()
*      4 MAY 2015 jcs  Build 31: Fully-qualified std:: (compiler)
*     17 FEB 2016 jcs  Build 32: SetUserPubMsgTy(); Watch._upd
*     26 MAY 2017 jcs  Build 34: StartUDP()
*     16 MAR 2022 jcs  Build 51: De-lint
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

#define _MAX_CHAIN (16*K)
#define _LCL_PORT  4321

using namespace RTEDGE;

static bool _bBin;

////////////////////
//
//  W a t c h
//
////////////////////
class Watch
{
private:
   std::string _tkr;
   rtBUF       _upd;
public:
   void       *_arg;
   int         _rtl;

   ///////////////////
   // Constructor
   ///////////////////
public:
   Watch( const char *tkr, void *arg ) :
      _tkr( tkr ),
      _arg( arg ),
      _rtl( 1 )
   {
      ::memset( &_upd, 0, sizeof( _upd ) );
   }

   ~Watch()
   {
      if ( _upd._data )
         delete[] _upd._data;
   }


   ///////////////////
   // Access / Mutator
   ///////////////////
public:
   rtBUF *upd()
   {
      return _upd._data ? &_upd : (rtBUF *)0;
   }

   const char *tkr()
   {
      return _tkr.c_str();
   }

   void SetUpd( PubChannel &pub )
   {
      rtBUF lwc;
      u_int sz;

      lwc = pub.PubGetData();
      sz  = lwc._dLen;
      if ( sz && !_upd._data ) {
         _upd._dLen = sz;
         _upd._data = new char[sz+4];
         ::memset( _upd._data, 0, sz+4 );
         ::memcpy( _upd._data, lwc._data, sz );
      }
   }
};


////////////////////////
//
//     M y P u b
//
////////////////////////

typedef std::map<std::string, Watch *> WatchList;

class MyChannel : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   WatchList    _wl;
   Mutex        _mtx;
   const char **_chn;
   int          _nLnk;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyChannel( const char *pPub ) :
      PubChannel( pPub ),
      _wl(),
      _mtx(),
      _chn( (const char **)0 ),
      _nLnk( 0 )
   {
      SetUserPubMsgTy( true );
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
public:
   Watch *AddWatch( const char *tkr, void *arg )
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      std::string         s( tkr );

      if ( (it=_wl.find( s )) == _wl.end() )
         _wl[s] = new Watch( tkr, arg );
      it = _wl.find( s );
      return (*it).second;
   }

   void SetChain( const char **chn, int nLnk )
   {
      _chn  = chn;
      _nLnk = nLnk;
   }

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

   void PubTkr_THIN( Watch &w )
   {
      Locker  lck( _mtx );
      Update &u = upd();
      bool    bImg;

      bImg = ( w._rtl == 1 );
      u.Init( w.tkr(), w._arg, bImg );
      u.AddField(  22, 1.02 );
      u.AddField(  25, 1.03 );
      u.AddField(  30, 100 );
      u.AddField(  31, 100 );
      u.AddField( 1021, w._rtl );
      u.Publish();
      if ( bImg )
         ::fprintf( stdout, "IMAGE  %s\n", w.tkr() );
      w._rtl += 1;
   }

   void PubTkr( Watch &w )
   {
      Locker    lck( _mtx );
      Update   &u = upd();
      rtBUF    *pb;
      bool      bImg;
      int       fid;
      u_int64_t i64;

      fid  = 6;
      bImg = ( w._rtl == 1 );
      i64  = 7723845300000; 
      u.Init( w.tkr(), w._arg, bImg );
      u.AddField(  fid++, (double)2500000.12 );
      u.AddField(  fid++, i64 );
      u.Publish();
      pb = (rtBUF *)0;
/*
      if ( !bImg ) {
         u.AddField( fid++, w._rtl );
         u.AddField( fid++, (int)TimeSec() );
         u.AddField( fid++, "BID" );
         u.AddField( fid++, ::rand() % 10000 );
         u.Publish();
      }
      else  {
         double dv = 870.123;

         u.AddField( fid++, w._rtl );
         u.AddField( fid++, (int)TimeSec() );
         u.AddField( fid, "Duplicate 1" );
         u.AddField( fid, "Hi Mom" );
         u.AddField( fid, "Duplicate 2" );
         u.AddField( fid++, "BID" );
         u.AddField( fid++, ::rand() % 10000 );
         u.AddField( 70, "+10 43/64" );
         fid = 10000;
         for ( int i=0; i<10; i++, dv+=100.0 ) {
            u.AddField( fid++, dv );
            u.AddField( fid++, -dv );
         }
         u.AddField( fid++, "0" );
         u.AddField( fid++, "0" );
         u.AddField( fid++, "BLOOMBERG|VGH5 INDEX|LAST_PRICE" );
         u.AddField( fid++, "3236" );
         u.AddField( fid++, "NULL" );
         u.AddField( fid++, "0" );
         u.Publish();
         ::fprintf( stdout, "IMAGE  %s\n", w.tkr() ); ::fflush( stdout );
      }
 */
      w._rtl += 1;
   }

   void PubChainLink( int lnk, const char *chain, void *arg )
   {
      Locker       lck( _mtx );
      Update      &u = upd();
      const char **ldb;
      bool         bFinal;
      int          nl, off;

      // Up to _NUM_LINK per link

      u.Init( chain, arg, true );
      off  = lnk * _NUM_LINK;
      ldb  = _chn;
      ldb += off;
      nl   = gmin( _NUM_LINK, _nLnk-off );
      if ( nl > 0 ) {
         bFinal = ( _nLnk <= ( (lnk+1)*_NUM_LINK ) );
         u.PubChainLink( chain, arg, lnk, bFinal, ldb, nl );
         ::fprintf( stdout, "IMAGEc %d#%s\n", lnk, chain ); 
         ::fflush( stdout );
      }
   }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      ::fprintf( stdout, "CONN %s : %s\n", pUp, msg );
      ::fflush( stdout );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      Locker      lck( _mtx );
      Watch      *w;
      std::string tmp( tkr );
      char       *pfx, *ric;
      bool        bChn;

      pfx  = ::strtok( (char *)tmp.data(), "#" );
      ric  = ::strtok( NULL, "#" );
      bChn = _chn && ric;
      ::fprintf( stdout, "OPEN %s\n", tkr ); ::fflush( stdout );
      if ( bChn )
         PubChainLink( atoi( pfx ), ric, arg );
      else {
         w = AddWatch( tkr, arg );
         PubTkr( *w );
      }
   }

   virtual void OnPubClose( const char *tkr )
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      std::string         s( tkr );

      ::fprintf( stdout, "CLOSE %s\n", tkr );
      ::fflush( stdout );
      if ( (it=_wl.find( s )) != _wl.end() ) {
         _wl.erase( it );
         delete (*it).second;
      }
   }

   virtual void OnSymListQuery( int nSym )
   {
      ::fprintf( stdout, "OnSymListQuery( %d )\n", nSym );
      ::fflush( stdout );
   }

   virtual void OnRefreshImage( const char *tkr, void *arg )
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      Watch              *w;
      std::string         s( tkr );

      ::fprintf( stdout, "OnRefreshImage( %s )\n", tkr );
      ::fflush( stdout );
      if ( (it=_wl.find( s )) != _wl.end() ) {
         w = (*it).second;
         PubTkr( *w );
      }
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
   const char *pSvr, *pPub, *pChn, *pc;
   const char *lnks[_MAX_CHAIN];
   char       *cp;
   double      tSlp, tApp, d0, dn;
   int         i, nl, nt, StreamID;
   bool        bUDP;
   rtBuf64     chn;

_bBin = (cp=::getenv( "_JD_MF" )) ? false : true;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }

   // cmd-line args

   if ( argc < 5 ) {
      pc = "Usage: %s <hosts> <Svc> <pubTmr> <tAppMs> <ChainFile>";
      printf( pc, argv[0] );
      printf( "; Exitting ...\n" );
      return 0;
   }
   pPub = argv[2];
   tSlp = atof( argv[3] );
   tApp = atof( argv[4] );
   pChn = argv[5];

   MyChannel   pub( pPub );
   std::string s, ss( argv[1] );

   pSvr = ::strtok( (char *)ss.data(), ":" );
   bUDP = !::strcmp( pSvr, "udp" );
   pSvr = bUDP ? ::strtok( NULL, "\n" ) : argv[1];
   if ( (chn=pub.MapFile( pChn ))._dLen ) {
      std::string tmp( chn._data, chn._dLen );

      s = tmp;
   }
   cp = ::strtok( (char *)s.data(), "\n" );
   for ( nl=0; nl<_MAX_CHAIN-1 && cp; nl++ ) {
      lnks[nl] = cp;
      cp       = ::strtok( NULL, "\n" );
   }
   lnks[nl] = NULL;
   pub.SetChain( lnks, nl );
   ::fprintf( stdout, "%s\n", pub.Version() );
   if ( bUDP ) {
      ::fprintf( stdout, "%s\n", pub.StartConnectionless( pSvr, _LCL_PORT ) );
      StreamID = 1;
      pub.AddWatch( "ticker1", (VOID_PTR)StreamID++ );
      pub.AddWatch( "ticker2", (VOID_PTR)StreamID++ );
      pub.AddWatch( "ticker3", (VOID_PTR)StreamID++ );
   }
   else {
      pub.SetBinary( _bBin );
      pub.SetHeartbeat( 60 );
      pub.SetHeartbeat( 15 );
      pub.SetPerms( true );
      ::fprintf( stdout, "%s\n", pub.Start( pSvr ) );
   }
   ::fprintf( stdout, "Running for %.1fs; Publish every %.1fs\n", tApp, tSlp );
   ::fflush( stdout );
   for ( i=0,d0=dn=pub.TimeNs(); ( dn-d0 ) < tApp; i++ ) {
      pub.Sleep( tSlp );
      nt = pub.PublishAll();
      dn = pub.TimeNs();
      if ( nt )
         ::fprintf( stdout, "[%04d,%6.1f] Publish %d tkrs\n", i, dn-d0, nt ); 
      ::fflush( stdout );
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub.Stop();
   pub.UnmapFile( chn );
   printf( "Done!!\n " );
   return 1;
}
