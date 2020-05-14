/******************************************************************************
*
*  GLedgDTD.cpp
*     rtEdgeCache DTD
*
*  REVISION HISTORY:
*     21 JAN 2002 jcs  Created.
*     18 SEP 2013 jcs  libmddWire; RTL
*     12 NOV 2014 jcs  GLedgDTD.cpp
*     12 SEP 2015 jcs  Build 10: _mdd_xxx; MDDWIRE_PRIVATE
*      5 MAR 2015 jcs  Build 10b:_pXmlMsg[xm_query]
*     16 APR 2016 jcs  Build 11: More _mdd_xxx in libmddWire.h
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#include <libmddWire.h>
#include <GLedgDTD.h>

using namespace MDDWIRE_PRIVATE;

// DTD

char  _mdd_FS[]       = { FS, '\0' };
bool  _mdd_bSpaces    = false;
char *_mdd_pVal       = "v";
char *_mdd_pReqSiz    = "NumReq";
char *_mdd_pBndWid    = "Bandwidth";
char *_mdd_pNagle     = "Nagle";
char *_mdd_pPriority  = "Priority";
char *_mdd_pInsID     = "ID";
char *_mdd_pInsAck    = "ACK";
char *_mdd_pTplNum    = "TPL";
char *_mdd_pFid       = "FID";
char *_mdd_pSep       = ",";
char *_mdd_pDown      = "DOWN";
char *_mdd_pGblSts    = "__GLOBAL__";
char *_mdd_pItem      = "Item";
char *_mdd_pSvcSep    = ";";
char *_mdd_pSvcUpSep  = ":";
char *_mdd_pAttrAgg   = "ABSE";
char *_mdd_pAttrDec   = "Decimal";
char *_mdd_pAttrFIDs  = "FIDs";
char *_mdd_pAttrFldNm = "FieldName";
char *_mdd_pAttrPeer  = "PEER";
char *_mdd_pAttrPID   = "PID";
char *_mdd_pAttrPrty  = "Priority";
char *_mdd_pAttrRTL   = "RTL";
char *_mdd_pAttrTime  = "Time";
char *_mdd_pAttrTpl   = "Template";

char *_mdd_pXmlMsg[] = { "Undefined",
                         "MNT",
                         "IMG",
                         "UPD",
                         "STS",
                         "DRP",
                         "OPN",
                         "CLS",
                         "CTL",
                         "Ping",
                         "INSERT",
                         "INSACK",
                         "QUERY"
                       };

void MDDWIRE_PRIVATE::breakpoint() { ; }
