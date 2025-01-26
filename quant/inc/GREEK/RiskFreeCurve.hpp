/******************************************************************************
*
*  RiskFreeCurve.hpp
*     Risk-Free Rates
*
*  REVISION HISTORY:
*     17 DEC 2023 jcs  Created.
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __GREEK_RISKFREE_HPP
#define __GREEK_RISKFREE_HPP
#ifndef DOXYGEN_OMIT
#include <map>
#include <string.h>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>
typedef __int64        u_int64_t;
#else
#include <sys/time.h>
#endif // WIN32
#include <QUANT/CubicSpline.hpp>

static double _zMIL = 1000000.0;

typedef std::map<u_int64_t, double> RiskFreeMap;
typedef std::map<u_int64_t, int>    Int64Map;

#endif // DOXYGEN_OMIT

namespace QUANT
{
////////////////////////////////////////////////
//
//    c l a s s   R i s k F r e e C u r v e
//
////////////////////////////////////////////////

/**
 * \class RiskFreeCurve
 * \brief The Risk-Free Rate Curve (spline) by Day
 */
class RiskFreeCurve
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * You may optionally pass in the current time if your app is being 
	 * fed recorded data from a tape.
	 *
	 * \param now - Time Now; 0 to snap
	 */
	RiskFreeCurve( time_t now=0 ) :
	   _X(),
	   _Y(),
	   _r(),
	   _rMap(),
	   _julNumMap(),
	   _now( now ? now : _tvNow().tv_sec ),
	   _lt( _snap_localtime() )
	{ ; }

	/**
	 * \brief Constructor.
	 *
	 * You may optionally pass in the current time if your app is being 
	 * fed recorded data from a tape.
	 *
	 * \param X - Maturity (i.e., X-axis) values as julNum 
	 * \param Y - Risk-Free Rate (i.e., Y-axis) values
	 * \param n - Num X, Y
	 * \param bJul - true if X in julNum; false if YYYYMMDD
	 * \param now - Time Now; 0 to snap
	 */
	RiskFreeCurve( double *X, double *Y, size_t n, bool bJul, time_t now=0 ) :
	   _X(),
	   _Y(),
	   _r(),
	   _rMap(),
	   _julNumMap(),
	   _now( now ? now : _tvNow().tv_sec ),
	   _lt( _snap_localtime() )
	{
	   DoubleList XX, YY;

	   for ( size_t i=0; i<n; XX.push_back( X[i++] ) );
	   for ( size_t i=0; i<n; YY.push_back( Y[i++] ) );
	   _Init( XX, YY, bJul );
	}

	/**
	 * \brief Constructor.
	 *
	 * \param X - Maturity (i.e., X-axis) values as julNum 
	 * \param Y - Risk-Free Rate (i.e., Y-axis) values
	 * \param bJul - true if X in julNum; false if YYYYMMDD
	 */
	RiskFreeCurve( DoubleList &X, DoubleList &Y, bool bJul ) :
	   _X(),
	   _Y(),
	   _r(),
	   _rMap(),
	   _julNumMap(),
	   _now( _tvNow().tv_sec ),
	   _lt( _snap_localtime() )
	{
	   _Init( X, Y, bJul );
	}


	////////////////////////////////////
	// Initialization
	////////////////////////////////////
	/**
	 * \brief Initialize and calculate Spline.
	 *
	 * \param X - Maturity (i.e., X-axis) values as julNum 
	 * \param Y - Risk-Free Rate (i.e., Y-axis) values
	 * \param bJul - true if X in julNum; false if YYYYMMDD
	 * \return Spline Size
	 */
	size_t Init( DoubleList &X, DoubleList &Y, bool bJul )
	{
	   _Init( X, Y, bJul );
	   return _rMap.size();
	}

	/**
	 * \brief Initialize and calculate Spline.
	 *
	 * \param X - Maturity (i.e., X-axis) values as julNum 
	 * \param Y - Risk-Free Rate (i.e., Y-axis) values
	 * \param n - Num X, Y
	 * \param bJul - true if X in julNum; false if YYYYMMDD
	 * \return Spline Size
	 */
	size_t Init( double *X, double *Y, size_t n, bool bJul )
	{
	   DoubleList XX, YY;

	   for ( size_t i=0; i<n; XX.push_back( X[i++] ) );
	   for ( size_t i=0; i<n; YY.push_back( Y[i++] ) );
	   return Init( XX, YY, bJul );
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Risk-Free Rate at julNum
	 *
	 * \param jul - Julian Date
	 * \return Risk-Free Rate at jul
	 */
	double r( u_int64_t jul )
	{
	   RiskFreeMap::iterator it;
	   double                rc;

	   rc = 0.0;
	   if ( (it=_rMap.find( jul )) != _rMap.end() )
	      rc = (*it).second;
	   return rc;
	}

	/**
	 * \brief Risk-Free Rate at YYYYMMDD
	 *
	 * \param ymd - YYYYMMDD
	 * \return Risk-Free Rate at ymd
	 */
	double r( int ymd )
	{
	   return r( _ymd2julNum( ymd ) );
	}

	/**
	 * \brief Risk-Free Rate at Time to Expiration in % years
	 *
	 * \param Tt - Time to Expiration in % years
	 * \return Risk-Free Rate at Tt
	 */
	double r( double Tt )
	{
	   return r( JulNum( Tt ) );
	}


	////////////////////////////////////
	// Calculations
	////////////////////////////////////
public:
	/**
	 * \brief Calculate Daily Curve
	 *
	 * \return Curve size
	 * \see r()
	 */
	size_t Calc()
	{
	   QUANT::CubicSpline cs( _X, _Y );
	   size_t             n;
	   double             r;
	   u_int64_t          x0, x1;

	   // Pre-condition(s)

	   _r.clear();
	   _rMap.clear();
	   if ( !(n=_X.size()) )
	      return _rMap.size();

	   // Rock on

	   x0 = _X[0];
	   x1 = _X[n-1];
	   for ( u_int64_t x=x0; x<=x1; x++ ) {
	      r        = cs.ValueAt( x );
	      _rMap[x] = r;
	      _r.push_back( r );
	   }
	   return _rMap.size();
	}

	/**
	 * \brief Convert YYYYMMDD to JulNum
	 *
	 * \param ymd - YYYYMMDD
	 * \return JulNum
	 */
	u_int64_t JulNum( u_int64_t ymd )
	{
	   Int64Map          &jdb = _julNumMap;
	   Int64Map::iterator it;
	   u_int64_t          rc;

	   if ( !(rc=_ymd2julNum( ymd )) ) {
	      rc       = _ymd2julNum( ymd );
	      jdb[ymd] = rc;
	   }
	   else
	      rc = (*it).second;
	   return rc;
	}

	/**
	 * \brief Convert Time to Expiration in % years to julNum
	 *
	 * \param Tt - Time to Expiration in % years
	 * \return julNum
	 */
	u_int64_t JulNum( double Tt )
	{
	   double jul = _now / 86400;

	   jul += ( 365.25 * Tt );
	   return (u_int64_t)jul;
	}

	/**
	 * \brief YYYYMMDD to % year
	 *
	 * \param ymd - YYYYMMDD
	 * \return % years
	 */
	double ymd2Tt( int ymd )
	{
	   time_t t0 = _ymd2unix( ymd, _lt );

	   return ( 1.0 / 365.25 ) * ( t0 - _now );
	}


	////////////////////////
	// Helpers
	////////////////////////
private:
	/**
	 * \brief Initialize and calculate Spline.
	 *
	 * \param X - Maturity (i.e., X-axis) values as julNum 
	 * \param Y - Risk-Free Rate (i.e., Y-axis) values
	 * \param bJul - true if X in julNum; false if YYYYMMDD
	 */
	void _Init( DoubleList &X, DoubleList &Y, bool bJul )
	{
	   size_t n = gmin( X.size(), Y.size() );

	   _X.clear();
	   _Y.clear();
	   _r.clear();
	   _rMap.clear();
	   if ( bJul )
	      for ( size_t i=0; i<n; _X.push_back( X[i++] ) );
	   else
	      for ( size_t i=0; i<n; _X.push_back( JulNum( X[i++] ) ) );
	   for ( size_t i=0; i<n; _Y.push_back( Y[i++] ) );
	   Calc();
	}

	u_int64_t _ymd2julNum( u_int64_t ymd )
	{
	   return _ymd2unix( ymd, _lt ) / 86400;
	}

	struct tm _snap_localtime()
	{
	   struct tm rc;

	   ::localtime_r( &_now, &rc );
	   return rc;
	}

	////////////////////////
	// Class-wide
	////////////////////////
public:
	/**
	 * \brief Convert YYYYMMDD to Unix Time
	 *
	 * \param ymd - YYYYMMDD
	 * \param lt - Existing localtime struct
	 * \return Unix Time
	 */
	static time_t _ymd2unix( u_int64_t ymd, struct tm &lt )
	{
	   struct tm ll;
	   time_t    rc;

	   ll          = lt;
	   ll.tm_year  = ( ymd / 10000 );
	   ll.tm_year -= 1900;
	   ll.tm_mon   = ( ymd / 100 ) % 100;
	   ll.tm_mon  -= 1;
	   ll.tm_mday  = ( ymd % 100 );
	   rc          = ::mktime( &ll );
	   return rc;
	}

	static struct timeval _tvNow()
	{    
	   struct timeval tv;

	   // Platform-dependent
#ifdef WIN32
	   struct _timeb tb;

	   ::_ftime( &tb );
	   tv.tv_sec  = tb.time;
	   tv.tv_usec = tb.millitm * 1000;
#else
	   ::gettimeofday( &tv, (struct timezone *)0 );
#endif // WIN32
	   return tv;
	}    

	static double _Time2dbl( struct timeval t0 ) 
	{    
	   double rtn; 
	   static double _num = 1.0 / _zMIL;

	   rtn = t0.tv_sec + ( t0.tv_usec *_num );
	   return rtn; 
	}    

	static double _dNow()
	{    
	   return _Time2dbl( _tvNow() );
	}    

	////////////////////////
	// Private Members
	////////////////////////
private:
	DoubleList  _X;
	DoubleList  _Y;
	DoubleList  _r; // Daily 
	RiskFreeMap _rMap;
	Int64Map    _julNumMap;
	time_t      _now;
	struct tm   _lt;

};  // class RiskFreeCurve

} // namespace QUANT

#endif // __GREEK_RISKFREE_HPP 
