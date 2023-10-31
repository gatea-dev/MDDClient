/******************************************************************************
*
*  LU.hpp
*     LU Decomposition : From Numerical Recipes in C, (c) 1986-1992
*
*  REVISION HISTORY:
*     14 FEB 2023 jcs  Created.
*     31 OCT 2023 jcs  Move out of librtEdge
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
	LU( QUANT::DoubleGrid &A ) :
	   _A(),
	   _N( A.size() ),
	   _LU(),
	   _lu_idx(),
	   _lu_d( 0 ),
	   _B()
	{
	   _Copy( A, _A );
	}

	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return original matrix
	 *
	 * \return original matrix
	 */
	QUANT::DoubleGrid &A_matrix() { return _A; }

	/**
	 * \brief Return matrix size
	 *
	 * \return Matrix Size
	 */
	size_t N() { return _N; }

	/**
	 * \brief Return decomposed matrix LU
	 *
	 * \return decomposed matrix LU
	 */
	QUANT::DoubleGrid &LU_matrix() { return _LU; }

	/**
	 * \brief Return true if decomposed
	 *
	 * \return true if decomposed
	 */
	bool IsDecomposed() { return( _LU.size() > 0 ); }


	////////////////////////////////////
	// Operations
	////////////////////////////////////
public:
	/**
	 * \brief Solve set of n linear equations A * X = B, returning B
	 *
	 * \param X : Right hand size vector of length n
	 * \return B
	 */
	QUANT::DoubleList &BackSub( QUANT::DoubleList &X )
	{
	   QUANT::DoubleGrid &A   = _LU;
	   QUANT::DoubleList &B   = _B;
	   QUANT::DoubleList &idx = _lu_idx;
	   int                 i, ii, ip, j, N;
	   double              sum;

	   B.clear();
	   if ( !A.size() )
	      Decompose();
	   N = (int)_N;
	   for ( i=0; i<N; B.push_back( X[i++] ) );
	   for ( i=0,ii=-1; i<N; i++ ) {
	      ip    = idx[i];
	      sum   = B[ip];
	      B[ip] = B[i];
	      if ( ii >= 0 )
	         for ( j=ii; j<i; sum -= ( A[i][j] * B[j] ), j++ );
	      else if ( sum )
	         ii = i;
	      B[i] = sum; 
	   }
	   for ( i=N-1; i>=0; i-- ) { // TODO : OK??
	      sum = B[i];
	      for ( j=i+1; j<N; sum -= ( A[i][j] * B[j] ), j++ );
	      B[i] = sum / A[i][i];
	   }    
	   return B;
	}

	/**
	 * \brief Decompose _N x _N matrix _A via LU decomposition
	 *
	 * \return LU 
	 */
	QUANT::DoubleGrid &Decompose()
	{
	   int                i, j, k, imax, N;
	   double             big, dum, sum, tmp;
	   QUANT::DoubleList vv;

	   _lu_d = 1.0;
	   _lu_idx.clear();
	   for ( size_t ii=0; ii<_N; _lu_idx.push_back( 0.0 ), ii++ );
	   _Copy( _A, _LU );
	   N = (int)_N;
	   for ( i=0; i<N; i++ ) {
	      big = 0.0;
	      for ( j=0; j<N; j++ ) {
	         tmp = ::fabs( _LU[i][j] );
	         big = ( tmp > big ) ? tmp : big;
	      }
	      if ( big == 0.0 )
	         return _LU; // nrerror("Singular matrix in routine ludcmp"); 
	      vv.push_back( 1.0 / big );
	   }
	   for ( j=0; j<N; j++ ) {
	      for ( i=0; i<j; i++ ) {
	         sum = _LU[i][j];   
	         for ( k=0; k<i; k++ )
	            sum -= ( _LU[i][k] * _LU[k][j] );
	         _LU[i][j] = sum;   
	      }
	      big=0.0;  
	      for ( i=j; i<N; i++ ) {
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
	         for ( k=0; k<N; k++ ) {
	            dum          = _LU[imax][k];
	            _LU[imax][k] = _LU[j][k]; 
	            _LU[j][k]    = dum;
	         }
	         _lu_d    = -_lu_d;
	         vv[imax] = vv[j];
	      }
	      _lu_idx[j] = imax;
	      if ( _LU[j][j] == 0.0 ) 
	         _LU[j][j] = _TINY;
	      if ( j != (N-1) ) {
	         dum = 1.0 / _LU[j][j];
	         for ( i=j+1; i<N; _LU[i][j] *= dum, i++ );
	      }
	   }
	   return _LU;
	}


	/**
	 * \brief Invert A
	 *
	 * \return Inverted matrix 
	 */
	QUANT::DoubleGrid Invert()
	{
	   size_t     i, j;
	   QUANT::DoubleList X;
	   QUANT::DoubleGrid rc, inv;

	   _LU.clear();
	   Decompose();
	   for ( j=0; j<_N; j++ ) {
	      X.clear();
	      for ( i=0; i<_N; X.push_back( 0.0 ), i++ );
	      X[j] = 1.0;
	      rc.push_back( QUANT::DoubleList( BackSub( X ) ) ); 
	   }
	   return _Transpose( rc );
	}

	////////////////////////
	// private Helpers
	 ////////////////////////
private:
	void _Copy( QUANT::DoubleGrid &src, QUANT::DoubleGrid &dst )
	{
	   QUANT::DoubleList a;

	   dst.clear();
	   for ( size_t i=0; i<_N; i++ ) {
	      a.clear();
	      for ( size_t j=0; j<_N; a.push_back( src[i][j] ), j++ );
	      dst.push_back( a ); 
	   }    
	}

	QUANT::DoubleGrid _Transpose( QUANT::DoubleGrid &src )
	{
	   QUANT::DoubleGrid dst;
	   size_t             i, j, N;

	   // ENSURE : MUST BE N x N

	   _Copy( src, dst );
	   N = src.size();
	   for ( i=0; i<N; i++ )
	      for ( j=0; j<N; dst[i][j]=src[j][i], j++ );
	   return QUANT::DoubleGrid( dst );
	}

	////////////////////////
	// private Members
	 ////////////////////////
private:
	/** \brief Original Matrix */
	QUANT::DoubleGrid _A;
	/** \brief Matrix Struct : N x N */
	size_t             _N;
	/** \brief Decomposed Matrix */
	QUANT::DoubleGrid _LU;
	/** \brief Records the row permutation effected by partial pivoting */
	QUANT::DoubleList _lu_idx;
	/** \brief 1 if even row interchanges; -1 if odd */
	int                _lu_d;
	/** \brief Result Matrix */
	QUANT::DoubleList _B;

}; // class LU

} // namespace QUANT

#endif // __LU_H 
