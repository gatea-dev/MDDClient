/******************************************************************************
*
*  MDDirect.h
*     Python 'C' extension : MD-Direct
*
*  REVISION HISTORY:
*      7 APR 2011 jcs  Created.
*      . . . 
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     18 NOV 2020 jcs  Build  2: hash_map iff !defined
*     22 NOV 2020 jcs  Build  3: PyObjects
*      3 FEB 2022 jcs  Build  5: PyList_PackX()
*     16 MAR 2022 jcs  Build  6: error C2760
*     19 JUL 2022 jcs  Build  8: LVCAdmin
*     11 JAN 2023 jcs  Build  9: Python 3.x on Linux
*     29 AUG 2023 jcs  Build 10: EVT_BDS
*     20 SEP 2023 jcs  Build 11: mdd_PyList_PackX()
*     17 OCT 2023 jcs  Build 12: No mo Book
*      8 JUL 2025 jcs  Build 77: PubChannel.h; ChartDbSvr.h
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#ifndef _MDDPY_PYTHON_H
#define _MDDPY_PYTHON_H
/*
 * Handle error C2760: syntax error: unexpected token 'identifier', 
 * expected 'type specifier'
 */
#if defined(WIN32)
typedef struct IUnknown IUnknown;
#endif // defined(WIN32)
/*
 * Normal Headers follow the typedef
 */
#include <Python.h>
#include <math.h>
#include <stdio.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <librtEdge.h>
#include <LocklessFifo>

// Python 3

#if PY_MAJOR_VERSION >= 3
#define _PyInt_AsInt                  PyLong_AsLong
#define PyInt_AsLong                  PyLong_AsLong
#define PyInt_Check                   PyLong_Check
#define PyInt_FromLong                PyLong_FromLong
// #define PyString_AsString             PyBytes_AsString
#define PyString_Check                PyUnicode_Check
#define PyString_FromString           PyUnicode_FromString
#define PyString_FromStringAndSize    PyUnicode_FromStringAndSize
#define PyString_FromFormat           PyUnicode_FromFormat
#endif // PY_MAJOR_VERSION >= 3

// stl::unordered_map

#undef K
#if !defined(hash_map)
#if !defined(_GLIBCXX_UNORDERED_MAP) && !defined(WIN32)
#include <tr1/unordered_map>
#define hash_map tr1::unordered_map
#else
#include <unordered_map>
#define hash_map unordered_map
#endif // !defined(_GLIBCXX_UNORDERED_MAP)
#endif // !defined(hash_map)
#define K 1024

using namespace std;

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


// librtEdge stuff

#define _pGblSts    "__GLOBAL__"
#define dNow        mddWire_TimeNs

// Useful Macros / Fcns

#define _MAX_PYCHAN  1024

// Read() events

#define EVT_CONN   0x0001
#define EVT_SVC    0x0002
#define EVT_UPD    0x0004
#define EVT_STS    0x0008
#define EVT_SCHEMA 0x0010
#define EVT_OPEN   0x0020
#define EVT_CLOSE  0x0040
#define EVT_BYSTR  0x0080
#define EVT_RECOV  0x0100
#define EVT_DONE   0x0200
#define EVT_BDS    0x0400
#define EVT_OVFLOW 0x0800
#define EVT_IDLE   0x1000
#define EVT_CHAN   ( EVT_CONN | EVT_SVC )
#define EVT_ALL    0xffff

// Ioctl() cmds

#define IOCTL_RTD  0x0001

//////////////////////////
// structs / collections
//////////////////////////
class rtMsg;
class Record;

class Update
{
public:
   int     _mt;
   Record *_rec;
   string *_msg;
   void   *_arg;
   int     _nUpd;
   double  _tMsg;
   rtBUF   _bStr;
};

#define _INIT_MDDPY_UPD { 0, \
                          (Record *)0, \
                          (string *)0, \
                          (void *)0, \
                          0, \
                          0.0, \
                          { (char *)0, 0 } \
                        }

typedef set<string, less<string> > SortedStringSet;
typedef hash_map<int, string>      IntStrMap;
typedef hash_map<string, int>      StrIntMap;
typedef hash_map<int, Record *>    RecByOid;
typedef hash_map<string, Record *> RecByName;
typedef hash_map<int, rtFIELD>     Fields;
typedef vector<string *>           Strings;
typedef vector<PyObject *>         PyObjects;
typedef vector<int>                Ints;
typedef vector<Update>             Updates;
typedef vector<rtMsg *>            rtMsgs;
typedef LocklessFifo<Update>       UpdateFifo;

///////////////
// Us
///////////////
#include <ChartDbSvr.h>
#include <EventPump.h>
#include <RecCache.h>
#include <LVC.h>
#include <LVCAdmin.h>
#include <Stats.h>
#include <PubChannel.h>
#include <SubChannel.h>

// version.cpp

extern void        m_breakpoint();
extern int         strncpyz( char *, char *, int );
extern int         atoin( char *, int );
extern char       *MDDirectID();
extern PyObject   *mdd_PyList_Pack2( PyObject *, PyObject * );
extern PyObject   *mdd_PyList_Pack3( PyObject *, PyObject *, PyObject * );
extern PyObject   *mdd_PyList_Pack4( PyObject *, PyObject *, PyObject *, PyObject * );
extern PyObject   *_PyReturn( PyObject * );
extern const char *_Py_GetString( PyObject *, string & );

#endif // _MDDPY_PYTHON_H
