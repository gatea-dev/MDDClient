/******************************************************************************
*
*  GLmmap.h
*     Memory-mapped file class.
*
*  REVISION HISTORY:
*     20 MAR 1998 jcs  Created.
*      2 SEP 2010 jcs  libyamr
*     23 APR 2012 jcs  Build 19: namespace LIBRTEDGE
*     10 JUL 2013 jcs  Build 27: OFF_T in libyamr.h
*     10 SEP 2014 jcs  Build 28: Shutdown(); YAMR_PRIVATE
*     12 OCT 2015 jcs  Build 32: Internal.h
*      3 JUL 2016 jcs  Build 33: GLasciiFile
*     12 OCT 2017 jcs  Build 36: OFF_T siz(); _w32XxSz / _w32FileMapping()
*      7 NOV 2017 jcs  Build 38: pFile()
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_GLMMAP_H
#define __YAMR_GLMMAP_H
#include <Internal.h>
#include <sys/stat.h>

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

#if !defined(OFF_T)
#define OFF_T  u_int64_t
#endif // !defined(OFF_T)


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
#define Bool                  bool
#define True                  true
#define False                 false

///////////////////////////////////////////
// Memory mapped file
///////////////////////////////////////////
namespace YAMR_PRIVATE
{
class GLmmap
{
protected:
	string  _file;
	MMAP_FD _fd;	// File descriptor (if we opened it)
	MMAP_FD _hMap;	// File mapping (WIN32 only)
	Bool    _bOurFd;
	char   *_base;	// Location in virtual memory; Page-aligned
	char   *_pa;	// ... what the user asked for
	OFF_T   _len;
	OFF_T   _off;
	OFF_T   _mLen;	// Mapped '_len' (Debugging)
	OFF_T   _mOff;	// Mapped '_off' (Debugging)
	int     _prot;
	int     _flags;
	int     _errno;
	DWORD   _w32LoSz;
	DWORD   _w32HiSz;

	// Constructor / Destructor
public:
	GLmmap( MMAP_FD, OFF_T,	// Map opened file
		int   prot  = PROT_READ | PROT_WRITE,
		int   flags = MAP_SHARED,
		char *addr  = (char *)0,
		OFF_T off   = 0 );
	GLmmap( char *, OFF_T,	// Open and map file
		int   prot  = PROT_READ | PROT_WRITE,
		int   flags = MAP_SHARED,
		char *addr  = (char *)0,
		OFF_T off   = 0 );
	GLmmap( char *,		// Open existing one for reading
		char  *addr  = (char *)0,
		OFF_T  off   = 0,
		OFF_T  len   = 0 );
	~GLmmap();
	void Shutdown();

	// Access

	const char *filename();
	char       *data();
	MMAP_FD     fd();
	MMAP_FD     hMap();
	Bool        isOurFile();
	Bool        isValid();
	OFF_T       siz();
	OFF_T       offset();
	int         error();

	// Operations

	char *map( OFF_T off=0, OFF_T len=0, char *addr=0 );
	void  unmap();
private:
	Bool  _w32FileMapping( OFF_T );

	// Platform-Independent File Operations
public:
	static FPHANDLE Open( const char *, const char * );
	static void     Close( FPHANDLE );
	static int      Read( void *, int, FPHANDLE );
	static int      Write( void *, int, FPHANDLE );
	static int      Grow( void *, int, FPHANDLE );
	static OFF_T    Seek( FPHANDLE, OFF_T, Bool bCur=False );
	static OFF_T    SeekEnd( FPHANDLE );
	static OFF_T    Tell( FPHANDLE );
	static OFF_T    Stat( FPHANDLE );
	static void     Flush( FPHANDLE );
	static int      GetPageSize();
};

/////////////////////////////////////////
// Memory-mapped file view
/////////////////////////////////////////
class GLmmapView : public GLmmap
{
	// Constructor / Destructor
public:
	GLmmapView( FPHANDLE, OFF_T, OFF_T, Bool );
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
	OFF_T       _rp;
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
	OFF_T       rp();
	int         num();

	// Operations

	char *gets( char *, int );
	void  rpSet( OFF_T );
	int   GetAll();
	int   GetAll( char * );
	void  Insert( char * );
};

} // namespace YAMR_PRIVATE

#endif 	// __YAMR_GLMMAP_H
