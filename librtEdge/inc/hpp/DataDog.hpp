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

using namespace std;

namespace RTEDGE
{

#ifndef DOXYGEN_OMIT
/** \brief Maximum allowable number of Dog Tags to send */

#define _MAX_TAG  1024

////////////////////////////////////////////////
//
//        c l a s s   D o g T a g s
//
////////////////////////////////////////////////
/**
 * \class DogTags
 * \brief Structure to hold up to 1024 dog tags
 *
 * You populate this struct if you choose to send optional tags with each metric 
 * to the datadog-agent.
 */
class DogTags
{
public:
	/** \brief Up to 1024 DogTag keys */
	const char *_key[_MAX_TAG];
	/** \brief Up to 1024 DogTag values */
	const char *_val[_MAX_TAG];
	/** \brief Number of DogTags */
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
 * \brief DataDog channel to  channel from data source - rtEdgeCache3 or Tape File
 *
 * The 1st argument to Start() defines your data source:
 * 1st Arg | Data Source | Data Type
 * --- | --- | ---
 * host:port | rtEdgeCache3 | Streaming real-time data
 * filename | Tape File (64-bit only) | Recorded market data from tape
 *
 * This class ensures that data from both sources - rtEdgeCache3 and Tape
 * File - is streamed into your application in the exact same manner:
 * + Subscribe()
 * + OnData()
 * + OnDead()
 * + Unsubscribe()
 *
 * Lastly, the Tape File data source is specifically driven from this class:
 * API | Action
 * --- | ---
 * StartTape() | Pump data for Subscribe()'ed tkrs until end of file
 * StartTapeSlice() | Pump data for Subscribe()'ed tkrs in a time interval
 * StopTape() | Stop Tape Pump
 */
class DataDog
{
private:
	string             _dogHost;
	int                _dogPort;
	string             _dogPrefix;
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
	   _fd( ::socket( AF_INET, SOCK_DGRAM, 0 ) )
	{
	   // statsd daemon destination address

	   ::memset( &_dst, 0, sizeof( _dst ) );
	   _dst.sin_family      = AF_INET;
	   _dst.sin_port        = htons( port );
	   _dst.sin_addr.s_addr = inet_addr( host );
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
	// Operations
	//////////////////////////////
public:
	/**
	 * \brief Format and send string-ified Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Gauge( string &metric, string &value )
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
	   string           s;
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
