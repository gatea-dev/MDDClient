/******************************************************************************
*
*  _Option.hpp
*     Options valuation classes : From Hull
*
*  REVISION HISTORY:
*      7 MAY 2000 jcs  Created.
*      . . .
*     31 OCT 2023 jcs  namespace GREEK
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __QUANT_OPTIONS_HPP
#define __QUANT_OPTIONS_HPP

#ifndef DOXYGEN_OMIT

namespace GREEK
{

////////////////////////////////////////////////
//
//     c l a s s   S t a t s
//
////////////////////////////////////////////////
/**
 * \class Stats
 * \brief Variance
 */
class Stats
{
private:
	double _dMean;
	double _dVar;

	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	Stats( QUANT::DoubleList &ts ) :
	   _dMean( 0.0 ),
	   _dVar( 0.0 )
	{
	   double dd;
	   size_t i, n, nTs;

	   // Variance = _stDev * _stDev

	   _dMean = 0.0;  // Arithmetic Mean
	   _dVar  = 0.0;  // Variance
	   nTs    = ts.size();
	   for ( i=0; i<nTs; _dMean += ts[i++] );
	   n   = nTs ? nTs : 1;
	   _dMean /= n;
	   for ( i=0; i<nTs; i++ ) {
	      dd     = ( ts[i] - _dMean );
	      _dVar += ( dd*dd );
	   }
	   _dVar /= n;
	}

	///////////////////////////////
	// Access
	///////////////////////////////
	double mean()     { return _dMean; }
	double variance() { return _dVar; }
	double stDev()    { return ::sqrt( mean() ); }

}; // class Stats


////////////////////////////////////////////////
//
//     c l a s s   N o r m a m l D i s t
//
////////////////////////////////////////////////

static double _gamma =  0.2316419;
static double _a1    =  0.319381530;
static double _a2    = -0.356563782;
static double _a3    =  1.781477937;
static double _a4    = -1.821255978;
static double _a5    =  1.330274429;
static double _sqpi  =  1 / sqrt( 2 * M_PI );

/**
 * \class NormamlDist
 * \brief Normal Distribution 
 */
class NormalDist
{
private:
	double _dVal;
	double _nPrime;

	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	NormalDist( double x )
	{
	   bool   bNeg;
	   double k, n, z;

	   /*
	    * From Hull, pp 243:
	    *    For standardized normal distribution (i.e., mean=0.0; StdDev=1.0),
	    *    The cumulative probability that the variable is less than x
	    */
	   bNeg    = ( x < 0 );
	   z       = ::fabs( x );
	   _nPrime = pow( M_E, -( pow(x,2.0) / 2.0 ) ) * _sqpi;
	   k       = 1.0 / ( 1.0 + ( z * _gamma ) );
	   n       = _nPrime * ( _a1*k +
	                         _a2*pow(k,2.0) +
	                         _a3*pow(k,3.0) +
	                         _a4*pow(k,4.0) +
	                         _a5*pow(k,5.0) );
	   _dVal = bNeg ? ( 1.0 - ( 1.0 - n ) ) : 1.0 - n;
	}

	///////////////////////////////
	// Access
	///////////////////////////////
	double operator()() { return value(); }
	double value()      { return _dVal; }
	double prime()      { return _nPrime; } 

}; // class NormalDist


////////////////////////////////////////////////
//
//     c l a s s   B l a c k S c h o l e s
//
////////////////////////////////////////////////
/**
 * \class BlackScholes
 * \brief Black-Scholes Pricing Class
 */
class BlackScholes
{
public:
	double _X;	   // Strike Price
	double _r;	   // Risk-free Interest Rate
	double _stDev;	// Standard Deviation

	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	BlackScholes( QUANT::DoubleList &ts, double X, double r ) :
	   _X( IsZero( X ) ? 1 : X ),
	   _r( IsZero( r ) ? 1 : r ),
	   _stDev( 0.0 )
	{
	   Stats st( ts );

	   _stDev = st.stDev();
	}

	BlackScholes( double stDev, double X, double r ) :
	   _X( IsZero( X ) ? 1 : X ),
	   _r( IsZero( r ) ? 1 : r ),
	   _stDev( stDev )
	{ ; }

	BlackScholes( BlackScholes &b ) :
	   _X( b._X ),
	   _r( b._r ),
	   _stDev( b._stDev )
	{ ; }


