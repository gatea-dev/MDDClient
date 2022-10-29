/******************************************************************************
*
*  MDW_Internal.h
*     libmddWire Internals
*
*  REVISION HISTORY:
*     18 SEP 2013 jcs  Created.
*     17 JAN 2015 jcs  Build  9: Kill 4244 / 4267 on WIN32
*     12 SEP 2015 jcs  Build 10: namespace MDDWIRE_PRIVATE
*     12 OCT 2015 jcs  Build 11: MDW_dNow(); MDW_SLEEP(); MDW_GLxml.h 
*     29 OCT 2022 jcs  Build 16: hash_map
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __MDW_INTERNAL_H
#define __MDW_INTERNAL_H

// MF Msgs

#define MT_PING         300      // mf_ping
#define MT_UPDATE       316      // edg_update
#define MT_REC_RES      340      // edg_image
#define MT_STAT_RES     407      // Item Status
#define MT_STATUS_MESS  408      // Global Status
#define MT_COM_RESP       2      // Command

// Hard-Coded

#define K              1024
#define _MAX_FLD_LEN   16*K


/////////////////////////////////
// Definitions / Structures / Forwards ...
/////////////////////////////////
#ifdef WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4786)
#define MDW_SLEEP(d)       ::Sleep( (DWORD)( (d)*1000.0 ) )
#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <sys/timeb.h>
#else
#if defined(linux)
#define MDW_SLEEP(d)       ::usleep( (int)( (d)*1000000.0 ) )
#else
#define MDW_SLEEP(d)       ::sleep( gmax( 1, (int)(d) ) )
#endif // defined(linux)
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#endif // WIN32
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#include <map>
#include <set>
#include <string>
#include <vector>

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
#define MDW_dNow             MDDWIRE_PRIVATE::Logger::dblNow
#define _tvNow               MDDWIRE_PRIVATE::Logger::tvNow

using namespace std;

/////////////
// Us
/////////////
extern "C" {
#include <libmddWire.h>
}

typedef map<string, int> Str2IntMap;

#include <Binary.h>
#include <GLvector>
#include <MDW_GLxml.h>
#include <MDW_Mutex.h>
#include <Data.h>
#include <MDW_Logger.h>
#include <Schema.h>
#include <Subscribe.h>
#include <Publish.h>

// version.cpp

extern char *libmddWireID();

#endif // __MDW_INTERNAL_H
