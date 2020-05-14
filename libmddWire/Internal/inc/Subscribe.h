/******************************************************************************
*
*  Subscribe.h
*     MD-Direct subscription channel data
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     17 JAN 2015 jcs  Build  9: _MF_str2dbl()
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_GLxml.h; MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __MDW_SUBSCRIBE_H
#define __MDW_SUBSCRIBE_H
#include <MDW_Internal.h>
#include <MDW_GLxml.h>

#define MAX_FLD 128*K


////////////////////////
// Forward declarations
////////////////////////

/////////////////////////////////////////
//  MD-Direct Consumed Data
/////////////////////////////////////////
namespace MDDWIRE_PRIVATE
{
class Subscribe : public Data
{
protected:
	string _gblSts;
	string _tkrSts;

	// Constructor / Destructor
public:
	Subscribe();
	~Subscribe();

	// API - Data Interface

	virtual int Parse( mddMsgBuf, mddWireMsg & );
	virtual int ParseHdr( mddMsgBuf, mddMsgHdr & );

	// MarketFeed
protected:
	int    _MF_Parse( mddMsgBuf, mddWireMsg & );
	int    _MF_ParseHdr( mddMsgBuf, mddMsgHdr & );
	void   _MF_Time2Native( mddField &, bool bDate=false );
	double _MF_str2dbl( mddBuf );
	double _MF_atofn( mddBuf );
	int    _MF_atoin( mddBuf );

	// Binary
protected:
	int  _Binary_Parse( mddMsgBuf, mddWireMsg & );
	int  _Binary_ParseHdr( mddMsgBuf, mddMsgHdr & );
	int  _Binary_ParseHdr( mddMsgBuf, mddBinHdr & );
};

} // namespace MDDWIRE_PRIVATE

#endif // __MDW_SUBSCRIBE_H
