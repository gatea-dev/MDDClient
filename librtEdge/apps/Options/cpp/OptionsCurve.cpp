/******************************************************************************
*
*  OptionsCurve.cpp
*
*  REVISION HISTORY:
*     13 SEP 2023 jcs  Created (from LVCDump.cpp)
*     20 SEP 2023 jcs  FullSpline
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <OptionsBase.cpp>
#include <list>

// Configurable Precision

static int l_Precision =     2;

// Record Templates

static int xTy_NUM     =     1;
static int l_fidInc    =     6; // X-axis Increment
static int l_fidXTy    =     7; // X-axis type : xTy_DATE = Date; Else Numeric
static int l_fidX0     =     8; // If Numeric, first X value
static int l_fidUNCV   = -8002;

// Forwards / Collections

class OptionsSpline;
class Underlyer;

typedef hash_map<string, OptionsSpline *> SplineMap;
typedef vector<Underlyer *>               Underlyers;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *OptionsCurveID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)OptionsCurve Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

////////////////////////////////////////////////
//
//     c l a s s   O p t i o n s C u r v e
//
////////////////////////////////////////////////
class OptionsCurve : public OptionsBase
{
public:
   bool _oblong;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsCurve( const char *svr, bool oblong ) :
      OptionsBase( svr ),
      _oblong( oblong )
   { ; }

}; // class OptionsCurve


////////////////////////////////////////////////
//
//      c l a s s   U n d e r l y e r
//
////////////////////////////////////////////////
class Underlyer : public string
{
private:
   OptionsCurve  &_lvc;
   Ints           _puts;
   Ints           _calls;
   Ints           _both;
   SortedInt64Set _exps;
   DoubleList     _strikes;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   Underlyer( OptionsCurve &lvc, const char *und ) :
      string( und ),
      _lvc( lvc ),
      _puts(),
      _calls(),
      _both(),
      _exps(),
      _strikes()
   { ; }

   /////////////////////////
   // Access
   /////////////////////////
public:
   OptionsCurve   &lvc()     { return _lvc; }
   const char     *name()    { return data(); }
   Ints           &puts()    { return _puts; }
   Ints           &calls()   { return _calls; }
   Ints           &both()    { return _both; }
   SortedInt64Set &exps()    { return _exps; }
   DoubleList     &strikes() { return _strikes; }

   /**
    * \brief Return true if valid expiration date
    *
    * \param tExp : User-supplied expiration date
    * \return true if valid expiration date
    * \see OptionBase::Expiration()
    */
   bool IsValidExp( u_int64_t tExp )
   {
      return( _exps.find( tExp ) != _exps.end() );
   }

   /////////////////////////
   // Operations
   /////////////////////////
public:
   void Init( LVCAll &all )
   {
      Messages &msgs = all.msgs();
      Message  *msg;
      Ints      both;
      int       ix;

      /*
       * 1) d/b Indices of all Puts and Calls
       */
      _puts  = _lvc.GetUnderlyer( all, name(), spline_put );
      _calls = _lvc.GetUnderlyer( all, name(), spline_call );
      _both  = _lvc.GetUnderlyer( all, name(), spline_both );
      both   = _puts;
      /*
       * 2) All Expiration Dates in sorted order
       */
      both  = _puts;
      both.insert( both.end(), _calls.begin(), _calls.end() );
      for ( size_t i=0; i<both.size(); i++ ) {
         ix   = both[i];
         msg  = msgs[ix];
         _exps.insert( _lvc.Expiration( *msg, false ) );
      }
      /*
       * 3) Strikes too
       */
      SortedInt64Set           strikes;
      SortedInt64Set::iterator it;

      for ( size_t i=0; i<both.size(); i++ ) {
         ix   = both[i];
         msg  = msgs[ix];
         strikes.insert( (u_int64_t)( _lvc.StrikePrice( *msg ) * 1000.0 ) );
      }
      for ( it=strikes.begin(); it!=strikes.end(); it++ )
         _strikes.push_back( 0.001 * (*it) );
   }

}; // class Underlyer



