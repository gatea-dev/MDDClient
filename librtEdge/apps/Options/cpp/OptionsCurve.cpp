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
#include <assert.h>
#include <list>

// Configurable Precision

static int     l_Precision =     2;
static double  l_StrikeMul =   100.0;
static size_t  l_NoIndex   =    -1;

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
      cp += sprintf( cp, rtEdge::Version() );
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
   OptionsSpline *_splineX; // Non-zero implies calc each time 
   OptionsSpline *_splineE; // Non-zero implies calc each time 
   u_int64_t      _strike;
   u_int64_t      _exp;
   size_t         _idx; // Non-zero implies real-time

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
   bool   _square;
   bool   _trim;
   double _xInc;
   double _yInc;
   int    _maxX;

   /////////////////////////
   // Constructor
   /////////////////////////
public:
   OptionsCurve( const char *svr, 
                 bool        square,
                 bool        trim,
                 double      xInc,
                 double      yInc,
                 int         maxX ) :
      OptionsBase( svr ),
      _square( square ),
      _trim( trim ),
      _xInc( xInc ),
      _yInc( yInc ),
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
   DoubleList         _X;
   DoubleList         _Y;
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
   DoubleList &X()        { return _X; }
   DoubleList &Y()        { return _Y; }
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
      DoubleList   &xdb  = _und.strikes();
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
      RTEDGE::DoubleXYList &XY = _CS.XY();
      size_t                nk, nx;
      double                x, x0, x1;

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
      for ( x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      _Y  = _CS.Spline( _X );
      return true;
   }

   void Publish( Update &u, bool bImg=false )
   {
      double     x0, xInc;
      int        xTy;

      // Pre-condition

      if ( !IsWatched() )
         return;

      // Initialize / Publish

      u.Init( name(), _StreamID, bImg );
      if ( _X.size() ) {
         DoubleList unx;
         DoubleList &X = byExp() ? _X : _lvc.julNum2Unix( _X, unx );

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
         u.AddVector( l_fidVecY, _Y, l_Precision );
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
      k._splineX = (OptionsSpline *)0;
      k._splineE = (OptionsSpline *)0;
      for ( size_t i=0; i<udb.size(); i++ ) {
         ix        = udb[i];
         msg       = msgs[ix];
         k._exp    = _lvc.Expiration( *msg, false );
         k._strike = (u_int64_t)( _lvc.StrikePrice( *msg ) * l_StrikeMul );
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
   KnotGrid      _kdb;
   DoubleList    _X;
   DoubleList    _Y;
   DoubleGrid    _Z;
   double        _xInc;
   double        _yInc;
   int           _StreamID;

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
    */
   OptionsSurface( Underlyer &und, bool bPut, LVCAll &all ) :
      string( und ),
      _und( und ),
      _lvc( und.lvc() ),
      _bPut( bPut ),
      _kdb(),
      _X(),
      _Y(),
      _Z(),
      _xInc( und.lvc()._xInc ),
      _yInc( und.lvc()._yInc ),
      _StreamID( 0 )
   {
      _Init( all );
   }

   /////////////////////////
   // Access
   /////////////////////////
public:
   const char *name() { return data(); }
   DoubleList &X()    { return _X; }
   DoubleList &Y()    { return _Y; }
   DoubleGrid &Z()    { return _Z; }

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
      OptionsCurve  &lvc  = _und.lvc();
      Messages      &msgs = all.msgs();
      KnotGrid      &kdb  = _kdb;
      KnotDef        k;
      Message       *msg;
      OptionsSpline *splX, *splE;
      size_t         r, c, nr, nc, ix;
      double         z, zX, zE, dX;
      DoubleGrid     Z;

      // Pre-condition(s)
#ifdef TODO
      if ( _tCalc == now )
         return false;
#endif // TODO
      if ( !bForce && !IsWatched() )
         return false;
      if ( !(nr=_kdb.size()) )
         return false;

      /*
       * 1) Pull out real-time Knot values from LVC
       */
      for ( r=0; r<nr; r++ ) {
         DoubleList zRow;

         nc = kdb[r].size();
         for ( c=0; c<nc; c++ ) {
            z = 0.0;
            k = kdb[r][c];
            if ( (ix=k._idx) != l_NoIndex ) {
               msg = msgs[ix];
               z   = lvc.MidQuote( *msg );
            }
            else {
               /*
                * Take mid-point if both splines available; 
                * Else, the only available spline
                */
               if ( (splX=k._splineX) ) {
                  dX  = 1.0 / l_StrikeMul;
                  dX *= k._strike;
                  zX  = splX->CS( all, now ).ValueAt( dX );
               }
               if ( (splE=k._splineE) )
                  zE = splE->CS( all, now ).ValueAt( k._exp );
               if ( splX && splE )
                  z = ( zX + zE ) / 2.0;
               else if ( splX )
                  z = zX;
               else if ( splE )
                  z = zE;
            }
            zRow.push_back( z );
         }
         Z.push_back( zRow );
      }
      /*
       * 2) Fill in empty knot w/ Splines
       */
      for ( r=0; r<nr; r++ ) {
         nc = kdb[r].size();
         for ( c=0; c<nc; c++ ) {
            z = -1.0;
            k = kdb[r][c];
            if ( (ix=k._idx) ) {
            }
            Z[r][c] = z;
         }
      }

      QUANT::CubicSurface cs( _X, _Y, Z );

#ifdef TODO_SURFACE
      for ( x=x0; x<=x1; _X.push_back( x ), x+=_xInc );
      _Y = cs.Spline( _X );
#endif // TODO_SURFACE
      return true;
   }

   void Publish( Update &u )
   {
      double x0, xInc;
      int    xTy;

      // Pre-condition

      if ( !IsWatched() )
         return;

      // Initialize / Publish

      u.Init( name(), _StreamID, true );
      if ( !_X.size() ) {
         u.PubError( "Empty Surface" );
         return;
      }

      // Rock on

      DoubleList X;

      _lvc.julNum2Unix( _X, X );
      x0   = X[0];
      xTy  = xTy_DATE;
      xInc = ( 12.0 / 365.0 );
      u.AddField( l_fidInc, xInc );
      u.AddField( l_fidXTy, xTy );
      u.AddField( l_fidX0,  x0 );
      /*
       * X-axis : Expiration
       * Y-axis : Stike
       * Z-axis : Surface
       */
      u.AddVector( l_fidVecX, X, 0 );             // Expiration 
      u.AddVector( l_fidVecY, _Y, 2 );            // Strike
      u.AddSurface( l_fidVecZ, _Z, l_Precision ); // Surface
      u.Publish();
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
      KnotList       kdb;
      u_int64_t      jExp, jStr, key;
      size_t         i;
      double         ds;
      SortedInt64Set edb, sdb;
      KnotDefMap     kMap;

      /*
       * 1) Put / Call
       */
      udb = _bPut ? byExp._puts : byExp._calls;
      xdb = _bPut ? byStr._puts : byStr._calls;
      /*
       * 2) Clear shit out; Jam away ...
       */
      k._splineX = (OptionsSpline *)0;
      k._splineE = (OptionsSpline *)0;
      for ( i=0; i<udb.size(); i++ ) {
         k._idx    = udb[i];
         msg       = msgs[k._idx];
         k._exp    = _lvc.Expiration( *msg, false );
         k._strike = (u_int64_t)( _lvc.StrikePrice( *msg ) * l_StrikeMul );
         key       = ( k._exp << 32 );
         key      += k._strike;
         kMap[key] = k;
         /*
          * Build ( Expiration, Strike ) matrix
          */
         edb.insert( k._exp );
         sdb.insert( k._strike );
      }
      /*
       * 2) Square Up by Expiration
       */
      SortedInt64Set::iterator et, st;
      KnotDefMap::iterator     kt;

      for ( i=0,et=edb.begin(); et!=edb.end(); i++,et++ ) {
         jExp = (*et);
         _X.push_back( jExp );
         for ( st=sdb.begin(); st!=sdb.end(); st++ ) {
            jStr = (*st);
            if ( !i )
               _Y.push_back( jStr );
            key  = ( jExp << 32 );
            key += jStr;
            if ( (kt=kMap.find( key )) != kMap.end() ) {
               k = (*kt).second;
               kdb.push_back( k );
            }
            else {
               ds          = 1.0 / l_StrikeMul;
               ds         *= k._strike;
               kz._splineX = _und.GetSpline( ds, _bPut );
               kz._splineE = _und.GetSpline( jExp, _bPut );
assert( kz._splineX || kz._splineE );
               kz._exp     = jExp;
               kz._strike  = jStr;
               kz._idx     = l_NoIndex;
               kdb.push_back( kz );
            }
         }
         _kdb.push_back( KnotList( kdb ) );
         kdb.clear();
      }
      /*
       * 3) Validate; Trim; Log
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
      cp += sprintf( cp, "%-4s :", data() );
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
      _surfaces(),
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
      LVCAll &all = _lvc.ViewAll();
      Update &u   = upd();
      time_t  now = TimeSec();
      int     rc;

      rc  = _CalcSplines( all, now, bForce, u );
      rc += _CalcSurfaces( all, now, bForce, u );
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
      SurfaceMap &sdb = _surfaces;
      LVCAll     &all = _lvc.ViewAll();
      Underlyers &udb = _underlyers;
      Underlyer  *und;
      string      s;

      for ( size_t i=0; i<udb.size(); i++ ) {
         und    = udb[i];
         s      = und->data();
         sdb[s] = new OptionsSurface( *und, true, all );
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
      time_t              now = TimeSec();
      SplineMap::iterator it;
      string              tm, s( tkr );
      OptionsSpline      *spl;

      LOG( "[%s] OPEN  %s", pDateTimeMs( tm ), tkr );
      if ( (it=sdb.find( s )) != sdb.end() ) {
         spl = (*it).second;
         spl->SetWatch( (size_t)arg );
         spl->Calc( all, now, true );
         spl->Publish( u, true );
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
   }

   /////////////////////
   // (private) Helpers
   /////////////////////
private:
   int _CalcSplines( LVCAll &all, time_t now, bool bForce, Update &u )
   {
      SplineMap          &sdb = _splines;
      SplineMap::iterator it;
      OptionsSpline      *spl;
      int                 rc;
bool bImg = true; // VectorView.js needs fidVecX

      for ( rc=0,it=sdb.begin(); it!=sdb.end(); it++ ) {
         spl = (*it).second;
         if ( spl->Calc( all, now, bForce ) ) {
            spl->Publish( u, bImg );
            rc += 1;
         }
      }
      return rc;
   }

   int _CalcSurfaces( LVCAll &all, time_t now, bool bForce, Update &u )
   {
      SurfaceMap          &sdb = _surfaces;
      SurfaceMap::iterator it;
      OptionsSurface      *srf;
      int                  rc;

      for ( rc=0,it=sdb.begin(); it!=sdb.end(); it++ ) {
         srf = (*it).second;
         if ( srf->Calc( all, now, bForce ) ) {
#ifdef TODO_SURFACE_PUBLISH
            spl->Publish( u, bImg );
#endif // TODO_SURFACE_PUBLISH
            rc += 1;
         }
      }
      return rc;
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
   bool        aOK, bCfg, sqr, trim;
   double      rate, xInc, yInc;
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
   yInc = 1.0; // 1 dollareeny
   maxX = 1000;
   sqr  = false;
   trim = false;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -svr     <MDD Edge3 host:port> ] \\ \n";
      s += "       [ -svc     <MDD Publisher Service> ] \\ \n";
      s += "       [ -rate    <SnapRate> ] \\ \n";
      s += "       [ -xInc    <Spline Increment> ] \\ \n";
      s += "       [ -yInc    <Surface Increment> ] \\ \n";
      s += "       [ -maxX    <Max Spline Values> ] \\ \n";
      s += "       [ -square  <true to 'square up' splines> ] \\ \n";
      s += "       [ -trim    <true to 'trim' surfaces> ] \\ \n";
      LOG( (char *)s.data(), argv[0] );
      LOG( "   Defaults:" );
      LOG( "      -db      : %s", db );
      LOG( "      -svr     : %s", svr );
      LOG( "      -svc     : %s", svc );
      LOG( "      -rate    : %.2f", rate );
      LOG( "      -xInc    : %.2f", xInc );
      LOG( "      -yInc    : %.2f", yInc );
      LOG( "      -maxX    : %d", maxX );
      LOG( "      -square  : %s", _pBool( sqr ) );
      LOG( "      -trim    : %s", _pBool( trim ) );
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
      else if ( !::strcmp( argv[i], "-yInc" ) )
         yInc = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-maxX" ) )
         maxX = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-rate" ) )
         rate = atof( argv[++i] );
      else if ( !::strcmp( argv[i], "-square" ) )
         sqr = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-trim" ) )
         trim = _IsTrue( argv[++i] );
   }

   /////////////////////
   // Do it
   /////////////////////
   OptionsCurve    lvc( db, sqr, trim, xInc, yInc, maxX );
   SplinePublisher pub( lvc, svr, svc, rate );
   double          d0, age;
   size_t          ns;

   /*
    * Dump Config
    */
   LOG( OptionsCurveID() );
   LOG( "Config._square = %s", _pBool( lvc._square ) );
   LOG( "Config._trim   = %s", _pBool( lvc._trim ) );
   LOG( "Config._xInc   = %.2f", lvc._xInc );
   LOG( "Config._yInc   = %.2f", lvc._yInc );
   LOG( "Config._maxX   = %d", lvc._maxX );
   /*
    * Load Splines
    */
   LOG( "Loading splines ..." );
   d0  = lvc.TimeNs();
   ns    = pub.LoadSplines();
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
   ns  = pub.Calc( true );
   age = 1000.0 * ( lvc.TimeNs() - d0 );
   LOG( "%d surfaces / splines cal'ed in %dms", ns, (int)age );
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
