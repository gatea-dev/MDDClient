/******************************************************************************
*
*  Publish.cpp
*
*  REVISION HISTORY:
*     20 JAN 2015 jcs  Created.
*     28 FEB 2015 jcs  Build 30: SetHeartbeat()
*      4 MAY 2015 jcs  Build 31: Fully-qualified  (compiler)
*     17 FEB 2016 jcs  Build 32: SetUserPubMsgTy(); Watch._upd
*     26 MAY 2017 jcs  Build 34: StartUDP()
*     16 MAR 2022 jcs  Build 51: De-lint
*     29 MAR 2022 jcs  Build 52: ::getenv( "__JD_UNPACKED" )
*     22 OCT 2022 jcs  Build 58: -s service -t ticker
*     29 OCT 2022 jcs  Build 60: AddVector()
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>

#if !defined(M_PI)
#define M_E            2.7182818284590452354   /* e */
#define M_PI           3.14159265358979323846  /* pi */
#endif // !defined(M_PI)

#ifdef WIN32
static double drand48() { return ( (double)::rand() / (double)RAND_MAX ); }
#define srand48 srand
#endif // WIN32

#define _MAX_CHAIN (16*K)
#define _LCL_PORT  4321

using namespace RTEDGE;
using namespace std;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *PublishID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)Publish Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

////////////////////
//
//  W a t c h
//
////////////////////
class Watch
{
private:
   string _tkr;
   rtBUF  _upd;
public:
   void  *_arg;
   int    _rtl;

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

}; // class Watch


////////////////////
//
//  M y V e c t o r
//
////////////////////
class MyVector : public Vector
{
private:
   int    _Size;
   int    _StreamID;
   size_t _RTL;

   ///////////////////
   // Constructor
   ///////////////////
public:
   MyVector( const char *svc, 
             const char *tkr, 
             int         vecSz,
             int         precision,
             void       *arg ) :
      Vector( svc, tkr, precision ),
      _Size( vecSz ),
      _StreamID( (int)(size_t)arg ),
      _RTL( 1 )
   {
      for ( int i=0; i<vecSz; UpdateAt( i, M_PI * i ), i++ );
   }


   ///////////////////
   // Operations
   ///////////////////
public:
   void PubVector( RTEDGE::Update &u )
   {
      size_t ix  = ( _RTL % _Size );
 
      // Every 5th time

      _RTL += 1;
      Publish( u, _StreamID );
      if ( ( _RTL % 5 ) == 0 )
         UpdateAt( ix, M_E );
      else
         ShiftRight( 1 );
   }

}; // class MyVector


////////////////////////
//
//     M y P u b
//
////////////////////////

typedef map<string, Watch *>    WatchList;
typedef map<string, MyVector *> WatchListV;

