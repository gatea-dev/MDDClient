/******************************************************************************
*
*  Stats.h
*     MD-Direct run-time Stats File
*
*  REVISION HISTORY:
*     20 SEP 2023 jcs  Created.
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_STATS_H
#define __MDDPY_STATS_H
#include <MDDirect.h>
#include <BBStats.h>


/////////////////////////////////////////
// MDD Run-Time Stats File
/////////////////////////////////////////
class MDDpyStats : public RTEDGE::rtEdge
{
private:
	rtBuf64 _map;

	// Constructor / Destructor
public:
	MDDpyStats( const char * );
	~MDDpyStats();

	// Operations
public:
	PyObject *PyBBDailyStats();

	// Helpers
private:
	void _SetBBStat( PyObject *, int, const char *, BBAPIStat & ); 

};  // class MDDpyStats

#endif // __MDDPY_STATS_H
