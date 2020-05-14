/******************************************************************************
*
*  BinProtocol.h
*     Gatea binary channel protocol
*
*  REVISION HISTORY:
*      1 FEB 2006 jcs  Created.
*     16 SEP 2013 jcs  BinProtocol
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015, Gatea Ltd.
******************************************************************************/
#ifndef __MD_BIN_PROTO_H
#define __MD_BIN_PROTO_H
#include <MDW_Internal.h>

#define PROTO_VER1   1

/////////////////////////
// Message Header - Wire
/////////////////////////
struct wmddBinHdr
{
   u_char _len[4];    // NOT packable
   u_char _tag[4];    // Packable
   u_char _dt;        // mddDataType
   u_char _mt;        // Msg Type
   u_char _protocol;
   u_char _reserved;
   u_char _time[4];   // Packable
   u_char _RTL[4];    // Packable
};


/////////////////////////
// Message Header - Struct 
/////////////////////////
struct mddBinHdr
{
   u_int       _hdrLen;
   u_int       _len;
   u_int       _tag;
   mddDataType _dt;
   mddMsgType  _mt;
   u_char      _protocol;
   u_char      _reserved;
   u_int       _time;      // 100 micros since midnight
   u_int       _RTL;
   bool        _bPack;
};

#endif // __MD_BIN_PROTO_H
