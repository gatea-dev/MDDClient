/******************************************************************************
*
*  libmdEnum.h
*     Enumerated types for market data library.
*
*  REVISION HISTORY:
*     15 FEB 1999 jcs  Created.
*     . . .
*     20 SEP 2023 jcs  MDDirect.py
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __LIBMDENUM_H
#define __LIBMDENUM_H


/////////////////////////////////
// 64-bit
/////////////////////////////////
typedef struct timeval GLtimeval;


/////////////////////////////////////////
// Sink channel stats
/////////////////////////////////////////
class GLmdSinkStats
{
public:
   u_int64_t _nMsg;      // Num msgs read
   u_int64_t _nByte;     // Num bytes read
   GLtimeval _lastMsg;   // Timestamp of last msg
   int       _nOpen;     // Num item open reqs sent
   int       _nClose;    // Num item close reqs sent
   int       _nImg;      // Num image msgs received
   int       _nUpd;      // Num update msgs received
   int       _nDead;     // Num inactive received
   int       _nInsAck;
   int       _nInsNak;
   time_t    _lastConn;
   time_t    _lastDisco;
   int       _nConn;
   long      _iVal[20];
   double    _dVal[20];
   char      _objname[MAXLEN];
   char      _dstConn[MAXLEN];
   char      _bUp;
   char      _pad[7];
};

/////////////////////////////////
// GLmdStatsHdr
/////////////////////////////////
class GLmdStatsHdr
{
public:
   u_int     _version;
   u_int     _fileSiz;
   GLtimeval _tStart;
   char      _exe[K];
   char      _build[K];
   int       _nSnk;
   int       _nSrc;
};

////////////////////////
// GLmdSink / GLmdService
////////////////////////
typedef enum {
   up,
   down
} GLmdSinkState;

////////////////////////
// Field types
////////////////////////
typedef enum {
   fldDb_undef,
   fldDb_gfif,
   fldDb_appxa,
   fldDb_fix
} GLmdFldDbType;

typedef enum {
   fldtype_string,
   fldtype_binary,
   fldtype_int,
   fldtype_double,
   fldtype_date,
   fldtype_time,
   fldtype_timeSec,
   fldtype_error,
   fldtype_enum,
   fldtype_undef
} GLmdFieldType;
 
#endif  // __LIBMDENUM_H
