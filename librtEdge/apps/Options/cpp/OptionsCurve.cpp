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
      _calls = _lvc.GetUnderlyer( all, name(), true );
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

      sprintf( buf, "%s %ld %c", und.name(), tExp, ty );
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

      if ( !_StreamID )
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
         jExp = lvc.Expiration( *msg );
         if ( jExp == _tExp )
            _kdb.push_back( ix );
      }
   }

}; // class OptionsSpline


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids;
   string      s;
   bool        aOK, bCfg, bPut;
   double      rate, xInc;
   const char *svr, *tkr, *exp;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", OptionsCurveID() );
      return 0;
   }

   // cmd-line args

   svr  = "./cache.lvc";
   tkr  = "AAPL";
   rate = 1.0;
   bPut = true;
   exp  = NULL;
   xInc = 1.0; // 1 dollareeny
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -u       <Underlyer> \\ \n";
      s += "       [ -r       <DumpRate> ] \\ \n";
      s += "       [ -put     <Dump PUT> ] \\ \n";
      s += "       [ -exp     <Expiration Date> ] \\ \n";
      s += "       [ -xInc    <Spline Increment> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -db      : %s\n", svr );
      printf( "      -u       : %s\n", tkr );
      printf( "      -r       : %.2f\n", rate );
      printf( "      -put     : %s\n", _pBool( bPut ) );
      printf( "      -exp     : <empty>\n" );
      printf( "      -xInc    : %.2f\n", xInc );
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
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-u" ) )
         tkr = argv[++i];
      else if ( !::strcmp( argv[i], "-r" ) )
         rate = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-put" ) )
         bPut = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-exp" ) )
         exp = argv[++i];
   }
   if ( !exp ) {
      printf( "Must specify -exp; Exitting ...\n" );
      return 0;
   }

   /////////////////////
   // Do it
   /////////////////////
   OptionsCurve lvc( svr );
   LVCAll      &all = lvc.ViewAll();
   u_int64_t    tExp = lvc.ParseDate( exp, false );
   u_int64_t    jExp = lvc.ParseDate( exp, true );
   Underlyer    und( lvc, tkr );
   double       d0, age;

   /*
    * First kiss : Walk all; build sorted list of indices by ( Expire, Strike )
    */
   d0     = lvc.TimeNs();
   und.Init( all );
   age   = 1000.0 * ( lvc.TimeNs() - d0 );
   ::fprintf( stdout, "Underlyer.Init() in %dms\n", (int)age );
   /*
    * Rock on
    */
   if ( und.IsValidExp( tExp ) ) {
      OptionsSpline spline( und, jExp, bPut, xInc, all );
      int          ms   = spline.Calc( all );
      DoubleList   &X   = spline.X();
      DoubleList   &Y   = spline.Y();

      ::fprintf( stdout, "ViewAll() in %.2fms\n", all.dSnap()*1000.0 );
      ::fprintf( stdout, "Strike,Price,\n" );
      for ( size_t i=0; i<X.size(); i++ )
         ::fprintf( stdout, "%.2f,%.2f,\n", X[i], Y[i] );
      ::fprintf( stdout, "CubicSpline() in %dmS\n", ms );
   }
   else
      ::fprintf( stdout, "ERROR : Invalid Expire Date %s for %s\n", exp, tkr );
   ::fprintf( stdout, "Done!!\n " );
   ::fflush( stdout );
   return 1;
}
