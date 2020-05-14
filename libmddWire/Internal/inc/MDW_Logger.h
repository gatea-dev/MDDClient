/******************************************************************************
*
*  MDW_Logger.h
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge)
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __MDW_LOGGER_H
#define __MDW_LOGGER_H
#include <MDW_Internal.h>

//////////////////////////
// Forwards
//////////////////////////

//////////////////////////
// Library Logger
//////////////////////////
namespace MDDWIRE_PRIVATE
{
class Logger
{
private:
	FILE  *_log;
	Mutex  _mtx;
	string _name;
	int    _debugLevel;

	// Constructor / Destructor
public:
	Logger( const char *, int );
	~Logger();

	// Access

	Mutex &mtx();

	// Operations

	void log( int, char *, ... );
	void logT( int, char *, ... );
	void Write( int, const char *, int );
	void HexLog( int, const char *, int );

	// Class-wide
public:
	static struct timeval tvNow();
	static time_t         tmNow();
	static double         dblNow();
	static double         Time2dbl( struct timeval );
	static const char    *GetTime( string & );
};

} // namespace MDDWIRE_PRIVATE

#endif // __MDW_LOGGER_H
