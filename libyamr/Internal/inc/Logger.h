/******************************************************************************
*
*  Logger.h
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_LOGGER_H
#define __YAMR_LOGGER_H
#include <Internal.h>

namespace YAMR_PRIVATE
{

//////////////////////////
// Library Logger
//////////////////////////
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
	bool  CanLog( int );

	// Operations

	void log( int, char *, ... );
	void logT( int, char *, ... );
	void logT( int, const char *, ... );
	void Write( int, const char *, int );
	void HexLog( int, const char *, int );

	// Class-wide
public:
	static struct timeval tvNow();
	static time_t         tmNow();
	static double         dblNow();
	static u_int64_t      NanoNow();
	static double         Time2dbl( struct timeval );
	static const char    *GetTime( string & );

}; // class Logger

} // namespace YAMR_PRIVATE

#endif // __YAMR_LOGGER_H
