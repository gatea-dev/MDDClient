/******************************************************************************
*
*  LVCAdmin.cpp
*     MD-Direct LVCAdmin Channel
*
*  REVISION HISTORY:
*     19 JUL 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>


////////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      M D D p y L V C A d m i n
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
MDDpyLVCAdmin::MDDpyLVCAdmin( const char *pAdmin ) :
   RTEDGE::LVCAdmin( pAdmin )
{
}


///////////////////////////////
// Operations
///////////////////////////////
void MDDpyLVCAdmin::PyAddBDS( const char *svc, const char *bds )
{
   AddBDS( svc, bds );
}

void MDDpyLVCAdmin::PyAddTicker( const char *svc, const char *tkr )
{
   AddTicker( svc, tkr );
}

void MDDpyLVCAdmin::PyAddTickers( const char *svc, const char **tkrs )
{
   AddTickers( svc, tkrs );
}

void MDDpyLVCAdmin::PyRefreshTickers( const char *svc, const char **tkrs )
{
   RefreshTickers( svc, tkrs );
}

void MDDpyLVCAdmin::PyRefreshAll()
{
   RefreshAll();
}
