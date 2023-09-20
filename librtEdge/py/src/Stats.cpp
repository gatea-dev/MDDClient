/******************************************************************************
*
*  Stats.cpp
*     MD-Direct run-time Stats File
*
*  REVISION HISTORY:
*     20 SEP 2023 jcs  Created.
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>


// DTD, of sorts

static const char *_bb_appName     = "bbPortal3";
static size_t      _bbSz           = sizeof( GLbb3Stats );

static const char *_dawg_bbSub     = "BBSubscribe";
static const char *_dawg_bbUnSub   = "BBUnSubscribe";
static const char *_dawg_bbSubElem = "BBSubscribeElem";
static const char *_dawg_bbReq     = "BBRequest";
static const char *_dawg_bbReqElem = "BBRequestElem";
static const char *_dawg_bbRT      = "BBRealTime";
static const char *_dawg_bbRTElem  = "BBRealTimeElem";
static const char *_dawg_bbRsp     = "BBRefResponse";
static const char *_dawg_bbRspElem = "BBRefResponseElem";

////////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      M D D p y S t a t s
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
MDDpyStats::MDDpyStats( const char *file ) :
   _map( MapFile( file ) )
{
}

MDDpyStats::~MDDpyStats()
{
   UnmapFile( _map );
}


///////////////////////////////
// Operations
///////////////////////////////
PyObject *MDDpyStats::PyBBDailyStats()
{
   GLbb3Stats      *bb  = (GLbb3Stats *)_map._data;
   BBAPIDailyStats &bd = bb->_daily;
   PyObject        *rc;
   int              i;

   // Pre-condition(s)

   rc = Py_None;
   if ( !bb )
      return rc;
   if ( _map._dLen != _bbSz )
      return rc;

   // Map : Return [ [ 'Stat1', val1 ], [ 'Stat2', val2 ], ... ]

	if ( ::strcmp( bb->_exe, _bb_appName ) )
	   return rc;
	rc = ::PyList_New( 9 );
	i  = 0;
   _SetBBStat( rc, i++, _dawg_bbSub,     bd._Subscribe );
   _SetBBStat( rc, i++, _dawg_bbUnSub,   bd._Unsubscribe );
   _SetBBStat( rc, i++, _dawg_bbSubElem, bd._SubscribeElem );
   _SetBBStat( rc, i++, _dawg_bbReq,     bd._RefRequest );
   _SetBBStat( rc, i++, _dawg_bbReqElem, bd._RefRequestElem );
   _SetBBStat( rc, i++, _dawg_bbRT,      bd._RTResponse );
   _SetBBStat( rc, i++, _dawg_bbRTElem,  bd._RTResponseElem );
   _SetBBStat( rc, i++, _dawg_bbRsp,     bd._RefResponse );
   _SetBBStat( rc, i++, _dawg_bbRspElem, bd._RefResponseElem );
   return rc;
}


///////////////////////////////
// (private) Helpers
///////////////////////////////
void MDDpyStats::_SetBBStat( PyObject *lst,
                             int       num,
                             const char *bbName, 
                             BBAPIStat &st )
{
   PyObject *kv;

	kv = ::mdd_PyList_Pack2( PyString_FromString( bbName ),
                            PyInt_FromLong( st._num ) );
   ::PyList_SetItem( lst, num, kv );
}
