/******************************************************************************
*
*  RecCache.cpp
*     librtEdge conflated record cache
*
*  REVISION HISTORY:
*     20 MAR 2012 jcs  Created (from rtInC).
*     22 APR 2012 jcs  Build 19: rtFIELD, not FIELD
*      6 MAY 2013 jcs  Build 25: rtBUF
*     12 NOV 2014 jcs  Build 28: Record._fl
*     12 OCT 2015 jcs  Build 32: EDG_Internal.h
*
*  (c) 1994-2015 Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>


using namespace RTEDGE_PRIVATE; 

////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      F i e l d
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
Field::Field( Record &rec, rtFIELD *fld ) :
   _rec( rec ),
   _cache( fld ),
   _bUpd( true ),
   _tUpd( 0.0 )
{
   ::memset( _cache, 0, sizeof( rtFIELD ) );
   ::memset( &_buf, 0, sizeof( _buf ) );
}

Field::~Field()
{
   ::mddBldBuf_Free( _buf );
}


///////////////////////////////
// Access
///////////////////////////////
rtFIELD &Field::cache()
{
   return *_cache;
}

char *Field::data()
{
   return _buf._data ? _buf._data : (char *)"";
}

int Field::dLen()
{
   return _buf._dLen;
}

int Field::Fid()
{
   return cache()._fid;
}

const char *Field::name()
{
   return cache()._name;
}

rtFldType Field::type()
{
   return cache()._type;
}


///////////////////////////////
// Mutator
///////////////////////////////
void Field::Cache( rtFIELD f )
{
   rtVALUE &v = f._val;
   rtBUF   &b = v._buf;

   ::memcpy( _cache, &f, sizeof( f ) );
   _bUpd   = true;
   switch( f._type ) {
      case rtFld_undef:
         break;
      case rtFld_string:
      case rtFld_bytestream:
      {
         _buf._dLen = gmax( b._dLen, 0 );
         if ( _buf._dLen >= _buf._nAlloc ) {
            ::mddBldBuf_Free( _buf );
            _buf          = ::mddBldBuf_Alloc( _buf._dLen+4 );
            _buf._data[0] = '\0';
         }
         if ( b._data && _buf._dLen ) {
            ::memcpy( _buf._data, b._data, _buf._dLen );
            _buf._data[_buf._dLen] = '\0';
         }
         cache()._val._buf._data = _buf._data;
         cache()._val._buf._dLen = _buf._dLen;
         break;
      }
      case rtFld_int:
      case rtFld_double:
      case rtFld_date:
      case rtFld_time:
      case rtFld_timeSec:
      case rtFld_float:
      case rtFld_int8:
      case rtFld_int16:
      case rtFld_int64:
      case rtFld_real:
         break;
   }
   _tUpd = &_rec ? _rec._tUpd : 0;
}

void Field::ClearUpd()
{
   _bUpd = false;
}



////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      R e c o r d
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
Record::Record( const char *pSvc, const char *pTkr, int StreamID ) :
   _svc( pSvc ),
   _tkr( pTkr ),
   _StreamID( StreamID ),
   _mtx(),
   _flds(),
   _upds(),
   _bQ( false ),
   _tUpd( 0.0 )
{
   ::memset( &_fl, 0, sizeof( _fl ) );
}

Record::~Record()
{
   FldMap::iterator it;

   for ( it=_flds.begin(); it!=_flds.end(); it++ )
      delete (*it).second;
   _flds.clear();
   ::mddFieldList_Free( _fl );
}


///////////////////////////////
// Access
///////////////////////////////
const char *Record::pSvc()
{
   return _svc.c_str();
}

const char *Record::pTkr()
{
   return _tkr.c_str();
}

int Record::StreamID()
{
   return _StreamID;
}

Field *Record::GetField( int fid )
{
   FldMap::iterator it;
   Field           *fld;

   fld = (Field *)0;
   if ( (it=_flds.find( fid )) != _flds.end() )
      fld = (*it).second;
   return fld;
}

int Record::GetUpds( rtFIELD *fdb )
{
   Locker  lck( _mtx );
   Field  *fld;
   rtFIELD f;
   rtBUF  &b = f._val._buf;
   int     i, nf;

   nf = _upds.size();
   for ( i=0; i<nf; i++ ) {
      fld     = _upds[i];
      f._fid  = fld->Fid();
      b._data = fld->data();
      b._dLen = fld->dLen();
      f._name = fld->name();
      f._type = fld->type();
      fdb[i]  = f;
      fld->ClearUpd();
   }
   _upds.clear();
   return nf;
}

mddFieldList Record::GetCache()
{
   return _fl;
}


///////////////////////////////
// Real-Time Data
///////////////////////////////
void Record::Cache( mddFieldList &fl )
{
   Locker   lck( _mtx );
   rtFIELD *fdb, *cdb, *c;
   rtFIELD  f;
   rtBUF   &b = f._val._buf;
   Field   *fld;
   char    *pd;
   bool     bImg;
   int      i, fid, dLen, nFld;

   fdb   = (rtFIELD *)fl._flds;
   nFld  = fl._nFld;
   _tUpd = nFld ? dNow() : _tUpd;
   bImg  = ( _fl._nAlloc == 0 );
   if ( bImg )
      _fl = ::mddFieldList_Alloc( nFld+4 );
   cdb = (rtFIELD *)_fl._flds;
   for ( i=0; i<nFld; i++ ) {
      f    = fdb[i];
      fid  = f._fid;
      pd   = b._data;
      dLen = b._dLen;
      fld  = GetField( fid );
      if ( !fld && bImg ) {
         c          = &cdb[_fl._nFld++];
         fld        = new Field( *this, c );
         _flds[fid] = fld;
         _upds.push_back( fld );
      }
      if ( fld ) {
         if ( !fld->_bUpd )
            _upds.push_back( fld );
         fld->Cache( f );
      }
   }
}




////////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      E v e n t P u m p
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
EventPump::EventPump() :
   _updMtx(),
   _upds(),
   _evt()
{
}

EventPump::~EventPump()
{
   Locker ul( _updMtx );

   _upds.clear();
}


///////////////////////////////
// Operations
///////////////////////////////
void EventPump::Add( Update &u )
{
   Locker  ul( _updMtx );
   Record *rec;

   rec = u._rec;
   if ( !rec || !rec->_bQ )
      _upds.push_back( u );
   if ( rec )
      rec->_bQ = true;
}

void EventPump::AddAndNotify( Update &u )
{
   Add( u );
   Notify();
}

bool EventPump::GetOneUpd( Update &u )
{
   Locker            ul( _updMtx );
   Updates::iterator ut;
   Record           *rec;
   bool              bUpd;

   // Pull 1st update off _upds queue

   bUpd = ( (ut=_upds.begin()) != _upds.end() );
   if ( bUpd ) {
      u = (*ut);
      if ( (rec=u._rec) )
         rec->_bQ = false;
      _upds.erase( ut );
   }      
   return bUpd;
}         

void EventPump::Close( Record *rec )
{
   Locker            ul( _updMtx ); 
   Updates::iterator ut;
   Update            upd;

   for ( ut=_upds.begin(); ut!=_upds.end(); ut++ ) {
      upd = (*ut);
      if ( upd._rec == rec ) {
         _upds.erase( ut );
         return;
      }
   }
}


///////////////////////////////
// Threading Synchronization
///////////////////////////////
void EventPump::Notify()
{
   _evt.SetEvent();
}

void EventPump::Wait( double dWait )
{
   _evt.WaitEvent( dWait );
}