class MyChannel : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   int          _vecSz;
   int          _vecPrec;
   WatchList    _wl;
   WatchListV   _wlV;
   Mutex        _mtx;
   const char **_chn;
   int          _nLnk;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyChannel( const char *svc, int vecSz, int vecPrec ) :
      PubChannel( svc ),
      _vecSz( vecSz ),
      _vecPrec( vecPrec ),
      _wl(),
      _wlV(),
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
      string         s( tkr );

      if ( (it=_wl.find( s )) == _wl.end() )
         _wl[s] = new Watch( tkr, arg );
      it = _wl.find( s );
      return (*it).second;
   }

   MyVector *AddVector( const char *tkr, void *arg )
   {
      Locker               lck( _mtx );
      WatchListV::iterator vt;
      string               s( tkr );

      if ( (vt=_wlV.find( s )) == _wlV.end() )
         _wlV[s] = new MyVector( pPubName(), tkr, _vecSz, _vecPrec, arg );
      vt = _wlV.find( s );
      return (*vt).second;
   }

   void SetChain( const char **chn, int nLnk )
   {
      _chn  = chn;
      _nLnk = nLnk;
   }

   size_t PublishAll()
   {
      Locker               lck( _mtx );
      WatchList::iterator  it;
      WatchListV::iterator vt;
      Watch               *w;
      MyVector            *v;

      // Not synchronized; Quick Hack

      for ( it=_wl.begin(); it!=_wl.end(); it++ ) {
         w = (*it).second;
         PubTkr( *w );
      }
      for ( vt=_wlV.begin(); vt!=_wlV.end(); vt++ ) {
         v = (*vt).second;
         v->PubVector( upd() );
      }
      return( _wl.size() + _wlV.size() );
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
      Locker         lck( _mtx );
      Update        &u = upd();
      rtDateTime     dtTm;
      bool           bImg;
      int            i, fid;
      u_int64_t      i64;
      double         r64;
      Doubles        vdb;
      struct timeval tv;

      bImg       = ( w._rtl == 1 );
      u.Init( w.tkr(), w._arg, bImg );
      fid        = 6;
      tv.tv_sec  = TimeSec();
      tv.tv_usec = 0;
      dtTm       = unix2rtDateTime( tv );
      i64        = 7723845300000;
      i64        = 4503595332403200;
      r64        = 123456789.987654321 /* + w._rtl */;
      u.AddField(  fid++, r64 );
      r64        = 6120.987654321 + w._rtl;
      u.AddField(  fid++, r64 );
      r64        = 3.14159265358979323846;
      u.AddField(  fid++, r64 );
      u.AddField(  fid++, i64 );
      u.AddField(  fid++, dtTm );
      u.AddFieldAsUnixTime(  fid++, dtTm );
      u.AddField(  fid++, dtTm._date );
      u.AddField(  fid++, dtTm._time );
/*
      u.AddField(  2147483647, "2147483647" );
      u.AddField( -2147483647, "-2147483647" );
      u.AddField( 16260000, "16260000" );
      u.AddField( 536870911, "536870911" );
 */
      for ( i=0; i<10; vdb.push_back( ::drand48() * 100.0 ), i++ );
      u.AddVector( -7151, vdb );
      u.Publish();
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
      Locker    lck( _mtx );
      string    tmp( tkr );
      Watch    *w;
      MyVector *v;
      char     *pfx, *ric;
      bool      bChn;

      pfx  = ::strtok( (char *)tmp.data(), "#" );
      ric  = ::strtok( NULL, "#" );
      bChn = _chn && ric;
      ::fprintf( stdout, "OPEN %s\n", tkr ); ::fflush( stdout );
      if ( bChn )
         PubChainLink( atoi( pfx ), ric, arg );
      else if ( _vecSz ) {
         v = AddVector( tkr, arg );
         v->PubVector( upd() );
      }
      else {
         w = AddWatch( tkr, arg );
         PubTkr( *w );
      }
   }

   virtual void OnPubClose( const char *tkr )
   {
      Locker               lck( _mtx );
      WatchList::iterator  it;
      WatchListV::iterator vt;
      string               s( tkr );

      ::fprintf( stdout, "CLOSE %s\n", tkr );
      ::fflush( stdout );
      if ( (it=_wl.find( s )) != _wl.end() ) {
         _wl.erase( it );
         delete (*it).second;
      }
      if ( (vt=_wlV.find( s )) != _wlV.end() ) {
         _wlV.erase( vt );
         delete (*vt).second;
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
      string         s( tkr );

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

}; // class MyChannel


//////////////////////////
// main()
//////////////////////////
static bool _IsTrue( const char *p )
{
   return( !::strcmp( p, "YES" ) || !::strcmp( p, "true" ) );
}

int main( int argc, char **argv )
{
   const char *svr, *svc, *pChn;
   const char *lnks[_MAX_CHAIN];
   char       *cp;
   double      tPub, tRun, d0, dn;
   int         i, nl, nt, hBeat, vecSz, vPrec;
   bool        bCfg, bPack, aOK;
   string      s;
   rtBuf64     chn;

   /////////////////////////////
   // Quickie checks
   /////////////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", PublishID() );
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }
   svr   = "localhost:9995";
   svc   = "my_publisher";
   pChn  = NULL;
   hBeat = 15;
   tRun  = 60.0;
   tPub  = 1.0;
   vecSz = 0;
   vPrec = 2;
   bPack = true;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -h       <Source : host:port> ] \\ \n";
      s += "       [ -s       <Service> ] \\ \n";
      s += "       [ -pub     <Publication Interval> ] \\ \n";
      s += "       [ -run     <App Run Time> ] \\ \n";
      s += "       [ -vector  <Non-zero for vector; 0 for Field List> ] \\ \n";
      s += "       [ -vecPrec <Vector Precision> ] \\ \n";
      s += "       [ -packed  <true for packed; false for UnPacked> ] \\ \n";
      s += "       [ -hbeat   <Heartbeat> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -h       : %s\n", svr );
      printf( "      -s       : %s\n", svc );
      printf( "      -pub     : %.1f\n", tPub );
      printf( "      -run     : %.1f\n", tRun );
      printf( "      -vector  : %d\n", vecSz );
      printf( "      -vecPrec : %d\n", vPrec );
      printf( "      -packed  : %s\n", bPack ? "YES" : "NO" );
      printf( "      -hbeat   : %d\n", hBeat );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-h" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-s" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-pub" ) )
         tPub = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-run" ) )
         tRun = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-vector" ) )
         vecSz = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-packed" ) )
         bPack = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-packed" ) )
         hBeat = _IsTrue( argv[++i] );
   }

   MyChannel pub( svc, vecSz, vPrec );

   ::srand48( pub.TimeSec() ); 
   s = "";
   if ( pChn && (chn=pub.MapFile( pChn ))._dLen ) {
      string tmp( chn._data, chn._dLen );

      s = tmp;
      pub.UnmapFile( chn );
   }
   cp = ::strtok( (char *)s.data(), "\n" );
   for ( nl=0; nl<_MAX_CHAIN-1 && cp; nl++ ) {
      lnks[nl] = cp;
      cp       = ::strtok( NULL, "\n" );
   }
   lnks[nl] = NULL;
   pub.SetUnPacked( !bPack );
   pub.SetChain( lnks, nl );
   ::fprintf( stdout, "%s\n", pub.Version() );
   ::fprintf( stdout, "%sPACKED\n", pub.IsUnPacked() ? "UN" : "" );
   pub.SetBinary( true );
   pub.SetHeartbeat( hBeat );
   pub.SetPerms( true );
   ::fprintf( stdout, "%s\n", pub.Start( svr ) );
   pub.SetMDDirectMon( "./MDDirectMon.stats", "Pub", "Pub" );
   ::fprintf( stdout, "Running for %.1fs; Publish every %.1fs\n", tRun, tPub );
   ::fflush( stdout );
   for ( i=0,d0=dn=pub.TimeNs(); ( dn-d0 ) < tRun; i++ ) {
      pub.Sleep( tPub );
      nt = pub.PublishAll();
      dn = pub.TimeNs();
      if ( nt )
         ::fprintf( stdout, "[%04d,%6.1f] Publish %d tkrs\n", i, dn-d0, nt ); 
      ::fflush( stdout );
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub.Stop();
   printf( "Done!!\n " );
   return 1;
}
