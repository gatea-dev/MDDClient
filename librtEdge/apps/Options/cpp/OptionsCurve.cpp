/******************************************************************************
*
*  OptionsCurve.cpp
*     TODO :
*     2) Spline.Calc_ByStrike()
*     3) Surface
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

static int     l_Precision =     2;
static double  l_StrikeMul =   100;

// Record Templates

static int xTy_DATE    =     0;
static int xTy_NUM     =     1;
static int l_fidInc    =     6; // X-axis Increment
static int l_fidXTy    =     7; // X-axis type : xTy_DATE = Date; Else Numeric
static int l_fidX0     =     8; // If Numeric, first X value
static int l_fidFidX   = -8001;
static int l_fidFidY   = -8002;

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
   bool   _square;
   double _xInc;
   int    _maxX;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsCurve( const char *svr, 
                 bool        square,
                 double      xInc,
                 int         maxX ) :
      OptionsBase( svr ),
      _square( square ),
      _xInc( xInc ),
      _maxX( maxX )
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
   IndexCache     _byExp; // Expiration
   IndexCache     _byStr; // Strike Price
   SortedInt64Set _exps;
   DoubleList     _strikes;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   Underlyer( OptionsCurve &lvc, const char *und ) :
      string( und ),
      _lvc( lvc ),
      _byExp(),
      _byStr(),
      _exps(),
      _strikes()
   { ; }

   /////////////////////////
   // Access
   /////////////////////////
public:
   OptionsCurve   &lvc()     { return _lvc; }
   const char     *name()    { return data(); }
   IndexCache     &byExp()   { return _byExp; }
   IndexCache     &byStr()   { return _byStr; }
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
      Ints     &both = _byExp._both;
      Message  *msg;
      int       ix;

      /*
       * 1) d/b Indices of all Puts and Calls
       */
      _byExp._puts     = _lvc.GetUnderlyer( all, name(), spline_put,  true  );
      _byExp._calls = _lvc.GetUnderlyer( all, name(), spline_call, true  );
      _byExp._both  = _lvc.GetUnderlyer( all, name(), spline_both, true  );
      _byStr._puts  = _lvc.GetUnderlyer( all, name(), spline_put,  false );
      _byStr._calls = _lvc.GetUnderlyer( all, name(), spline_call, false );
      _byStr._both  = _lvc.GetUnderlyer( all, name(), spline_both, false );
      /*
       * 2) All Expiration Dates in sorted order
       */
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
         strikes.insert( (u_int64_t)( _lvc.StrikePrice( *msg ) * l_StrikeMul ) );
      }
      for ( it=strikes.begin(); it!=strikes.end(); it++ )
         _strikes.push_back( (*it) / l_StrikeMul );
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
   Underlyer    &_und;
   OptionsCurve &_lvc;
   IndexCache   &_idx;
   u_int64_t     _tExp;
   u_int64_t     _dStr;
   SplineType    _type;
   Ints          _kdb;
   DoubleList    _X;
   DoubleList    _Y;
   double        _xInc;
   int           _StreamID;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   /** \brief By Expiration */
   OptionsSpline( Underlyer &und, 
                  u_int64_t  tExp, 
                  SplineType splineType,
                  LVCAll    &all ) :
      string(),
      _und( und ),
      _lvc( und.lvc() ),
      _idx( und.byExp() ),
      _tExp( tExp ),
      _dStr( 0 ),
      _type( splineType ),
      _kdb(),
      _X(),
      _Y(),
      _xInc( und.lvc()._xInc ),
      _StreamID( 0 )
   {
      const char *ty[] = { ".P", ".C", "" };
      char        buf[K];

      sprintf( buf, "%s %ld.E%s", und.name(), tExp, ty[splineType] );
      string::assign( buf );
      _Init( all );
   }

   /** \brief By Strike  Price */
   OptionsSpline( Underlyer &und, 
                  double     dStr, 
                  SplineType splineType,
                  LVCAll    &all ) :
      string(),
      _und( und ),
      _lvc( und.lvc() ),
      _idx( und.byStr() ),
      _tExp( 0 ),
      _dStr( (u_int64_t)( dStr * l_StrikeMul ) ),
      _type( splineType ),
      _kdb(),
      _X(),
      _Y(),
      _xInc( und.lvc()._xInc ),
      _StreamID( 0 )
   {
      const char *ty[] = { ".P", ".C", "" };
      char        buf[K];

      sprintf( buf, "%s %.2f.X%s", und.name(), dStr, ty[splineType] );
      string::assign( buf );
      _Init( all );
   }

   /////////////////////////
   // Access
   /////////////////////////
