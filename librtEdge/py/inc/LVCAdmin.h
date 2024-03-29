/******************************************************************************
*
*  LVCAdmin.h
*     MD-Direct LVCAdmin Channel
*
*  REVISION HISTORY:
*     19 JUL 2022 jcs  Created.
*      4 SEP 2023 jcs  Build 10: Named Schema; DelTicker()
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_LVCAdmin_H
#define __MDDPY_LVCAdmin_H
#include <MDDirect.h>

/////////////////////////////////////////
// Last Value Cache (LVCAdmin) 
/////////////////////////////////////////
class MDDpyLVCAdmin : public RTEDGE::LVCAdmin
{
	// Constructor / Destructor
public:
	MDDpyLVCAdmin( const char * );

	// Operations
public:
	void PyAddBDS( const char *, const char * );
	void PyAddTicker( const char *, const char *, const char * );
	void PyAddTickers( const char *, const char **, const char * );
	void PyDelTicker( const char *, const char *, const char * );
	void PyDelTickers( const char *, const char **, const char * );
	void PyRefreshTickers( const char *, const char ** );
	void PyRefreshAll();

};  // class MDDpyLVCAdmin

#endif // __MDDPY_LVCAdmin_H