	///////////////////////////////
	// Operations
	///////////////////////////////
public:
	double call( double S, double tExp )
	{
	   double     cd1 = d1( S, tExp );
	   double     cd2 = cd1 - ( _stDev * ::sqrt( tExp ) );
	   double     rtn;
	   NormalDist n1( cd1 );
	   NormalDist n2( cd2 );

	   rtn = ( S * (n1)() ) - ( _X * ::pow( M_E, -( _r*tExp ) ) * (n2)() );
	   return rtn;
	}

	double put( double S, double tExp )
	{
	   double     cd1 = d1( S, tExp );
	   double     cd2 = cd1 - ( _stDev * ::sqrt( tExp ) );
	   double     rtn;
	   NormalDist n1( -cd1 );
	   NormalDist n2( -cd2 );

	   rtn = ( _X * ::pow( M_E, -( _r*tExp ) ) * (n2)() ) - ( S * (n1)() );
	   return rtn;
	}

	double d1( double S, double tExp, double q=0.0 )
	{
	   double rtn;

	   rtn  = ::log( S/_X );
	   rtn += ( ( _r - q ) + ( ( _stDev*_stDev ) / 2 ) ) * tExp;
	   rtn /= ( _stDev * ::sqrt( tExp ) );
	   return rtn;
	}

	double d2( double S, double tExp, double q=0.0 )
	{
	   double rtn;

	   rtn  = ::log( S/_X );
	   rtn += ( ( _r - q ) - ( ( _stDev*_stDev ) / 2 ) ) * tExp;
	   rtn /= ( _stDev * ::sqrt( tExp ) );
	   return rtn;
	}

}; // class BlackScholes


////////////////////////////////////////////////////////////////////////
//
//                       T H E   G R E E K S
//
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////
//
//     c l a s s   O p t i o n D e l t a
//
////////////////////////////////////////////////
/**
 * \class OptionDelta
 * \brief Delta : Sensitivity to Change in Underlyer
 */
class OptionDelta
{
private:
	BlackScholes _bs;

	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	OptionDelta( BlackScholes &bs ) :
	   _bs( bs )
	{ ; }

	OptionDelta( double stDev, double X, double r ) :
	   _bs( stDev, X, r )
	{ ; }

	///////////////////////////////
	// Operations
	///////////////////////////////
public:
	double call( double S, double tExp, double q=0.0 )
	{
	   double     d1 = _bs.d1( S, tExp, q );
	   double     rtn;
	   double     eq, n;
	   NormalDist n1( d1 );

	   eq   = ::pow( M_E, -( q*tExp ) );
	   if ( (n=(n1)()) || !n )
	      QUANT::breakpoint();
	   rtn  = ( (n1)() * eq );
	   return rtn;
	}

	double put( double S, double tExp, double q=0.0 )
	{
	   double rtn;
	   double eq;

	   eq   = ::pow( M_E, -( q*tExp ) );
	   rtn  = call( S, tExp, q );
	   rtn -= ( 1 * eq );
	   return rtn;
	}

}; // class OptionDelta


////////////////////////////////////////////////
//
//     c l a s s   O p t i o n T h e t a
//
////////////////////////////////////////////////
/**
 * \class OptionTheta
 * \brief Theta : Sensitivity to Change in Time to Expiration
 */
class OptionTheta
{
private:
	BlackScholes _bs;

	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	OptionTheta( BlackScholes &bs ) :
	   _bs( bs )
	{ ; }

	OptionTheta( double stDev, double X, double r ) :
	   _bs( stDev, X, r )
	{ ; }


	///////////////////////////////
	// Operations
	///////////////////////////////
public:
	double call( double S, double tExp, double q=0.0 )
	{
	   double     d1 = _bs.d1( S, tExp, q );
	   double     d2 = _bs.d2( S, tExp, q );
	   double     rtn;
	   double     eq, er;
	   NormalDist n1( d1 );
	   NormalDist n2( d2 );

	   eq   = ::pow( M_E, -( q * tExp ) );
	   er   = ::pow( M_E, -( _bs._r * tExp ) );
	   rtn  = - ( S * n1.prime() * _bs._stDev ) / ( 2 * ::sqrt( tExp ) );
	   rtn *= eq;
	   rtn += ( q * S * (n1)() * eq );
	   rtn -= ( _bs._r * _bs._X * er * (n2)() );
	   return rtn;
	}

