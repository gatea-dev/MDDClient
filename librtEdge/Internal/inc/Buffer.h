/******************************************************************************
*
*  Buffer.h
*
*  REVISION HISTORY:
*      5 JAN 2024 jcs  Created (from Socket.h)
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_BUFFER_H
#define __EDGLIB_BUFFER_H
#include <librtEdge.h>

// EDG_Internal.h

#ifdef WIN32
#define READ(fd,b,sz)  ::recv( fd, (b), (sz), 0 )
#define WRITE(fd,b,sz) ::send( fd, (b), (sz), 0 )
#include <winsock.h>
#else
#define READ(fd,b,sz)  ::read( fd, (b), (sz) )
#define WRITE(fd,b,sz) ::write( fd, (b), (sz) )
#include <sys/socket.h>
#endif // WIN32

#define b_gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )
#define b_gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define b_InRange( a,b,c )     ( ((a)<=(b)) && ((b)<=(c)) )
#define b_WithinRange( a,b,c ) ( b_gmin( b_gmax((a),(b)), (c)) )
#define _MAX_BUF_SIZ   10*K*K // 10 MB

namespace RTEDGE_PRIVATE
{

///////////////////
// Buffer
///////////////////
class Buffer
{
protected:
	char *_bp;
	char *_cp;
	int   _qAlloc;
	int   _qMax;
	bool  _bConnectionless;

	// Constructor / Destructor
public:
	Buffer( int maxSiz=_MAX_BUF_SIZ );
	virtual ~Buffer();

	// Common Operations
public:
	char *bp()     { return _bp; }
	int  &nAlloc() { return _qAlloc; }
	int  &maxSiz() { return _qMax; }
	void  SetConnectionless() { _bConnectionless = true; }
	void  Init( int );
	bool  Grow( int );
	rtBUF buf();
	int   nLeft();
	void  Set( int );
	int   ReadIn( int, int );

	// Instance-Specific Operations

	virtual char *cp()  { return _cp; }
	virtual void  Reset();
	virtual int   bufSz();
	virtual int   WriteOut( int, int, int );
	virtual bool  Push( char *, int );
	virtual void  Move( int, int );

}; // class Buffer

///////////////////
// Curcular Buffer
///////////////////
class CircularBuffer : public Buffer
{
protected:
	int _beg;
	int _end;
	int _qSz;

	// Constructor / Destructor
public:
	CircularBuffer( int maxSiz=_MAX_BUF_SIZ );

	// Instance-Specific Operations

	virtual void  Reset();
	virtual int   bufSz();
	virtual int   WriteOut( int, int, int );
	virtual bool  Push( char *, int );
	virtual void  Move( int, int ) { ; }

	// Helpers
private:
	int _memcpy( char *, int );

}; // class CircularBuffer

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_BUFFER_H