////////////////////////////////////////////////
//
//    c l a s s   O p t i o n s S p l i n e
//
////////////////////////////////////////////////
class OptionsSpline : public string
{
private:
   Underlyer &_und;
   u_int64_t  _tExp;
   SplineType _type;
   double     _xInc;
   Ints       _kdb;
   DoubleList _X;
   DoubleList _Y;
   int        _StreamID;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsSpline( Underlyer &und, 
                  u_int64_t  tExp, 
                  SplineType splineType,
                  double     xInc,
                  LVCAll    &all ) :
      string(),
      _und( und ),
      _tExp( tExp ),
      _type( splineType ),
      _xInc( xInc ),
      _kdb(),
      _X(),
      _Y(),
      _StreamID( 0 )
   {
      const char *ty[] = { ".P", ".C", "" };
      char        buf[K];

      sprintf( buf, "%s.%ld%s", und.name(), tExp, ty[splineType] );
      string::assign( buf );
      _Init( all );
   }

   /////////////////////////
   // Access
   /////////////////////////
public:
   const char *name() { return data(); }
   DoubleList &X()    { return _X; }
   DoubleList &Y()    { return _Y; }

   /////////////////////////
   // WatchList
   /////////////////////////
public:
   bool IsWatched()              { return( _StreamID != 0 ); }
   void SetWatch( int StreamID ) { _StreamID = StreamID; }

   /////////////////////////
   // Operations
   /////////////////////////
public:
   /**
    * \brief Build Spline from current LVC Snap
    *
    * \param all - Current LVC Snap
    * \param bForce - true to calc if not watched
    * \return true if Calc'ed; false if not
    */ 
   bool Calc( LVCAll &all, bool bForce=false )
   {
      OptionsCurve &lvc  = _und.lvc();
      Messages     &msgs = all.msgs();
      DoubleList   &xdb  = _und.strikes();
      double        x0   = xdb[0];
      double        x1   = xdb[xdb.size()-1];
      Message      *msg;
      int           ix;
      size_t        i, nk;
      double        x, y, m, dy;
      DoubleList    lX, lY, X, Y;

      // Pre-condition(s)

      if ( !bForce && !IsWatched() )
         return false;
      if ( !(nk=_kdb.size()) )
         return false;

      // OK to continue

      _X.clear();
      _Y.clear();
      for ( i=0; i<nk; i++ ) {
         ix  = _kdb[i];
         msg = msgs[ix];
         x   = lvc.StrikePrice( *msg );
         y   = lvc.MidQuote( *msg );
         lX.push_back( x );
         lY.push_back( y );
      }
      /*
       * 2a) Straight-line beginning to min Strike
       */
      if ( !lvc._oblong && ( nk >= 2 ) && ( x0 < lX[0] ) ) {
         m  = ( lY[0] - lY[1] ) / ( lX[0] - lX[1] );
         dy = m * ( x0 - lX[0] );
         y  = lY[0] + dy;
         X.push_back( x0 );
         Y.push_back( y );
      }
      for ( i=0; i<nk; X.push_back( lX[i] ), i++ );
      for ( i=0; i<nk; Y.push_back( lY[i] ), i++ );
      /*
       * 2b) Straight-line end to max Strike
       */
      if ( !lvc._oblong && ( nk >= 2 ) && ( lX[nk-1] < x1 ) ) {
         m  = ( lY[nk-2] - lY[nk-1] ) / ( lX[nk-2] - lX[nk-1] );
         dy = m * ( x1 - lX[nk-1] );
         y  = lY[nk-1] + dy;
         X.push_back( x1 );
         Y.push_back( y );
      }

      QUANT::CubicSpline cs( X, Y );

      for ( x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      for ( i=0; i<_X.size(); _Y.push_back( cs.Spline( _X[i] ) ), i++ );
      return true;
   }

   void Publish( Update &u )
   {
      // Pre-condition

      if ( !IsWatched() )
         return;

      // Initialize / Publish

      u.Init( name(), _StreamID, true );
      if ( _X.size() ) {
         u.AddField( l_fidInc, _xInc );
         u.AddField( l_fidXTy, xTy_NUM );
         u.AddField( l_fidX0,  _X[0] );
         u.AddVector( l_fidUNCV, _Y, l_Precision );
         u.Publish();
      }
      else
         u.PubError( "Empty Spline" );
   }

   /////////////////////////
   // (private) Helpers
   /////////////////////////
private:
   void _Init( LVCAll &all )
   {
      OptionsCurve &lvc  = _und.lvc();
      Messages     &msgs = all.msgs();
      Ints          udb;
      Message      *msg;
      u_int64_t    jExp;
      int          ix;

      /*
       * 1) Put / Call / Both??
       */
      switch( _type ) {
         case spline_put:  udb = _und.puts();  break; 
         case spline_call: udb = _und.calls(); break; 
         case spline_both: udb = _und.both();  break; 
      }
      /*
       * 2) Clear shit out; Jam away ...
       */
      _X.clear();
      _Y.clear();
      for ( size_t i=0; i<udb.size(); i++ ) {
         ix   = udb[i];
         msg  = msgs[ix];
         jExp = lvc.Expiration( *msg, false );
         if ( jExp == _tExp )
            _kdb.push_back( ix );
      }
   }

}; // class OptionsSpline


////////////////////////////////////////
//
//    S p l i n e P u b l i s h e r
//
////////////////////////////////////////
class SplinePublisher : public RTEDGE::PubChannel
{
private:
   OptionsCurve &_lvc;
   string        _svr;
   double        _pubRate;
   Underlyers    _underlyers;
   SplineMap     _splines;
   double        _tPub;

