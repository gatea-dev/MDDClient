/******************************************************************************
*
*  GLedgDTD.h
*     rtEdgeCache DTD
*
*  REVISION HISTORY:
*     21 JAN 2002 jcs  Created.
*     18 SEP 2013 jcs  libmddWire; RTL
*     12 NOV 2014 jcs  GLedgDTD.cpp
*     12 SEP 2015 jcs  Build 10: _mdd_xxx
*     16 APR 2016 jcs  Build 11: More _mdd_xxx in libmddWire.h
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#ifndef __MDD_DTD_H
#define __MDD_DTD_H
#include <sys/types.h>

// XML Channel Message Types

typedef enum {
   xm_undef  =   0,
   xm_mount  =   1,
   xm_image  =   2,
   xm_update =   3,
   xm_status =   4,
   xm_drop   =   5,
   xm_open   =   6,
   xm_close  =   7,
   xm_ioctl  =   8,
   xm_ping   =   9,
   xm_insert =  10,
   xm_insAck =  11,
   xm_query  =  12
} GLxmlMsgType;

// DTD

extern char  _mdd_FS[];
extern bool  _mdd_bSpaces;
extern char *_mdd_pVal;
extern char *_mdd_pReqSiz;
extern char *_mdd_pBndWid;
extern char *_mdd_pNagle;
extern char *_mdd_pPriority;
extern char *_mdd_pInsID;
extern char *_mdd_pInsAck;
extern char *_mdd_pTplNum;
extern char *_mdd_pFid;
extern char *_mdd_pSep;
extern char *_mdd_pDown;
extern char *_mdd_pGblSts;
extern char *_mdd_pItem;
extern char *_mdd_pSvcSep;
extern char *_mdd_pSvcUpSep;
extern char *_mdd_pAttrAgg;
extern char *_mdd_pAttrDec;
extern char *_mdd_pAttrFIDs;
extern char *_mdd_pAttrFldNm;
extern char *_mdd_pAttrPeer;
extern char *_mdd_pAttrPID;
extern char *_mdd_pAttrPrty;
extern char *_mdd_pAttrRTL;
extern char *_mdd_pAttrTime;
extern char *_mdd_pAttrTpl;
/*
 * end 03-09-20
 */
extern char *_mdd_pXmlMsg[];

namespace MDDWIRE_PRIVATE
{
extern void breakpoint();
} // namespace MDDWIRE_PRIVATE

#endif // __MDD_DTD_H
