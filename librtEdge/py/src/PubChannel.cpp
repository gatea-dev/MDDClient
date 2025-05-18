/******************************************************************************
*
*  PubChannel.cpp
*     MD-Direct Publication Channel
*
*  REVISION HISTORY:
*     15 MAY 2025 jcs  Created.
*
*  (c) 1994-2025, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>

using namespace MDDPY;

#define INT_PTR int)(size_t
#define PTR_INT void *)(size_t

////////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      M D D p y P u b C h a n
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
MDDpyPubChan::MDDpyPubChan( const char *host, const char *svc ) :
   RTEDGE::PubChannel( svc ),
   _pmp( *this ),
   _host( host ),
   _svc( svc ),
   _byName(),
   _byOid(),
   _fl( ::mddFieldList_Alloc( K ) ),
   _bdsStreamID( 0 ),
   _mtx(),
   _strs()
{
   SetIdleCallback( true );
}

MDDpyPubChan::~MDDpyPubChan()
{
   ::mddFieldList_Free( _fl );
   for ( size_t i=0; i<_strs.size(); delete _strs[i++] );
   _strs.clear();
}


///////////////////////////////
// Operations
///////////////////////////////
PyObject *MDDpyPubChan::Read( double dWait )
{
   PyObject *rtn;

   // First check if there is data

   if ( (rtn=_Get1stUpd()) != Py_None )
      return rtn;

   // Safe to wait for it

   _pmp.Wait( dWait );
   return _Get1stUpd();
}

int MDDpyPubChan::pyPublish( const char *tkr, int ReqID, PyObject *lst )
{
   RTEDGE::Locker l( _mtx );
   RTEDGE::Update &u = upd();

   if ( _py2mdd( lst ) ) {
      u.Init( tkr, ReqID );
      u.AddFieldList( _fl );
      return u.Publish();
   }
   return 0;
}


///////////////////////////////
// RTEDGE::PubChannel Notifications
///////////////////////////////
void MDDpyPubChan::OnConnect( const char *pConn, bool bUp )
{
   const char *ty;
   char        buf[K];
   Update      u = _INIT_MDDPY_UPD;

   // Update queue

   ty  = bUp ?  "UP  " : "DOWN";
   sprintf( buf, "%s|%s", ty, pConn );
   u._mt  = EVT_CONN;
   u._msg = new string( buf );
   _pmp.Add( u );
}

void MDDpyPubChan::OnPubOpen( const char *tkr, void *arg )
{
   RTEDGE::Locker lck( _mtx );
   Update         u   = _INIT_MDDPY_UPD;
   StrIntMap     &ndb = _byName;
   IntStrMap     &idb = _byOid;
   int            rid = (INT_PTR)arg;
   string         s( tkr );
   const char    *tkrs[] = { tkr, (const char *)0 };

   // 1) Maps

   idb[rid] = string( tkr );
   ndb[s]   = rid;

   // 2) Update queue

   u._mt  = EVT_OPEN;
   u._msg = new string( tkr );
   u._arg = arg;
   _pmp.Add( u );

   // 3) BDS

   if ( _bdsStreamID )
      PublishBDS( pPubName(), _bdsStreamID, (char **)tkrs );
}

void MDDpyPubChan::OnPubClose( const char *tkr )
{
   RTEDGE::Locker lck( _mtx );
   Update              u   = _INIT_MDDPY_UPD;
   StrIntMap          &ndb = _byName;
   IntStrMap          &idb = _byOid;
   StrIntMap::iterator nt;
   IntStrMap::iterator it;
   string              s( tkr );
   int                 StreamID;

   // 1) Maps

   StreamID = 0;
   if ( (nt=ndb.find( s )) != ndb.end() ) {
      StreamID = (*nt).second;
      ndb.erase( nt );
   }
   if ( (it=idb.find( StreamID )) != idb.end() )
      idb.erase( it );

   // 2) Update queue

   u._mt  = EVT_CLOSE;
   u._msg = new string( tkr );
   u._arg = (PTR_INT)StreamID;
   if ( StreamID )
      _pmp.Add( u );
}

void MDDpyPubChan::OnOpenBDS( const char *tkr, void *tag )
{
   RTEDGE::Locker            lck( _mtx );
   RTEDGE::Update           &u   = upd();
   StrIntMap                &rdb = _byName;
   StrIntMap::iterator       rt;
   vector<char *>            tkrs;
   SortedStringSet           srt;
   SortedStringSet::iterator it;
   char                      buf[K];
   int                       StreamID;

   // Correct BDS Name?

   StreamID = (INT_PTR)tag;
   if ( !::strcmp( tkr, pPubName() ) ) {
      _bdsStreamID = StreamID;
      for ( rt=rdb.begin(); rt!=rdb.end(); rt++ )
         srt.insert( string( (*rt).first ) );
      for ( it=srt.begin(); it!=srt.end(); it++ )
         tkrs.push_back( ::strdup( (*it).data() ) );
      tkrs.push_back( (char *)0 );
      PublishBDS( tkr, StreamID, tkrs.data() );
      for ( size_t i=0; i<tkrs.size(); ::free( tkrs[i] ), i++ );
   }
   else {
      sprintf( buf, "Wrong BDS : Use %s", pPubName() );
      u.Init( tkr, tag, false );
      u.PubError( buf );
      OnCloseBDS( tkr );
   }
}

void MDDpyPubChan::OnCloseBDS( const char *tkr )
{
   _bdsStreamID = 0;
}

void MDDpyPubChan::OnOverflow()
{
   Update u = _INIT_MDDPY_UPD;

   u._mt  = EVT_OVFLOW;
   _pmp.Add( u );
}

void MDDpyPubChan::OnIdle()
{
   Update u = _INIT_MDDPY_UPD;

   u._mt  = EVT_IDLE;
   _pmp.Add( u );
}

RTEDGE::Update *MDDpyPubChan::CreateUpdate()
{
   return new RTEDGE::Update( *this );
}


///////////////////////////////
// Helpers
///////////////////////////////
int MDDpyPubChan::_py2mdd( PyObject *lst )
{
   RTEDGE::Locker l( _mtx );
   PyObject      *kv, *fid, *val;
   mddField      *fdb, f;
   mddValue      &v = f._val;
   mddBuf        &b = v._buf;
   string        *s;
   int            i, j, nf, nk;

   // Pre-condition(s)

   if ( !PyList_Check( lst ) )
      return 0;
   if ( !(nf=PyList_Size( lst )) )
      return 0;

   // Rock on

   for ( size_t ii=0; ii<_strs.size(); delete _strs[ii++] );
   _strs.clear(); 
   if ( nf > _fl._nAlloc ) {
      ::mddFieldList_Free( _fl );
      _fl = ::mddFieldList_Alloc( nf+4 );
   }
   fdb = _fl._flds;
   for ( i=0,j=0; i<nf; i++ ) {
      kv = PyList_GetItem( lst, i );
      if ( (nk=PyList_Size( kv )) != 2 )
         continue; // for-i
      ::memset( &f, 0, sizeof( f ) );
      fid     = PyList_GetItem( kv, 0 );
      val     = PyList_GetItem( kv, 1 );
      f._type = mddFld_undef;
      f._fid  = PyInt_AsLong( fid );
      /*
       * Supported Field Types
       */
      if ( val == Py_None )
         f._type = mddFld_undef; 
      else if ( PyInt_Check( val ) ) {
         f._type = mddFld_int32; 
         v._i32  = ::_PyInt_AsInt( val );
      }
      else if ( PyLong_Check( val ) ) {
         f._type = mddFld_int64; 
         v._i64  = ::PyLong_AsLong( val );
      }
      else if ( PyFloat_Check( val ) ) {
         f._type = mddFld_double;
         v._r64  = ::PyFloat_AsDouble( val );
      }
      else if ( PyString_Check( val ) ) {
         s       = new string();
         f._type = mddFld_string;
         b._data = (char *)_Py_GetString( val, *s );
         b._dLen = strlen( b._data );
         _strs.push_back( s );
      }
