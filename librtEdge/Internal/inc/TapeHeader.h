/******************************************************************************
*
*  TapeHeader.h
*     Platform-agnostic tape header
*
*  REVISION HISTORY:
*     16 JAN 2023 jcs  Created
*      2 MAR 2023 jcs  hdr()
*     12 JAN 2024 jcs  librtEdge
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __TAPE_HDR_H
#define __TAPE_HDR_H
#include <EDG_GLmmap.h>
#include <HdrConversion.h>

typedef enum {
   tape_native  = 0,
   tape_win64   = 1,  // WIN64 on Linux64
   tape_linux64 = 2   // Linux64 on WIN64
} TapeType;

namespace RTEDGE_PRIVATE 
{

/////////////////////////////////////////
// Platform-agnostic File Header
/////////////////////////////////////////
class TapeHeader : public string
{
protected:
	GLmmap                   &_vw;
	TapeType                  _type;
	GLrecTapeHdr             *_hdr;
	Win64Hdr::GLrecTapeHdr   *_hdr4; // WIN64 on Linux64
	Linux64Hdr::GLrecTapeHdr *_hdr8; // Linux64 on WIN64
	int                       _sizeofLong;

	// Constructor / Destructor
public:
	TapeHeader( GLmmap & );

	// Access / Operations

	const char *pType();
	char       *hdr();
	TapeType    type();
	Bool        Map();

	// Tape Sizing

	u_int64_t _DbHdrSize( int, int, int );
	u_int64_t _hSz();
	u_int64_t _sSz();
	u_int64_t _RecSiz();
	u_int64_t _DailyIdxSize();

	// Platform-agnostic Accessor
public:
	Bool            IsValid();
	u_int64_t      &_fileSiz();
	u_int64_t      &_hdrSiz();
	u_int64_t      &_winSiz();
	Bool            _bMDDirect();
	Bool            _bWrite();
	time_t          _tCreate();
	time_t          _tEOD();
	u_int64_t      &_curLoc();
	struct timeval  _curTime();
	double          _dCpu(); 
	int             _numDictEntry(); 
	int             _secPerIdxT(); 
	int             _numSecIdxT(); 
	int             _secPerIdxR(); 
	int             _numSecIdxR(); 
	int            &_numRec(); 
	int             _maxRec(); 
	long            _tStart(); 
	const char     *_version(); 
	const char     *_signature(); 
	GLrecUpdStats   _tapeStats();
	GLrecChanStats  _chanStats();
	Sentinel        _sentinel();

}; // class TapeHeader


/////////////////////////////////////////
// Platform-agnostic Record Header
/////////////////////////////////////////
class TapeRecHdr
{
protected:
	TapeHeader               &_tapeHdr;
	TapeType                  _type;
	GLrecTapeRec             *_hdr;
	Win64Hdr::GLrecTapeRec   *_hdr4; // WIN64 on Linux64
	Linux64Hdr::GLrecTapeRec *_hdr8; // Linux64 on WIN64

	// Constructor / Destructor
public:
	TapeRecHdr( TapeHeader &, char * );

	// Platform-agnostic Accessor
public:
	struct timeval _tMsg();
	u_int64_t      _nMsg();
	u_int64_t      _nByte();
	int            _dbIdx();
	int            _StreamID();
	char          *_svc(); 
	char          *_tkr(); 
	int            _channelID();
	u_int64_t      _loc();
	u_int64_t      _locImg();
	u_int64_t      _rSz();

	// Class-wide
public:
	static u_int64_t _RecSiz( TapeHeader & );

}; // class TapeRecHdr

} // namespace RTEDGE_PRIVATE 

#endif // __TAPE_HDR_H
