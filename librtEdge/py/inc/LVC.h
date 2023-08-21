/******************************************************************************
*
*  LVC.h
*     MD-Direct LVC Channel
*
*  REVISION HISTORY:
*      3 APR 2019 jcs  Created.
*     19 NOV 2020 jcs  Build  2: PyGetTickers()
*     21 AUG 2023 jcs  Build 10: PySnapAll()
*
*  (c) 1994-2023, Gatea, Ltd.
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
	PyObject *PySnapAll();

	// Helpers
private:
	PyObject *_rt2py( RTEDGE::Message & );

};  // class MDDpyLVC

#endif // __MDDPY_LVC_H