   /////////////////////
   // Constructor
   /////////////////////
public:
   SplinePublisher( OptionsCurve &lvc,
                    const char   *svr, 
                    const char   *svc,
                    double        pubRate ) :
      PubChannel( svc ),
      _lvc( lvc ),
      _svr( svr ),
      _pubRate( pubRate ),
      _underlyers(),
      _splines(),
      _tPub( 0.0 )
   {
      SetBinary( true );
      SetIdleCallback( true );
   }

   /////////////////////
   // Operations
   /////////////////////
public:
   int Calc( bool bForce=false )
   {
      LVCAll             &all = _lvc.ViewAll();
      SplineMap          &sdb = _splines;
      SplineMap::iterator it;
      OptionsSpline      *spl;
      int                 rc;

      for ( rc=0,it=sdb.begin(); it!=sdb.end(); it++ ) {
         spl = (*it).second;
         rc += spl->Calc( all, bForce ) ? 1 : 0;
      }
      return rc;
   }

   size_t LoadSplines( double xInc )
   {
      LVCAll                   &all = _lvc.ViewAll();
      Underlyers               &udb = _underlyers;
      SplineMap                &sdb = _splines;
      SortedStringSet           ndb = _lvc.Underlyers( all );
      SortedStringSet::iterator nt;
      Underlyer                *und;
      OptionsSpline            *spl;
      const char               *tkr;
      u_int64_t                 tExp;
      string                    s;
      SplineType                sTy;

      /*
       * 1) Underlyers
       */
      for ( nt=ndb.begin(); nt!=ndb.end(); nt++ ) {
         tkr = (*nt).data();
         und = new Underlyer( _lvc, tkr );
         und->Init( all );
         udb.push_back( und );
      }
      /*
       * 2) Splines, from each Underlyer
       */
      for ( size_t i=0; i<udb.size(); i++ ) {
         SortedInt64Set          &edb = udb[i]->exps();
         SortedInt64Set::iterator et;

         und = udb[i];
         for ( et=edb.begin(); et!=edb.end(); et++ ) {
            tExp = (*et);
            for ( int o=0; o<=(int)spline_both; o++ ) {
               sTy    = (SplineType)o;
               spl    = new OptionsSpline( *und, tExp, sTy, xInc, all );
               s      = spl->name();
               sdb[s] = spl;
            }
         }
      }
      return sdb.size();
   }

   const char *PubStart()
   {
      return Start( _svr.data() );
   }

   /////////////////////
   // RTEDGE::PubChannel Interface
   /////////////////////
protected:
   virtual void OnConnect( const char *msg, bool bUp )
   {
      const char *ty = bUp ? "UP" : "DOWN";
      string      tm;

      LOG( "[%s] PUB-CONN %s : %s", pDateTimeMs( tm ), msg, ty );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      LVCAll             &all = _lvc.ViewAll();
      SplineMap          &sdb = _splines;
      Update             &u   = upd();
      SplineMap::iterator it;
      string              tm, s( tkr );
      OptionsSpline      *spl;

      LOG( "[%s] OPEN  %s", pDateTimeMs( tm ), tkr );
      if ( (it=sdb.find( s )) != sdb.end() ) {
         spl = (*it).second;
         spl->SetWatch( (size_t)arg );
         spl->Calc( all );
         spl->Publish( u );
      }
      else {
         u.Init( tkr, arg );
         u.PubError( "non-existent ticker" );
      }
   }

