/******************************************************************************
*
*  FFT.hpp
*     Fast Fourier : From Numerical Recipes in C, (c) 1986-1992
*
*  REVISION HISTORY:
*     28 MAY 2023 jcs  Created.
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __FFT_H
#define __FFT_H

using namespace std;

namespace QUANT
{

#ifndef DOXYGEN_OMIT
#define _TINY   1.0e-20
#define _PI_2   M_PI * 2.0

#define _gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define _gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )

#endif // DOXYGEN_OMI

////////////////////////////////////////////////
//
//           c l a s s   F F T 
//
////////////////////////////////////////////////

/**
 * \class FFT
 * \brief Fast Fourier Transform of order 2
 *
 * The sampled data is passed into the constructor has the following properties:
 * -# Complex array containing N values
 * -# Array length = 2*N
 * -# N must be divisible   length = 2*N
 */
class FFT : public RTEDGE::rtEdge
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor : Natural spline
	 *
	 * \param A - Input Array : Complex of length _NN or Real of length 2*_NN 
	 */
	FFT( RTEDGE::DoubleList &A ) :
	   RTEDGE::rtEdge(),
	   _A(),
	   _N( A.size() / 2 ),
	   _FFT()
	{
assert( _IsPowerOfTwo( A.size() ) );
	   for ( size_t i=0; i<_N; _A.push_back( A[i] ), i++ );
	}

	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return original array
	 *
	 * \return Original array
	 */
	RTEDGE::DoubleList &A_matrix() { return _A; }

	/**
	 * \brief Return array size
	 *
	 * \return Array Size
	 */
	size_t N() { return _N; }

	/**
	 * \brief Return FFT array
	 *
	 * \return FFT array
	 */
	RTEDGE::DoubleList &operator()() { return _FFT; }


	////////////////////////////////////
	// Operations
	////////////////////////////////////
public:
	/**
	 * \brief Calculate and return Discrete Transform
	 *
	 * \return FFT array
	 */
	RTEDGE::DoubleList &Discrete()
	{
	   _four1( 1 );
	   return (*this)();
	}
	/**
	 * \brief Calculate and return Inverse Transform
	 *
	 * \return FFT array
	 */
	RTEDGE::DoubleList &Inverse()
	{
	   _four1( -1 );
	   return (*this)();
	}


	////////////////////////
	// private Helpers
	 ////////////////////////
private:
	/**
	 * \brief four1.c : From Numerical Recipes in C, (c) 1986-1992
	 */
	RTEDGE::DoubleList &_four1( int isign )
	{
	   size_t n, mmax, m, j, istep, i;
	   double wtemp, wr, wpr, wpi, wi, theta, tempr, tempi;

	   /*
	    * Bit Reversal : ;Exchange the two complex numbers
	    */
	   _FFT.clear();
	   for ( i=0; i<_N; _FFT.push_back( 0.0 ), i++ );
	   n = _N << 1;
	   j = 0;
	   for ( i=0; i<n; i+= 2 ) {
	      if ( j > i ) { 
	         _FFT[j]   = _A[i];
	         _FFT[j+1] = _A[i+1];
	      }
	      m = ( n >> 1 );
	      while ( ( m > 2 ) && ( j > m ) ) {
	         j -= m;
	         m >>= 1;
	      }
	      j += m;
	   }
	   /*
	    * Danielson-Lanczos Section, executed O( log2 _N ) times
	    */
	   for ( mmax=2; n>mmax; ) {
	      istep = mmax << 1;
	      theta = isign * ( _PI_2 / mmax );
	      wtemp = ::sin( 0.5 * theta );
	      wpr   = -2.0 * ( wtemp * wtemp );
	      wpi   = ::sin( theta );
	      wr    = 1.0;
	      wi    = 0.0;
	      for ( m=0; m<mmax; m+=2 ) {
	         for ( i=m; i<n; i+=istep ) {
	            /*
	             * Danielson-Lanczos Formula
	             */
	            j          = i + mmax;
	            tempr      = wr * ( _FFT[j]   - wi * _FFT[j+1] );
	            tempi      = wr * ( _FFT[j+1] + wi * _FFT[j] );
	            _FFT[j]    = _FFT[i]   - tempr;
	            _FFT[j+1]  = _FFT[i+1] - tempi;
	            _FFT[i]   += tempr;
	            _FFT[i+1] += tempi; 
	         }
	         /*
	          * Trigonometric recurrence
	          */
	         wtemp = wr;
	         wr    = ( wr * wpr ) - ( wi    * wpi ) + wr;
	         wi    = ( wi * wpr ) + ( wtemp * wpi ) + wi;
	      }
	      mmax = istep;
	   }
	   return _FFT;
	}

	bool _IsPowerOfTwo( size_t x )
	{
	   if ( !x )
	      return false;
	   return !( x & ( x-1 ) ) ? true : false;
	}

	////////////////////////
	// private Members
	 ////////////////////////
private:
	/** \brief Original List */
	RTEDGE::DoubleList _A;
	/** \brief _A.size() */
	size_t             _N;
	/** \brief FFT */
	RTEDGE::DoubleList _FFT;

}; // class FFT

} // namespace QUANT

#endif // __FFT_H 
