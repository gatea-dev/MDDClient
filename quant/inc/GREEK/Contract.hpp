/******************************************************************************
*
*  Contract.hpp
*     A single OptionGreeks Contract
*
*  REVISION HISTORY:
*      7 APR 2016 jcs  Created.
*     . . .
*     31 OCT 2023 jcs  Created (from libOptionGreeks)
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __GREEK_CONTRACT_HPP
#define __GREEK_CONTRACT_HPP
#include <string.h>
#ifndef DOXYGEN_OMIT
#include <GREEK/_Option.hpp>
#include <GREEK/_Volatility.hpp>
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
	 * \param bCall - true if CALL; false if PUT
	 * \param X - Contract Strike Price
	 * \param Tt - Time to expiration
	 */
	Contract( bool bCall, double X, double Tt ) :
	   _bCall( bCall ),
	   _X( X ),
	   _Tt( Tt ),
	   _dtExp( 0 ),
	   _tCalcUs( 0.0 )
	{
	}

	/**
	 * \brief Constructor.
	 *
	 * \param bCall - true if CALL; false if PUT
	 * \param X - Contract Strike Price
	 * \param dtExp - Expiration date in YYYYMMDD
	 */
	Contract( bool bCall, double X, int dtExp ) :
	   _bCall( bCall ),
	   _X( X ),
	   _Tt( 0.0 ),
	   _dtExp( dtExp )
	{ ; }


	////////////////////////////////////
	// Calculations
	////////////////////////////////////
public:
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
	 * \param stDev - Volatility (Standard Deviation)
	 * \param r - Risk-Free Rate
	 * \param q - Foreign Risk-Free Rate
	 * \return All Greeks
	 */
	Greeks AllGreeks( double C, 
	                  double S, 
	                  double stDev, 
	                  double r, 
	                  double q=0.0 )
	{
	   Greeks rc;

	   rc._impVol = ImpliedVolatility( C, S, r );
	   rc._delta  = Delta( S, stDev, r, q );
	   rc._theta  = Theta( S, stDev, r, q );
	   rc._gamma  = Gamma( S, stDev, r, q );
	   rc._vega   = Vega( S, stDev, r, q );
	   rc._rho    = Rho( S, stDev, r, q );
	   rc._tCalcUs = 0.0; // TODO
	   return rc;
	}

	/**
	 * \brief Calculate and return implied volatility
	 *
	 * \param C - Option Price
	 * \param S - Underlyer Price
	 * \param r - Risk-Free Rate
	 * \param precision - Precision of calculation
	 * \return Implied volatility
	 */
	double ImpliedVolatility( double C, double S, double r, double precision=0.001 )
	{
	   Volatility v( S, _X, r, _Tt, _bCall, precision ); 
	   bool       bBi;

	   return v.volatility( C, bBi );
	}

	/**
	 * \brief Calculate and return Option Delta
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param r - Risk-Free Rate
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Delta
	 */
	double Delta( double S, double stDev, double r, double q=0.0 )
	{
	   OptionDelta grk( stDev, _X, r );

	   return _bCall ? grk.call( S, _Tt, q ) : grk.put( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Theta
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param r - Risk-Free Rate
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Theta
	 */
	double Theta( double S, double stDev, double r, double q=0.0 )
	{
	   OptionTheta grk( stDev, _X, r );

	   return _bCall ? grk.call( S, _Tt, q ) : grk.put( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Gamma
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param r - Risk-Free Rate
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Gamma
	 */
	double Gamma( double S, double stDev, double r, double q=0.0 )
	{
	   OptionGamma grk( stDev, _X, r );

	   return grk.value( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Vega
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param r - Risk-Free Rate
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Vega
	 */
	double Vega( double S, double stDev, double r, double q=0.0 )
	{
	   OptionVega grk( stDev, _X, r );

	   return grk.value( S, _Tt, q );
	}

	/**
	 * \brief Calculate and return Option Rho
	 *
	 * \param S - Underlyer Price
	 * \param stDev - Volatility (Standard Deviation)
	 * \param r - Risk-Free Rate
	 * \param q - Foreign Risk-Free Rate
	 * \return Option Rho
	 */
	double Rho( double S, double stDev, double r, double q=0.0 )
	{
	   OptionRho grk( stDev, _X, r );

	   return _bCall ?  grk.call( S, _Tt, q ) : grk.put( S, _Tt, q );
	}


	////////////////////////
	// Private Members
	////////////////////////
private:
	bool   _bCall;
	double _X;       // Strike Price
	double _Tt;      // Time to expire
	int    _dtExp;   // Expiration date in YYYYMMDD
	double _tCalcUs; // Time of last calc in micros

};  // class Contract

} // namespace QUANT

#endif // __GREEK_CONTRACT_HPP 
