/******************************************************************************
*
*  LVC.h
*     MD-Direct LVC Channel
*
*  REVISION HISTORY:
*      3 APR 2019 jcs  Created.
*     19 NOV 2020 jcs  Build  2: PyGetTickers()
*
*  (c) 1994-2020 Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_LVC_H
#define __MDDPY_LVC_H
#include <MDDirect.h>

/////////////////////////////////////////
// Last Value Cache (LVC) 
/////////////////////////////////////////
class MDDpyLVC : public RTEDGE::LVC
{
	// Constructor / Destructor
public:
	MDDpyLVC( const char * );

	// Operations
public:
	PyObject *PySchema();
	PyObject *PyGetTickers();
	PyObject *PySnap( const char *, const char * );

};  // class MDDpyLVC

#endif // __MDDPY_LVC_H
