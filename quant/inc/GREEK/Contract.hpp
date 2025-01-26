/******************************************************************************
*
*  Contract.hpp
*     A single OptionGreeks Contract
*
*  REVISION HISTORY:
*      7 APR 2016 jcs  Created.
*     . . .
*     31 OCT 2023 jcs  Created (from libOptionGreeks)
*     17 DEC 2023 jcs  Build  2: RiskFreeCurve
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __GREEK_CONTRACT_HPP
#define __GREEK_CONTRACT_HPP
#include <string.h>
#ifndef DOXYGEN_OMIT
#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif // WIN32
#include <GREEK/RiskFreeCurve.hpp>
#include <GREEK/_Option.hpp>
#include <GREEK/_Volatility.hpp>

static double _ZMIL = 1000000.0;

#endif // DOXYGEN_OMIT

namespace QUANT
{
////////////////////////////////////////////////
//
//      c l a s s   C o n t r a c t
//
////////////////////////////////////////////////

/**
 * \class Contract
 * \brief This class is a single Options Contract associated with a specific 
 * Calculator.
 */
class Contract
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * \param R - RiskFreeCuve
	 * \param bCall - true if CALL; false if PUT
	 * \param X - Contract Strike Price
	 * \param Tt - Time to expiration in % years
	 * \param maxItr - Max Iterations for Newton-Raphson
	 */
	Contract( RiskFreeCurve &R, bool bCall, double X, double Tt, int maxItr=5 ) :
	   _R( R ),
	   _bCall( bCall ),
	   _X( X ),
	   _Tt( Tt ),
	   _ymd( 0 ),
	   _rate( R.r( Tt ) ),
	   _maxItr( maxItr ),
	   _tCalcUs( 0.0 ),
	   _bBi( false )
	{ ; }

	/**
	 * \brief Constructor.
	 *
	 * \param R - RiskFreeCurve
	 * \param bCall - true if CALL; false if PUT
	 * \param X - Contract Strike Price
	 * \param ymd - Expiration date in YYYYMMDD
	 * \param maxItr - Max Iterations for Newton-Raphson
	 */
	Contract( RiskFreeCurve &R, bool bCall, double X, int ymd, int maxItr=5 ) :
	   _R( R ),
	   _bCall( bCall ),
	   _X( X ),
	   _Tt( R.ymd2Tt( ymd ) ),
	   _ymd( ymd ),
	   _rate( R.r( R.ymd2Tt( ymd ) ) ),
	   _maxItr( maxItr ),
	   _bBi( false )
	{ ; }


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return Time to Expiration in % years
	 *
	 * \return Time to Expiration in % years
	 */
	double Tt()
	{
	   return _Tt;
	}

	/**
	 * \brief Return Contract Strike Price
	 *
	 * \return Contract Strike Price
	 */
	double X()
	{
	   return _X;
	}

	/**
	 * \brief Last calc time in micros - ImpliedVolatility(), Delta(), etc.
	 *
	 * \return Last calc time in micros
	 */
	double LastCalcTimeMicros()
	{
	   return _tCalcUs;
	}


	////////////////////////////////////
	// Calculations
	////////////////////////////////////
public:
	/**
	 * \brief Calculate and return all Greeks
	 *
	 * \param C - Contract Price
	 * \param S - Underlyer Price
	 * \param q - Foreign Risk-Free Rate
	 * \return All Greeks
	 */
	Greeks AllGreeks( double C, double S, double q=0.0 )
	{
	   Greeks rc;
	   double d0;

	   d0          = _dNow();
	   rc._impVol  = ImpliedVolatility( C, S );
	   rc._delta   = Delta( S, rc._impVol, q );
	   rc._theta   = Theta( S, rc._impVol, q );
	   rc._gamma   = Gamma( S, rc._impVol, q );
	   rc._vega    = Vega( S, rc._impVol, q );
	   rc._rho     = Rho( S, rc._impVol, q );
	   rc._tCalcUs = ( _dNow() - d0 ) * _ZMIL;
	   _tCalcUs    = rc._tCalcUs;
	   return rc;
	}

	/**
	 * \brief Calculate and return implied volatility
	 *
	 * \param C - Option Price
	 * \param S - Underlyer Price
	 * \param precision - Precision of calculation
	 * \return Implied volatility
	 */
	double ImpliedVolatility( double C, double S, double precision=0.001 )
	{
	   Volatility v( S, _X, _rate, _Tt, precision, _bCall ); 

	   return v.volatility( C, _bBi, _maxItr );
	}

	/**
	 * \brief Return calculation type of last ImpliedVolatility()
	 *
	 * \return Calculation Type of last call to ImpliedVolatility()
	 * \see ImpliedVolatility()
	 */
	const char *ImpVolCalcType()
	{
	   return _bBi ? "Bi-Section" : "Newton-Raphson";
	}

	/**
	 * \brief Calculate and return Option Delta
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Delta
	 */
	double Delta( double S, double stDev, double q=0.0 )
	{
	   OptionDelta grk( stDev, _X, _rate );

	   return _bCall ? grk.call( S, _Tt, q ) : grk.put( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Theta
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Theta
	 */
	double Theta( double S, double stDev, double q=0.0 )
	{
	   OptionTheta grk( stDev, _X, _rate );

	   return _bCall ? grk.call( S, _Tt, q ) : grk.put( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Gamma
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Gamma
	 */
	double Gamma( double S, double stDev, double q=0.0 )
	{
	   OptionGamma grk( stDev, _X, _rate );

	   return grk.value( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Vega
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Vega
	 */
	double Vega( double S, double stDev, double q=0.0 )
	{
	   OptionVega grk( stDev, _X, _rate );

	   return grk.value( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Rho
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Rho
	 */
	double Rho( double S, double stDev, double q=0.0 )
	{
	   OptionRho grk( stDev, _X, _rate );

	   return _bCall ?  grk.call( S, _Tt, q ) : grk.put( S, _Tt, q );
	}

	////////////////////////
	// (Private) Helpers
	////////////////////////
private:
	struct timeval _tvNow()
	{
	   return RiskFreeCurve::_tvNow();
	}

	double _Time2dbl( struct timeval t0 )
	{
	   return RiskFreeCurve::_Time2dbl( t0 );
	}

	double _dNow()
	{
	   return RiskFreeCurve::_dNow();
	}

	////////////////////////
	// Private Members
	////////////////////////
private:
	RiskFreeCurve &_R;
	bool           _bCall;
	double         _X;       // Strike Price
	double         _Tt;      // Time to expire
	int            _ymd;     // Expiration date in YYYYMMDD
	double         _rate;    // Risk-Free Rate from _R @ _Tt
	int            _maxItr;
	double         _tCalcUs; // Time of last calc in micros
	bool           _bBi;

};  // class Contract

} // namespace QUANT

#endif // __GREEK_CONTRACT_HPP 
