/******************************************************************************
*
*  DataDog.h
*
*  REVISION HISTORY:
*      3 MAY 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#endif // DOXYGEN_OMIT

namespace librtEdge
{


////////////////////////////////////////////////
//
//       c l a s s   D a t a D o g
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
public ref class DataDog : public librtEdge::rtEdge
{
private: 
	RTEDGE::DataDog *_cpp;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	/**
	 * \brief Constructor.  Create UDP socket to DataDog agent
	 *
	 * \param host : datadog-agent IP address as x.y.z.w
	 * \param port : datadog-agent port
	 */
	DataDog( String ^host, int port ) :
	   _cpp( new RTEDGE::DataDog( _pStr( host ), port ) )
	{ ; }

	/**
	 * \brief Constructor.  Create UDP socket to DataDog agent
	 *
	 * \param host : datadog-agent IP address as x.y.z.w
	 * \param port : datadog-agent port
	 * \param prefix : Prefix added to each metric
	 */
	DataDog( String ^host, int port, String ^prefix ) :
	   _cpp( new RTEDGE::DataDog( _pStr( host ), port, _pStr( prefix ) ) )
	{ ; }

	/** \brief Destructor */
	~DataDog()
	{
	   delete _cpp;
	   _cpp = (RTEDGE::DataDog *)0;
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
	void AddTag( String ^tag, String ^val )
	{
	   _cpp->AddTag( _pStr( tag ), _pStr( val ) );
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
	void Gauge( String ^metric, String ^value )
	{
	   _cpp->Gauge( _pStr( metric ), _pStr( value ) );
	}

	/**
	 * \brief Format and send int32 Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : int value
	 */
	void Gauge( String ^metric, int value )
	{
	   _cpp->Gauge( _pStr( metric ), value );
	}

	/**
	 * \brief Format and send u_int64_t Gauge metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Gauge( String ^metric, long long value )
	{
	   _cpp->Gauge( _pStr( metric ), (u_int64_t)value );
	}

	/**
	 * \brief Format and send string-ified Set metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Set( String ^metric, String ^value )
	{
	   _cpp->Set( _pStr( metric ), _pStr( value ) );
	}

	/**
	 * \brief Format and send int32 Set metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : int value
	 */
	void Set( String ^metric, int value )
	{
	   _cpp->Set( _pStr( metric ), value );
	}

	/**
	 * \brief Format and send u_int64_t Set metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Set( String ^metric, long long value )
	{
	   _cpp->Set( _pStr( metric ), (u_int64_t)value );
	}

	/**
	 * \brief Format and send string-ified Count metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : String-ified value
	 */
	void Count( String ^metric, String ^value )
	{
	   _cpp->Count( _pStr( metric ), _pStr( value ) );
	}

	/**
	 * \brief Format and send int32 Count metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Count( String ^metric, int value )
	{
	   _cpp->Count( _pStr( metric ), value );
	}

	/**
	 * \brief Format and send u_int64_t Count metric to datadog-agent
	 *
	 * \param metric : Metric name
	 * \param value : u_int64_t value
	 */
	void Count( String ^metric, long long value )
	{
	   _cpp->Count( _pStr( metric ), (u_int64_t)value );
	}

}; // class DataDog

} // namespace librtEdge
