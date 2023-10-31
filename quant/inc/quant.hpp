/******************************************************************************
*
*  quant.hpp
*     quant library - Cubic Spline / Surface and Greeks
*
*  REVISION HISTORY:
*     31 OCT 2023 jcs  Created (from librtEdge).
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __MDD_QUANT_HPP
#define __MDD_QUANT_HPP
#include <string>
#include <vector>

namespace QUANT
{

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

#include <QUANT/CubicSpline.hpp>
#include <QUANT/FFT.hpp>
#include <QUANT/LU.hpp>
#ifdef _SURFACE_NOT_READY
#include <struct/Surface.hpp>
#endif /* _SURFACE_NOT_READY */

#endif // __MDD_QUANT_HPP