#ifdef VECTOR_NOT_SUPPORTED
      else if ( PyList_Check( pyV ) ) {
         f._type   = mddFld_vector;
         v._Vector = pyV;
         Py_INCREF( pyV );
      }
#endif // VECTOR_NOT_SUPPORTED
      fdb[j]  = f;
      j++;
   }
   _fl._nFld = j;
   return j;
}

PyObject *MDDpyPubChan::_Get1stUpd()
{
   RTEDGE::Locker l( _mtx );
   PyObjects      v;
   PyObject      *rtn, *pym, *pyd;
   string        *s;
   Update         upd;
   const char    *ps;

   // Pull 1st update off _pmp queue

   if ( !_pmp.GetOneUpd( upd ) )
      return Py_None;

   /*
    * ( EVT_CONN, pMsg )
    * ( EVT_OPEN, tkr, StreamID )
    */
   switch( upd._mt ) {
      case EVT_CONN:
         s   = upd._msg;
         ps  = s->data();
         pyd = PyString_FromString( ps );
         break;
      case EVT_OPEN:
      case EVT_CLOSE:
         s   = upd._msg;
         ps  = s->data();
         pyd = ::PyList_New( 2 );
         ::PyList_SetItem( pyd, 0, PyString_FromString( s->data() ) );
         ::PyList_SetItem( pyd, 1, PyInt_FromLong( (INT_PTR)upd._arg ) );
         break;
      case EVT_OVFLOW:
         pyd = PyString_FromString( "OnOverflow" );
         break;
      case EVT_IDLE:
         pyd = PyString_FromString( "OnIdle" );
         break;
      default:
         pyd = PyString_FromString( "Unknown msg type" );
         break;
   }
   if ( upd._msg )
      delete upd._msg;
   pym = PyInt_FromLong( upd._mt );
   rtn = ::mdd_PyList_Pack2( pym, pyd );
   return rtn;
}

