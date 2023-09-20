/******************************************************************************
*
*  OptionsCurve.cpp
*
*  REVISION HISTORY:
*     13 SEP 2023 jcs  Created (from LVCDump.cpp)
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <OptionsBase.cpp>

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
   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsCurve( const char *svr ) :
      OptionsBase( svr )
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
   SortedInt64Set _exps;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   Underlyer( OptionsCurve &lvc, const char *und ) :
      string( und ),
      _lvc( lvc ),
      _puts(),
      _calls(),
      _exps()
   { ; }

   /////////////////////////
   // Access
   /////////////////////////
public:
   OptionsCurve   &lvc()   { return _lvc; }
   const char     *name()  { return data(); }
   Ints           &puts()  { return _puts; }
   Ints           &calls() { return _calls; }
   SortedInt64Set &exps()  { return _exps; }

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
      _puts  = _lvc.GetUnderlyer( all, name(), true );
      _calls = _lvc.GetUnderlyer( all, name(), false );
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
   bool       _bPut;
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
                  bool       bPut, 
                  double     xInc,
                  LVCAll    &all ) :
      string(),
      _und( und ),
      _tExp( tExp ),
      _bPut( bPut ),
      _xInc( xInc ),
      _kdb(),
      _X(),
      _Y(),
      _StreamID( 0 )
   {
      const char ty = bPut ? 'P' : 'C';
      char       buf[K];

      sprintf( buf, "%s.%ld.%c", und.name(), tExp, ty );
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
    * \return Millis to calc and build spline
    */ 
   int Calc( LVCAll &all )
   {
      OptionsCurve &lvc  = _und.lvc();
      Messages     &msgs = all.msgs();
      Message      *msg;
      int           ix;
      double        d0, age;
      DoubleList    X, Y;

      d0 = lvc.TimeNs();
      _X.clear();
      _Y.clear();
      for ( size_t i=0; i<_kdb.size(); i++ ) {
         ix   = _kdb[i];
         msg  = msgs[ix];
         X.push_back( lvc.StrikePrice( *msg ) );
         Y.push_back( lvc.MidQuote( *msg ) ); 
      }
      if ( !_kdb.size() )
         return 0;

      double             x0 = X[0];
      double             x1 = X[X.size()-1];
      QUANT::CubicSpline cs( X, Y );

      for ( double x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      for ( size_t i=0; i<_X.size(); _Y.push_back( cs.Spline( _X[i] ) ), i++ );
      age = ( lvc.TimeNs() - d0 ) * 1000.0;
      return (int)age;
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
      Ints         &udb  = _bPut ? _und.puts() : _und.calls();
      Message      *msg;
      u_int64_t    jExp;
      int          ix;

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
   Underlyers    _underlyers;
   SplineMap     _splines;

   /////////////////////
   // Constructor
   /////////////////////
public:
   SplinePublisher( OptionsCurve &lvc,
                    const char   *svr, 
                    const char   *svc ) :
      PubChannel( svc ),
      _lvc( lvc ),
      _svr( svr ),
      _underlyers(),
      _splines()
   {
      SetBinary( true );
   }

   /////////////////////
   // Operations
   /////////////////////
public:
   void Calc()
   {
      LVCAll             &all = _lvc.ViewAll();
      SplineMap          &sdb = _splines;
      SplineMap::iterator it;
      OptionsSpline      *spl;

      for ( it=sdb.begin(); it!=sdb.end(); it++ ) {
         spl = (*it).second;
         spl->Calc( all );
      }
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
      bool                      bPut;

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
            for ( int o=0; o<2; o++ ) {
               bPut = ( o == 0 );
               spl    = new OptionsSpline( *und, tExp, bPut, xInc, all );
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
      SplineMap          &sdb = _splines;
      Update             &u   = upd();
      SplineMap::iterator it;
      string              tm, s( tkr );
      OptionsSpline      *spl;

      LOG( "[%s] OPEN  %s", pDateTimeMs( tm ), tkr );
      if ( (it=sdb.find( s )) != sdb.end() ) {
         spl = (*it).second;
         spl->SetWatch( (size_t)arg );
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

}; // SplinePublisher


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids;
   string      s;
   bool        aOK, bCfg, bds;
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
   bds  = true;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -svr     <MDD Edge3 host:port> ] \\ \n";
      s += "       [ -svc     <MDD Publisher Service> ] \\ \n";
      s += "       [ -r       <SnapRate> ] \\ \n";
      s += "       [ -xInc    <Spline Increment> ] \\ \n";
      s += "       [ -bds     <true for BDS> ] \\ \n";
      LOG( (char *)s.data(), argv[0] );
      LOG( "   Defaults:" );
      LOG( "      -db      : %s", db );
      LOG( "      -svr     : %s", svr );
      LOG( "      -svc     : %s", svc );
      LOG( "      -r       : %.2f", rate );
      LOG( "      -xInc    : %.2f", xInc );
      LOG( "      -bds     : %s", _pBool( bds ) );
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
      else if ( !::strcmp( argv[i], "-r" ) )
         rate = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-bds" ) )
         bds = _IsTrue( argv[++i] );
   }

   /////////////////////
   // Do it
   /////////////////////
   OptionsCurve    lvc( db );
   SplinePublisher pub( lvc, svr, svc );
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
   pub.Calc();
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
