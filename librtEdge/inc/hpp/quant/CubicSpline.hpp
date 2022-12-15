/******************************************************************************
*
*  CubicSpline.hpp
*     Cubic Spline : From Numerical Recipes in C, (c) 1986-1992
*
*  REVISION HISTORY:
*     21 OCT 2022 jcs  Created.
*     31 OCT 2022 jcs  Surface.
*     26 NOV 2022 jcs  RTEDGE::DoubleXY / RTEDGE::DoubleXYZ
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __CubicSpline_H
#define __CubicSpline_H

using namespace std;

namespace QUANT
{

#ifndef DOXYGEN_OMIT
#define _HUGE 0.99e30

#define _gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define _gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )

#endif // DOXYGEN_OMI

////////////////////////////////////////////////
//
//     c l a s s   C u b i c S p l i n e
//
////////////////////////////////////////////////

/**
 * \class CubicSpline
 * \brief Cubic Spline from sampled values
 *
 * You pass in the sampled values into the Constructor, which calculates and 
 * stores the 2nd order derivitive at each sampled point.  Then you iterate 
 * over the X-axis at your discretion and call Spline( double ) to get the value
 * at each iteration point
 */
class CubicSpline
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor : Natural spline
	 *
	 * A natural spline has 2nd order derivitive = 0.0 at the boundaries.
	 *
	 * \param X - Sampled X-axis values
	 * \param Y - Sampled Y-axis values
	 */
	CubicSpline( RTEDGE::DoubleList &X, RTEDGE::DoubleList &Y ) :
	   _XY(),
	   _Y2(),
	   _Size( _gmin( X.size(), Y.size() ) ),
	   _yp1( _HUGE ),
	   _ypn( _HUGE )
	{
	   RTEDGE::DoubleXY xy;

	   _XY.resize( _Size+1 );
	   _Y2.resize( _Size+1 );
	   /*
	    * Stupid fucking 1-based array
	    */
	   for ( size_t i=0; i<_Size; i++ ) {
	      xy._x    = X[i];
	      xy._y    = Y[i];
	      _XY[i+1] = xy;
	   }
	   _Calc();
	}

	/**
	 * \brief Constructor : Natural spline
	 *
	 * A natural spline has 2nd order derivitive = 0.0 at the boundaries.
	 * 
	 * \param XY - Sampled ( x,y ) values
	 */
	CubicSpline( RTEDGE::DoubleXYList &XY ) :
	   _XY(),
	   _Y2(),
	   _Size( XY.size() ),
	   _yp1( _HUGE ),
	   _ypn( _HUGE )
	{
	   _XY.resize( _Size+1 );
	   _Y2.resize( _Size+1 );
	   /*
	    * Stupid fucking 1-based array
	    */
	   for ( size_t i=0; i<_Size; _XY[i+1]=XY[i], i++ );
	   _Calc();
	}

	/**
	 * \brief Constructor : Natural spline
	 *
	 * \param XY - Sampled ( x,y ) values
	 * \param yp1 - 2nd order derivitive at left boundary 
	 * \param ypN - 2nd order derivitive at right boundary 
	 */
	CubicSpline( RTEDGE::DoubleXYList &XY, double yp1, double ypN ) : 
	   _XY(),
	   _Y2(),
	   _Size( XY.size() ),
	   _yp1( yp1 ),
	   _ypn( ypN )
	{
	   _XY.resize( _Size+1 );
	   _Y2.resize( _Size+1 );
	   /*
	    * Stupid fucking 1-based array
	    */
	   for ( size_t i=0; i<_Size; _XY[i+1]=XY[i], i++ );
	   _Calc();
	}


	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return calculated 2nd order derivitive array
	 *
	 * \return Calculated 2nd order derivitive array
	 */
	RTEDGE::DoubleList &Y2()
	{
	   return _Y2;
	}

	/**
	 * \brief Calculate and return interpolated value at x
	 *
	 * From splint.c : From Numerical Recipes in C, (c) 1986-1992
	 *
	 * \param x - Independent variable
	 * \return Interpolated value at x
	 */
	double Spline( double x )
	{
	   size_t klo, khi, k;
	   double h, b, a, y;
	   double y1, y2, y3;

	   klo = 1;
	   khi = _Size;
	   for ( ; ( khi-klo ) > 1; ) {
	      k = ( khi+klo ) >> 1;
	      if ( X( k ) > x )
	         khi = k;
	      else
	         klo = k;
	   }
	   h = X( khi ) - X( klo );
	   if ( h == 0.0 )
	      return -1.0;   // nrerror("Bad _X input to routine splint");
	   a   = ( X( khi ) - x ) / h;
	   b   = ( x - X( klo ) ) / h;
	   y1  = ( a * Y( klo ) ) + ( b * Y( khi ) );
	   y2  = ( ( a * a * a - a ) * _Y2[klo] );
	   y2 += ( ( b * b * b - b ) * _Y2[khi] );
	   y3  = ( h * h ) / 6.0;
	   y   = y1 + y2 * y3;
	   return y;
	}

