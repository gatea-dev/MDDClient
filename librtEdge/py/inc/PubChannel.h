/******************************************************************************
*
*  PubChannel.h
*     MD-Direct Publication Channel
*
*  REVISION HISTORY:
*     15 MAY 2025 jcs  Created.
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#ifndef __MDDPY_PUBCHAN_H
#define __MDDPY_PUBCHAN_H
#include <MDDirect.h>
#include <set>

using namespace MDDPY;

/////////////////////////////////////////
// Edge3 Publication Channel
/////////////////////////////////////////
class MDDpyPubChan : public RTEDGE::PubChannel
{
private:
	EventPump     _pmp;
	string        _host;
	string        _svc;
	StrIntMap     _byName;
	IntStrMap     _byOid;
	mddFieldList  _fl;
	int           _bdsStreamID;
	RTEDGE::Mutex _mtx;
	Strings       _strs;

	// Constructor / Destructor
public:
	MDDpyPubChan( const char *, const char * );
	~MDDpyPubChan();

	// Operations
public:
	PyObject *Read( double );
	int       pyPublish( const char *, int, PyObject * );

	// RTEDGE::PubChannel Interface
protected:
	virtual void            OnConnect( const char *, bool );
	virtual void            OnPubOpen( const char *, void * );
	virtual void            OnPubClose( const char * );
	virtual void            OnOpenBDS( const char *, void * );
	virtual void            OnCloseBDS( const char * );
	virtual void            OnOverflow();
	virtual void            OnIdle();
	virtual RTEDGE::Update *CreateUpdate();

	// Helpers
private:
	int       _py2mdd( PyObject* );
	PyObject *_Get1stUpd();

};  // class MDDpyPubChan

#endif // __MDDPY_PUBCHAN_H
