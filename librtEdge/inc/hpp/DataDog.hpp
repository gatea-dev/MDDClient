/******************************************************************************
*
*  DataDog.h
*     Release the Hounds : DataDog StatsD Protocol
*
*  REVISION HISTORY:
*     15 JAN 2021 jcs  Created
*     26 APR 2022 jcs  FactSetBridge
*     27 APR 2022 jcs  Up to K tags
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_DataDog_H
#define __RTEDGE_DataDog_H
#ifdef WIN32
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // WIN32
#include <string>

namespace RTEDGE
{

#ifndef DOXYGEN_OMIT

#define _MAX_TAG  1024

class DogTags
{
public:
	const char *_key[_MAX_TAG];
	const char *_val[_MAX_TAG];
	int         _nTag;

}; // class DogTags

#endif // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//        c l a s s   D a t a D o g
//
////////////////////////////////////////////////
/**
 * \class DataDog
 * \brief DataDog channel to spit named metrics to the datadog-agent.
 *
 * 3 DataDog metric types are supported:
 * -# Gauge
 * -# Set
 * -# Count
 *
 * Metrics may be tagged via AddTag().  Up to 1024 tags are supported per
 * DataDog instance
 */
class DataDog
{
private:
	std::string        _dogHost;
	int                _dogPort;
	std::string        _dogPrefix;
	int                _fd;
	struct sockaddr_in _dst;
	DogTags            _dt;

	//////////////////////////////
	// Constructor / Destructor
	//////////////////////////////
public:
	/**
	 * \brief Constructor.  Create UDP socket to DataDog agent
	 *
	 * \param host : datadog-agent IP address as x.y.z.w
	 * \param port : datadog-agent port
	 * \param prefix : optional prefix added to each metric
	 */
	DataDog( const char *host, int port, const char *prefix="" ):
	   _dogHost( host ),
	   _dogPort( port ),
	   _dogPrefix( prefix ),
	   _fd( 0 )
	{
	   // 1) statsd daemon destination address

	   ::memset( &_dst, 0, sizeof( _dst ) );
	   _dst.sin_family      = AF_INET;
	   _dst.sin_port        = htons( port );
	   _dst.sin_addr.s_addr = inet_addr( host );

	   // 2) DogTags

	   ::memset( &_dt, 0, sizeof( _dt ) );
	}

	/** \brief Destructor.  Close UDP socket */
	~DataDog()
	{
#ifdef WIN32
	   ::closesocket( _fd );
#else
	   ::close( _fd );
#endif // WIN32
	   for ( int i=0; i<_dt._nTag; i++ ) {
	      delete[] _dt._key[i];
	      delete[] _dt._val[i];
	   }
	}

	//////////////////////////////
	// Initialize
	//////////////////////////////
public:
	/**
	 * \brief Create socket / bind
	 *
	 * On WIN64, you must call this AFTER a consumer or publisher has 
	 * been instantiated as this loads the Winsock library
	 */
	bool Start()
	{
	   struct sockaddr_in a;
	   int                rc;

	   // create socket / bind

	   _fd = ::socket( AF_INET, SOCK_DGRAM, 0 );
	   ::memset( &a, 0, sizeof( a ) );
	   a.sin_family      = AF_INET;
	   a.sin_port        = 0; // ephemeral
	   a.sin_addr.s_addr = INADDR_ANY;
	   rc = ::bind( _fd, (struct sockaddr *)&a, sizeof( a ) );
	   return( _fd && ( rc == 0 ) );
	}

	//////////////////////////////
	// Dog Tags
	//////////////////////////////
public:
	/**
	 * \brief Add Dog Tag to be appended to all metrics to datadog-agent
	 *
	 * You may add up to 1024 tags
	 *
	 * \param tag : Tag key
	 * \param val : Tag val
	 */
	void AddTag( const char *tag, const char *val )
	{
	   int nt = _dt._nTag;

	   if ( tag && val ) {
	      _dt._key[nt] = ::strdup( tag );
	      _dt._val[nt] = ::strdup( val );
	      _dt._nTag   += 1;
	   }
	}

	//////////////////////////////
	// DataDog Operations
	//////////////////////////////
public:
	/**
	 * \brief Format and send string-ified Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Gauge( std::string &metric, std::string &value )
	{
	   Gauge( metric.data(), value.data() );
	}

	/**
	 * \brief Format and send string-ified Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Gauge( const char *metric, const char *value )
	{
	   _Send( metric, value, 'g' );
	}

	/**
	 * \brief Format and send int32 Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : int value
	 */
	void Gauge( const char *metric, int value )
	{
	   Gauge( metric, (u_int64_t)value );
	}

	/**
	 * \brief Format and send u_int64_t Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Gauge( const char *metric, u_int64_t value )
	{
	   char sVal[K];

	   _sprintf64( sVal, value );
	   Gauge( metric, sVal );
	}

	/**
	 * \brief Format and send string-ified Set metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Set( const char *metric, const char *value )
	{
	   _Send( metric, value, 's' );
	}

	/**
	 * \brief Format and send int32 Set metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : int value
	 */
	void Set( const char *metric, int value )
	{
	   Set( metric, (u_int64_t)value );
	}

	/**
	 * \brief Format and send u_int64_t Set metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Set( const char *metric, u_int64_t value )
	{
	   char sVal[K];

	   _sprintf64( sVal, value );
	   Set( metric, sVal );
	}

	/**
	 * \brief Format and send string-ified Count metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Count( const char *metric, const char *value )
	{
	   _Send( metric, value, 'c' );
	}

	/**
	 * \brief Format and send int32 Count metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Count( const char *metric, int value )
	{
	   Count( metric, (u_int64_t)value );
	}

	/**
	 * \brief Format and send u_int64_t Count metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Count( const char *metric, u_int64_t value )
	{
	   char sVal[K];

	   _sprintf64( sVal, value );
	   Count( metric, sVal );
	}

	//////////////////////////////
	// Helpers
	//////////////////////////////
private:
#ifndef DOXYGEN_OMIT
	void _Send( const char *metric, const char *value, char type )
	{
	   struct sockaddr *sa;
	   std::string      s;
	   char             bp[K];
	   int              i, nt, flags;

	   sprintf( bp, "%s%s:%s|%c", _dogPrefix.data(), metric, value, type );
	   s   = bp;
	   if ( (nt=_dt._nTag) ) {
	      s += "|#";
	      for ( i=0; i<nt; i++ ) {
	         sprintf( bp, "%s:%s", _dt._key[i], _dt._val[i] );
	         s += i ? "," : "";
	         s += bp;
	      }
	   }
	   flags = 0;
	   sa    = (struct sockaddr *)&_dst;
	   ::sendto( _fd, s.data(), s.length(), flags, sa, sizeof( _dst ) );
	}

	int _sprintf64( char *buf, u_int64_t value )
	{
#ifdef WIN32
	   return sprintf( buf, "%llu", value );
#else
	   return sprintf( buf, "%lu", value );
#endif // WIN32
	}
#endif // DOXYGEN_OMIT

}; // class DataDog

} // namespace RTEDGE

#endif // __RTEDGE_DataDog_H