	double put( double S, double tExp, double q=0.0 )
	{
	   double     d1 = _bs.d1( S, tExp, q );
	   double     d2 = _bs.d2( S, tExp, q );
	   double     rtn;
	   double     eq, er;
	   NormalDist n1( d1 );
	   NormalDist _n1( -d1 );
	   NormalDist _n2( -d2 );

	   er   = ::pow( M_E, -( _bs._r * tExp ) );
	   eq   = ::pow( M_E, -( q * tExp ) );
	   rtn  = - ( S * n1.prime() * _bs._stDev ) / ( 2 * ::sqrt( tExp ) );
	   rtn *= eq;
	   rtn -= ( q * S * (_n1)() * eq );
	   rtn += ( _bs._r * _bs._X * er * (_n2)() );
	   return rtn;
	}

}; // class OptionTheta


////////////////////////////////////////////////
//
//     c l a s s   O p t i o n G a m m a
//
////////////////////////////////////////////////
/**
 * \class OptionGamma
 * \brief Gamma : Sensitivity to Change in Delta of Time (2nd order Delta)
 */
class OptionGamma
{
private:
	BlackScholes _bs;

	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	OptionGamma( BlackScholes &bs ) :
	   _bs( bs )
	{ ; }

	OptionGamma( double stDev, double X, double r ) :
	   _bs( stDev, X, r )
	{ ; }

	///////////////////////////////
	// Operations
	///////////////////////////////
public:
	double value( double S, double tExp, double q=0.0 )
	{
	   double     d1 = _bs.d1( S, tExp, q );
	   double     rtn, den;
	   double     eq;
	   NormalDist n1( d1 );

	   eq   = ::pow( M_E, -( q * tExp ) );
	   den  = ( S * _bs._stDev * ::sqrt( tExp ) );
	   rtn  = IsZero( den ) ? 0.0 : ( n1.prime() * eq ) / den;
	   return rtn;
	}

}; // class OptionGamma


////////////////////////////////////////////////
//
//     c l a s s   O p t i o n V e g a
//
////////////////////////////////////////////////
/**
 * \class OptionVega
 * \brief Vega : Sensitivity to Change in Implied Volatility
 */
class OptionVega
{
private:
	BlackScholes _bs;
 
	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	OptionVega( BlackScholes &bs ) :
	   _bs( bs )
	{ ; }

	OptionVega( double stDev, double X, double r ) :
	   _bs( stDev, X, r )
	{ ; }

 
	///////////////////////////////
	// Operations
	///////////////////////////////
public:
	double value( double S, double tExp, double q=0.0 )
	{
	   double     _d1 = _bs.d1( S, tExp, q );
	   double     rtn;
	   double     eq;
	   NormalDist n1( _d1 );

	   eq   = ::pow( M_E, -( q * tExp ) );
	   rtn  = ( S * ::sqrt( tExp ) * n1.prime() * eq );
	   return rtn;
	}

}; // class OptionVega
 

////////////////////////////////////////////////
//
//     c l a s s   O p t i o n R h o
//
////////////////////////////////////////////////
/**
 * \class OptionRho
 * \brief Rho : Sensitivity to Change in Risk-Free Rate of Interest
 */
class OptionRho
{
private:
	BlackScholes _bs;
 
	///////////////////////////////
	// Constructor
	///////////////////////////////
public:
	OptionRho( BlackScholes &bs ) :
	   _bs( bs )
	{ ; }

	OptionRho( double stDev, double X, double r ) :
	   _bs( stDev, X, r )
	{ ; }
 
	///////////////////////////////
	// Operations
	///////////////////////////////
public:
	double call( double S, double tExp, double q )
	{
	   double     _d2 = _bs.d2( S, tExp, q );
	   double     rtn;
	   double     eq;
	   NormalDist n2( _d2 );

	   eq  = ::pow( M_E, -( _bs._r * tExp ) );
	   rtn = ( _bs._X * tExp * eq * (n2)() );
	   return rtn;
	}

	double put( double S, double tExp, double q )
	{
	   double     _d2 = _bs.d2( S, tExp, q );
	   double     rtn;
	   double     eq;
	   NormalDist n2( -_d2 );

	   eq  = ::pow( M_E, -( _bs._r * tExp ) );
	   rtn = -( _bs._X * tExp * eq * (n2)() );
	   return rtn;
	}

}; // class OptionRho

}  // namespace GREEK

#endif // DOXYGEN_OMIT
 
#endif // __QUANT_OPTIONS_HPP
