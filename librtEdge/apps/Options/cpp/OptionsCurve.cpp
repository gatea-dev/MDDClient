/******************************************************************************
*
*  OptionsCurve.cpp
*
*  REVISION HISTORY:
*     13 SEP 2023 jcs  Created (from LVCDump.cpp)
*     20 SEP 2023 jcs  FullSpline
*     13 OCT 2023 jcs  OptionsSurface
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <OptionsBase.cpp>
#ifndef WIN32
#include <SigHandler.h>
#endif // WIN32
#include <assert.h>
#include <list>

// Configurable Precision

static double      l_StrikeMul =   100.0;
static double      l_StrikeDiv =   1.0 / l_StrikeMul;
static size_t      l_NoIndex   =    -1;
static const char *l_Undef     = "???";

// Record Templates

static int xTy_DATE    =     0;
static int xTy_NUM     =     1;
static int l_fidInc    =     6; // X-axis Increment
static int l_fidXTy    =     7; // X-axis type : xTy_DATE = Date; Else Numeric
static int l_fidX0     =     8; // If Numeric, first X value
static int l_fidVecX   = -8001;
static int l_fidVecY   = -8002;
static int l_fidVecZ   = -8003;

// Forwards / Collections

class OptionsSpline;
class OptionsSurface;
class Underlyer;

typedef hash_map<string, OptionsSpline *>  SplineMap;
typedef hash_map<string, OptionsSurface *> SurfaceMap;
typedef vector<Underlyer *>                Underlyers;

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
      cp += sprintf( cp, "%s\n", rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


////////////////////////////////////////////////
//
//       c l a s s    K n o t D e f
//
////////////////////////////////////////////////
class KnotDef
{
public:
   OptionsSpline *_splineX;  // Non-zero implies calc each time 
   OptionsSpline *_splineE;  // Non-zero implies calc each time 
   const char    *_pSplX;    // _splineX->name()
   const char    *_pSplE;    // _splineE->name()
   char           _lvcName[128];
   u_int64_t      _strike;
   u_int64_t      _exp;
   u_int64_t      _jExp;    // julNum( exp )
   size_t         _idx;     // Non-zero implies real-time
   double         _tUpd;    // Time of last update in LVC
   double         _lwc;     // Last Value from LVC

}; // KnotDef

typedef vector<KnotDef>              KnotList;
typedef vector<KnotList>             KnotGrid;
typedef hash_map<u_int64_t, KnotDef> KnotDefMap;


////////////////////////////////////////////////
//
//     c l a s s   O p t i o n s C u r v e
//
////////////////////////////////////////////////
class OptionsCurve : public OptionsBase
{
public:
   bool    _square;
   bool    _trim;
   bool    _dump;
   bool    _knotCalc;
   int     _precision;
   double  _xInc;
   double  _yInc;
   int     _maxX;
   KnotDef _kz;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsCurve( const char *svr, 
                 bool        square,
                 bool        trim,
                 bool        dump,
                 bool        knotCalc,
                 int         precision,
                 double      xInc,
                 double      yInc,
                 int         maxX ) :
      OptionsBase( svr ),
      _square( square ),
      _trim( trim ),
      _dump( dump ),
      _knotCalc( knotCalc ),
      _precision( precision ),
      _xInc( xInc ),
      _yInc( yInc ),
      _maxX( maxX )
   {
      ::memset( &_kz, 0, sizeof( _kz ) );
   }

}; // class OptionsCurve



////////////////////////////////////////////////
//
//    c l a s s   R i s k F r e e S p l i n e
//
////////////////////////////////////////////////
class RiskFreeSpline : public string
{
private:
   OptionsCurve      &_lvc;
   XmlElem           &_xe;
   string             _svc;
   int                _fid;
   KnotList           _kdb;
   _DoubleList         _X;
   _DoubleList         _Y;
   QUANT::CubicSpline _CS; // Last Calc
   double             _xInc;
   time_t             _tCalc;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   RiskFreeSpline( OptionsCurve &lvc, XmlElem &xe ) :
      _lvc( lvc ),
      _xe( xe ),
      _svc( xe.getAttrValue( "Service", "velocity" ) ),
      _fid( xe.getAttrValue( "FieldID", 6 ) ),
      _kdb(),
      _X(),
      _Y(),
      _CS(),
      _xInc( 1 ), // Daily
      _tCalc( 0 )
   { ; }

   /////////////////////////
   // Access
   /////////////////////////
public:
   _DoubleList &X()        { return _X; }
   _DoubleList &Y()        { return _Y; }
   size_t      NumKnots() { return _kdb.size(); }

   QUANT::CubicSpline &CS( LVCAll &all, time_t now )
   {
      SnapKnots( all, now );
      return _CS;
   }

   /////////////////////////
   // Operations
   /////////////////////////
public:
   /**
    * \brief Initialize / Load Spline from LVC
    *
    * \param all - Current LVC Snap
    */ 
   void Load( LVCAll &all )
   {
      XmlElemVector &edb  = _xe.elements();
      Messages      &msgs = all.msgs();
      const char    *svc, *tkr;
      Message       *msg;
      XmlElem       *xk;
      KnotDef        k;

      // Pre-condition

      if ( _kdb.size() )
         return;

      // Safe to to continue ...

      svc = _svc.data();
      for ( size_t i=0; i<edb.size(); i++ ) {
         ::memset( &k, 0, sizeof( k ) );
         xk = edb[i];
         if ( !(tkr=xk->getAttrValue( "Ticker", (const char *)0 )) )
            continue; // for-i
         if ( !(k._exp=xk->getAttrValue( "Interval", (int)0 )) )
            continue; // for-i
         strcpy( k._lvcName, tkr );
         k._lwc = xk->getAttrValue( "Value", 0.0 );
         if ( k._lwc ) { 
            _kdb.push_back( k );
            continue; // for-i
         }
         /*
          * Stupid : Do it once to figure out LVC Index since
          * LVC_Snap() / LVC_View() do not give you the index
          */
         for ( size_t j=0; j<msgs.size(); j++ ) {
            msg = msgs[j];
            if ( ::strcmp( msg->Ticker(), tkr ) )
               continue; // for-j
            if ( ::strcmp( msg->Service(), svc ) )
               continue; // for-j
            k._idx = j;
            _kdb.push_back( k );
            break; // for-j
         }
      }
   }

   /**
    * \brief Build Spline from current LVC Snap
    *
    * \param all - Current LVC Snap
    * \param now - Current Unix Time
    * \return true if Calc'ed; false if not
    */ 
   bool SnapKnots( LVCAll &all, time_t now )
   {
      Messages    &msgs = all.msgs();
      Message     *msg;
      KnotDef      k;
      size_t       i, nk;
      DoubleXYList XY;
      DoubleXY     pt;

      // Pre-condition(s)

      if ( _tCalc == now )
         return false;
      if ( !(nk=_kdb.size()) )
         return false;

      /*
       * 1) Pull out real-time Knot values from LVC
       */
      _tCalc = now;
      _X.clear();
      _Y.clear();
      for ( i=0; i<nk; i++ ) {
         k     = _kdb[i];
         msg   = k._idx ? msgs[k._idx] : (Message *)0;
         pt._x = _kdb[i]._exp;
         pt._y = msg ? _lvc.GetAsDouble( *msg, _fid ) : k._lwc;
         XY.push_back( pt );
      }

      QUANT::CubicSpline cs( XY );

      _CS = cs;
      return true;
   }

   /**
    * \brief Build Spline from current LVC Snap
    *
    * \param all - Current LVC Snap
    * \param now - Current Unix Time
    */ 
   void Calc( LVCAll &all, time_t now )
   {
      DoubleXYList &XY = _CS.XY();
      size_t         nx;
      double         x, x0, x1;

      // Pre-condition(s)

      if ( !SnapKnots( all, now ) )
         return;

      // Rock on

      nx = XY.size();
      x0 = XY[1]._x;
      x1 = XY[nx-1]._x;
      for ( x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      _Y  = _CS.Spline( _X );
   }

}; // class RiskFreeSpline



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
   _DoubleList    _strikes;
   SplineMap      _splines;

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
      _strikes(),
      _splines()
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
   _DoubleList     &strikes() { return _strikes; }

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
         _strikes.push_back( l_StrikeDiv * (*it)  );
   }


   /////////////////////
   // Spline Lookup
   /////////////////////