   virtual void OnPubClose( const char *tkr )
   {
      SplineMap          &sdb = _splines;
      SplineMap::iterator it;
      string              tm, s( tkr );
      OptionsSpline      *spl;

      LOG( "[%s] CLOSE %s", pDateTimeMs( tm ), tkr );
      if ( (it=sdb.find( s )) != sdb.end() ) {
         spl = (*it).second;
         spl->SetWatch( 0 );
      }
   }

   virtual void OnOpenBDS( const char *bds, void *tag )
   {
      SplineMap                &sdb = _splines;
      SplineMap::iterator       it;
      SortedStringSet           srt;
      SortedStringSet::iterator st;
      string                    tm, k;
      char                     *tkr;
      vector<char *>            tkrs;

      // We're a whore

      LOG( "[%s] OPEN.BDS %s", pDateTimeMs( tm ), bds );
      for ( it=sdb.begin(); it!=sdb.end(); srt.insert( (*it).first ), it++ );
      for ( st=srt.begin(); st!=srt.end(); st++ ) {
         k   = (*st);
         if ( (it=sdb.find( k )) != sdb.end() ) {
            tkr = (char *)(*it).second->name();
            tkrs.push_back( tkr );
         }
      }
      tkrs.push_back( (char *)0 );
      PublishBDS( bds, (size_t)tag, tkrs.data() );
   }

   virtual void OnCloseBDS( const char *bds )
   {
      string tm;

      LOG( "[%s] CLOSE.BDS %s", pDateTimeMs( tm ), bds );
   }

   virtual Update *CreateUpdate()
   {
      return new Update( *this );
   }

   virtual void OnIdle()
   {
      double now = TimeNs();
      double age = now - _tPub;
      int    n;

      // Pre-condition

      if ( age < _pubRate )
         return;

      // Rock on

      _tPub = now;
      if ( (n=Calc()) )
         LOG( "OnIdle() : %d calc'ed", n );
   }

}; // SplinePublisher


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids;
   string      s;
   bool        aOK, bCfg, obl;
   double      rate, xInc;
   const char *db, *svr, *svc;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      LOG( "%s", OptionsCurveID() );
      return 0;
   }

   // cmd-line args

   db   = "./cache.lvc";
   svr  = "localhost:9015";
   svc  = "options.curve";
   rate = 1.0;
   xInc = 1.0; // 1 dollareeny
   obl  = true;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -svr     <MDD Edge3 host:port> ] \\ \n";
      s += "       [ -svc     <MDD Publisher Service> ] \\ \n";
      s += "       [ -rate    <SnapRate> ] \\ \n";
      s += "       [ -xInc    <Spline Increment> ] \\ \n";
      s += "       [ -oblong  <false to 'square up'> ] \\ \n";
      LOG( (char *)s.data(), argv[0] );
      LOG( "   Defaults:" );
      LOG( "      -db      : %s", db );
      LOG( "      -svr     : %s", svr );
      LOG( "      -svc     : %s", svc );
      LOG( "      -rate    : %.2f", rate );
      LOG( "      -xInc    : %.2f", xInc );
      LOG( "      -oblong  : %s", _pBool( obl ) );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( int i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-db" ) )
         db = argv[++i];
      else if ( !::strcmp( argv[i], "-svr" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-svc" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-xInc" ) )
         xInc = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-rate" ) )
         rate = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-oblong" ) )
         obl = _IsTrue( argv[++i] );
   }

   /////////////////////
   // Do it
   /////////////////////
   OptionsCurve    lvc( db, obl );
   SplinePublisher pub( lvc, svr, svc, rate );
   double          d0, age;
   size_t          ns;

   /*
    * Load Splines
    */
   d0  = lvc.TimeNs();
   ns    = pub.LoadSplines( xInc );
   age = 1000.0 * ( lvc.TimeNs() - d0 );
   LOG( "%ld splines loaded in %dms", ns, (int)age );
   d0  = lvc.TimeNs();
   pub.Calc( true );
   age = 1000.0 * ( lvc.TimeNs() - d0 );
   LOG( "%ld splines calculated in %dms", ns, (int)age );
   /*
    * Rock on
    */
   LOG( "%s", pub.PubStart() );
   LOG( "Hit <ENTER> to quit..." );
   getchar();
   LOG( "Shutting down ..." );
   pub.Stop();
   LOG( "Done!!" );
   return 1;
}
