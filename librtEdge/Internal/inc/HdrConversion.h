/******************************************************************************
*
*  HdrConversion.h
*     Platform-agnostic tape header conversion
*
*  REVISION HISTORY:
*     16 JAN 2023 jcs  Created.
*     25 JAN 2023 jcs  Linux64Hdr.Long = 8 bytes - DUH!!
*     12 JAN 2024 jcs  librtEdge
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __HDR_CONVERSION_H
#define __HDR_CONVERSION_H
#include <EDG_GLrecDb.h>

namespace RTEDGE_PRIVATE
{

////////////////////////////////////////////////////////
//
//             W I N 6 4   T a p e
//
////////////////////////////////////////////////////////

namespace Win64Hdr
{
typedef int Long; 

class Timeval
{
public:
   Win64Hdr::Long tv_sec;
   Win64Hdr::Long tv_usec;

}; // class  Timeval

class Sentinel
{
public:
   char           _version[79];
   Bool           _bWrite;        // >= REG_SIG_005
   char           _signature[16];
   Win64Hdr::Long _tStart;

}; // class Sentinel

class GLrecUpdStats
{
public:
   Win64Hdr::Timeval _tMsg;
   u_int64_t         _nMsg;
   u_int64_t         _nByte;

}; // class GLrecUpdStats

class GLrecChanStats : public GLrecUpdStats
{
public:
   char      _dstConn[K-(3*_VAL_NUM*8)];
   u_int64_t _dUPA[_VAL_NUM];
   u_int64_t _dLockIns[_VAL_NUM];
   u_int64_t _dIns[_VAL_NUM];

}; // class GLrecChanStats

class GLrecTapeHdr
{
public:
   /*
    * Tape Stats
    */
   u_int64_t                _fileSiz;
   u_int64_t                _hdrSiz;
   Bool                     _bBigEndian;
   Bool                     _bMDDirect;
   int                      _sizeofLong; // 8 == 64-bits; 4 = 32-bits
   u_int64_t                _curLoc;
   u_int64_t                _winPtr;
   u_int64_t                _winSiz;
   time_t                   _tCreate;
   time_t                   _tEOD;
   Win64Hdr::Timeval        _curTime;
   double                   _dCpu;
   int                      _numDictEntry;
   int                      _secPerIdxT;
   int                      _numSecIdxT;  // Tape : SECPERDAY / _secPerIdxT
   int                      _secPerIdxR;
   int                      _numSecIdxR;  // Record : SECPERDAY / _secPerIdxR
   Win64Hdr::Timeval        _curIdxTm;
   int                      _curIdx;
   int                      _maxRec;
   int                      _numRec;
   int                      _minFreeMb;
   Win64Hdr::Sentinel       _sentinel;
   int                      _pid;
   int                      _pageSize;
   Win64Hdr::GLrecUpdStats  _tapeStats;
   Win64Hdr::GLrecChanStats _chanStats;
/*
   GLrecDictEntry           _dict[_numDictEntry];
   GLrecDailyIdx            _idx;
   Win64Hdr::GLrecTapeRec   _recs[_maxRec];
 */

}; // class GLrecTapeHdr

class GLrecTapeRec : public GLrecUpdStats
{
public:
   int               _dbIdx;
   int               _StreamID;
   char              _svc[_MAX_NAME_LEN];
   char              _tkr[_MAX_NAME_LEN-4];
   int               _channelID;
   u_int64_t         _loc;
   u_int64_t         _locImg;
   Win64Hdr::Timeval _curIdxTm;
   int               _curIdx;
   int               _tDelay;
/*
   GLrecDailyIdx     _idx;
 */

}; // class GLrecTapeRec

} // namespace Win64Hdr


////////////////////////////////////////////////////////
//
//             L i n u x 6 4   T a p e
//
////////////////////////////////////////////////////////

namespace Linux64Hdr
{
typedef int64_t Long; 

class Timeval
{
public:
   Linux64Hdr::Long tv_sec;
   Linux64Hdr::Long tv_usec;

}; // class  Timeval

class Sentinel
{
public:
   char             _version[79];
   Bool             _bWrite;        // >= REG_SIG_005
   char             _signature[16];
   Linux64Hdr::Long _tStart;

}; // class Sentinel

class GLrecUpdStats
{
public:
   Linux64Hdr::Timeval _tMsg;
   u_int64_t           _nMsg;
   u_int64_t           _nByte;

}; // class GLrecUpdStats

class GLrecChanStats : public GLrecUpdStats
{
public:
   char      _dstConn[K-(3*_VAL_NUM*8)];
   u_int64_t _dUPA[_VAL_NUM];
   u_int64_t _dLockIns[_VAL_NUM];
   u_int64_t _dIns[_VAL_NUM];

}; // class GLrecChanStats

class GLrecTapeHdr
{
public:
   /*
    * Tape Stats
    */
   u_int64_t                  _fileSiz;
   u_int64_t                  _hdrSiz;
   Bool                       _bBigEndian;
   Bool                       _bMDDirect;
   int                        _sizeofLong; // 8 == 64-bits; 4 = 32-bits
   u_int64_t                  _curLoc;
   u_int64_t                  _winPtr;
   u_int64_t                  _winSiz;
   time_t                     _tCreate;
   time_t                     _tEOD;
   Linux64Hdr::Timeval        _curTime;
   double                     _dCpu;
   int                        _numDictEntry;
   int                        _secPerIdxT;
   int                        _numSecIdxT;  // Tape : SECPERDAY / _secPerIdxT
   int                        _secPerIdxR;
   int                        _numSecIdxR;  // Record : SECPERDAY / _secPerIdxR
   Linux64Hdr::Timeval        _curIdxTm;
   int                        _curIdx;
   int                        _maxRec;
   int                        _numRec;
   int                        _minFreeMb;
   Linux64Hdr::Sentinel       _sentinel;
   int                        _pid;
   int                        _pageSize;
   Linux64Hdr::GLrecUpdStats  _tapeStats;
   Linux64Hdr::GLrecChanStats _chanStats;
/*
   GLrecDictEntry             _dict[_numDictEntry];
   GLrecDailyIdx              _idx;
   Linux64Hdr::GLrecTapeRec   _recs[_maxRec];
 */

}; // class GLrecTapeHdr

class GLrecTapeRec : public GLrecUpdStats
{
public:
   int                 _dbIdx;
   int                 _StreamID;
   char                _svc[_MAX_NAME_LEN];
   char                _tkr[_MAX_NAME_LEN-4];
   int                 _channelID;
   u_int64_t           _loc;
   u_int64_t           _locImg;
   Linux64Hdr::Timeval _curIdxTm;
   int                 _curIdx;
   int                 _tDelay;
/*
   GLrecDailyIdx     _idx;
 */

}; // class GLrecTapeRec

} // namespace Linux64Hdr

} // namespace RTEDGE_PRIVATE

#endif // __HDR_CONVERSION_H