public:
   /**
    * \brief Get best spline by Strike
    *
    * \param strike - Strike Price
    * \param bPut - true for put; false for call
    */
   OptionsSpline *GetSpline( double strike, bool bPut )
   {
      SplineMap          &sdb   = _splines;
      SplineType          splTy = bPut ? spline_put : spline_call; 
      string              sx( SplineName( strike, splTy ) );
      SplineMap::iterator it;
      OptionsSpline      *spl;

      spl = (OptionsSpline *)0;
      if ( (it=sdb.find( sx )) != sdb.end() )
         spl = (*it).second;
      return spl;
   }

   /**
    * \brief Get best spline by Expiration
    *
    * \param tExp - Expiration Date
    * \param bPut - true for put; false for call
    */
   OptionsSpline *GetSpline( u_int64_t tExp, bool bPut )
   {
      SplineMap          &sdb   = _splines;
      SplineType          splTy = bPut ? spline_put : spline_call; 
      string              se( SplineName( tExp, splTy ) );
      SplineMap::iterator it;
      OptionsSpline      *spl;

      spl = (OptionsSpline *)0;
      if ( (it=sdb.find( se )) != sdb.end() )
         spl = (*it).second;
      return spl;
   }

   void Register( OptionsSpline *spl, string &splineName )
   {
      string s( splineName );

      _splines[s] = spl;
   }

   /////////////////////////
   // Spline Namespace
   /////////////////////////
