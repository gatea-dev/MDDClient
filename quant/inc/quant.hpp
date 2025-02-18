/******************************************************************************
*
*  quant.hpp
*     quant library - Cubic Spline / Surface and Greeks
*
*  REVISION HISTORY:
*     31 OCT 2023 jcs  Created (from librtEdge).
*     18 FEB 2025 jcs  Build  3: WIN64; mainpage
*
*  (c) 1994-2025, Gatea Ltd.
******************************************************************************/
#ifndef __MDD_QUANT_HPP
#define __MDD_QUANT_HPP
#include <math.h>
#include <string>
#include <vector>

/**
 * \mainpage quant API Reference Manual
 *
 * quant is a set of header files for doing basic FinTech quantative 
 * calculations.  2 namespaces are available:
 *
 * Namespace | Source | Description
 * --- | --- | ---
 * GREEK | Hull | Option Greeks - Implied Vol, Delta, Gamma and friends
 * QUANT | NumRec in C | Cubic Spline, Fast Fourier Transform and friends
 *
 * The following sources are used:
 *
 * Source | Book
 * --- | ---
 * Hull | Options, Futures and Other Derivatives by John C Hull, (c) 1997
 * NumRec in Cc | Numerical Recipes in C by Press, Vetterling, et al (c) 1992
 * 
 * ### Usage 
 *
 * This module is a set of header files, rather than a compiled library.  To 
 * use the functionality, simply  add the following to your code:
 *
 * \code
 * #include <quant.hpp>
 * \endcode
 *
 */

#ifdef WIN32
#define M_PI           3.14159265358979323846  /* pi */
#define M_E            2.7182818284590452354   /* e */
#endif // WIN32

namespace QUANT
{

#ifndef DOXYGEN_OMIT

static void breakpoint() { ; }

#define fpu_error(x)          ( isinf(x) || isnan(x) )
#define DZERO                 (double)0.0000001    // (double) zero
#define IsZero(c)             InRange( -DZERO, (c), DZERO )

#endif // DOXYGEN_OMIT

////////////////////////////////////////////////
//
//      c l a s s   D o u b l e X Y
//
////////////////////////////////////////////////

/**
 * \class DoubleXY
 * \brief ( x,y ) tuple
 */
class DoubleXY
{
public:
	/** \brief X value */
	double _x;
	/** \brief Y value */
	double _y;

}; // class DoubleXY


////////////////////////////////////////////////
//
//      c l a s s   D o u b l e X Y Z
//
////////////////////////////////////////////////

/**
 * \class DoubleXYZ
 * \brief ( x,y ) tuple
 */
class DoubleXYZ : public DoubleXY
{
public:
	/** \brief Z value */
	double _z;

}; // class DoubleXYZ

typedef std::vector<std::string>  Strings;
typedef std::vector<double>       DoubleList;
typedef std::vector<DoubleXY>     DoubleXYList;
typedef std::vector<DoubleXYZ>    DoubleXYZList;
typedef std::vector<DoubleList>   DoubleGrid;

} // namespace QUANT

/**
 * \struct Greeks
 * \brief Return value from call to Contract::AllGreeks()
 */
typedef struct {
   /** \brief Calculated Implied Volatility */
   double _impVol;
   /** \brief Calculated Option Delta = 1st Derivitive wrt Underlyer Price */
   double _delta;
   /** \brief Calculated Option Theta = 1st Derivitive wrt Time to Expiration */
   double _theta;
   /** \brief Calculated Option Gamma = 2nd Derivitive wrt Underlyer Price */
   double _gamma;
   /** \brief Calculated Option Vega = 1st Derivitive wrt Volatility */
   double _vega;
   /** \brief Calculated Option Rho = 1st Derivitive wrt Risk-free Rate */
   double _rho;
   /** \brief Micros to calculate and return */
   double _tCalcUs;

} Greeks;

/*
 * Numerical Recipes in C
 */
#include <QUANT/CubicSpline.hpp>
#include <QUANT/FFT.hpp>
#include <QUANT/LU.hpp>

/*
 * Hull : Options, Futures and Other Derivitives
 */
#include <GREEK/RiskFreeCurve.hpp>
#include <GREEK/Contract.hpp>

#endif // __MDD_QUANT_HPP
