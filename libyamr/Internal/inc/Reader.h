/******************************************************************************
*
*  Reader.h
*     yamrCache tape Reader
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_READER_H
#define __YAMR_READER_H
#include <Internal.h>

namespace YAMR_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class GLmmap;


/////////////////////////////////////////
// yamRecorder Tape Reader
/////////////////////////////////////////
class Reader : public GLmmap
{
protected:
	yamrTape_Context _cxt;
	string           _tape;
	GLyamrTapeHdr    _hdr;
	u_int64_t        _pos;
	bool             _bOK;

	// Constructor / Destructor
public:
	Reader( const char *, yamrTape_Context );
	~Reader();

	// Access / Operations

	GLyamrTapeHdr   &hdr();
	yamrTape_Context cxt();
	const char      *tape();
	u_int64_t       *idb();
	u_int64_t        HdrSz();
	u_int64_t        off();
	u_int64_t        NumLeftMap();
	u_int64_t        NumLeftTape();
	bool             ReadWindow( u_int64_t winSz=0 );
	u_int64_t        Rewind( u_int64_t pos=0 );
	u_int64_t        RewindTo( u_int64_t );
	bool             Read( yamrMsg &, bool bViewOnly=false );

}; // class Reader

} // namespace YAMR_PRIVATE

#endif // __YAMR_READER_H
