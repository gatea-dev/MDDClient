/******************************************************************************
*
*  WireMold64.h
*     MD-Direct input multicast channel(s)
*
*  REVISION HISTORY:
*     25 MAY 2017 jcs  Created (from RecordUDP).
*
*  (c) 1994-2017, Gatea Ltd.
******************************************************************************/
#ifndef __WIRE64_H
#define __WIRE64_H

#pragma pack (push, 1)              // Byte Alignment

#define _FID_STREAMID  1
#define _FID_TICKER    3
#define _MTU           2*K  // 1500 bytes will do ...

/////////////////////
// WireMold64 - Wire
/////////////////////
class Mold64PktHdr
{
public:
   char      _session[10];
   u_int64_t _wire_seqNum;
   u_short   _wire_numMsg;
};

class Mold64Pkt : public Mold64PktHdr
{
public:
   u_char _data[_MTU];
};

class Mold64MsgHdr
{
public:
   u_short   _len;
};

#pragma pack(pop)

#endif // __WIRE64_H
