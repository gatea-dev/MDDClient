/*****************************************************************************
*
*  EDG_GLmmap.cpp
*     Memory-mapped file class.
*
*  REVISION HISTORY:
*     20 MAR 1998 jcs  Created.
*      2 SEP 2010 jcs  librtEdge
*     22 APR 2012 jcs  Build 19: LIBRTEDGE
*     10 SEP 2014 jcs  Build 28: Shutdown(); RTEDGE_PRIVATE
*     20 MAR 2016 jcs  Build 32: EDG_Internal.h
*      3 JUL 2016 jcs  Build 33: GLasciiFile
*     12 OCT 2017 jcs  Build 36: u_int64_t siz(); _w32XxSz / _w32FileMapping()
*      7 NOV 2017 jcs  Build 38: pFile()
*     14 JAN 2024 jcs  Build 67: No mo OFF_T
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;

#define _mm_r "rm"  // "r"

#ifdef WIN32
static LPSECURITY_ATTRIBUTES _sa = NULL;
#endif // WIN32

#define chop( b )                           \
   do {                                     \
      char *bp = (b);                       \
                                            \
      bp += ( strlen(b) - 1 );              \
      if ( ( *bp==0x0d ) || ( *bp==0x0a ) ) \
         *bp = '\0';                        \
   } while( 0 )

static int GetErrno()
{
#ifdef WIN32
   return ::GetLastError();
#else
   return errno;
#endif // WIN32
}


//////////////////////////////////////////////////////////////////////////////
//
//                    c l a s s      G L m m a p
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLmmap::GLmmap( MMAP_FD fd,
                u_int64_t   len,
                int     prot, 
                int     flags, 
                char   *addr, 
                u_int64_t   off ) :
   _file(),
   _fd( fd ),
   _hMap( (MMAP_FD)0 ),
   _bOurFd( False ),
   _base( (char *)MAP_FAILED ),
   _pa( (char *)0 ),
   _len( len ),
   _off( off ),
   _mLen( 0 ),
   _mOff( 0 ),
   _prot( prot ),
   _flags( flags ),
   _errno( 0 ),
   _w32LoSz( 0 ),
   _w32HiSz( 0 )
{
   // Map to existing file, if non-zero len

   GetPageSize();
   if ( _w32FileMapping( 0 ) && ( len > 0 ) )
      map( off, len, addr );
}

GLmmap::GLmmap( char  *fname, 
                u_int64_t  len, 
                int    prot, 
                int    flags, 
                char  *addr, 
                u_int64_t  off ) :
   _file( fname ),
   _fd( INVALID_HANDLE_VALUE ),
   _hMap( (MMAP_FD)0 ),
   _bOurFd( True ),
   _base( (char *)MAP_FAILED ),
   _pa( (char *)0 ),
   _len( len ),
   _off( off ),
   _mLen( 0 ),
   _mOff( 0 ),
   _prot( prot ),
   _flags( flags ),
   _errno( 0 ),
   _w32LoSz( 0 ),
   _w32HiSz( 0 )
{
   FPHANDLE    fp;
   const char *openFlags;
   Bool        bRd = ( ( _prot & PROT_READ  ) == PROT_READ  );
   Bool        bWr = ( ( _prot & PROT_WRITE ) == PROT_WRITE );
   u_int64_t       i, nWr;

   // Object ID

   GetPageSize();

   // 1) Open

   openFlags = _mm_r;
   if ( bRd && bWr )
//      openFlags = "r+";
      openFlags = "w+";
   else if ( bRd )
      openFlags = _mm_r;
   else if ( bWr )
      openFlags = "a";

#ifdef WIN32
   _fd = Open( fname, openFlags );
   fp  = _fd;
   if ( !_w32FileMapping( _len ) ) {
#else
   if ( !(fp=Open( fname, openFlags )) ) {
      _errno = errno;
#endif // WIN32
      return;
   }

   // 2) Zero out

   int   pgSz = GetPageSize();
   char *buf  = new char[pgSz];

   ::memset( buf, 0, pgSz );
   for ( i=0; i<_len; ) {
      nWr = gmin( (u_int64_t)pgSz, _len-i );
      if ( Write( buf, nWr, fp ) != (int)nWr )
         return;
      i += nWr;
   }
   delete[] buf;
#ifndef WIN32
   _fd = fileno( fp );
   ::rewind( fp );
#endif // WIN32

   // 2) Map

   map( off, len, addr );
}

GLmmap::GLmmap( char *fname, char *addr, u_int64_t off, u_int64_t len ) :
   _file( fname ),
   _fd( INVALID_HANDLE_VALUE ),
   _hMap( (MMAP_FD)0 ),
   _bOurFd( True ),
   _base( (char *)MAP_FAILED ),
   _pa( (char *)0 ),
   _len( len ),
   _off( off ),
   _mLen( 0 ),
   _mOff( 0 ),
   _prot( PROT_READ ),
   _flags( MAP_SHARED ),
   _errno( 0 ),
   _w32LoSz( 0 ),
   _w32HiSz( 0 )
{
   FPHANDLE fp;
   u_int64_t    stSz;

   // Object ID

   GetPageSize();

   // 1) Get file size

   if ( !(fp=Open( fname, _mm_r )) ) {
      _errno = GetErrno();
      return;
   }
   stSz = Stat( fp );
   Close( fp );
   _len  = gmin( _len, stSz );

   // 2) Open existing for reading

#ifdef WIN32
   _fd = Open( fname, _mm_r );
   if ( !_w32FileMapping( _len ) ) {
#else
   if ( (fp=Open( fname, _mm_r )) )
      _fd = fileno( fp );
   else {
      _errno = errno;
#endif // WIN32
      return;
   }

   // 3) Map

   map( off, _len, addr );
}

GLmmap::~GLmmap()
{
   Shutdown();
}

void GLmmap::Shutdown()
{
   // Un-map

   unmap();
#ifdef WIN32
   if ( _hMap )
      ::CloseHandle( _hMap );
   _hMap = (MMAP_FD)0;
#endif // WIN32

   // Close file, if ours

   if ( isOurFile() ) {
      if ( _fd != INVALID_HANDLE_VALUE )
#ifdef WIN32
         ::CloseHandle( _fd );
#else
         ::close( _fd );
#endif // WIN32
   }
   _fd   = INVALID_HANDLE_VALUE;
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
const char *GLmmap::filename()
{
   return _file.data();
}

char *GLmmap::data()
{
   return _pa;
}

MMAP_FD GLmmap::fd()
{
   return _fd;
}

MMAP_FD GLmmap::hMap()
{
   return _fd;
}

Bool GLmmap::isOurFile()
{
   return _bOurFd;
}

Bool GLmmap::isValid()
{
   return( _base != (char *)MAP_FAILED );
}

u_int64_t GLmmap::siz()
{
   return _len;
}

u_int64_t GLmmap::offset()
{
   return _off;
}

int GLmmap::error()
{
   return _errno;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
char *GLmmap::map( u_int64_t off, u_int64_t len, char *addr )
{
   int nPad;
   int pgSz = GetPageSize();

   // Unmap / Map

   unmap();
   if ( !_w32FileMapping( 0 ) )
      return data();
   _len  = len;
   _off  = off;
   nPad  = ( off % pgSz );
   _mOff = off - nPad;
   _mLen = _len + nPad;
#ifdef WIN32
   DWORD dwFlags;
   DWORD dwHi, dwLo;
   u_int64_t oh;

   oh      = ( _mOff >> 32 );
   dwHi    = (DWORD)( oh    & 0x00000000ffffffffL );
   dwLo    = (DWORD)( _mOff & 0x00000000ffffffffL );
   dwFlags = ( _prot & PROT_WRITE ) ? FILE_MAP_WRITE : FILE_MAP_READ;
   _base   = (char *)::MapViewOfFileEx( _hMap,
                                        dwFlags,
                                        dwHi, // 0,
                                        dwLo, // _mOff,
                                        _mLen,
                                        addr );
   _base = !_base ? (char *)MAP_FAILED : _base;
#else
#if defined(_LARGEFILE64_SOURCE)
   _base = (char *)::mmap64( addr, _mLen, _prot, _flags, _fd, _mOff );
#else
   _base = (char *)::mmap( addr, _mLen, _prot, _flags, _fd, _mOff );
#endif // defined(_LARGEFILE64_SOURCE)
#endif // WIN32
   _pa   = _base;
   if ( isValid() )
      _pa += nPad;
   else
#ifdef WIN32
      _errno = ::GetLastError();
#else
      _errno = errno;
#endif // WIN32
   return data();
}

void GLmmap::unmap()
{
   int nPad;

   // Pre-condition

   if ( !isValid() )
      return;

   // Unmap

   nPad = _pa - _base;
#ifdef WIN32
   if ( _base != (char *)MAP_FAILED )
      ::UnmapViewOfFile( _base );
#else
   if ( ::munmap( (caddr_t)_base, _len+nPad ) == -1 )
      _errno = errno;
#endif // WIN32
   _base = (char *)MAP_FAILED;
   _pa   = (char *)0;
   _len  = 0;
   _off  = 0;
}

Bool GLmmap::_w32FileMapping( u_int64_t len )
{
#ifdef WIN32
   DWORD iLo, iHi;
   Bool  bWr  = ( ( _prot & PROT_WRITE ) == PROT_WRITE );
   DWORD flPr = bWr ? PAGE_READWRITE : PAGE_READONLY;

   // 1) Valid File?

   if ( _fd == INVALID_HANDLE_VALUE )
      return False;

   // 2) Get Size

   if ( len ) {
      iHi = 0;
      iLo = len;
   }
   else if ( (iLo=::GetFileSize( _fd, &iHi )) == INVALID_FILE_SIZE )
      return False;

   // 3) OK, Re-map if 1st time or different sized files

   if ( !_hMap || ( iLo != _w32LoSz ) || ( iHi != _w32HiSz ) ) {
      if ( _hMap )
         ::CloseHandle( _hMap );
        _hMap = ::CreateFileMapping( _fd, _sa, flPr, iHi, iLo, NULL );
      _w32LoSz = iLo;
      _w32HiSz = iHi;
   }
   if ( !_hMap )
      _errno = GetErrno();
   return( _hMap != (MMAP_FD)0 );
#else
   return True;
#endif // WIN32
}


////////////////////////////////////////////
// Platform-Independent File Operations
////////////////////////////////////////////
FPHANDLE GLmmap::Open( const char *pName, const char *flags )
{
   FPHANDLE  rtn;

#ifdef WIN32
   DWORD     dwAccess, dwShare, dwCreate;

   // 1) Map from 'flags'

   dwAccess = 0;
   dwShare  = 0;
   dwCreate = 0;
   if ( !::strcmp( flags, "w" ) ) {
      dwAccess = GENERIC_WRITE;
      dwShare  = FILE_SHARE_READ;
      dwCreate = CREATE_ALWAYS;
   }
   else if ( !::strcmp( flags, "r" ) ) {
      dwAccess = GENERIC_READ;
      dwShare  = FILE_SHARE_READ;
      dwCreate = OPEN_EXISTING;
   }
   else if ( !::strcmp( flags, _mm_r ) ) {  // mmap()
      dwAccess = GENERIC_READ;
      dwShare  = FILE_SHARE_READ | FILE_SHARE_WRITE;
      dwCreate = OPEN_EXISTING;
   }
   else if ( !::strcmp( flags, "w+" ) ) {
      dwAccess = GENERIC_WRITE | GENERIC_READ;
      dwShare  = FILE_SHARE_READ;
      dwCreate = CREATE_ALWAYS;
   }
   else if ( !::strcmp( flags, "r+" ) || !::strcmp( flags, "a" )) {
      dwAccess = GENERIC_WRITE | GENERIC_READ;
      dwShare  = FILE_SHARE_READ;
      dwCreate = OPEN_ALWAYS;
   }

   // 2) OK to open

   rtn = ::CreateFile( pName,
                       dwAccess,
                       dwShare,
                       (LPSECURITY_ATTRIBUTES)NULL,
                       dwCreate,
                       FILE_ATTRIBUTE_NORMAL,
                       (HANDLE)NULL );   // No attribute template
   return ( rtn == INVALID_HANDLE_VALUE ) ? (FPHANDLE)0 : rtn;
#endif // WIN32
   flags = !::strcmp( flags, _mm_r ) ? "r" : flags;
   rtn = ::FOPEN( pName, flags );
   return rtn;
}

void GLmmap::Close( FPHANDLE hFile )
{
   if ( hFile )
#ifdef WIN32
      ::CloseHandle( hFile );
#else
      ::fclose( hFile );
#endif // WIN32
}

int GLmmap::Read( void *p, int siz, FPHANDLE hFile )
{
#ifdef WIN32
   DWORD dRtn, dErr;

   dErr = 0;
   if ( !::ReadFile( hFile, p, (DWORD)siz, &dRtn, NULL ) )
      dErr = ::GetLastError();
   return dRtn;
#else
   return ::fread( p, siz, 1, hFile );
#endif // WIN32

}

int GLmmap::Write( void *p, int siz, FPHANDLE hFile )
{
#ifdef WIN32
   DWORD dRtn, dErr;

   dErr = 0;
   if ( !::WriteFile( hFile, p, (DWORD)siz, &dRtn, NULL ) )
      dErr = ::GetLastError();
   return dRtn;
#else
   return ::fwrite( p, 1, siz, hFile );
#endif // WIN32
}

int GLmmap::Grow( void *p, int siz, FPHANDLE hFile )
{
   int rtn;

   rtn = SeekEnd( hFile );
   rtn = Write( p, siz, hFile );
   Flush( hFile );
   return rtn;
}

u_int64_t GLmmap::Seek( FPHANDLE hFile, u_int64_t off, Bool bCur )
{
   u_int64_t rtn;

#ifdef WIN32
   LONG  dLo, dHi;
   DWORD dRtn;

   rtn  = ( off >> 32 );
   dHi  = (LONG)( rtn & 0x00000000ffffffffL );
   dLo  = (LONG)( off & 0x00000000ffffffffL );
   dRtn = ::SetFilePointer( hFile,
                            dLo,
                            &dHi,
                            bCur ? FILE_CURRENT : FILE_BEGIN );
   rtn  = ( (u_int64_t)dHi << 32 ) + dRtn;
#else
   rtn  = ::FSEEK( hFile,
                   off,
                   bCur ? SEEK_CUR : SEEK_SET );
#endif // WIN32
   return rtn;
}

u_int64_t GLmmap::SeekEnd( FPHANDLE hFile )
{
   u_int64_t rtn;

#ifdef WIN32
   LONG dLo, dHi;

   dHi = 0;
   dLo = ::SetFilePointer( hFile, 0, &dHi, FILE_END );
   rtn = ( (u_int64_t)dHi << 32 ) + dLo;
#else
   rtn  = ::FSEEK( hFile, (u_int64_t)0, SEEK_END );
#endif // WIN32
   return rtn;
}

u_int64_t GLmmap::Tell( FPHANDLE hFile )
{
   u_int64_t rtn;

#ifdef WIN32
   rtn = Seek( hFile, 0, True ); // bCur
#else
   rtn = ::FTELL( hFile );
#endif // WIN32
   return rtn;
}

u_int64_t GLmmap::Stat( FPHANDLE hFile )
{
   struct STAT st;
   u_int64_t       fSz;

   fSz = 0;
#ifdef WIN32
   DWORD       dHi, dLo;

   dLo = ::GetFileSize( hFile, &dHi );
   if ( dLo != INVALID_FILE_SIZE )
      fSz = ( (u_int64_t)dHi << 32 ) + dLo;
#else
   if ( !::FSTAT( GLfileno( hFile ), &st ) )
      fSz = st.st_size;
#endif // WIN32
   return fSz;
}

void GLmmap::Flush( FPHANDLE hFile )
{
#ifdef WIN32
   ::FlushFileBuffers( hFile );
#else
   ::fflush( hFile );
#endif // WIN32
}

int GLmmap::GetPageSize()
{
   static int _pageSiz = -1;

   if ( _pageSiz == -1 )
#ifdef WIN32
   {
      SYSTEM_INFO si;

      ::GetSystemInfo( &si );
      _pageSiz = si.dwAllocationGranularity;
   }
#else
      _pageSiz = 8*K; // ::getpagesize();
#endif // WIN32
   return _pageSiz;
}





/////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L m m a p V i e w
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLmmapView::GLmmapView( FPHANDLE fp,
                        u_int64_t    len,
                        u_int64_t    offset,
                        Bool     bWrite ) :
   GLmmap( GLfileno( fp ),
           len,
           PROT_READ | ( bWrite ? PROT_WRITE : 0 ),
           MAP_SHARED,
           (char *)0,
           offset )
{
}

GLmmapView::~GLmmapView()
{
}



/////////////////////////////////////////////////////////////////////////////
//
//               c l a s s      G L a s c i i F i l e
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
GLasciiFile::GLasciiFile( char *pFile, Bool bCopy ) :
   _mm( pFile ? new GLmmap( pFile, (char *)0, 0, INFINITEs ) : (GLmmap *)0 ),
   _bCopy( bCopy ),
   _rp( 0 ),
   _buf( (GLasciiBuf *)0 ),
   _bp( (char *)0 ),
   _cp( (char *)0 ),
   _max( 0 ),
   _num( 0 )
{
   int sz;

   if ( _mm && _bCopy && InRange( 1, (sz=_mm->siz()), INFINITEs-1 ) ) {
      _bp = new char[sz];
      _cp = _bp;
      ::memset( _bp, 0, sz );
   }
}

GLasciiFile::~GLasciiFile()
{
   char *bp;

   if ( _bp )
      delete[] _bp;
   if ( (bp=(char *)_buf) )
      delete[] bp;
   if ( _mm )
      delete _mm;
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
GLasciiBuf &GLasciiFile::operator[]( int num )
{
   GLasciiBuf *rtn;

   num = WithinRange( 0, num, _num-1 );
   rtn = _buf ? &_buf[num] : (GLasciiBuf *)0;
   return *rtn;
}

GLmmap &GLasciiFile::mm()
{
   return *_mm;
}

int GLasciiFile::num()
{
   return _num;
}

u_int64_t GLasciiFile::rp()
{
   return _rp;
}


////////////////////////////////////////////
// Operations
////////////////////////////////////////////
#define _IsAscii(c)  ( IsAscii(c) || (c) == '\t' ) // TAB

char *GLasciiFile::gets( char *buf, int max )
{
   char *cp;
   int   i;
   u_int64_t sz;

   // Pre-condition(s)

   if ( !_mm )
      return (char *)0;
   if ( (cp=_mm->data()) == (char *)MAP_FAILED )
      return (char *)0;

   // Safe to continue ...

   max--;
   sz = _mm->siz();
   for ( i=0; ( _rp<(u_int64_t)sz ) && ( i<max ) && _IsAscii( cp[_rp] ); )
      buf[i++] = cp[_rp++];
   buf[i] = '\0';
   while ( ( _rp<(u_int64_t)sz ) && !IsAscii( cp[_rp] ) )
      _rp++;
   return i ? buf : (char *)0;
}

void GLasciiFile::rpSet( u_int64_t rp )
{
   _rp = WithinRange( 0, rp, (u_int64_t)_mm->siz() );
}

int GLasciiFile::GetAll()
{
   char buf[K], *mp;

   // Pre-condition(s)

   if ( !_mm )
      return 0;
   if ( ( (mp=_mm->data()) == (char *)MAP_FAILED ) || !mp )
      return 0;
   if ( !_cp )
      return 0;

   // Safe to read

   while( gets( buf, K ) )
      Insert( buf );
   return num();
}

int GLasciiFile::GetAll( char *pf )
{
   FILE       *fp;
   char        buf[K];
   struct STAT st;
   int         sz;

   // Pre-condition(s)

   if ( _bp || !pf )
      return 0;
   if ( ::STAT( pf, &st ) )
      return 0;
   if ( !(fp=::fopen( pf,"r" )) )
      return 0;

   // Allocate

   sz  = st.st_size;
   sz += K;
   _bp = new char[sz ];
   _cp = _bp;
   ::memset( _bp, 0, sz );

   // Read

   while( ::fgets( buf, K, fp ) ) {
      chop( buf );
      Insert( buf );
   }
   ::fclose( fp );
   return num();
}

void GLasciiFile::Insert( char *buf )
{
   char      *bp, *tp;
   int        sz, s0, sc;
   static int _sb = sizeof( GLasciiBuf );

   // Pre-condition

   if ( !(sc=strlen( buf )) )
      return;

   // Grow if necessary

   if ( _num == _max ) {
      s0    = _max * _sb;
      _max += !_max ? K : gmin( _max, 64*K );
      sz    = _max * _sb;
      bp    = new char[sz];
      ::memset( bp, 0, sz );
      if ( (tp=(char *)_buf) ) {
         ::memcpy( bp, tp, s0 );
         delete[] tp;
      }
      _buf = (GLasciiBuf *)bp;
   }
   if ( !_bp && (sz=_mm->siz()) > 0 ) {
      _bp = new char[sz];
      _cp = _bp;
      ::memset( _bp, 0, sz );
   }
assert( _cp );

   // Safe to add

   strcpy( _cp, buf );
   _buf[_num++]._data = _cp;
   _cp += strlen( buf );
   *_cp = '\0';
   _cp++;
}
