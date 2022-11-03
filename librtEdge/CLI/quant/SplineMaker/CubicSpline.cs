/******************************************************************************
*
*  CubicSpline.cs
*     Cubic Spline : From Numerical Recipes in C, (c) 1986-1992
*
*  REVISION HISTORY:
*     21 OCT 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections;

namespace QUANT
{

////////////////////////////////////////////////
//
//     c l a s s   C u b i c S p l i n e
//
////////////////////////////////////////////////

/**
 * \class CubicSpline
 * \brief Cubic Spline from sampled values : 1-based (not 0-based) array results
 *
 * You pass in the sampled values into the Constructor, which calculates and 
 * stores the 2nd order derivitive at each sampled point.  Then you iterate 
 * over the X-axis at your discretion and call Spline( double ) to get the value
 * at each iteration point
 */
class CubicSpline
{
   private static double _HUGE = 0.99e30;

   ////////////////////////////////////
   // Constructor / Destructor
   ////////////////////////////////////
   /**
    * \brief Constructor : Natural spline
    *
    * A natural spline has 2nd order derivitive = 0.0 at the boundaries.
    * 
    * \param X - Sampled X-axis values
    * \param Y - Sampled Y-axis values
    */
   public CubicSpline( double[] X, double[] Y )
   {
      _Size = Math.Min( X.Length, Y.Length );
      _X    = new double[_Size+1];
      _Y    = new double[_Size+1];
      _Y2   = new double[_Size+1];
      _yp1  = _HUGE;
      _ypn  = _HUGE;
      /*
       * Stupid fucking 1-based array
       */
      for ( int i=0; i<_Size; _X[i+1]=X[i], i++ );
      for ( int i=0; i<_Size; _Y[i+1]=Y[i], i++ );
      _Calc();
   }

   /**
    * \brief Constructor : Natural spline
    *
    * \param X - Sampled X-axis values
    * \param Y - Sampled Y-axis values
    * \param yp1 - 2nd order derivitive at left boundary 
    * \param ypN - 2nd order derivitive at right boundary 
    */
   public CubicSpline( double[] X, double[] Y, double yp1, double ypN ) 
   {
      _Size = Math.Min( X.Length, Y.Length );
      _X    = new double[_Size+1];
      _Y    = new double[_Size+1];
      _Y2   = new double[_Size+1];
      _yp1  = yp1; 
      _ypn  = ypN; 
      /*
       * Stupid fucking 1-based array
       */
      for ( int i=0; i<_Size; _X[i+1]=X[i], i++ );
      for ( int i=0; i<_Size; _Y[i+1]=Y[i], i++ ); 
      _Calc();
   }


   ////////////////////////////////////
   // Access / Operations
   ////////////////////////////////////
   /**
    * \brief Return calculated 2nd order derivitive array
    *
    * \return Calculated 2nd order derivitive array
    */
   public double[] Y2()
   {
      return _Y2;
   }

   /**
    * \brief Calculate and return interpolated value at x
    *
    * \param x - Independent variable
    * \return Interpolated value at x
    */
   public double Spline( double x )
   {
      int    klo, khi, k;
      double h, b, a, y;
      double y1, y2, y3;

      klo = 1;
      khi = _Size;
      for ( ; ( khi-klo ) > 1; ) {
         k = ( khi+klo ) >> 1;
         if ( _X[k] > x ) 
            khi = k;
         else
            klo = k;
      }
      h = _X[khi] - _X[klo];
      if ( h == 0.0 )
         return -1.0;   // nrerror("Bad _X input to routine splint");
      a   = ( _X[khi] - x ) / h;
      b   = ( x - _X[klo] ) / h;
      y1  = ( a * _Y[klo] ) + ( b * _Y[khi] );
      y2  = ( ( a * a * a - a ) * _Y2[klo] );
      y2 += ( ( b * b * b - b ) * _Y2[khi] );
      y3  = ( h * h ) / 6.0;
      y   = y1 + y2 * y3;
      return y;
   }

   ////////////////////////////////////
   // Helpers Interface
   ////////////////////////////////////
   /**
    * \brief spline.c : From Numerical Recipes in C, (c) 1986-1992
    */
   private void _Calc()
   {
      double[] u;
      int      i, k, n;
      double   p, qn, sig, un;

      n = _Size;
      u = new double[n+1];
      if ( _yp1 >= _HUGE ) {
         _Y2[1] = 0.0;
         u[1]   = 0.0;
      }
      else {
         _Y2[1] = -0.5;
         u[1]  = ( 3.0 / ( _X[2] - _X[1] ) );
         u[1] *= ( ( _Y[2] - _Y[1] ) / ( _X[2] -_X[1] ) - _yp1 );
      }

      for ( i=2; i<=n-1; i++ ) {
         sig    = ( _X[i] - _X[i-1] ) / ( _X[i+1] - _X[i-1] );
         p      = sig * _Y2[i-1] + 2.0;
         _Y2[i] = ( sig - 1.0 ) / p;
         u[i]   = ( _Y[i+1] - _Y[i] ) / ( _X[i+1] - _X[i] );
         u[i]  -= ( _Y[i] - _Y[i-1] ) / ( _X[i] - _X[i-1] );
         u[i]   = ( 6.0 * u[i] / ( _X[i+1] - _X[i-1] ) - sig * u[i-1] ) / p;
      }
      if ( _ypn >= _HUGE ) {
         qn = 0.0;
         un = 0.0;
      }
      else {
         qn  = 0.5;
         un  = ( 3.0 / ( _X[n] - _X[n-1] ) );
         un *= ( _ypn - ( _Y[n] - _Y[n-1] ) / ( _X[n] - _X[n-1] ) );
      }
      _Y2[n] = ( un - qn * u[n-1] ) / ( qn * _Y2[n-1] + 1.0 );
      for ( k=n-1; k>=1; k-- )
         _Y2[k] = ( _Y2[k] * _Y2[k+1] ) + u[k]; 
      return;
   }

   ////////////////////////
   // private Members
   ////////////////////////
   /** \brief Sampled X-axis values */
   private double[] _X;
   /** \brief Sampled Y-axis values */
   private double[] _Y;
   /** \brief gmin( _X.size(), _Y.size() ) */
   private int      _Size;
   /** \brief 2nd order derivative at _Y : Calculated Values */
   private double[] _Y2;
   /** 
    * \brief 2nd order derivative at _X[0]
    *
    * Set to _HUGE for 'natural spline' w/ 2nd order derivitive = 0
    */
   private double  _yp1;
   /** 
    * \brief 2nd order derivative at _X[n]
    *
    * Set to _HUGE for 'natural spline' w/ 2nd order derivitive = 0
    */
   private double  _ypn;

}; // class CubicSpline

} // namespace QUANT
