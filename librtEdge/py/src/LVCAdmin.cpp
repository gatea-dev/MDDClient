/******************************************************************************
*
*  LVCAdmin.cpp
*     MD-Direct LVCAdmin Channel
*
*  REVISION HISTORY:
*     19 JUL 2022 jcs  Created.
*      4 SEP 2023 jcs  Build 10: Named Schema; DelTicker()
*      5 FEB 2025 jcs  Build 14: _pmp / Read()
*
*  (c) 1994-2025, Gatea, Ltd.
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
   RTEDGE::LVCAdmin( pAdmin ),
   _pmp( *this )
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

PyObject *MDDpyLVCAdmin::Read( double dWait )
{
   PyObject *rtn;

   // First check if there is data

   if ( (rtn=_Get1stUpd()) != Py_None )
      return rtn;

   // Safe to wait for it

   _pmp.Wait( dWait );
   return _Get1stUpd();
}



///////////////////////////////////
// RTEDGE::LVCAdmin Notifications
///////////////////////////////////
void MDDpyLVCAdmin::OnConnect( const char *pConn, bool bUP )
{
   const char *ty  = bUP ?  "UP  " : "DOWN";
   char        buf[K];
   Update      u = _INIT_MDDPY_UPD;

   // Update queue

   sprintf( buf, "%s|%s", ty, pConn );
   u._mt  = EVT_CONN;
   u._msg = new string( buf );
   _pmp.Add( u );
}

bool MDDpyLVCAdmin::OnAdminACK( bool bAdd, const char *svc, const char *tkr )
{
   const char *ty = bAdd ? "ADD" : "DEL";
   char        buf[K];
   Update      u = _INIT_MDDPY_UPD;

   // Update queue

   sprintf( buf, "ACK|%s|%s|%s", ty, svc, tkr );
   u._mt  = EVT_SVC;
   u._msg = new string( buf );
   _pmp.Add( u );
   return true;
}

bool MDDpyLVCAdmin::OnAdminNAK( bool bAdd, const char *svc, const char *tkr )
{
   const char *ty = bAdd ? "ADD" : "DEL";
   char        buf[K];
   Update      u = _INIT_MDDPY_UPD;

   // Update queue

   sprintf( buf, "NAK|%s|%s|%s", ty, svc, tkr );
   u._mt  = EVT_SVC;
   u._msg = new string( buf );
   _pmp.Add( u );
   return true;
}


///////////////////////////////
// Helpers
///////////////////////////////
PyObject *MDDpyLVCAdmin::_Get1stUpd()
{
   PyObject   *rtn, *pym, *pyd;
   string     *s;
   Update      upd;
   const char *ps;

   // Pull 1st update off _pmp queue

   if ( !_pmp.GetOneUpd( upd ) )
      return Py_None;

   /*
    * ( EVT_CONN, UP|Message )
    * ( EVT_SVC, ACK|ADD|service|ticker )
    * ( EVT_SVC, ACK|DEL|service|ticker )
    * ( EVT_SVC, NAK|ADD|service|ticker )
    * ( EVT_SVC, NAK|DEL|service|ticker )
    */
   switch( upd._mt ) {
      case EVT_CONN:
      case EVT_SVC:
         s   = upd._msg;
         ps  = s->data();
         pyd = PyString_FromString( ps );
         delete s;
         break;
      default:
         pyd = PyString_FromString( "Unknown msg type" );
         break;
   }
   pym = PyInt_FromLong( upd._mt );
   rtn = ::mdd_PyList_Pack2( pym, pyd );
   return rtn;
}
