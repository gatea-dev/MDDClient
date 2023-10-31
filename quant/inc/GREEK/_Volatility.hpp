/******************************************************************************
*
*  Volatility.h
*     Implied volatility classes
*
*  REVISION HISTORY:
*     22 OCT 2000 jcs  Created.
*      . . .
*     31 OCT 2023 jcs  namespace QUANT
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __QUANT_VOL_HPP
#define __QUANT_VOL_HPP
#include <GREEK/_Option.hpp>

#ifndef DOXYGEN_OMIT

namespace QUANT
{

////////////////////////////////////////////////
//
//       c l a s s   V o l a t i l i t y
//
////////////////////////////////////////////////
/**
 * \class Volatility
 * \brief Volatility : Implied Volatility
 */
class Volatility
{
private:
	double _S;	// Underlyer price
	double _X;	// Strike price
	double _r;	// Risk-free Interest Rate
	double _Tt;	// Time to expiration (% years)
	double _precision;
	bool   _bCall;

	///////////////////////////////
	// Constructor
	///////////////////////////////
	/**
	 * \brief Constructor
	 *
	 * \param S - Underlyer Price
	 * \param X - Option Strike Price
	 * \param r - Risk-free Rate
	 * \param Tt - Time to expire (% year)
	 * \param precision - Precision of calculation
	 * \param bCall - true for call option; false for put
	 */
public:
	Volatility( double S, double X, double r, double Tt,
		double precision = 0.001,
		bool   bCall     = true ) :
	   _S( S ),
	   _X( X ),
	   _r( r ),
	   _Tt( Tt ),
	   _precision( precision ),
	   _bCall( bCall )
	{ ; }

	///////////////////////////////
	// Operations
	///////////////////////////////
	/**
	  * \brief Calculate and return implied volatility
	  *
	  * \param C - Option Contract Price
	  * \param bBiSection - [OUT] true if bi-secition used; false if Newton-Raphson
	  * \param maxItr - Max iterations
	  * \return Implied Volatility
	  */
	double volatility( double C, bool &bBiSection, int maxItr=5 )
	{
	   double est, rtn;

	   /*
	    * 1) Estimate : Brenner / Subrahmanyam, 1988
	    * 2) Newton-Raphson 1st; Else Bi-Section
	    */
	   est = ::sqrt( 2*M_PI / _Tt ) * ( C / _S );
	/*
	 *
	 * http://quant.stackexchange.com/questions/16705/why-an-option-has-sometimes-and-implied-volatility-greater-than-100
	 *
	   if ( !InRange( 0.0, est, 1.0 ) )
	      est = 0.25; // 0.0;
	 */
	   est = ( est == 0.0 ) ? 0.25 : est;
	   bBiSection = !_volNewtonRaphson( C, est, maxItr, rtn );
	   if ( bBiSection )
	      _volBisection( C, maxItr, rtn );
	   return rtn;
	}

	///////////////////////////////
	// Helpers
	///////////////////////////////
private:
	bool _volNewtonRaphson( double C, double x0, int maxItr, double &rtn )
	{
	   double xn, db, vg, v[K];
	   int    i;
	   
	   /*
	    * Newton-Raphson Method:
	    *   1) Only works with "nice vega" (1st derivitive)
	    *   2) Falls apart for out-of-the-money options
	    */
	   xn  = x0;
	   x0  = 0;
	   rtn = -1.0;
	   ::memset( &v, 0, sizeof( v ) );
	   for ( i=0; !i || ( i<maxItr && ::fabs( xn-x0 ) > _precision ); i++ ) {
	      x0   = xn;
	      v[i] = xn;
	      db   = _fcn( x0 );
	      vg   = _fcnd( x0 );
	      if ( IsZero( vg ) )
	         return false;
	      xn   = x0 - ( db - C ) / vg;
	      if ( fpu_error( xn ) || ( xn < 0.0 ) ) {
	         return false;
	      }
	      if ( !InRange( 0.0, xn, 1.0 ) )
	         return false;
	   }
	   rtn = xn;
	   return InRange( 0.0, xn, 1.0 );
	}

	bool _volBisection( double C, int maxItr, double &rtn )
	{
	   double dl, dh, dx, dxold, f, df, xl, xh, tmp;

	   /*
	    * rtsafe.c : "Numerical Recipes in C" pg 366
	    */
	 
	   // 1) Volatility band = { 0.0, 1.0 }

	   static double _r0 = 0.0;
	   static double _r1 = 1.0;

	   xl = _r0;
	   xh = _r1;
	   dl = _fcn( xl );
	   dh = _fcn( xh );
	   if ( C <= dl ) {
	      rtn = 0.0;
	      return true;
	   }
	   if ( dh <= C ) {
	      rtn = 1.0;
	      return true;
	   }
	   rtn   = 0.5 * ( xl+_r1 );
	   dxold = ::fabs( xh-xl );
	   dx    = dxold;
	   f      = _fcn( rtn );
	   df     = _fcnd( rtn );
	   for ( int i=0; i<maxItr; i++ ) {
	      tmp  = ( ( rtn-xh ) * df ) - f;
	      tmp *= ( ( rtn-xl ) * df ) - f;
	      if ( ( tmp >= 0.0 ) || ( ::fabs( 2.0*f ) > ::fabs( dxold*df ) ) ) {
	         dxold = dx;
	         dx    = 0.5 * ( xh-xl );
	         rtn   = xl + dx;
	         if ( xl == rtn ) 
	            return true;
	      }
	      else {
	         dxold = dx;
	         dx    = f / df;
	         tmp   = rtn;
	         rtn  -= dx;
	         if ( tmp == rtn ) 
	            return true;
	      }
	      if ( ::fabs( dx ) < _precision )
	         return true;
	      f  = _fcn( rtn );
	      df = _fcnd( rtn );
	      if ( f < 0.0 )
	         xl = rtn;
	      else
	         xh = rtn;
	   }
	   return false;
	}

	double _fcn( double vol )
	{
	   BlackScholes bs( vol, _X, _r );

	   // Function : Black-Scholes

	   return _bCall ? bs.call( _S, _Tt ) : bs.put( _S, _Tt );
	}

	double _fcnd( double vol )
	{
	   OptionVega vega( vol, _X, _r );

	   // 1st-order derivitive of _fcn() = OptionVega

	   return vega.value( _S, _Tt );
	}

}; // class Volatility

}  // namespace QUANT

#endif // DOXYGEN_OMIT

#endif // __QUANT_VOL_HPP
