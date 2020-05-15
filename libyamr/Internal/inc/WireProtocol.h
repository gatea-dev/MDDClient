/******************************************************************************
*
*  WireProtocol.h
*     yamRecorder Wire Protocol
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created.
*
*  (c) 1994-2019, Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_PROTOCOL_H
#define __YAMR_PROTOCOL_H

/////////////////////////////////////////
// YAMR Protocol 1.0
/////////////////////////////////////////
#pragma pack (push, 1)              // Byte Alignment

#define _YAMR_VERSION         1
#define _YAMR_MAGIC      0xbeef
#define _YAMR_1BYTE_LEN  0x0001
#define _YAMR_2BYTE_LEN  0x0002
#define _YAMR_4BYTE_LEN  0x0004
#define _YAMR_LEN_MASK   ( _YAMR_1BYTE_LEN | _YAMR_2BYTE_LEN | _YAMR_4BYTE_LEN )
#define _YAMR_MAX_MASK   0x0000ffffffffffff // Also max SeqNum = 255 PB (peta)

class yamrBaseHdr
{
public:
   u_int64_t _SeqNum;       // Session SeqNum; Top 16-bits = UniqueID
   u_int16_t _Magic;        // _YAMR_MAGIC
   u_int8_t  _Version;      // _YAMR_VERSION
   u_int8_t  _Flags;        // _YAMR_2BYTE_LEN, etc.
   u_int16_t _MsgProtocol;  // < _YAMR_PROTO_MAX
   u_int16_t _WireProtocol; // < _YAMR_PROTO_MAX

}; // class yamrBaseHdr

class yamrHdr8 : public yamrBaseHdr
{
public:
   u_int8_t _MsgLen;

}; // class yamrHdr16

class yamrHdr16 : public yamrBaseHdr
{
public:
   u_int16_t _MsgLen;

}; // class yamrHdr16

class yamrHdr32 : public yamrBaseHdr
{
public:
   u_int32_t _MsgLen;

}; // class yamrHdr32

#pragma pack(pop)


/******************************************************
*
*  Tape Layout
*
 *****************************************************/

#define _MAX_NAME_LEN  256
#define _MAX_MSG_LEN   64*K
#define _MAX_YAMR_CLI  K   // < 1024 channels

#define YAMR_SIG_001  "001 yamr"

// Tape Stats

class GLyamrTapeStats
{
public:
   u_int64_t _tMsg;
   u_int64_t _nMsg;
   u_int64_t _nByte;

}; // class GLyamrTapeStats

class GLyamrCliStats : public GLyamrTapeStats
{
public:
   char _dstConn[64];

}; // class GLyamrCliStats


/////////////////////
// File Header
/////////////////////
class GLyamrTapeHdr
{
public:
   /*
    * Tape Stats
    */
   u_int64_t       _fileSiz;
   u_int64_t       _hdrSiz;
   char            _version[80];
   char            _signature[16];
   u_int64_t       _curLoc;
   u_int64_t       _winPtr;
   u_int64_t       _winSiz;
   time_t          _tCreate;
   u_int64_t       _curTime;
   double          _dCpu;
   int             _MaxClient;
   int             _secPerIdxT;
   int             _numSecIdxT;  // Tape : SECPERDAY / _secPerIdxT
   u_int64_t       _curIdxTm;
   int             _curIdx;
   int             _minFreeMb;
   int             _pid;
   int             _pageSize;
   GLyamrTapeStats _tapeStats;
   GLyamrCliStats  _totStats;
   GLyamrCliStats  _cliStats[_MAX_YAMR_CLI];
/*
   u_int64_t       _idx[_numSecIdxT];
 */

}; // class GLyamrTapeHdr


////////////////////////
//
// Message on Tape
//
////////////////////////

#pragma pack (push, 1)              // Byte Alignment

class yamrTapeMsg
{
public:
   u_int32_t _MsgLen;
   u_int16_t _SessionID;    // Client -supplied ID : Must be unique on _Host
   u_int16_t _MsgProtocol;
   u_int16_t _WireProtocol;
   u_int32_t _Host;
   u_int64_t _Timestamp;
   u_int64_t _SeqNum;
// char      _data[_msgLen-sizeof(yamrMsgHdr)];
};

#pragma pack (pop)              // Byte Alignment

#endif // __YAMR_PROTOCOL_H
