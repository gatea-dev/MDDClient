/******************************************************************************
*
*  LVCAdmin.cpp
*     MD-Direct LVCAdmin Channel
*
*  REVISION HISTORY:
*     19 JUL 2022 jcs  Created.
*      4 SEP 2023 jcs  Build 10: Named Schema; DelTicker()
*
*  (c) 1994-2023, Gatea, Ltd.
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

void MDDpyLVCAdmin::PyAddTicker( const char *svc, 
                                 const char *tkr,
                                 const char *schema )
{
   AddTicker( svc, tkr, schema );
}

void MDDpyLVCAdmin::PyAddTickers( const char  *svc, 
                                  const char **tkrs,
                                  const char  *schema )
{
   AddTickers( svc, tkrs, schema );
}

void MDDpyLVCAdmin::PyDelTicker( const char *svc, 
                                 const char *tkr,
                                 const char *schema )
{
   DelTicker( svc, tkr, schema );
}

void MDDpyLVCAdmin::PyDelTickers( const char  *svc, 
                                  const char **tkrs,
                                  const char  *schema )
{
   DelTickers( svc, tkrs, schema );
}


void MDDpyLVCAdmin::PyRefreshTickers( const char *svc, const char **tkrs )
{
   RefreshTickers( svc, tkrs );
}

void MDDpyLVCAdmin::PyRefreshAll()
{
   RefreshAll();
}
