/******************************************************************************
*
*  GLrecDb.h
*     gateaRecorder tape layout
*
*  REVISION HISTORY:
*      1 MAY 2016 jcs  Created.
*     16 MAY 2016 jcs  Build  2: GLrecTapeHdr._curTime
*     29 MAY 2016 jcs  Build  3: GLrecTapeHdr._tCreate / _tEOD
*     10 SEP 2016 jcs  Build  4: GLrecTapeHdr._bMDDirect
*     10 DEC 2017 jcs  Build  7: _USE_UPA; _channelID; SIG_005 : No htonl()
*     20 FEB 2018 jcs  Build 10: GLrecDailyHdr
*      2 JUN 2022 jcs  Build 55: No mo _BLD
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __REC_DB_H
#define __REC_DB_H
#ifdef _USE_UPA
#include <libUPA.h>
#else
typedef enum {
   upaFld_undef      =  0,
   upaFld_int        =  3,
   upaFld_uint       =  4,
   upaFld_float      =  5,
   upaFld_double     =  6,
   upaFld_real       =  8,
   upaFld_date       =  9,
   upaFld_time       = 10,
   upaFld_dateTime   = 11,
   upaFld_qos        = 12,
   upaFld_state      = 13,
   upaFld_enum       = 14,
   upaFld_buffer     = 19
} upaFldType;
#endif // _USE_UPA

#define _MAX_NAME_LEN  256
#define _MAX_MSG_LEN   64*K

#define REC_SIG_004  "004 gateaRecorder"
#define REC_SIG_005  "005 gateaRecorder"

#define _VAL_MAX          0
#define _VAL_TOTAL        _VAL_MAX   + 1
#define _VAL_NUM          ( _VAL_TOTAL + 1 )

////////////////////////
//
// Indexed Tape Hdr
//
////////////////////////

/**
 * \brief Field Definition (match to RsslDictionaryEntry)
 *
 * No support for enumerated types or DDE acroynms
 */
class GLrecDictEntry
{
public:
   char       _acronym[_MAX_NAME_LEN];
   int        _fid;
   upaFldType _type;
};

//////////////////////////
// Daily Header
//////////////////////////
/**
 * \brief Daily Sentinel
 */
class Sentinel
{
public:
   char _version[80];
   char _signature[16];
   long _tStart;

}; // class Sentinel

/**
 * \brief Daily index
 */
class GLrecDailyIdx
{
public:
/*
   u_int64_t    _locIdx[_numSecIdx];
 */

}; // class GLrecDailyIdx

// Update (Stream) Stats - UPA, Tape, Record

class GLrecUpdStats
{
public:
   struct timeval _tMsg;
   u_int64_t      _nMsg;
   u_int64_t      _nByte;
};

// Channel (Stream) Stats

class GLrecChanStats : public GLrecUpdStats
{
public:
   char      _dstConn[K-(3*_VAL_NUM*8)];
   u_int64_t _dUPA[_VAL_NUM];
   u_int64_t _dLockIns[_VAL_NUM];
   u_int64_t _dIns[_VAL_NUM];
/*
 * Publication Channel Only in libUPA ???
 *
   int  _maxBuf;
   int  _guaranBuf;
   int  _bufSiz;
 */
};


/////////////////////
// File Header
/////////////////////
class GLrecTapeHdr
{
public:
   /*
    * Tape Stats
    */
   u_int64_t      _fileSiz;
   u_int64_t      _hdrSiz;
   Bool           _bBigEndian;
   Bool           _bMDDirect;
   int            _sizeofLong; // 8 == 64-bits; 4 = 32-bits
   u_int64_t      _curLoc;
   u_int64_t      _winPtr;
   u_int64_t      _winSiz;
   time_t         _tCreate;
   time_t         _tEOD;
   struct timeval _curTime;
   double         _dCpu;
   int            _numDictEntry;
   int            _secPerIdxT;
   int            _numSecIdxT;  // Tape : SECPERDAY / _secPerIdxT
   int            _secPerIdxR;
   int            _numSecIdxR;  // Record : SECPERDAY / _secPerIdxR
   struct timeval _curIdxTm;
   int            _curIdx;
   int            _maxRec;
   int            _numRec;
   int            _minFreeMb;
   Sentinel       _sentinel;
   int            _pid;
   int            _pageSize;
   GLrecUpdStats  _tapeStats;
   GLrecChanStats _chanStats;
/*
   GLrecDictEntry _dict[_numDictEntry];
   GLrecDailyIdx  _idx;
   GLrecTapeRec   _recs[_maxRec];
 */

}; // class GLrecTapeHdr


// TREP Record

// sizeof( GLrecTapeRec ) = 592

class GLrecTapeRec : public GLrecUpdStats
{
public:
   int            _dbIdx;
   int            _StreamID;
   char           _svc[_MAX_NAME_LEN];
   char           _tkr[_MAX_NAME_LEN-4];
   int            _channelID;
   u_int64_t      _loc;
   u_int64_t      _locImg;
   struct timeval _curIdxTm;
   int            _curIdx;
   int            _tDelay;
/*
   GLrecDailyIdx  _idx;
 */

}; // class GLrecTapeRec

// GLrecTapeRec + daily index

class TapeRecAndIdx
{
public:
   GLrecTapeRec *_rec;
   u_int64_t    *_idx;

}; // class TapeRec


////////////////////////
//
// Message on Tape
//
////////////////////////

#define _MAX_LAST4    0x80000000L

#pragma pack (push, 1)              // Byte Alignment

class GLrecTapeMsg
{
public:
   int      _msgLen;
   int      _dbIdx;
   int      _tv_sec;
   int      _tv_usec;
   int      _nUpd;
   u_char   _nFldMod;   // Number of fields modified
   u_char   _bLast4;    // _last is 4 bytes
   u_char   _last[8];   // 4 bytes if _bLast4
// char   _data[_msgLen-sizeof(GLrecMsgHdr)];
};

#pragma pack (pop)              // Byte Alignment

#endif // __REC_DB_H