public:
   const char *name()  { return data(); }
   DoubleList &X()     { return _X; }
   DoubleList &Y()     { return _Y; }
   bool        byExp() { return( _tExp != 0 ); }

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
      int           ix, np;
      size_t        i, nk;
      double        x, y, m, dy, xi;
      double        dExp, dStr;
      DoubleList    lX, lY, X, Y;

      // Pre-condition(s)

      if ( !bForce && !IsWatched() )
         return false;
      if ( !(nk=_kdb.size()) )
         return false;

      /*
       * 1) Pull out real-time Knot values from LVC
       */
      _X.clear();
      _Y.clear();
      for ( i=0; i<nk; i++ ) {
         ix   = _kdb[i];
         msg  = msgs[ix];
         dStr = lvc.StrikePrice( *msg );
         dExp = lvc.Expiration( *msg, true );
         x    = byExp() ? dStr : dExp;
         y    = lvc.MidQuote( *msg );
         lX.push_back( x );
         lY.push_back( y );
      }
      /*
       * 2a) Straight-line beginning to min Strike (or Expiration)
       */
      if ( !lvc._square ) {
         x0 = lX[0];
         x1 = lX[nk-1];
      }
      if ( ( nk >= 2 ) && ( x0 < lX[0] ) ) {
         m  = ( lY[0] - lY[1] ) / ( lX[0] - lX[1] );
         dy = m * ( x0 - lX[0] );
         y  = lY[0] + dy;
         X.push_back( x0 );
         Y.push_back( y );
      }
      for ( i=0; i<nk; X.push_back( lX[i] ), i++ );
      for ( i=0; i<nk; Y.push_back( lY[i] ), i++ );
      /*
       * 2b) Straight-line end to max Strike (or Expiration)
       */
      if ( ( nk >= 2 ) && ( lX[nk-1] < x1 ) ) {
         m  = ( lY[nk-2] - lY[nk-1] ) / ( lX[nk-2] - lX[nk-1] );
         dy = m * ( x1 - lX[nk-1] );
         y  = lY[nk-1] + dy;
         X.push_back( x1 );
         Y.push_back( y );
      }
      /*
       * 3) Max Number of Data Points 
       */
      np = ( x1-x0 ) / _xInc;
      xi = _xInc;
      for ( ; np > lvc._maxX; _xInc+=xi, np = ( x1-x0 ) / _xInc );

      QUANT::CubicSpline cs( X, Y );

      for ( x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      _Y = cs.Spline( _X );
      return true;
   }

   void Publish( Update &u )
   {
      double     x0, xInc;
      int        xTy;

      // Pre-condition

      if ( !IsWatched() )
         return;

      // Initialize / Publish

      u.Init( name(), _StreamID, true );
      if ( _X.size() ) {
         DoubleList unx;
         DoubleList &X = byExp() ? _X : _lvc.julNum2Unix( _X, unx );

         x0   = X[0];
         xInc = _xInc;
         xTy  = byExp() ? xTy_NUM : xTy_DATE;
         if ( !byExp() )
            xInc *= byExp() ? 1.0 : ( 12.0 / 365.0 ); // xInc in Months ...
         u.AddField( l_fidInc, xInc );
         u.AddField( l_fidXTy, xTy );
         u.AddField( l_fidX0,  x0 );
         /*
          * X-axis values if Strike Spline (X-axis == Date)
          */
         if ( xTy == xTy_DATE )
            u.AddVector( l_fidFidX, X, 0 );
         u.AddVector( l_fidFidY, _Y, l_Precision );
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
      Messages     &msgs  = all.msgs();
      Ints          udb;
      Message      *msg;
      u_int64_t    jExp, jStr;
      double       dStr;
      int          ix;

      /*
       * 1) Put / Call / Both??
       */
      switch( _type ) {
         case spline_put:  udb = _idx._puts;  break;
         case spline_call: udb = _idx._calls; break;
         case spline_both: udb = _idx._both;  break;
      }
      /*
       * 2) Clear shit out; Jam away ...
       */
      _X.clear();
      _Y.clear();
      for ( size_t i=0; i<udb.size(); i++ ) {
         ix   = udb[i];
         msg  = msgs[ix];
         if ( byExp() ) {
            jExp = _lvc.Expiration( *msg, false );
            if ( jExp == _tExp )
               _kdb.push_back( ix );
         }
         else {
            dStr = _lvc.StrikePrice( *msg );
            jStr = (u_int64_t)( dStr * l_StrikeMul );
            if ( jStr == _dStr )
               _kdb.push_back( ix );
         }
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

   size_t LoadSplines()
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
      double                    dX;
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
         DoubleList              &xdb = udb[i]->strikes();
         SortedInt64Set::iterator et;
         DoubleList::iterator     xt;

         und = udb[i];
         for ( et=edb.begin(); et!=edb.end(); et++ ) {
            tExp = (*et);
            for ( int o=0; o<=(int)spline_both; o++ ) {
               sTy    = (SplineType)o;
               spl    = new OptionsSpline( *und, tExp, sTy, all );
               s      = spl->name();
               sdb[s] = spl;
            }
         }
         for ( xt=xdb.begin(); xt!=xdb.end(); xt++ ) {
            dX = (*xt);
            for ( int o=0; o<=(int)spline_call; o++ ) {
               sTy    = (SplineType)o;
               spl    = new OptionsSpline( *und, dX, sTy, all );
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
      n     = Calc();
#ifdef DEBUG
      if ( n )
         LOG( "OnIdle() : %d calc'ed", n );
#endif // DEBUG
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
   bool        aOK, bCfg, sqr;
   double      rate, xInc;
   int         maxX;
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
   maxX = 1000;
   sqr  = false;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -svr     <MDD Edge3 host:port> ] \\ \n";
      s += "       [ -svc     <MDD Publisher Service> ] \\ \n";
      s += "       [ -rate    <SnapRate> ] \\ \n";
      s += "       [ -xInc    <Spline Increment> ] \\ \n";
      s += "       [ -maxX    <Max Spline Values> ] \\ \n";
      s += "       [ -square  <true to 'square up'> ] \\ \n";
      LOG( (char *)s.data(), argv[0] );
      LOG( "   Defaults:" );
      LOG( "      -db      : %s", db );
      LOG( "      -svr     : %s", svr );
      LOG( "      -svc     : %s", svc );
      LOG( "      -rate    : %.2f", rate );
      LOG( "      -xInc    : %.2f", xInc );
      LOG( "      -maxX    : %d", maxX );
      LOG( "      -square  : %s", _pBool( sqr ) );
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
      else if ( !::strcmp( argv[i], "-maxX" ) )
         maxX = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-rate" ) )
         rate = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-square" ) )
         sqr = _IsTrue( argv[++i] );
   }

   /////////////////////
   // Do it
   /////////////////////
   OptionsCurve    lvc( db, sqr, xInc, maxX );
   SplinePublisher pub( lvc, svr, svc, rate );
   double          d0, age;
   size_t          ns;

   /*
    * Load Splines
    */
   LOG( "Config._square = %s", _pBool( lvc._square ) );
   LOG( "Config._xInc   = %.2f", lvc._xInc );
   LOG( "Config._maxX   = %d", lvc._maxX );
   d0  = lvc.TimeNs();
   ns    = pub.LoadSplines();
   age = 1000.0 * ( lvc.TimeNs() - d0 );
   LOG( "%ld splines loaded in %dms", ns, (int)age );
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
