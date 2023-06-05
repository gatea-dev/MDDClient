/******************************************************************************
*
*  Logger.h
*
*  REVISION HISTORY:
*     30 JUL 2009 jcs  Created.
*      6 AUG 2009 jcs  Build  2: tmNow()
*      5 OCT 2009 jcs  Build  4: Time2dbl()
*      1 OCT 2010 jcs  Build  8a:time_t, not long
*     24 JAN 2012 jcs  Build 17: HexLog()
*     12 NOV 2014 jcs  Build 28: CanLog(); RTEDGE_PRIVATE
*     21 MAR 2016 jcs  Build 32: EDG_Internal.h; 2 logT()'s
*      3 JUN 2023 jcs  Build 63: HexDump()
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_LOGGER_H
#define __EDGLIB_LOGGER_H
#include <EDG_Internal.h>

namespace RTEDGE_PRIVATE
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
	void HexDump( int, const char *, int );
	void HexLog( int, const char *, int );

	// Class-wide
public:
	static struct timeval tvNow();
	static time_t         tmNow();
	static double         dblNow();
	static double         Time2dbl( struct timeval );
	static const char    *GetTime( string & );
};

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_LOGGER_H
