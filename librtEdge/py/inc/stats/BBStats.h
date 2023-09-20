/******************************************************************************
*
*  BBStats.h
*     bbPortal3 Stats (from libmdsnkbb3.h)
*
*  REVISION HISTORY:
*      1 FEB 2023 jcs  Created.
*     20 SEP 2023 jcs  MDDirect.py
*
*  (c) 1994-2023, Gatea, Ltd.
*******************************************************************************/
#ifndef __BB_STATS_H
#define __BB_STATS_H
#include <libmdEnum.h>

///////////////////////
// BB3 Usage Stats
///////////////////////
class BBAPIStat
{
public:
   struct timeval _tm;
   int            _num;

}; // class BBAPIStat

class BBAPIDailyStats
{
public:
   long _tMidNt;
   /*
    * Requests
    */
   BBAPIStat _Subscribe;        // blpapi_Session_subscribe()
   BBAPIStat _SubscribeElem;    // blpapi_SubscriptionList_add() - subscribe
   BBAPIStat _Unsubscribe;      // blpapi_Session_unsubscribe()
   BBAPIStat _UnsubscribeElem;  // blpapi_SubscriptionList_add() - unsubscribe
   BBAPIStat _RefRequest;       // blpapi_Session_sendRequest()
   BBAPIStat _RefRequestElem;   // blpapi_Service_createRequest() - sendRequest
   /*
    * Responses
    */
   BBAPIStat _RTResponse;      // BLPAPI_EVENTTYPE_SUBSCRIPTION_DATA
   BBAPIStat _RTResponseElem;  // BLPAPI_EVENTTYPE_SUBSCRIPTION_DATA - Num field
   BBAPIStat _RefResponse;     // BLPAPI_EVENTTYPE_RESPONSE
   BBAPIStat _RefResponseElem; // BLPAPI_EVENTTYPE_RESPONSE - Num Field

}; // class BBAPIDailyStats

///////////////////////
// BB3 Bridge Stats
///////////////////////

#define BB3_MAX_SNK    64

class GLbb3Stats : public GLmdStatsHdr
{
public:
   GLmdSinkStats   _snk;
   GLmdSinkStats   _src;
   BBAPIDailyStats _daily;
   int             _nSnk;
   GLmdSinkStats   xt_snk[BB3_MAX_SNK];

}; // class GLbb3Stats

#endif // __BB_STATS_H