#ifndef DOXYGEN_OMIT
	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	/**
	 * \brief spline.c : From Numerical Recipes in C, (c) 1986-1992
	 */
	void _Calc()
	{
	   RTEDGE::DoubleList &y2 = _Y2;
	   RTEDGE::DoubleList  u;
	   size_t              i, k, n;
	   double              p, qn, sig, un;

	   n = _Size;
	   u.resize( n+1 );
	   if ( _yp1 >= _HUGE ) {
	      y2[1] = 0.0;
	      u[1]  = 0.0;
	   }
	   else {
	      y2[1] = -0.5;
	      u[1]  = ( 3.0 / ( X( 2 ) - X( 1 ) ) );
	      u[1] *= ( ( Y( 2 ) - Y( 1 ) ) / ( X( 2 ) - X( 1 ) ) - _yp1 );
	   }

	   for ( i=2; i<=n-1; i++ ) {
	      sig   = ( X( i ) - X( i-1 ) ) / ( X( i+1 ) - X( i-1 ) );
	      p     = sig * y2[i-1] + 2.0;
	      y2[i] = ( sig - 1.0 ) / p;
	      u[i]  = ( Y( i+1 ) - Y( i ) ) / ( X( i+1 ) - X( i ) );
	      u[i] -= ( Y( i ) - Y( i-1 ) ) / ( X( i ) - X( i-1 ) );
	      u[i]  = ( 6.0 * u[i] / ( X( i+1 ) - X( i-1 ) ) - sig * u[i-1] ) / p;
	   }
	   if ( _ypn >= _HUGE ) {
	      qn = 0.0;
	      un = 0.0;
	   }
	   else {
	      qn  = 0.5;
	      un  = ( 3.0 / ( X( n ) - X( n-1 ) ) );
	      un *= ( _ypn - ( Y( n ) - Y( n-1 ) ) / ( X( n ) - X( n-1 ) ) );
	   }
	   y2[n] = ( un - qn * u[n-1] ) / ( qn * y2[n-1] + 1.0 );
	   for ( k=n-1; k>=1; k-- )
	      y2[k] = ( y2[k] * y2[k+1] ) + u[k]; 
	   return;
	}

	/**
	 * \brief Return the i'th x value from _XY
	 *
	 * \return the i'th x value from _XY
	 */
	double X( size_t i )
	{
	   return _XY[i]._x;
	}

	/**
	 * \brief Return the i'th x value from _XY
	 *
	 * \return the i'th x value from _XY
	 */
	double Y( size_t i )
	{
	   return _XY[i]._y;
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	/** \brief Sampled ( x,y ) values */
	RTEDGE::DoubleXYList _XY;
	/** \brief 2nd order derivative at _Y : Calculated Values */
	RTEDGE::DoubleList   _Y2;
	/** \brief _XY.size() */
	size_t               _Size;
	/** 
	 * \brief 2nd order derivative at _X[0]
	 *
	 * Set to _HUGE for 'natural spline' w/ 2nd order derivitive = 0
	 */
	double  _yp1;
	/** 
	 * \brief 2nd order derivative at _X[n]
	 *
	 * Set to _HUGE for 'natural spline' w/ 2nd order derivitive = 0
	 */
	double  _ypn;
#endif // DOXYGEN_OMIT

}; // class CubicSpline


