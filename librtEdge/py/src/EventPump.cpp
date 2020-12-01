/******************************************************************************
*
*  EventPump.cpp
*     MDDirect Event Pump
*
*  REVISION HISTORY:
*      7 APR 2011 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     20 NOV 2020 jcs  Build  2: Tape stuff
*
*  (c) 1994-2020 Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>

#define _MAX_FIFOQ 128*K  // TODO : Configurable


////////////////////////////////////////////////////////////////////////////////
//
//                  c l a s s      E v e n t P u m p
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
EventPump::EventPump( MDDpySubChan &ch ) :
   _ch( ch ),
   _mtx(),
   _upds(),
   _updFifo( _MAX_FIFOQ ),
   _msgs(),
   _Notify( false ),
   _SleepMillis( 1 )
{
}

EventPump::~EventPump()
{
   RTEDGE::Locker ul( _mtx );

   _upds.clear();
}


///////////////////////////////
// Operations
///////////////////////////////
int EventPump::nMsg()
{
   return _msgs.size();
}

void EventPump::Add( Update &u )
{
   RTEDGE::Locker ul( _mtx );

   _upds.push_back( u );
}

void EventPump::Add( rtMsg *rt )
{
   RTEDGE::Locker ul( _mtx );

   _msgs.push_back( rt );
}

bool EventPump::GetOneUpd( Update &rtn )
{
   RTEDGE::Locker    l( _mtx );
   Updates::iterator ut;
   rtMsgs::iterator  rt;
   Update            upd;
   rtMsg            *msg;
   bool              bUpd;

   /*
    * 1) Pull 1st update off conflated _upds queue,
    * 2) Else pull off non-conflated _msgs queue
    */
   bUpd = ( (ut=_upds.begin()) != _upds.end() );
   if ( bUpd ) {
      upd = (*ut);
      rtn = upd;
      _upds.erase( ut );
   }
   else {
      bUpd = ( (rt=_msgs.begin()) != _msgs.end() );
      if ( bUpd ) {
         msg = (*rt);
         upd = _ch.ToUpdate( *msg );
         rtn = upd;
         _msgs.erase( rt );
         delete msg;

      }
   }
   return bUpd;
}         

void EventPump::Drain( int iFilter )
{
   RTEDGE::Locker    l( _mtx );
   Updates::iterator ut;
   Update            upd;
   int               mt;

   // Drain events off queue that don't match

   for ( ut=_upds.begin(); ut!=_upds.end(); ut++ ) {
      upd = (*ut);
      mt  = upd._mt;
      if ( !( mt & iFilter ) )
         _upds.erase( ut );
   }
   
}

void EventPump::Close( Book *bk )
{
   RTEDGE::Locker    l( _mtx ); 
   Updates::iterator ut;
   Update            upd;

   for ( ut=_upds.begin(); ut!=_upds.end(); ut++ ) {
      upd = (*ut);
      if ( upd._bk == bk ) {
         _upds.erase( ut );
         break; // for-loop
      }
   }
}


///////////////////////////////
// Threading Synchronization
///////////////////////////////
void EventPump::SleepMillis( int tMs )
{
   _SleepMillis = WithinRange( 1, tMs, 1000 );
}

void EventPump::Notify()
{
   _Notify = true;
}

void EventPump::Wait( double dWait )
{
   PyThreadState *_save;
   double         d0, age, dSlp;

   // Safe to wait for it

   Py_UNBLOCK_THREADS
   d0   = dNow();
   dSlp = 0.001 * (double)_SleepMillis;
   age  = 0;
   for ( ; !_Notify && ( age < dWait ); _ch.Sleep( dSlp ), age = dNow()-d0 );
   _Notify = false;
   Py_BLOCK_THREADS
}



////////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      r t M s g
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
rtMsg::rtMsg( const char *rawData, int dLen, int oid ) :
   _err( (string *)0 ),
   _oid( oid )
{
   _data = (char *)0;
   _dLen = dLen;
   if ( dLen ) {
      if ( ( _dLen+1 ) < sizeof( _upd ) )
         _data = _upd;
      else
         _data = new char[dLen+1];
      ::memcpy( _data, rawData, dLen );
      _data[dLen] = '\0';
   }
}

rtMsg::rtMsg( const char *pErr, int oid ) :
   _err( new string( pErr ) ),
   _oid( oid )
{
   _data = (char *)0;
   _dLen = 0;
}

rtMsg::~rtMsg()
{
   if ( _data && ( _data != _upd ) )
      delete[] _data;
   if ( _err )
      delete _err;
}