public:
   /**
    * \brief Return Spline Name by expiration
    *
    * \param tExp : Expiration
    * \param splineType : Put, Call, Both
    * \return Spline Name by expiration
    */
   string SplineName( u_int64_t tExp, SplineType splineType )
   {
      const char *ty[] = { ".P", ".C", "" };
      char        buf[K];

      sprintf( buf, "%s %ld.E%s", name(), tExp, ty[splineType] );
      return string( buf );
   }

   /**
    * \brief Return Spline Name by Strike
    *
    * \param dStr : Strike Price
    * \param splineType : Put, Call, Both
    * \return Spline Name by Strike
    */
   string SplineName( double dStr, SplineType splineType )
   {
      const char *ty[] = { ".P", ".C", "" };
      char        buf[K];

      sprintf( buf, "%s %.2f.X%s", name(), dStr, ty[splineType] );
      return string( buf );
   }

   void SnapLVCFlds( KnotDef &k, Message &m )
   {
      Field      *fld;
      const char *pf;

      // 1) Display Name

      pf = ( (fld=m.GetField( 3 )) ) ?  fld->GetAsString() : l_Undef;
      strcpy( k._lvcName, pf );

      // 2) Value

      k._lwc = _lvc.MidQuote( m );
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
   Underlyer         &_und;
   OptionsCurve      &_lvc;
   IndexCache        &_idx;
   u_int64_t          _tExp;
   u_int64_t          _dStr;
   SplineType         _type;
   KnotList           _kdb;
   _DoubleList         _X;
   _DoubleList         _Y;
   QUANT::CubicSpline _CS; // Last Calc
   double             _xInc;
   int                _StreamID;
   time_t             _tCalc;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   /** \brief By Expiration */
   OptionsSpline( Underlyer &und, 
                  u_int64_t  tExp, 
                  SplineType splineType,
                  LVCAll    &all ) :
      string( und.SplineName( tExp, splineType ) ),
      _und( und ),
      _lvc( und.lvc() ),
      _idx( und.byExp() ),
      _tExp( tExp ),
      _dStr( 0 ),
      _type( splineType ),
      _kdb(),
      _X(),
      _Y(),
      _CS(),
      _xInc( und.lvc()._xInc ),
      _StreamID( 0 ),
      _tCalc( 0 )
   {
      _Init( all );
   }

   /** \brief By Strike  Price */
   OptionsSpline( Underlyer &und, 
                  double     dStr, 
                  SplineType splineType,
                  LVCAll    &all ) :
      string( und.SplineName( dStr, splineType ) ),
      _und( und ),
      _lvc( und.lvc() ),
      _idx( und.byStr() ),
      _tExp( 0 ),
      _dStr( (u_int64_t)( dStr * l_StrikeMul ) ),
      _type( splineType ),
      _kdb(),
      _X(),
      _Y(),
      _CS(),
      _xInc( und.lvc()._xInc ),
      _StreamID( 0 ),
      _tCalc( 0 )
   {
      _Init( all );
   }

   /////////////////////////
   // Access
   /////////////////////////
public:
   const char *name()     { return data(); }
   _DoubleList &X()        { return _X; }
   _DoubleList &Y()        { return _Y; }
   u_int64_t   tExp()     { return _tExp; };
   u_int64_t   dStr()     { return _dStr; };
   bool        byExp()    { return( _tExp != 0 ); }
   size_t      NumKnots() { return _kdb.size(); }

   QUANT::CubicSpline &CS( LVCAll &all, time_t now )
   {
      SnapKnots( all, now );
      return _CS;
   }

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
    * \param now - Current Unix Time
    * \return true if Calc'ed; false if not
    */ 
   bool SnapKnots( LVCAll &all, time_t now )
   {
      OptionsCurve &lvc  = _und.lvc();
      Messages     &msgs = all.msgs();
      _DoubleList   &xdb  = _und.strikes();
      double        x0   = xdb[0];
      double        x1   = xdb[xdb.size()-1];
      Message      *msg;
      int           ix, np;
      size_t        i, nk;
      double        m, dy, xi;
      double        dExp, dStr, dUpd;
      DoubleXYList  lXY, XY;
      DoubleXY      pt;
      bool          bUpd;

      // Pre-condition(s)

      if ( _tCalc == now )
         return false;
      if ( !(nk=NumKnots()) )
         return false;

      /*
       * 1) Pull out real-time Knot values from LVC
       */
      _tCalc = now;
      _X.clear();
      _Y.clear();
      for ( i=0,bUpd=false; i<nk; i++ ) {
         ix    = _kdb[i]._idx;
         msg   = msgs[ix];
         dUpd  = msg->MsgTime();
         bUpd |= ( (time_t)dUpd > _tCalc );
         dStr  = lvc.StrikePrice( *msg );
         dExp  = lvc.Expiration( *msg, true );
         pt._x = byExp() ? dStr : dExp;
         pt._y = lvc.MidQuote( *msg );
         lXY.push_back( pt );
      }
      /*
       * 2a) Straight-line beginning to min Strike (or Expiration)
       */
      if ( !lvc._square ) {
         x0 = lXY[0]._x;
         x1 = lXY[nk-1]._x;
      }
      if ( ( nk >= 2 ) && ( x0 < lXY[0]._x ) ) {
         m     = ( lXY[0]._y - lXY[1]._y );
         m    /= ( lXY[0]._x - lXY[1]._x );
         dy    = m * ( x0 - lXY[0]._x );
         pt._x = x0;
         pt._y = lXY[0]._y + dy;
         XY.push_back( pt );
      }
      for ( i=0; i<nk; XY.push_back( lXY[i] ), i++ );
      /*
       * 2b) Straight-line end to max Strike (or Expiration)
       */
      if ( ( nk >= 2 ) && ( lXY[nk-1]._x < x1 ) ) {
         m     = ( lXY[nk-2]._y - lXY[nk-1]._y );
         m    /= ( lXY[nk-2]._x - lXY[nk-1]._x );
         dy    = m * ( x1 - lXY[nk-1]._x );
         pt._x = x1;
         pt._y = lXY[nk-1]._y + dy;
         XY.push_back( pt );
      }
      /*
       * 3) Max Number of Data Points 
       */
      np = ( x1-x0 ) / _xInc;
      xi = _xInc;
      for ( ; np > lvc._maxX; _xInc+=xi, np = ( x1-x0 ) / _xInc );

      QUANT::CubicSpline cs( XY );

      _CS = cs;
      return bUpd;
   }

   /**
    * \brief Build Spline from current LVC Snap
    *
    * \param all - Current LVC Snap
    * \param now - Current Unix Time
    * \param bForce - true to calc if not watched
    * \return true if Calc'ed; false if not
    */ 
   bool Calc( LVCAll &all, time_t now, bool bForce=false )
   {
      DoubleXYList &XY = _CS.XY();
      size_t         n, nk, nx;
      double         x, x0, x1, minInc;

      // Pre-condition(s)

      if ( _tCalc == now )
         return false;
      if ( !bForce && !IsWatched() )
         return false;
      if ( !(nk=NumKnots()) )
         return false;
      if ( !SnapKnots( all, now ) && !bForce )
         return false;

      // Rock on

      nx = XY.size();
      x0 = XY[1]._x;
      x1 = XY[nx-1]._x;
      for ( n=1,minInc=99999.0; byExp() && n<nx; n++ )
         minInc = gmin( minInc, XY[n]._x - XY[n-1]._x );
      for ( x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      _Y  = _CS.Spline( _X );
      return true;
   }

   int Publish( Update &u, bool bImg=false )
   {
      double     x0, xInc;
      int        xTy;

      // Pre-condition

      if ( !IsWatched() )
         return 0;
      if ( !_X.size() )
         return 0;

      // Initialize / Publish

      _DoubleList unx;
      _DoubleList &X = byExp() ? _X : _lvc.julNum2Unix( _X, unx );

      u.Init( name(), _StreamID, bImg );
      x0   = X[0];
      xInc = _xInc;
      xTy  = byExp() ? xTy_NUM : xTy_DATE;
      /*
       * VectorView.js expects xInc to be in months
       */
      if ( !byExp() )
         xInc *= byExp() ? 1.0 : ( 12.0 / 365.0 );
      u.AddField( l_fidInc, xInc );
      u.AddField( l_fidXTy, xTy );
      u.AddField( l_fidX0,  x0 );
      /*
       * X-axis values if Strike Spline (X-axis == Date)
       */
      if ( xTy == xTy_DATE )
         u.AddVector( l_fidVecX, X, 0 );
      u.AddVector( l_fidVecY, _Y, _lvc._precision );
      return u.Publish();
   }

   /////////////////////////
   // (private) Helpers
   /////////////////////////
private:
   void _Init( LVCAll &all )
   {
      Messages &msgs  = all.msgs();
      Ints      udb;
      Message  *msg;
      KnotDef   k;
      int       ix;

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
      k = _lvc._kz;
      for ( size_t i=0; i<udb.size(); i++ ) {
         ix         = udb[i];
         msg        = msgs[ix];
         _und.SnapLVCFlds( k, *msg );
         k._exp     = _lvc.Expiration( *msg, false );
         k._jExp    = _lvc.Expiration( *msg, true );
         k._strike  = (u_int64_t)( _lvc.StrikePrice( *msg ) * l_StrikeMul );
         k._tUpd    = msg->MsgTime();
         if ( byExp() ) {
            if ( k._exp == _tExp ) {
               k._idx = ix;
               _kdb.push_back( k );
            }
         }
         else {
            if ( k._strike == _dStr ) {
               k._idx = ix;
               _kdb.push_back( k );
            }
         }
      }
   }

}; // class OptionsSpline



////////////////////////////////////////////////
//
//    c l a s s   O p t i o n s S u r f a c e
//
////////////////////////////////////////////////
/**
 * \brief An options surface : PUT or CALL
 *
 * The surface is built from a M x N sampled grid of ( x,y,z ) Knots as follows:
 * Axis | Size | Elements
 * --- | --- | ---
 * X | M | Expiration
 * Y | N | Strike Price
 * Z | M x N | Option Value at ( Expiration, Strike )
 *
 * This instance 'fills in' the Knot Grid if it is missing Knots or oherwise 
 * sparesely populated.  If a Knot is not available from the LVC, it is 
 * interpolated from the existing Options Spline.
 *
 * An OptionsSpline is chosen from the Underlyer to 'fill in' the Knot at 
 * the ( Expiration, Strike ) Knot point as follows:
 * -# Pull OptionsSpline for Expiration at the Knot point
 * -# Pull OptionsSpline for Strike at the Knot point
 * -# At least one will exist
 * -# If only one exists, use that one
 * -# If both exist, use the one with the most Knots since it is most accurate 
 *
 * \see OptionsSpline
 * \see Underlyer
 */
class OptionsSurface : public string
{
private:
   Underlyer    &_und;
   OptionsCurve &_lvc;
   bool          _bPut;
   bool          _surfCalc;
   KnotGrid      _kdb;
   _DoubleList   _X;
   _DoubleList   _Y;
   _DoubleList   _XX; // Published
   _DoubleList   _YY; // Published
   _DoubleGrid   _Z;
   double        _xInc;
   double        _yInc;
   int           _StreamID;
   time_t        _tCalc;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   /** 
    * \brief Constructor
    *
    * \param und - Underlyer
    * \param bPut - true for surface of puts; false for calls
    * \param all - Current LVC Snapshot
    * \param surfCalc - true for QUANT::CubicSurface; false for knots only
    */
   OptionsSurface( Underlyer &und, bool bPut, LVCAll &all, bool surfCalc ) :
      string( und ),
      _und( und ),
      _lvc( und.lvc() ),
      _bPut( bPut ),
      _surfCalc( surfCalc ),
      _kdb(),
      _X(),
      _Y(),
      _XX(),
      _YY(),
      _Z(),
      _xInc( und.lvc()._xInc ),
      _yInc( und.lvc()._yInc ),
      _StreamID( 0 ),
      _tCalc( 0 )
   {
      string     &s  = *this;
      const char *ty = bPut ? ".P" : ".C";

      if ( !_surfCalc )
         s.insert( 0, "Knot_" );
      s += ty;
      _Init( all );
   }

   /** 
    * \brief Copy Constructor : Trim down from Last +/ Range
    *
    * \param c - Existing Full OptionsSurface
    * \param tkr - Ticker Name
    * \param last - Last Value
    * \param lastRange - Pct Strike Price Range +/-
    * \param months - Num months
    */
   OptionsSurface( OptionsSurface &c, 
                   const char     *tkr,
                   double          last, 
                   double          lastRange,
                   int             months ) :
      string( tkr ),
      _und( c._und ),
      _lvc( c._und.lvc() ),
      _bPut( c._bPut ),
      _surfCalc( c._surfCalc ),
      _kdb(),
      _X(),
      _Y(),
      _Z(),
      _xInc( c._und.lvc()._xInc ),
      _yInc( c._und.lvc()._yInc ),
      _StreamID( 0 ),
      _tCalc( 0 )
   {
      KnotGrid   &kdb = c._kdb;
      _DoubleList &X   = c._X;
      _DoubleList &Y   = c._Y;
      double      x0, x1, x, dr;
      KnotDef     k;
      size_t      i, M, N;
      int         dx;
      char        bp[K], *cp;

      // Find the Range

      dr = WithinRange( 0.0, lastRange / 100.0, 1.0 );
      x0 = last * ( 1.0 - dr );
      x1 = last * ( 1.0 + dr );
      for ( size_t i=0; i<X.size(); i++ ) {
         dx = X[i] - X[0];
         if ( !months || ( dx/30 ) < months   )
            _X.push_back( X[i] );
      }
      for ( size_t i=0; i<Y.size(); i++ ) {
         if ( InRange( x0, Y[i], x1 ) )
            _Y.push_back( Y[i] );
      }
      for ( size_t r=0; r<kdb.size(); r++ ) {
         KnotList src = kdb[r];
         size_t   nc  = src.size();
         KnotList row;

         for ( size_t c=0; c<nc; c++ ) {
            k = src[c];
            x = l_StrikeDiv * k._strike;
            if ( InRange( x0, x, x1 ) )
               row.push_back( k );
         }
         if ( !row.size() )
            continue; // for-r
         dx = row[0]._jExp - _X[0];
         if ( !months || ( dx/30 ) < months   )
            _kdb.push_back( KnotList( row ) );
      }

      // Log it

      M = _kdb.size();
      N = _kdb[0].size();
      for ( i=1; i<M; assert( _kdb[i].size() == N ), i++ );
      cp  = bp;
      cp += sprintf( cp, "%-4s :", data() );
      cp += sprintf( cp, " ( %3ld x %3ld )", M, N );
      LOG( bp );
   }


   /////////////////////////
   // Access
   /////////////////////////
public:
   const char *name() { return data(); }
   size_t      M()    { return _X.size(); }
   size_t      N()    { return _Y.size(); }
   _DoubleList &X()    { return _X; }
   _DoubleList &Y()    { return _Y; }
   _DoubleGrid &Z()    { return _Z; }

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
    * \param now - Current Unix Time
    * \param bForce - true to calc if not watched
    * \return true if Calc'ed; false if not
    */ 
   bool Calc( LVCAll &all, time_t now, bool bForce=false )
   {
      Messages  &msgs = all.msgs();
      KnotDef    k, k0, k1;
      Message   *msg;
      size_t     r, c, nr, nc, ix, m, n;
      double     z, zX, zE, dX;
      double     x0, x1, y0, y1;
      bool       bUpd;
      _DoubleGrid Z;

      // Pre-condition(s)

      if ( _tCalc == now )
         return false;
      if ( !bForce && !IsWatched() )
         return false;
      if ( !(nr=_kdb.size()) )
         return false;

      /*
       * 1) Build / Calc Z from Knots:
       *    a) Pull out real-time Knot values from LVC
       *    b) Else, calculate empty knots from Splines
       */
      _tCalc = now;
      nc     = 0;
      m      = M();
      n      = N();
      bUpd   = true;
      for ( r=0; r<nr; r++ ) {
         _DoubleList zRow, zSnap;
         double     tUpd;

         /*
          * 1a) Snap real-time Knot values from LVC
          */
         nc = _kdb[r].size();
         for ( c=0; c<nc; c++ ) {
            z = 0.0;
            k = _kdb[r][c];
            if ( (ix=k._idx) != l_NoIndex ) {
               msg   = msgs[ix];
               tUpd  = msg->MsgTime();
               z     = _lvc.MidQuote( *msg );
               bUpd &= ( tUpd > k._tUpd );
            }
            zSnap.push_back( z );
         }
         /*
          * 1b) Build row from snapped and interpolated values
          */
         for ( c=0; c<nc; c++ ) {
            z = 0.0;
            k = _kdb[r][c];
            if ( (ix=k._idx) != l_NoIndex )
               z = zSnap[c];
            else if ( _lvc._knotCalc ) {
               QUANT::CubicSpline *csX, *csE;

               /*
                * Take mid-point if both splines available; 
                * Else, the only available spline
                */
               csX = (QUANT::CubicSpline *)0;
               csE = (QUANT::CubicSpline *)0;
               if ( k._pSplX == k._pSplE )
                 _lvc.breakpoint();
               if ( k._splineX ) {
                  csX = &k._splineX->CS( all, now );
                  zX  = csX->ValueAt( k._jExp );
               }
               if ( k._splineE ) {
                  csE = &k._splineE->CS( all, now );
                  dX  = l_StrikeDiv * k._strike;
                  zE  = csE->ValueAt( dX );
               }
               /*
                * If both splines available, BestZ is as follows:
                *    1) If k._splineX and strikes match, use zX
                *    2) If k._splineE and expirations match, use zE
                *    3) Else _BestZ()
                */
               if ( csX && csE ) {
                  if ( k._strike == k._splineX->dStr() )
                     z = zX;
                  else if ( k._exp == k._splineE->tExp() )
                     z = zE;
                  else
                     z = _BestZ( *csX, *csE, zSnap, c, zX, zE );
               }
               /*
                * Else, Only 1 available : use it
                */
               else if ( csX )
                  z = zX;
               else if ( csE )
                  z = zE;
            }
            else
               z = -1.0;
            zRow.push_back( z );
         }
         Z.push_back( zRow );
      }
      if ( !bUpd && !bForce )
         _lvc.breakpoint();

      /*
       * 2) Calculate Surface
       */
      size_t nx, ny;
      double xi, yi, d0, age;
      int    np;

assert( nr == m );
assert( nc == n );
      k0 = _kdb[0][0];
      k1 = _kdb[m-1][n-1];
      x0 = k0._jExp;
      x1 = k1._jExp;
      y0 = l_StrikeDiv * k0._strike;
      y1 = l_StrikeDiv * k1._strike;
      /*
       * Trim down increments so we don't melt Greenland
       */
      xi    = _lvc._xInc;
      yi    = _lvc._yInc;
      _xInc = xi;
      _yInc = yi;
      np    = ( x1-x0 ) / _xInc;
      for ( ; np > _lvc._maxX; _xInc+=xi, np = ( x1-x0 ) / _xInc );
      np    = ( y1-y0 ) / _yInc;
      for ( ; np > _lvc._maxX; _yInc+=yi, np = ( y1-y0 ) / _yInc );
      /*
       * Rock on
       */
      d0 = _lvc.TimeNs();
      if ( _surfCalc ) {
         QUANT::CubicSurface srf( _X, _Y, Z );

         if ( !_XX.size() ) {
            for ( double x=x0; x<=x1; _XX.push_back( x ), x+=_xInc );
            for ( double y=y0; y<=y1; _YY.push_back( y ), y+=_yInc );
         }
         _Z = srf.Surface( _XX, _YY );
      }
      else {
         _XX = _X;
         _YY = _Y;
         _Z  = _DoubleGrid( Z );
      }
      age = _lvc.TimeNs() - d0;
      nx  = _XX.size();
      ny  = _YY.size();
      LOG( "%-16s Surface( %4ld x%4ld ) in %.3fs", name(), nx, ny, age );

      /*
       * 3) Dump Grid of Knots if bForce
       */
      if ( bForce && _lvc._dump ) {
         string dmp;
         char  *bp, *cp;

         bp  = new char[n*128];
         cp  = bp;
         cp += sprintf( cp, name() );
         for ( size_t x=0; x<m; x++ )
            cp += sprintf( cp, ",%ld", _kdb[x][0]._exp );
         dmp += bp;
         dmp += "\n";
         for ( size_t y=0; y<n; y++ ) {
            cp  = bp;
            cp += sprintf( cp, "%.2f", l_StrikeDiv * _kdb[0][y]._strike );
            for ( size_t x=0; x<m; x++ )
               cp += sprintf( cp, ",%.3f", Z[x][y] );
            dmp += bp;
            dmp += "\n";
         }
         delete[] bp;
         ::fwrite( dmp.data(), 1, dmp.size(), _log );
         ::fflush( _log );
      }
      return true;
   }

   int Publish( Update &u )
   {
      double x0, xInc;
      int    xTy;

      // Pre-condition

      if ( !IsWatched() )
         return 0;

      // Initialize / Publish

      u.Init( name(), _StreamID, true );
      if ( !_XX.size() ) {
         u.PubError( "Empty Surface" );
         return 0;
      }

      // Rock on

      _DoubleList X;

      _lvc.julNum2Unix( _XX, X );
      x0   = X[0];
      xTy  = xTy_DATE;
      xInc = ( 12.0 / 365.0 );
u.AddEmptyField( 3 );
      u.AddField( l_fidInc, xInc );
      u.AddField( l_fidXTy, xTy );
      u.AddField( l_fidX0,  x0 );
      /*
       * X-axis : Expiration
       * Y-axis : Stike
       * Z-axis : Surface
       */
      u.AddVector( l_fidVecX, X, 0 );                 // Expiration 
      u.AddVector( l_fidVecY, _YY, 2 );               // Strike
      u.AddSurface( l_fidVecZ, _Z, _lvc._precision ); // Surface
      return u.Publish();
   }

   /////////////////////////
   // (private) Helpers
   /////////////////////////
private:
   void _Init( LVCAll &all )
   {
      Messages      &msgs  = all.msgs();
      IndexCache    &byExp = _und.byExp();
      IndexCache    &byStr = _und.byStr();
      Ints           udb, xdb;
      Message       *msg;
      KnotDef        k, kz;
      KnotList       kRow;
      u_int64_t      exp, jExp, jStr, key;
      size_t         i;
      double         ds;
      SortedInt64Set edb, sdb, jdb;
      KnotDefMap     kMap;
      OptionsSpline *spl;

      /*
       * 1) Put / Call
       */
      udb = _bPut ? byExp._puts : byExp._calls;
      xdb = _bPut ? byStr._puts : byStr._calls;
      /*
       * 2) Clear shit out; Jam away ...
       */
      for ( i=0; i<udb.size(); i++ ) {
         k          = _lvc._kz;
         k._idx     = udb[i];
         msg        = msgs[k._idx];
         _und.SnapLVCFlds( k, *msg );
         k._exp     = _lvc.Expiration( *msg, false );
         k._jExp    = _lvc.Expiration( *msg, true );
         k._strike  = (u_int64_t)( _lvc.StrikePrice( *msg ) * l_StrikeMul );
         k._tUpd    = msg->MsgTime();
         key        = ( k._exp << 32 );
         key       += k._strike;
         kMap[key]  = k;
         /*
          * Build ( Expiration, Strike ) matrix
          */
         edb.insert( k._exp );
         jdb.insert( k._jExp );
         sdb.insert( k._strike );
      }
      /*
       * 3) Trim strike outliers
       */
      SortedInt64Set           tmp;
      SortedInt64Set::iterator et, jt, st;
      KnotDefMap::iterator     kt;
      u_int64_t                x0, x1;
      double                   dd;

      if ( sdb.size() ) {
         st = sdb.begin();
         x0 = (*st);
         for ( st++; st!=sdb.end(); st++ ) {
            x1 = (*st);
            dd = ( x1-x0 ) / ( 0.5 * ( x0+x1 ) );
            if ( dd < 1.0 )
               tmp.insert( x0 );
            else
               _lvc.breakpoint();
            x0 = x1;
         }
         tmp.insert( x1 );
         sdb = tmp;
      }
      /*
       * 4) Square Up by Expiration
       */
      et = edb.begin();
      jt = jdb.begin();
      for ( i=0; et!=edb.end(); i++,et++,jt++ ) {
         exp  = (*et);
         jExp = (*jt);
         _X.push_back( jExp );
         for ( st=sdb.begin(); st!=sdb.end(); st++ ) {
            jStr = (*st);
            if ( !i )
               _Y.push_back( l_StrikeDiv * jStr );
            key  = ( exp << 32 );
            key += jStr;
            if ( (kt=kMap.find( key )) != kMap.end() ) {
               k = (*kt).second;
               kRow.push_back( k );
            }
            else {
               ds          = l_StrikeDiv * k._strike;
               kz          = _lvc._kz;
               kz._splineX = _und.GetSpline( ds, _bPut );
               kz._splineE = _und.GetSpline( exp, _bPut );
               kz._pSplX   = (spl=kz._splineX) ? spl->name() : (const char *)0;
               kz._pSplE   = (spl=kz._splineE) ? spl->name() : (const char *)0;
assert( kz._splineX || kz._splineE );
               kz._exp     = exp;
               kz._jExp    = jExp;
               kz._strike  = jStr;
               kz._idx     = l_NoIndex;
               kz._tUpd    = 0.0;
               kRow.push_back( kz );
            }
         }
         _kdb.push_back( KnotList( kRow ) );
         kRow.clear();
      }
      /*
       * 5) Validate; Trim; Log
       */
      size_t M0, N0, M1, N1;
      char   bp[K], *cp;

      M0 = _kdb.size();
      N0 = _kdb[0].size();
      for ( i=1; i<M0; assert( _kdb[i].size() == N0 ), i++ );
      if ( _lvc._trim )
         _Trim( KnotGrid( _kdb ), M0, N0 );
      M1 = _kdb.size();
      N1 = _kdb[0].size();
      for ( i=1; i<M1; assert( _kdb[i].size() == N1 ), i++ );
      cp  = bp;
      cp += sprintf( cp, "%-16s :", data() );
      cp += sprintf( cp, " ( %3ld x %3ld )", M0, N0 );
      cp += sprintf( cp, " -> ( %3ld x %3ld )", M1, N1 );
      LOG( bp );
   }

   void _Trim( KnotGrid kdb, size_t M, size_t N )
   {
      KnotList  row, rowK;
      KnotDef   k0, k1;
      size_t    x0, x1, r, c;

      /*
       * 1) Only leave 1 empty on left side
       */
      x0 = 0;
      x1 = N;
      for ( r=0; r<M; r++ ) {
         row = kdb[r];
         for ( c=0; c<N-1; c++ ) {
            k0 = row[c];
            k1 = row[c+1];
            if ( k0._idx || k1._idx ) {
               x0 = gmax( x0, c );
               break; // for-c
            }
         }
      }
      /*
       * 2) Only leave 1 empty on right side
       */
      for ( r=0; r<M; r++ ) {
         row = kdb[r];
         for ( c=N-1; c && c>x0; c-- ) {
            k0 = row[c];
            k1 = row[c-1];
            if ( k0._idx || k1._idx ) {
               x1 = gmin( x1, c );
               break; // for-c
            }
         }
      }
      /*
       * 3) Rebuild Trimmed be-nop
       */
      _kdb.clear();
      for ( r=0; r<M; r++ ) {
         row = kdb[r];
         rowK.clear();
         for ( c=x0; c<x1; rowK.push_back( row[c] ), c++ );
         _kdb.push_back( rowK );
      }
   }

   /**
    * \brief Determine 'best' spline value from dX or dE
    *
    * TODO : closest to csX / csE knots
    *
    * \param csX - CubicSpline : Strike Price
    * \param csE - CubicSpline : Expiration Date
    * \param snap - Snapped LVC values
    * \param col -  Current column from snap
    * \param dX - Value from Strike Price Spline
    * \param dE - Value from Expiration Spline
    * \return 'Best' value for col based on snapped LVC values
    */ 
   double _BestZ( QUANT::CubicSpline &csX,
                  QUANT::CubicSpline &csE,
                  _DoubleList         &snap, 
                  size_t              col, 
                  double              dX, 
                  double              dE )
   {
      size_t c0, c1, sz;
      double ds, rc;
      bool   b0, b1;

      /*
       * 1) Find first non-zero snapped value up and down from col
       */
      sz = snap.size();
      for ( c0=col; c0>=0 && !snap[c0]; c0-- );
      for ( c1=col; c1<sz && !snap[c1]; c1++ );
      b0 = ( (::int64_t)c0 >= 0 );
      b1 = ( c1 < sz );
      /*
       * Return dX or dE depending on closest to non-zero snapped value
       * If no snapped values, then return midpoint
       */
      rc = ( dX+dE ) / 2.0;
      if ( b0 || b1 ) {
         if ( b0 && b1 ) {
            if ( ( col-c0 ) <= ( c1-col ) )
               ds = snap[c0];
            else
               ds = snap[c1];
         }
         else if ( b0 )
            ds = snap[c0];
         else if ( b1 )
            ds = snap[c1];
         rc = ( ::fabs( dX-ds ) < ::fabs( dE-ds ) ) ? dX : dE;
      }
      return rc;
   }

}; // class OptionsSurface


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
   SurfaceMap    _surfaces;
   SurfaceMap    _surfacesR;  // Ephemeral : Range
   Mutex         _wlMtxR;
   double        _tPub;
   bool          _bName;
   void         *_bdsStreamID;

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
      _surfaces(),
      _surfacesR(),
      _wlMtxR(),
      _tPub( 0.0 ),
      _bName( false ),
      _bdsStreamID( (void *)0 )
   {
      SetBinary( true );
      SetIdleCallback( true );
   }

   /////////////////////
   // Operations
   /////////////////////
public:
   int Calc( double &srfAge )
   {
      LVCAll &all    = _lvc.ViewAll();
      Update &u      = upd();
      time_t  now    = TimeSec();
      bool    bForce = true;
      double  t0;
      int     rc;

      rc     = _CalcSplines( all, now, bForce, u );
      t0     = _lvc.TimeNs();
      rc    += _CalcSurfaces( all, now, bForce, u );
      srfAge = _lvc.TimeNs() - t0;
      return rc;
   }

   int Calc( bool bForce=false )
   {
      LVCAll &all = _lvc.ViewAll();
      Update &u   = upd();
      time_t  now = TimeSec();
      int     rc;

      rc  = _CalcSplines( all, now, bForce, u );
      rc += _CalcSurfaces( all, now, bForce, u );
      return rc;
   }

   size_t LoadSplines( LVCAll &all )
   {
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
         _DoubleList              &xdb = udb[i]->strikes();
         SortedInt64Set::iterator et;
         _DoubleList::iterator     xt;

         und = udb[i];
         for ( et=edb.begin(); et!=edb.end(); et++ ) {
            tExp = (*et);
            for ( int o=0; o<=(int)spline_both; o++ ) {
               sTy    = (SplineType)o;
               spl    = new OptionsSpline( *und, tExp, sTy, all );
               s      = spl->name();
               sdb[s] = spl;
               und->Register( spl, s );
            }
         }
         for ( xt=xdb.begin(); xt!=xdb.end(); xt++ ) {
            dX = (*xt);
            for ( int o=0; o<=(int)spline_call; o++ ) {
               sTy    = (SplineType)o;
               spl    = new OptionsSpline( *und, dX, sTy, all );
               s      = spl->name();
               sdb[s] = spl;
               und->Register( spl, s );
            }
         }
      }
      return sdb.size();
   }

   size_t LoadSurfaces()
   {
      SurfaceMap     &sdb = _surfaces;
      LVCAll         &all = _lvc.ViewAll();
      Underlyers     &udb = _underlyers;
      Underlyer      *und;
      OptionsSurface *srf;
      string          s;
      bool            bPut, surfCalc;

      for ( size_t i=0; i<udb.size(); i++ ) {
         und    = udb[i];
         for ( size_t j=0; j<2; j++ ) {
            bPut = ( j == 0 );
            for ( size_t k=0; k<2; k++ ) {
               surfCalc = ( k == 0 );
               srf      = new OptionsSurface( *und, bPut, all, surfCalc );
               s        = srf->name();
               sdb[s]   = srf;
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
      const char *svc = pPubName();
      const char *ty  = bUp ? "UP" : "DOWN";

      LOG( "PUB-CONN %s@%s : %s", svc, msg, ty );
      _bName = false;
      if ( bUp )
         StartThread();
      else
         StopThread();
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      Locker               lck( _wlMtxR );
      LVCAll              &all = _lvc.ViewAll();
      SplineMap           &ldb = _splines;
      SurfaceMap          &fdb = _surfaces;
      SurfaceMap          &rdb = _surfacesR;
      Update              &u   = upd();
      time_t               now = TimeSec();
      SplineMap::iterator  lt;
      SurfaceMap::iterator ft;
      string               s, ss, tt( tkr ), err;
      char                *pt, *pl, *pr, *pm, *rp;
      int                  nb, mon;
      double               last, rng;
      OptionsSpline       *spl;
      OptionsSurface      *srf, *srf1;

      /*
       * Support <Surface> <Last> <+/- Strike> <NumMonths>
       *   e.g.. AAPL.P 100 50 0 for Last=$100 +/- $50 ALL months
       *   e.g.. AAPL.P 100 50 3 for Last=$100 +/- $50 next 3 months
       */
      pt   = (char *)tt.data();
      pt   = ::strtok_r( pt,   " ", &rp );
      pl   = ::strtok_r( NULL, " ", &rp );
      pr   = ::strtok_r( NULL, " ", &rp );
      pm   = ::strtok_r( NULL, " ", &rp );
      last = pl ? atof( pl ) : 0.0;
      rng  = pr ? atof( pr ) : 0.0;
      mon  = pm ? atoi( pm ) : 0;
      s    = tkr;
      ss   = pt; 
      nb   = 0;
      LOG( "OPEN  %s", tkr );
      if ( (lt=ldb.find( s )) != ldb.end() ) {
         spl = (*lt).second;
         if ( spl->X().size() ) {
            spl->SetWatch( (size_t)arg );
            spl->Calc( all, now, true );
            nb = spl->Publish( u, true );
         }
         else
            err = "Empty Spline";
      }
      else if ( (ft=fdb.find( s )) != fdb.end() ) {
         srf = (*ft).second;
         srf->SetWatch( (size_t)arg );
         srf->Calc( all, now, true );
         nb = srf->Publish( u );
      }
      else if ( rng && (ft=fdb.find( ss )) != fdb.end() ) {
         srf  = (*ft).second;
         srf1 = new OptionsSurface( *srf, tkr, last, rng, mon );
         if ( !srf1->M() || !srf1->N() ) {
            err = "Empty surface";
            u.Init( tkr, arg );
            u.PubError( "Empty surface" );
         }
         else {
            rdb[s] = srf1;
            srf1->SetWatch( (size_t)arg );
            srf1->Calc( all, now, true );
            nb = srf1->Publish( u );
         }
      }
      else
         err = "non-existent ticker";

      // Wanted : DEAD or Alive

      if ( nb )
         LOG( "IMG %s : %d bytes", tkr, nb );
      else if ( err.length() ) {
         u.Init( tkr, arg );
         u.PubError( err.data() );
         LOG( "DEAD %s : %s", tkr, err.data() );
      }
   }

   virtual void OnPubClose( const char *tkr )
   {
      Locker               lck( _wlMtxR );
      SplineMap           &ldb = _splines;
      SurfaceMap          &fdb = _surfaces;
      SurfaceMap          &rdb = _surfacesR;
      SplineMap::iterator  lt;
      SurfaceMap::iterator ft;
      string               s( tkr );
      OptionsSpline       *spl;
      OptionsSurface      *srf;

      LOG( "CLOSE %s", tkr );
      if ( (lt=ldb.find( s )) != ldb.end() ) {
         spl = (*lt).second;
         spl->SetWatch( 0 );
      }
      else if ( (ft=fdb.find( s )) != fdb.end() ) {
         srf = (*ft).second;
         srf->SetWatch( 0 );
      }
      else if ( (ft=rdb.find( s )) != rdb.end() ) {
         srf = (*ft).second;
         rdb.erase( ft );
         delete srf;
      }
   }

   virtual void OnOpenBDS( const char *bds, void *tag )
   {
      // Publisher name only

      LOG( "OPEN.BDS %s", bds );
      if ( ::strcmp( bds, pPubName() ) ) {
         Update &u = upd();
         string  err( "non-existent BDS stream : Try " );

         err += pPubName();
         u.Init( bds, tag );
         u.PubError( err.data() );
         return;
      }
      _bdsStreamID = tag;
      _PublishBDS();
   }

   virtual void OnCloseBDS( const char *bds )
   {
      LOG( "CLOSE.BDS %s", bds );
      _bdsStreamID = (void *)0;
   }

   virtual Update *CreateUpdate()
   {
      return new Update( *this );
   }

   virtual void OnIdle()
   {
      // Once

      if ( !_bName )
         SetThreadName( "MDD" );
      _bName = true;
   }

   virtual void OnWorkerThread()
   {
      SetThreadName( "QUANT" ); 
      for ( ; ThreadIsRunning(); Sleep( 0.25 ), _OnIdle() );
   }

   /////////////////////
   // (private) Helpers
   /////////////////////
private:
   void _PublishBDS()
   {
      SplineMap                &ldb = _splines;
      SurfaceMap               &fdb = _surfaces;
      const char               *bds = pPubName();
      SplineMap::iterator       lt;
      SurfaceMap::iterator      ft;
      SortedStringSet           srt;
      SortedStringSet::iterator st;
      string                    k;
      char                     *tkr;
      int                       nb;
      vector<char *>            tkrs;

      // Rock on ...

      for ( lt=ldb.begin(); lt!=ldb.end(); srt.insert( (*lt).first ), lt++ );
      for ( ft=fdb.begin(); ft!=fdb.end(); srt.insert( (*ft).first ), ft++ );
      for ( st=srt.begin(); st!=srt.end(); st++ ) {
         k = (*st);
         if ( (lt=ldb.find( k )) != ldb.end() ) {
            tkr = (char *)(*lt).second->data();
            tkrs.push_back( tkr );
         }
         else if ( (ft=fdb.find( k )) != fdb.end() ) {
            tkr = (char *)(*ft).second->data();
            tkrs.push_back( tkr );
         }
      }
      tkrs.push_back( (char *)0 );
      nb = PublishBDS( bds, (size_t)_bdsStreamID, tkrs.data() );
      LOG( "PUB-BDS : %d bytes", nb );
   }

   int _CalcSplines( LVCAll &all, time_t now, bool bForce, Update &u )
   {
      SplineMap          &sdb = _splines;
      SplineMap::iterator it;
      OptionsSpline      *spl;
      const char         *tkr;
      int                 nb, rc;
bool bImg = true; // VectorView.js needs fidVecX

      for ( rc=0,it=sdb.begin(); it!=sdb.end(); it++ ) {
         tkr = (*it).first.data();
         spl = (*it).second;
         if ( spl->Calc( all, now, bForce ) ) {
            rc += 1;
            if ( (nb=spl->Publish( u, bImg )) )
               LOG( "UPD %s : %d bytes", tkr, nb );
         }
      }
      return rc;
   }

   int _CalcSurfaces( LVCAll &all, time_t now, bool bForce, Update &u )
   {
      SurfaceMap          &sdb = _surfaces;
      SurfaceMap          &rdb = _surfacesR;
      SurfaceMap::iterator it;
      OptionsSurface      *srf;
      const char          *tkr;
      int                  rc, nb;

      // 1) Pre-built surfaces

      for ( rc=0,it=sdb.begin(); it!=sdb.end(); it++ ) {
         tkr = (*it).first.data();
         srf = (*it).second;
         if ( srf->Calc( all, now, bForce ) ) {
            rc += 1;
            if ( (nb=srf->Publish( u )) )
               LOG( "UPD %s : %d bytes", tkr, nb );
         }
      }

      // 2) User-defined surfaces

      Locker lck( _wlMtxR );

      for ( it=rdb.begin(); it!=rdb.end(); it++ ) {
         tkr = (*it).first.data();
         srf = (*it).second;
         if ( srf->Calc( all, now, bForce ) ) {
            rc += 1;
            if ( (nb=srf->Publish( u )) )
               LOG( "UPD %s : %d bytes", tkr, nb );
         }
      }
      return rc;
   }

   void _OnIdle()
   {
      double now = TimeNs();
      double age = now - _tPub;
      int    n;

      // Pre-condition

      if ( age < _pubRate )
         return;

      // Rock on

      _tPub = now;
      if ( !(n=Calc()) )
breakpoint();
   }

}; // SplinePublisher


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   XmlParser x;
   XmlElem  *xs;
   string    s;
   FILE     *fp;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      LOGRAW( "%s", OptionsCurveID() );
      return 0;
   }

   /////////////////////
   // Config
   /////////////////////
   if ( !x.Load( argv[1] ) ) {
      LOG( "Invalid XML file %s; Exitting ...", argv[1] );
      return 0;
   }
   _IsTrue( "mew" ); // So compiler won't complain

   XmlElem    &root  = *x.root();
   bool        fg    = root.getElemValue( "fg", true );  
   const char *db    = root.getElemValue( "db", "./cache.lvc" );
   const char *svr   = root.getElemValue( "svr", "localhost:9015" );
   const char *svc   = root.getElemValue( "svc", "options.curve" );
   double      rate  = root.getElemValue( "rate", 1.0 );
   double      xInc  = root.getElemValue( "xInc", 1.0 );
   double      yInc  = root.getElemValue( "yInc", 1.0 );
   int         maxX  = root.getElemValue( "maxX", 1000 );
   bool        sqr   = root.getElemValue( "square", false );
   bool        trim  = root.getElemValue( "trim", false );
   bool        dump  = root.getElemValue( "dump", false );
   bool        kCalc = root.getElemValue( "knotCalc", true );
   int         prec  = root.getElemValue( "precision", 2 );
   const char *pLog  = root.getElemValue( "log", "stdout" );
   const char *_UST  = "RiskFree";

   if ( !(xs=root.find( _UST )) ) {
      LOG( "<%s> element not found; Exitting ...", _UST );
      return 0;
   }
   if ( ::strcmp( pLog, "stdout" ) && (fp=::fopen( pLog, "wb" )) )
      _log = fp;

   /////////////////////
   // Do it
   /////////////////////
   OptionsCurve    lvc( db, sqr, trim, dump, kCalc, prec, xInc, yInc, maxX );
   RiskFreeSpline  rf( lvc, *xs );
   SplinePublisher pub( lvc, svr, svc, rate );
   double          d0, srfAge, age;
   size_t          ns;

   /*
    * Dump Config
    */
   LOG( OptionsCurveID() );
   LOG( "Config._square    = %s", _pBool( lvc._square ) );
   LOG( "Config._trim      = %s", _pBool( lvc._trim ) );
   LOG( "Config._dump      = %s", _pBool( lvc._dump ) );
   LOG( "Config._knotCalc  = %s", _pBool( lvc._knotCalc ) );
   LOG( "Config._precision = %d", lvc._precision );
   LOG( "Config._xInc      = %.2f", lvc._xInc );
   LOG( "Config._yInc      = %.2f", lvc._yInc );
   LOG( "Config._maxX      = %d", lvc._maxX );
   LOG( "Config._rate      = %.1fs", rate );
   /*
    * Load Splines
    */
   LOG( "Loading splines ..." );
   d0  = lvc.TimeNs();
   {
      LVCAll &all = lvc.ViewAll();

      rf.Load( all );
      rf.Calc( all, lvc.TimeSec() );
      ns = pub.LoadSplines( all );
   }
   age = 1000.0 * ( lvc.TimeNs() - d0 );
   LOG( "%ld splines loaded in %dms", ns, (int)age );
   /*
    * Load Surfaces
    */
   LOG( "Loading surfaces ..." );
   d0  = lvc.TimeNs();
   ns    = pub.LoadSurfaces();
   age = 1000.0 * ( lvc.TimeNs() - d0 );
   LOG( "%ld surfaces loaded in %dms", ns, (int)age );
   /*
    * Calc Splines / Surfaces
    */
   LOG( "Initial Calcs ..." );
   d0  = lvc.TimeNs();
   ns  = pub.Calc( srfAge );
   age = lvc.TimeNs() - d0;
   LOG( "Surface Calc in %.2fs", srfAge );
   LOG( "%d surfaces / splines cal'ed in %.2fs", ns, age );
   /*
    * Rock on
    */
   LOG( "%s", pub.PubStart() );
   if ( fg ) {
      LOG( "Hit <ENTER> to quit..." );
      getchar();
   }
#ifndef WIN32
   else {
      for ( forkAndSig( false ); !_pSigLog; pub.Sleep( 0.25 ) );
      LOG( "%s caught", _pSigLog );
   }
#endif // WIN32
   LOG( "Shutting down ..." );
   pub.Stop();
   LOG( "Done!!" );
   return 1;
}
