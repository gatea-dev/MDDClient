/******************************************************************************
*
*  ChartDbSvr.h
*     MD-Direct ChartDbSvr
*
*  REVISION HISTORY:
*      8 JUL 2025 jcs  Created.
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_CHART_H
#define __MDDPY_CHART_H
#include <MDDirect.h>

/////////////////////////////////////////
// Chart D/B Server
/////////////////////////////////////////
class MDDpyChartDB : public RTEDGE::ChartDB
{
	// Constructor / Destructor
public:
	MDDpyChartDB( const char * );

	// Operations
public:
	PyObject *PyGetTickers();
	PyObject *PySnap( const char *, const char *, int, size_t );

	// Helpers
private:
	PyObject *_cpp2py( RTEDGE::CDBData &, size_t );

};  // class MDDpyChartDB

#endif // __MDDPY_CHART_H
