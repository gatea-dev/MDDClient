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
*      3 NOV 2022 jcs  Build 60: AddVector()
*     26 JAN 2023 jcs  Build 62: PubTkr_SIMPLE()
*
*  (c) 1994-2023, Gatea Ltd.
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
   int    _StreamID;
   bool   _bImg;
   int    _rtl;

   ///////////////////
   // Constructor
   ///////////////////
public:
   Watch( const char *tkr, int StreamID ) :
      _tkr( tkr ),
      _StreamID( StreamID ),
      _bImg( true ),
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
             int         StreamID ) :
      Vector( svc, tkr, precision ),
      _Size( vecSz ),
      _StreamID( StreamID ),
      _RTL( 1 )
   {
      for ( int i=0; i<vecSz; UpdateAt( i, M_PI * i ), i++ );
   }


   ///////////////////
   // Operations
   ///////////////////
public:
   size_t PubVector( RTEDGE::Update &u )
   {
      size_t ix  = ( _RTL % _Size );
      size_t nt;
 
      // Every 5th time

      _RTL += 1;
      nt = Publish( u, _StreamID );
      if ( ( _RTL % 5 ) == 0 )
         UpdateAt( ix, M_E );
      else
         ShiftRight( 1 );
      return nt;
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
   bool         _bVecFld;
   WatchList    _wl;
   WatchListV   _wlV;
   Mutex        _mtx;
   const char **_chn;
   int          _nLnk;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyChannel( const char *svc, int vecSz, int vecPrec, bool bVecFld ) :
      PubChannel( svc ),
      _vecSz( vecSz ),
      _vecPrec( vecPrec ),
      _bVecFld( bVecFld ),
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
   Watch *AddWatch( const char *tkr, int sid )
   {
      Locker              lck( _mtx );
      WatchList::iterator it;
      Watch              *w;
      string              s( tkr );

      if ( (it=_wl.find( s )) == _wl.end() )
         _wl[s] = new Watch( tkr, sid );
      it = _wl.find( s );
      w  = (*it).second;
      w->_bImg = true;
      return w;
   }

   MyVector *AddVector( const char *tkr, int sid )
   {
      Locker               lck( _mtx );
      WatchListV::iterator vt;
      string               s( tkr );

      if ( (vt=_wlV.find( s )) == _wlV.end() )
         _wlV[s] = new MyVector( pPubName(), tkr, _vecSz, _vecPrec, sid );
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
      size_t               nt, nb;

      // Not synchronized; Quick Hack

      for ( nb=0,it=_wl.begin(); it!=_wl.end(); it++ ) {
         w   = (*it).second;
         nb += PubTkr( *w );
      }
      for ( vt=_wlV.begin(); vt!=_wlV.end(); vt++ ) {
         v   = (*vt).second;
         nb += v->PubVector( upd() );
      }
      nt = _wl.size() + _wlV.size();
      if ( nt )
         ::fprintf( stdout, "Publish %ld tkrs; %ld bytes\n", nt, nb );
      ::fflush( stdout );
      return nt;
   }

   size_t PubTkr( Watch &w )
   {
      return PubTkr_SIMPLE( w );
   }

   size_t PubTkr_SIMPLE( Watch &w )
   {
      Locker         lck( _mtx );
      Update        &u = upd();
      rtDateTime     dtTm;
      struct timeval tv;
      int            fid;

      if ( w._bImg )
         ::fprintf( stdout, "IMG [%d] : %s\n", w._StreamID, w.tkr() );
      u.Init( w.tkr(), w._StreamID, w._bImg );
      w._bImg    = false;
      fid        = 6;
      tv.tv_sec  = TimeSec();
      tv.tv_usec = 0;
      dtTm       = unix2rtDateTime( tv );
      u.AddField( fid++, dtTm );
      u.AddField( fid++, w._rtl++ );
      u.AddField( fid++, M_PI );
      return u.Publish();
   }

   size_t PubTkr_FULL( Watch &w )
   {
      Locker         lck( _mtx );
      Update        &u = upd();
      rtDateTime     dtTm;
      int            i, fid;
      u_int64_t      i64;
      double         r64;
      DoubleList     vdb;
      struct timeval tv;

      if ( w._bImg )
         ::fprintf( stdout, "IMG [%d] : %s\n", w._StreamID, w.tkr() );
      u.Init( w.tkr(), w._StreamID, w._bImg );
      w._bImg    = false;
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
      if ( _vecSz && _bVecFld ) {
         for ( i=0; i<_vecSz; vdb.push_back( ::drand48() * 100.0 ), i++ );
         u.AddVector( -7151, vdb, _vecPrec );
      }
      w._rtl += 1;
      return u.Publish();
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
      int       StreamID = (int)(size_t)arg;

      pfx  = ::strtok( (char *)tmp.data(), "#" );
      ric  = ::strtok( NULL, "#" );
      bChn = _chn && ric;
      ::fprintf( stdout, "OPEN [%6d] %s\n", StreamID, tkr ); ::fflush( stdout );
      if ( bChn )
         PubChainLink( atoi( pfx ), ric, arg );
      else if ( _vecSz && !_bVecFld ) {
         v = AddVector( tkr, StreamID );
         v->PubVector( upd() );
      }
      else {
         w = AddWatch( tkr, StreamID );
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

   virtual void OnRefreshImage( const char *tkr, int StreamID )
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

   virtual void OnPermQuery( const char **tuple, int reqID )
   {
      const char *svc = tuple[0];
      const char *tkr = tuple[1];
      const char *usr = tuple[2];
      const char *loc = tuple[3];
      const char *fmt = "OnPermQuery( %s,%s,%s,%s ) : %d\n";

      ::fprintf( stdout, fmt, svc, tkr, usr, loc, reqID );
      ::fflush( stdout );
   }


   ////////////////////////////////////
   // PubChannel Interface
   ////////////////////////////////////
protected:
   virtual Update *CreateUpdate()
   {
      return new RTEDGE::Update( *this );
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
   const char *svr, *svc, *pChn, *ty;
   const char *lnks[_MAX_CHAIN];
   char       *cp;
   double      tPub, tRun, d0, dn;
   int         i, nl, nt, hBeat, vecSz, vPrec;
   bool        bCfg, bPack, bFldV, aOK;
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
   bFldV = false;
   bPack = true;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -h       <Source : host:port> ] \\ \n";
      s += "       [ -s       <Service> ] \\ \n";
      s += "       [ -pub     <Publication Interval> ] \\ \n";
      s += "       [ -run     <App Run Time> ] \\ \n";
      s += "       [ -vector  <Vector length; 0 for no vector> ] \\ \n";
      s += "       [ -vecPrec <Vector Precision> ] \\ \n";
      s += "       [ -vecFld  <If vector, true to publish as field> ] \\ \n";
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
      printf( "      -vecFld  : %s\n", bFldV ? "YES" : "NO" );
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
      else if ( !::strcmp( argv[i], "-vecPrec" ) )
         vPrec = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-vecFld" ) )
         bFldV = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-packed" ) )
         bPack = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-packed" ) )
         hBeat = _IsTrue( argv[++i] );
   }

   MyChannel pub( svc, vecSz, vPrec, bFldV );

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
   if ( vecSz ) {
      ty = bFldV ? "FIELD" : "BYTESTREAM";
      ::fprintf( stdout, "%d VECTOR as %s\n", vecSz, ty );
   }
   pub.SetBinary( true );
   pub.SetHeartbeat( hBeat );
#ifdef TODO_PERMS_BUT_NOT_NOW
   pub.SetPerms( true );
#endif // TODO_PERMS_BUT_NOT_NOW
   ::fprintf( stdout, "%s\n", pub.Start( svr ) );
   pub.SetMDDirectMon( "./MDDirectMon.stats", "Pub", "Pub" );
   ::fprintf( stdout, "Running for %.1fs; Publish every %.1fs\n", tRun, tPub );
   ::fflush( stdout );
   for ( i=0,d0=dn=pub.TimeNs(); ( dn-d0 ) < tRun; i++ ) {
      pub.Sleep( tPub );
      nt = pub.PublishAll();
   }

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub.Stop();
   printf( "Done!!\n " );
   return 1;
}
