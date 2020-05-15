/******************************************************************************
*
*  Internal.h
*     libEdge Internals
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_INTERNAL_H
#define __YAMR_INTERNAL_H

// Buffer / Packet Sizes

#define _DFLT_BUF_SIZ   10*K*K

/////////////////////////////////
// 64-bit
/////////////////////////////////
#if defined(__LP64__) || defined(_WIN64)
#define PTRSZ size_t
#define GL64 "(64-bit)"
#else
#define PTRSZ int
#define GL64 "(32-bit)"
#endif // __LP64__

/////////////////////////////////
// Definitions / Structures / Forwards ...
/////////////////////////////////
#ifdef WIN32
#pragma warning(disable:4786)
#define THR_ARG        void 
#define THR_TID        HANDLE
#define READ(fd,b,sz)  ::recv( fd, (b), (sz), 0 )
#define WRITE(fd,b,sz) ::send( fd, (b), (sz), 0 )
#define CLOSE(fd)      ::closesocket( (fd) )
#define SLEEP(d)       ::Sleep( (DWORD)( (d)*1000.0 ) )
#define SIGPIPE        13
#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <sys/timeb.h>
#else
#define HANDLE         void *
#define THR_ARG        void *
#define THR_TID        pthread_t
#define READ(fd,b,sz)  ::read( fd, (b), (sz) )
#define CLOSE(fd)      ::close( (fd) )
#define WRITE(fd,b,sz) ::write( fd, (b), (sz) )
#define INFINITE       0xFFFFFFFF  // Infinite timeout
#if defined(linux)
#define SLEEP(d)       ::usleep( (u_int64_t)( (d)*1000000.0 ) )
#else
#define SLEEP(d)       ::sleep( gmax( 1, (int)(d) ) )
#endif // defined(linux)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#endif // WIN32
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#if !defined(linux)
#define socklen_t      int
#endif // !defined(linux)

#include <map>
#include <set>
#include <string>
#include <vector>

#undef K
#if !defined(_GLIBCXX_UNORDERED_MAP) && !defined(WIN32)
#include <tr1/unordered_map>
#define hash_map std::tr1::unordered_map
#else
#include <unordered_map>
#define hash_map std::unordered_map
#endif // !defined(_GLIBCXX_UNORDERED_MAP)
#define K 1024


#define _NANO                1000000000
#define u_int                unsigned int
#define u_long               unsigned long
#define ZMIL                 1000000.0
#define gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define INFINITEs            0x7fffffff           // Signed infinite
#define gmax( a,b )          ( ((a)>=(b)) ? (a) : (b) )
#define gmin( a,b )          ( ((a)<=(b)) ? (a) : (b) )
#define InRange( a,b,c )     ( ((a)<=(b)) && ((b)<=(c)) )
#define WithinRange( a,b,c ) ( gmin( gmax((a),(b)), (c)) )
#define IsDigit(c)           InRange( '0',(c),'9' )
#define IsAscii(c)           InRange( ' ', (c), '~' )
#define dNow                 Logger::dblNow
#define _tvNow               Logger::tvNow
#define HOME                 "\033[1;1H\033[K"

#define chop( b )                           \
   do {                                     \
      char *bp = (b);                       \
                                            \
      bp += ( strlen(b) - 1 );              \
      if ( ( *bp==0x0d ) || ( *bp==0x0a ) ) \
         *bp = '\0';                        \
   } while( 0 )

#define safe_strcpy( a,b )     \
   do {                        \
      int szA, szB, sz;        \
                               \
      szA = sizeof( a ) - 1;   \
      szB = strlen( b );       \
      sz  = gmin( szA, szB );  \
      ::memcpy( a,b,sz );      \
      a[sz] = '\0';            \
   } while( 0 )

// Atomic operations

extern "C"
{
#ifdef WIN32
#define ATOMIC_INC( pDest )           InterlockedIncrement( (LONG *)pDest )
#define ATOMIC_DEC( pDest )           InterlockedDecrement( (LONG *)pDest )
#define ATOMIC_ADD( pDest, val )      InterlockedAdd( (LONG *)pDest, val )
#define ATOMIC_SUB( pDest, val )      InterlockedAdd( (LONG *)pDest, -val )
#define ATOMIC_EXCH( pDest, newVal )              \
              InterlockedExchange( (LONG *)pDest, newVal )
#define ATOMIC_CMP_EXCH( pDest, oldVal, newVal )  \
              InterlockedCompareExchange( (LONG *)pDest, newVal, oldVal )
#else
#define ATOMIC_INC( pDest )           __sync_add_and_fetch( pDest, 1 )
#define ATOMIC_DEC( pDest )           __sync_sub_and_fetch( pDest, 1 )
#define ATOMIC_ADD( pDest, val )      __sync_add_and_fetch( pDest, val )
#define ATOMIC_SUB( pDest, val )      __sync_sub_and_fetch( pDest, val )
#define ATOMIC_EXCH( pDest, newVal )              \
              __sync_lock_test_and_set( pDest, newVal )
#define ATOMIC_CMP_EXCH( pDest, oldVal, newVal )  \
              __sync_val_compare_and_swap( pDest, oldVal, newVal )
#endif // WIN32
} // extern "C"


using namespace std;

//////////////////////
// Run-time Stats
//////////////////////
extern "C" {
#include <libyamr.h>
}

class GLmdStatsHdr
{
public:
   u_int _version;
   u_int _fileSiz;
   u_int _tStart;   /* Start - Secs */
   u_int _tStartUs; /* Start - micros */
   char  _exe[K];
   char  _build[K];
   int   _nSub;
   int   _nPub;
};

class GLlibStats : public GLmdStatsHdr
{
public:
   yamrChanStats _sub;
   yamrChanStats _pub;
};


/////////////
// Us
/////////////
namespace YAMR_PRIVATE
{
class TimerEvent
{
friend class Pump;
   // Constructor / Destructor
protected:
   TimerEvent() { ; }
   virtual ~TimerEvent() { ; }

   // Event Notification
protected:
   virtual void On1SecTimer() = 0;
};

} // namespace YAMR_PRIVATE

#include <Mutex.h>
#include <Logger.h>
#include <GLmmap.h>
#include <Socket.h>
#include <Pump.h>
#include <Thread.h>
#include <WireProtocol.h>
#include <Channel.h>
#include <Reader.h>

// version.cpp

extern char *libyamrID();

#endif // __YAMR_INTERNAL_H
