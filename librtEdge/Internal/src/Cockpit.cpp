/******************************************************************************
*
*  Cockpit.cpp
*     MD-Direct Cockpit Channel
*
*  REVISION HISTORY:
*     22 AUG 2017 jcs  Created.
*     21 JAN 2018 jcs  Build 39: _LVC
*      5 JAN 2024 jcs  Build 67: Buffer.h
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#include <EDG_Internal.h>

using namespace RTEDGE_PRIVATE;


/////////////////////////////////////////////////////////////////////////////
//
//                 c l a s s      C o c k p i t
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////
// Constructor / Destructor
////////////////////////////////////////////
Cockpit::Cockpit( CockpitAttr     attr, 
                  Cockpit_Context cxt,
                  GLlvcDb        *lvc ) :
   Socket( attr._pSvrHosts ),
   _cxt( cxt ),
   _con( attr._pSvrHosts ),
   _xml(),
   _LVC( lvc ),
   _lvcMtx(),
   _bLockedLVC( false )
{
   // Attributes / Idle

   _attr            = attr;
   _attr._pSvrHosts = _con.data();
   pump().AddIdle( Cockpit::_OnIdle, this );

   // Stats

   ::memset( &_zzz, 0, sizeof( _zzz ) );
   ::memset( &_dfltStats, 0, sizeof( _dfltStats ) );
   SetStats( &_dfltStats );

   // LVC

   Locker lck( _lvcMtx );

   if ( _LVC )
      _LVC->Attach( this );
}

Cockpit::~Cockpit()
{
   DetachLVC();
   if ( _log )
      _log->logT( 3, "~Cockpit( %s )\n", dstConn() );
}


////////////////////////////////////////////
// Access
////////////////////////////////////////////
CockpitAttr Cockpit::attr()
{
   return _attr;
}

Cockpit_Context Cockpit::cxt()
{
   return _cxt;
}


////////////////////////////////////////////
// LVC Destruction
////////////////////////////////////////////
void Cockpit::DetachLVC( bool bForced )
{
   Locker lck( _lvcMtx );

   if ( _LVC ) {
      if ( _bLockedLVC )
         _LVC->mtx().Unlock();
      if ( !bForced )
         _LVC->Detach( this );
   }
   _LVC = (GLlvcDb *)0;
}


////////////////////////////////////////////
// Thread Notifications
////////////////////////////////////////////
void Cockpit::OnConnect( const char *pc )
{
   if ( _attr._connCbk )
      (*_attr._connCbk)( _cxt, pc, edg_up );
}

void Cockpit::OnDisconnect( const char *reason )
{
   // Notify interested parties ...

   if ( _attr._connCbk )
      (*_attr._connCbk)( _cxt, reason, edg_down );
}

void Cockpit::OnRead()
{
   Locker           lck( _mtx );
   rtEdgeChanStats &st = stats();
   mddBuf           b;
   const char      *cp;
   struct timeval   tv;
   int              i, sz, nMsg, nb, nL;

   // 1) Base class drains channel and populates _in / _cp

   Socket::OnRead();

   // 2) OK, now we chop up ...

   cp = _in.bp();
   sz = _in.bufSz();
   for ( i=0,nMsg=0; i<sz; ) {
      _xml.reset();
      _xml.parse( cp, sz-i );
      if ( !_xml.isComplete() )
         break; // for-i
      nb = _xml.byteIndex();
      b._data = (char *)cp;
      b._dLen = nb;
      nMsg   += 1;
      _OnXML();
      if ( _log && _log->CanLog( 2 ) ) {
         Locker lck( _log->mtx() );
         string s( cp, nb );

         _log->logT( 2, "[XML-RX]" );
         _log->Write( 2, s.data(), nb );
      }
      cp += nb;
      i  += nb;

      // Stats

      tv            = Logger::tvNow();
      st._lastMsg   = tv.tv_sec;
      st._lastMsgUs = tv.tv_usec;
      st._nMsg     += 1; 
   }

   // Message spans 2+ network packets?

   nL = WithinRange( 0, sz-i, INFINITEs );
   if ( _log && _log->CanLog( 4 ) ) {
      Locker lck( _log->mtx() );

      _log->logT( 4, "%d of %d bytes processed\n", i, sz );
      if ( nL )
         _log->HexLog( 4, cp, nL );
   }
   if ( nMsg && nL )
      _in.Move( sz-nL, nL );
   _in.Set( nL );
}

void Cockpit::_OnXML()
{
   CockpitData d;

   //  Allocate / Populate / Dispatch / Free

   d = _Add( _xml.root() );
   if ( _attr._dataCbk )
      (*_attr._dataCbk)( _cxt, d );
   _Free( d );

}

CockpitData Cockpit::_Add( MDDWIRE_PRIVATE::GLxmlElem *x )
{
   CockpitData d;
   Tuple      &v = d._value;
   TupleList  &a = d._attrs;
   const char *pd;
   char        c;
   int         i;

   // 1) Element name / data

   v._name  = x->name();
   pd       = x->pData();
   for ( ; strlen( pd ); pd++ ) {
      c = pd[0];
      if ( ( c == ' ' ) || ( c == '\t' ) || ( c == '\r' ) || ( c == '\n' ) )
         continue; // for-i
      break; // for-i
   }
   v._value = pd;

   // 2) Attributes

   a._nTuple = x->attributes().size();
   a._tuples = a._nTuple ? new Tuple[a._nTuple] : (Tuple *)0;
   for ( i=0; i<a._nTuple; i++ ) {
      a._tuples[i]._name  = x->attributes()[i]->key();
      a._tuples[i]._value = x->attributes()[i]->value();
   }

   // 3) Elements

   d._nElem = x->elements().size();
   d._elems = d._nElem ? new CockpitData[d._nElem] : (CockpitData *)0;
   for ( i=0; i<d._nElem; i++ )
      d._elems[i] = _Add( x->elements()[i] );
   return d;
}

void Cockpit::_Free( CockpitData e )
{
   TupleList &a = e._attrs;
   int        i;

   // Attributes, then Sub-Elements

   if ( a._tuples )
      delete[] a._tuples;
   for ( i=0; i<e._nElem; _Free( e._elems[i++] ) );
   if ( e._nElem )
      delete[] e._elems;
}


////////////////////////////////////////////
// TimerEvent Notifications
////////////////////////////////////////////
void Cockpit::On1SecTimer()
{
   Socket::On1SecTimer();
   if ( _bIdleCbk && _attr._dataCbk )
      (*_attr._dataCbk)( _cxt, _zzz );
}


/////////////////////////////////////////
// Idle Loop Processing
////////////////////////////////////////////
void Cockpit::OnIdle()
{
breakpointE(); // TODO : Re-connect ...
}

void Cockpit::_OnIdle( void *arg )
{
   Cockpit *us;

   us = (Cockpit *)arg;
   us->OnIdle();
}
