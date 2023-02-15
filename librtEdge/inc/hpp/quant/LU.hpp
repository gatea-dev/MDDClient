/******************************************************************************
*
*  LU.hpp
*     LU Decomposition : From Numerical Recipes in C, (c) 1986-1992
*
*  REVISION HISTORY:
*     14 FEB 2023 jcs  Created.
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __LU_H
#define __LU_H

using namespace std;

namespace QUANT
{

#ifndef DOXYGEN_OMIT
#define _TINY 1.0e-20

#define _gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define _gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )

#endif // DOXYGEN_OMI

////////////////////////////////////////////////
//
//           c l a s s   L U 
//
////////////////////////////////////////////////

/**
 * \class LU
 * \brief LU Decomposition and Back substitution
 *
 * -# Decompose N x N matrix A
 * -# Solve() : B = A * X after decomposition
 */
class LU
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor : Natural spline
	 *
	 * \param A - Input Matrix
	 */
	LU( RTEDGE::DoubleGrid &A ) :
	   _A(),
	   _N( A.size() ),
	   _LU(),
	   _lu_idx(),
	   _lu_d( 0 ),
	   _B()
	{
	   _Copy( A, _A );
	   _Decompose();
	}

	////////////////////////////////////
	// Operations
	////////////////////////////////////
public:
	/**
	 * \brief Solve set of n linear equations A * X = B, returning B
	 *
	 * \param A : n x n Input Array
	 * \param X : Right hand size vector of length n
	 * \return B
	 */
	RTEDGE::DoubleList &Solve( RTEDGE::DoubleList &X )
	{
	   RTEDGE::DoubleGrid &A   = _A;
	   RTEDGE::DoubleList &B   = _B;
	   RTEDGE::DoubleList &idx = _lu_idx;
	   size_t              i, ii, ip, j;
	   double              sum;

	   B.clear();
	   for ( i=0; i<_N; B.push_back( X[i++] ) );
	   for ( i=0,ii=0; i<_N; i++ ) {
			ip    = idx[i];
			sum   = B[i];
	      B[ip] = B[i];
	      if ( ii )
	         for ( j=ii; j<=i-1; sum -= ( A[i][j] * B[j] ), j++ );
	      else if ( sum )
	         ii = i;
	      B[i] = sum; 
	   }
	   for ( i=_N-1; i>=0; i-- ) { // TODO : OK??
	      sum = B[i];
	      for ( j=i+1; j<_N; sum -= ( A[i][j] * B[j] ), j++ );
	      B[i] = sum / A[i][i];
	   }    
	   return B;
	}

	////////////////////////////////////
	// (private) Helpers
	////////////////////////////////////
private:
	/**
	 * \brief Decompose _N x _N matrix _A via LU decomposition
	 */
	void _Decompose()
	{
	   size_t             i, j, k, imax;
	   double             big, dum, sum, tmp;
	   RTEDGE::DoubleList vv;

	   _lu_d = 1.0;
	   _lu_idx.clear();
	   _Copy( _A, _LU );
	   for ( i=0; i<_N; i++ ) {
	      big = 0.0;
	      for ( j=0; j<_N; j++ ) {
	         tmp = ::fabs( _LU[i][j] );
	         big = ( tmp > big ) ? tmp : big;
	      }
	      if ( big == 0.0 )
	         return; // nrerror("Singular matrix in routine ludcmp"); 
	      vv.push_back( 1.0 / big );
	   }
	   for ( j=0; j<_N; j++ ) {
	      for ( i=0; i<_N; i++ ) {
	         sum = _LU[i][j];   
	         for ( i=0; k<i; k++ )
	            sum -= ( _LU[i][k] * _LU[k][j] );
	         _LU[i][j] = sum;   
	      }
	      big=0.0;  
	      for ( i=j; i<_N; i++ ) {
	         sum = _LU[i][j];   
	         for ( k=0; k<j; k++ ) 
	            sum -= ( _LU[i][k] * _LU[k][j] ); 
	         _LU[i][j] = sum;   
	         dum       = vv[i] * ::fabs( sum );
	         if ( dum >= big ) {
	            big  = dum;
	            imax = i;
	         }
	      }
	      if (j != imax ) {
	         for ( k=0; k<_N; k++ ) {
	            dum          = _LU[imax][k];
	            _LU[imax][k] = _LU[j][k]; 
	            _LU[j][k]    = dum;
	         }
	         _lu_d    = -_lu_d;
	         vv[imax] = vv[j];
	      }
	      _lu_idx.push_back( imax );
	      if ( _LU[j][j] == 0.0 ) 
	         _LU[j][j] = _TINY;
	      if ( j != (_N-1) ) {
	         dum = 1.0 / _LU[j][j];
	         for ( i=j+1; i<_N; _LU[i][j] *= dum, i++ );
	      }
	   }
	}

	void _Copy( RTEDGE::DoubleGrid &src, RTEDGE::DoubleGrid &dst )
	{
	   RTEDGE::DoubleList a;

	   dst.clear();
	   for ( size_t i=0; i<_N; i++ ) {
	      a.clear();
	      for ( size_t j=0; j<_N; a.push_back( src[i][j] ), j++ );
	      dst.push_back( a ); 
	   }    
	}


	////////////////////////
	// private Members
	 ////////////////////////
private:
	/** \brief Original Matrix */
	RTEDGE::DoubleGrid _A;
	/** \brief Matrix Struct : N x N */
	size_t             _N;
	/** \brief Decomposed Matrix */
	RTEDGE::DoubleGrid _LU;
	/** \brief Records the row permutation effected by partial pivoting */
	RTEDGE::DoubleList _lu_idx;
	/** \brief 1 if even row interchanges; -1 if odd */
	int                _lu_d;
	/** \brief Result Matrix */
	RTEDGE::DoubleList _B;

}; // class LU

} // namespace QUANT

#endif // __LU_H 
