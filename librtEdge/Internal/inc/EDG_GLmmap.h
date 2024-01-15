/******************************************************************************
*
*  EDG_GLmmap.h
*     Memory-mapped file class.
*
*  REVISION HISTORY:
*     20 MAR 1998 jcs  Created.
*      2 SEP 2010 jcs  librtEdge
*     23 APR 2012 jcs  Build 19: namespace LIBRTEDGE
*     10 JUL 2013 jcs  Build 27: u_int64_t in librtEdge.h
*     10 SEP 2014 jcs  Build 28: Shutdown(); RTEDGE_PRIVATE
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*      3 JUL 2016 jcs  Build 33: GLasciiFile
*     12 OCT 2017 jcs  Build 36: u_int64_t siz(); _w32XxSz / _w32FileMapping()
*      7 NOV 2017 jcs  Build 38: pFile()
*     12 JAN 2024 jcs  Build 67: TapeHeader.h
*
*  (c) 1994-2024, Gatea Ltd. 
******************************************************************************/
#ifndef __EDGLIB_GLMMAP_H
#define __EDGLIB_GLMMAP_H
#include <libmddWire.h>  // u_int64_t; <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>

// Gatea library macros ...

#ifdef WIN32
#define MMAP_FD              HANDLE
#define PROT_READ            0x01             // pages can be read
#define PROT_WRITE           0x02             // pages can be written
#define PROT_EXEC            0x04             // pages can be executed
#define MAP_SHARED           1                // share changes
#define MAP_PRIVATE          2                // changes are private
#define MAP_TYPE             0xf              // mask for share type
#define MAP_FAILED           ((void *) -1)
#define FPHANDLE             HANDLE
#define GLfileno(c)          (c)
#else
#include <sys/mman.h>
#include <unistd.h>
#define MMAP_FD               int
#define FPHANDLE              FILE *
#define GLfileno(c)          fileno(c)
#if !defined(DWORD)
#define DWORD                 u_long
#endif // !defined(DWORD)
#define INVALID_HANDLE_VALUE (int)-1
extern int getpagesize();
#endif // WIN32

#if !defined(u_int64_t)
#define u_int64_t  u_int64_t
#endif // !defined(u_int64_t)

using namespace std;

#ifdef _LARGEFILE64_SOURCE
#define STAT                  stat64
#define FOPEN                 fopen64
#define FSEEK                 fseeko64
#define FSTAT                 fstat64
#define FTELL                 ftello64
#else
#define STAT                  stat
#define FOPEN                 fopen
#define FSEEK                 fseek
#define FSTAT                 fstat
#define FTELL                 ftell
#endif // _LARGEFILE64_SOURCE

#define SIZE_T                size_t
#if !defined(Bool)
#define Bool                  bool
#define True                  true
#define False                 false
#define K                     1024
#endif // !defined(Bool)

///////////////////////////////////////////
// Memory mapped file
///////////////////////////////////////////
namespace RTEDGE_PRIVATE
{
class GLmmap
{
protected:
	string    _file;
	MMAP_FD   _fd;	// File descriptor (if we opened it)
	MMAP_FD   _hMap;	// File mapping (WIN32 only)
	Bool      _bOurFd;
	char     *_base;	// Location in virtual memory; Page-aligned
	char     *_pa;	// ... what the user asked for
	u_int64_t _len;
	u_int64_t _off;
	u_int64_t _mLen;	// Mapped '_len' (Debugging)
	u_int64_t _mOff;	// Mapped '_off' (Debugging)
	int       _prot;
	int       _flags;
	int       _errno;
	DWORD     _w32LoSz;
	DWORD     _w32HiSz;

	// Constructor / Destructor
public:
	GLmmap( MMAP_FD, u_int64_t,	// Map opened file
		int   prot  = PROT_READ | PROT_WRITE,
		int   flags = MAP_SHARED,
		char *addr  = (char *)0,
		u_int64_t off   = 0 );
	GLmmap( char *, u_int64_t,	// Open and map file
		int   prot  = PROT_READ | PROT_WRITE,
		int   flags = MAP_SHARED,
		char     *addr  = (char *)0,
		u_int64_t off   = 0 );
	GLmmap( char *,		// Open existing one for reading
		char  *addr  = (char *)0,
		u_int64_t  off = 0,
		u_int64_t  len = 0 );
	~GLmmap();
	void Shutdown();

	// Access

	const char *filename();
	char       *data();
	MMAP_FD     fd();
	MMAP_FD     hMap();
	Bool        isOurFile();
	Bool        isValid();
	u_int64_t   siz();
	u_int64_t   offset();
	int         error();

	// Operations

	char *map( u_int64_t off=0, u_int64_t len=0, char *addr=0 );
	void  unmap();
private:
	Bool  _w32FileMapping( u_int64_t );

	// Platform-Independent File Operations
public:
	static FPHANDLE  Open( const char *, const char * );
	static void      Close( FPHANDLE );
	static int       Read( void *, int, FPHANDLE );
	static int       Write( void *, int, FPHANDLE );
	static int       Grow( void *, int, FPHANDLE );
	static u_int64_t Seek( FPHANDLE, u_int64_t, Bool bCur=False );
	static u_int64_t SeekEnd( FPHANDLE );
	static u_int64_t Tell( FPHANDLE );
	static u_int64_t Stat( FPHANDLE );
	static void      Flush( FPHANDLE );
	static int       GetPageSize();
};

/////////////////////////////////////////
// Memory-mapped file view
/////////////////////////////////////////
class GLmmapView : public GLmmap
{
	// Constructor / Destructor
public:
	GLmmapView( FPHANDLE, u_int64_t, u_int64_t, Bool );
	~GLmmapView();
};

/////////////////////////////////////////
// Flat ASCII file
/////////////////////////////////////////
typedef struct {
 char *_data;
} GLasciiBuf;

class GLasciiFile
{
private:
	GLmmap     *_mm;
	Bool        _bCopy;
	u_int64_t   _rp;
	GLasciiBuf *_buf;
	char       *_bp;
	char       *_cp;
	int         _max;
	int         _num;

	// Constructor / Destructor
public:
	GLasciiFile( char *pFile=(char *)0, Bool bCopy=True );
	~GLasciiFile();

	// Access

	GLasciiBuf &operator[]( int );
	GLmmap     &mm();
	u_int64_t   rp();
	int         num();

	// Operations

	char *gets( char *, int );
	void  rpSet( u_int64_t );
	int   GetAll();
	int   GetAll( char * );
	void  Insert( char * );
};

} // namespace RTEDGE_PRIVATE

#endif 	// __EDGLIB_GLMMAP_H