////////////////////////////////////////////////
//
//     c l a s s   C u b i c S u r f a c e
//
////////////////////////////////////////////////

/**
 * \class CubicSurface
 * \brief M x N Cubic Surface from sampled values 
 *
 * You pass in the sampled values into the Constructor, which calculates and 
 * stores the 2nd order derivitive at each sampled point.  Then you iterate 
 * over the X-axis at your discretion and call 
 * Spline( RTEDGE::DoubleList, RTEDGE::DoubleList ) to get the value at each iteration point
 */
class CubicSurface
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor : Natural spline
	 *
	 * A natural spline has 2nd order derivitive = 0.0 at the boundaries.
	 * 
	 * \param X - M Sampled X-axis values
	 * \param Y - N Sampled Y-axis values
	 * \param Z - MxN Sampled Z-axis values
	 */
	CubicSurface( RTEDGE::DoubleList &X, RTEDGE::DoubleList &Y, RTEDGE::DoubleGrid &Z ) :
	   _X(),
	   _Y(),
	   _Z(),
	   _Z2(),
	   _M( X.size() ),
	   _N( Y.size() )
	{
	   RTEDGE::DoubleList z;

	   for ( size_t m=0; m<_M; _X.push_back( X[m] ), m++ );
	   for ( size_t n=0; n<_N; _Y.push_back( Y[n] ), n++ );
	   /*
	    * 0-based array
	    */
	   for ( size_t m=0; m<_M; m++ ) {
	      z.clear();
	      for ( size_t n=0; n<_N; z.push_back( Z[m][n] ), n++ );
	      _Z.push_back( z );
	   }
	   _Calc();
	}

	////////////////////////////////////
	// Access / Operations
	////////////////////////////////////
public:
	/**
	 * \brief Return calculated 2nd order derivitive grid
	 *
	 * \return Calculated 2nd order derivitive grid
	 */
	RTEDGE::DoubleGrid &Z2()
	{
	   return _Z2;
	}

	/**
	 * \brief Calculate and return interpolated value at ( x,y )
	 *
	 * splin2.c : From Numerical Recipes in C, (c) 1986-1992
	 *
	 * \param x - Independent variable at x
	 * \param y - Independent variable at Y
	 * \return Interpolated value at ( x, y )
	 */
	double Surface( double x, double y )
	{
	   RTEDGE::DoubleList y2;
	   double     z;

	   for ( size_t m=0; m<_M; m++ ) {
	      CubicSpline cs2( _Y, _Z[m] );

	      y2.push_back( cs2.Spline( y ) );
	   }

	   CubicSpline cs1( _X, y2 );

	   z = cs1.Spline( x );
	   return z;
	}

#ifndef DOXYGEN_OMIT
	////////////////////////////////////
	// Helpers Interface
	////////////////////////////////////
private:
	/**
	 * \brief splie2.c : From Numerical Recipes in C, (c) 1986-1992
	 */
	void _Calc()
	{
	   for ( size_t m=0; m<_M; m++ ) {
	      CubicSpline cs( _Y, _Z[m] );

	      _Z2.push_back( cs.Y2() );
	   }
	}

	////////////////////////
	// private Members
	////////////////////////
private:
	/** \brief M X-axis values */
	RTEDGE::DoubleList _X;
	/** \brief N Y-axis values */
	RTEDGE::DoubleList _Y;
	/** \brief MxN sampled Z-axis values */
	RTEDGE::DoubleGrid _Z;
	/** \brief 2nd order derivative at _Z : Calculated Values */
	RTEDGE::DoubleGrid _Z2;
	/** \brief _M x _N Grid of Points */
	size_t     _M;
	/** \brief _M x _N Grid of Points */
	size_t     _N;
#endif // DOXYGEN_OMIT

}; // class CubicSurface

} // namespace QUANT

#endif // __CubicSpline_H 
