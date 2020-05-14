/******************************************************************************
*
*  Publish.h
*     MD-Direct publication channel data
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created (from librtEdge).
*     10 JUN 2014 jcs  Build  7: _estFldSz
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 10a:MDW_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#ifndef __MDD_PUBLISH_H
#define __MDD_PUBLISH_H
#include <MDW_Internal.h>

#define PubFields GLvector<mddField>

/////////////////////////////////////////
// MD-Direct Published Data
/////////////////////////////////////////
namespace MDDWIRE_PRIVATE
{
class Publish : public Data
{
protected:
	PubFields _upds;
	int       _estFldSz;
	char     *_xTrans;

	// Constructor / Destructor
public:
	Publish();
	~Publish();

	// Access

	int nFld();

	// API

	int    AddFieldList( mddFieldList );
	mddBuf BuildMsg( mddMsgHdr, mddProtocol, mddBldBuf &, bool bFldNm=false );
	mddBuf BuildRawMsg( mddMsgHdr, mddBuf, mddBldBuf & );
	mddBuf SetHdrTag( u_int, mddBuf & );
	mddBuf ConvertFieldList( mddWireMsg, mddProtocol, mddBldBuf &, bool );

	// API - Data Interface

	virtual int Parse( mddMsgBuf, mddWireMsg & );
	virtual int ParseHdr( mddMsgBuf, mddMsgHdr & );

	// XML
protected:
	mddBuf _XML_Build( mddMsgHdr, mddBuf, mddBldBuf &, bool );
	int    _XML_BuildFld( char *, mddField, bool );

	// MarketFeed
protected:
	mddBuf _MF_Build( mddMsgHdr, mddBuf, mddBldBuf & );
	int    _MF_Header( char *, mddMsgHdr );
	int    _MF_BuildFld( char *, mddField );

	// Binary
protected:
	mddBuf _Binary_Build( mddMsgHdr, mddBuf, mddBldBuf & );
	mddBuf _Binary_SetHdrTag( u_int, mddBuf & );

	// Helpers
private:
	int   _ASCII_BuildFld( char *, mddField );
	char *_InitFieldListBuf( mddBldBuf & );
};

} // namespace MDDWIRE_PRIVATE

#endif // __MDD_PUBLISH_H
