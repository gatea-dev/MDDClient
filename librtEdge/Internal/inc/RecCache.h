/******************************************************************************
*
*  RecCache.h
*     librtEdge conflated record cache
*
*  REVISION HISTORY:
*     20 MAR 2012 jcs  Created (from rtInC).
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*     12 NOV 2014 jcs  Build 28: Record._fl
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*     24 AUG 2017 jcs  Build 35: hash_map
*
*  (c) 1994-2017 Gatea Ltd.
******************************************************************************/
#ifndef __EDGLIB_CACHE_H
#define __EDGLIB_CACHE_H
#include <EDG_Internal.h>

// Read() / Dispatch() events

#define EVT_NONE   0x0000
#define EVT_CONN   0x0001
#define EVT_SVC    0x0002
#define EVT_IMG    0x0004
#define EVT_UPD    0x0008
#define EVT_STS    0x0010
#define EVT_SCHEMA 0x0020

namespace RTEDGE_PRIVATE
{

/////////////////
// Forwards
/////////////////
class Field;
class Record;

class Update
{
public:
   int         _mt;
   rtEdgeState _state;
   Record     *_rec;
   string     *_msg;
};

typedef hash_map<int, Field *> FldMap;
typedef vector<Update>         Updates;
typedef vector<Field *>        FldUpds;
typedef FldUpds           FldList;


/////////////////////////////////////////
// Cached Field
/////////////////////////////////////////
class Field
{
private:
   Record   &_rec;
	rtFIELD  *_cache;
   mddBldBuf _buf;
public:
   bool    _bUpd;
   double  _tUpd;

   // Constructor / Destructor
public:
   Field( Record &, rtFIELD * );
   ~Field();

   // Access

   rtFIELD    &cache();
   char       *data();
   int         dLen();
   int         Fid();
   const char *name();
   rtFldType   type();

   // Mutator

   void Cache( rtFIELD );
   void ClearUpd();
};



/////////////////////////////////////////
// Cached Record
/////////////////////////////////////////
class Record
{
protected:
   string       _svc;
   string       _tkr;
   int          _StreamID;
   Mutex        _mtx;
   FldMap       _flds;
   FldUpds      _upds;
public:
   mddFieldList _fl;
   bool         _bQ;
   double       _tUpd;

   // Constructor / Destructor
public:
   Record( const char *, const char *, int );
   ~Record();

   // Access

   const char  *pSvc();
   const char  *pTkr();
   int          StreamID();
   Field       *GetField( int );
   int          GetUpds( rtFIELD * );
   mddFieldList GetCache();

   // Real-Time Data

   void Cache( mddFieldList & );
};

/////////////////////////////////////////
// Event Pump
/////////////////////////////////////////
class EventPump
{
protected:
   Mutex   _updMtx;
   Updates _upds;
   Event   _evt;

   // Constructor / Destructor
public:
   EventPump();
   ~EventPump();

   // Operations

   void Add( Update & );
   void AddAndNotify( Update & );
   bool GetOneUpd( Update & );
   void Close( Record * );

   // Threading Synchronization

   void Notify();
   void Wait( double );
};

} // namespace RTEDGE_PRIVATE

#endif // __EDGLIB_CACHE_H
