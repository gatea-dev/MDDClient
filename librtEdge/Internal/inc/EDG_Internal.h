/******************************************************************************
*
*  EDG_Internal.h
*     libEdge Internals
*
*  REVISION HISTORY:
*     21 JUL 2009 jcs  Created.
*     31 AUG 2009 jcs  Build  3: STRTOK
*      5 OCT 2009 jcs  Build  4: TimerEvent
*      2 SEP 2010 jcs  Build  6: LVC
*     18 SEP 2010 jcs  Build  7: LVC admin
*     23 SEP 2010 jcs  Build  8: PubChannel
*     31 DEC 2010 jcs  Build  9: 64-bit
*     24 JAN 2012 jcs  Build 17
*     20 MAR 2012 jcs  Build 18: RecCache
*     22 APR 2012 jcs  Build 19: namespace LIBRTEDGE
*     20 OCT 2012 jcs  Build 20: ChartDB
*     11 JUL 2013 jcs  Build 26a:_tvNow(); Stats
*     31 JUL 2013 jcs  Build 27: pthread_t - DUH
*     12 NOV 2014 jcs  Build 28: -Wall
*     20 JUN 2015 jcs  Build 31: HANDLE; _MAX_BUF_SIZ
*     13 SEP 2015 jcs  Build 32: Linux compatibility in libmddWire
*     13 JUL 2017 jcs  Build 34: chop()
*     12 SEP 2017 jcs  Build 35: Cockpit.h; hash_map
*     21 JAN 2018 jcs  Build 39: ATOMIC_CMP_EXCH
*      6 DEC 2018 jcs  Build 41: 64-bit PTRSZ : size_t
*     29 APR 2020 jcs  Build 43: hash_set
*     26 APR 2022 jcs  Build 53: MDDirectStats
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_INTERNAL_H
#define __EDGLIB_INTERNAL_H

#define _MAX_BUF_SIZ   10*K*K // 10 MB

/////////////////////////////////
// 64-bit
/////////////////////////////////
#ifdef WIN32
#define LVCint  int
#define LVClong long
#else
#define LVCint  int32_t
#define LVClong int32_t
#endif // WIN32
#define CDBint  LVCint
#define CDBlong LVClong

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
#include <GLvector>

#undef K
#if !defined(hash_map)
#if !defined(_GLIBCXX_UNORDERED_MAP) && !defined(WIN32)
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define hash_map std::tr1::unordered_map
#define hash_set std::tr1::unordered_set
#else
#include <unordered_map>
#include <unordered_set>
#define hash_map std::unordered_map
#define hash_set std::unordered_set
#endif // !defined(_GLIBCXX_UNORDERED_MAP)
#endif // !defined(hash_map)
#define K 1024

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
#include <librtEdge.h>
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

}; // class GLmdStatsHdr

class GLlibStats : public GLmdStatsHdr
{
public:
   rtEdgeChanStats _sub;
   rtEdgeChanStats _pub;

}; // class GLlibStats

/////////////
// Us
/////////////
namespace RTEDGE_PRIVATE
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

} // namespace RTEDGE_PRIVATE

typedef hash_map<int, int>      FidMap;
typedef hash_map<string, int>   RecMap;

#include <Mutex.h>
#include <Logger.h>
#include <EDG_GLmmap.h>
#include <EDG_GLmd5.h>
#include <EDG_GLchtDb.h>
#include <EDG_GLlvcDb.h>
#include <Socket.h>
#include <Pump.h>
#include <Thread.h>
#include <RecCache.h>
#include <EdgChannel.h>
#include <PubChannel.h>
#include <Cockpit.h>

// version.cpp

extern char *librtEdgeID();
extern void  breakpointE();

#endif // __EDGLIB_INTERNAL_H
