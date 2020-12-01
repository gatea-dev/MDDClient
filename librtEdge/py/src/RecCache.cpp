/******************************************************************************
*
*  RecCache.cpp
*     MDDirect record cache
*
*  REVISION HISTORY:
*      7 APR 2011 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     19 NOV 2020 jcs  Build  2: _bufcpy() : !bp
*     22 NOV 2020 jcs  Build  3: PyObjects
*
*  (c) 1994-2020 Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>

using namespace MDDPY;

#define _MDDPY_INT    1
#define _MDDPY_DBL    2
#define _MDDPY_STR    3
#define _MDDPY_DT     4 // i32 = ( y * 10000) + ( m * 100 ) + d
#define _MDDPY_TM     5 // r64 = i32 + mikes
#define _MDDPY_TMSEC  6 // i32 = ( h * 10000) + ( m * 100 ) + s

////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      F i e l d
//
////////////////////////////////////////////////////////////////////////////////

static mddBuf   _zBuf = { (char *)0, 0 };
static mddValue _zVal = { _zBuf };
static mddField _zFld = { 0, _zVal, (const char *)0, mddFld_undef }; 

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
Field::Field( mddField f, bool bSchema ) :
   _fld( f ),
   _b( _zBuf ),
   _nAlloc( 0 ),
   _bUpd( false )
{
   mddBuf b;
   char  *bp;

   if ( bSchema ) {
      b._data = (char *)f._name;
      b._dLen = strlen( b._data );
      bp = _InitBuf( b._dLen+4 );
      _bufcpy( bp, b );
   }
   else
      Update( f );
}

Field::Field() :
   _fld( _zFld ),
   _b( _zBuf ),
   _nAlloc( 0 ),
   _bUpd( false )
{
}

Field::~Field()
{
   if ( _b._data )
      delete[] _b._data;
}


///////////////////////////////
// Access
///////////////////////////////
mddField &Field::fld()
{
   return _fld;
}

char *Field::data()
{
   return _b._data ? _b._data : (char *)"";
}

int Field::dLen()
{
   return _b._dLen;
}

int Field::Fid()
{
   return _fld._fid;
}

const char *Field::name()
{
   return _fld._name;
}

mddFldType Field::type()
{
   return _fld._type;
}

PyObject *Field::GetValue( int &ty )
{
   mddField &f = _fld;
   mddValue &v = f._val;
   PyObject *py;
   double    r64;
   int       ymd;

   // Field Type : { _MDDPY_INT, _MDDPY_DBL, _MDDPY_STR }

   py = Py_None;
   ty = _MDDPY_STR;
   switch( f._type ) {
      case mddFld_undef:
      case mddFld_string:
         py = PyString_FromString( data() );
         ty = _MDDPY_STR;
         break;
      case mddFld_int8:
         py = PyInt_FromLong( v._i8 );
         ty = _MDDPY_INT;
         break;
      case mddFld_int16:
         py = PyInt_FromLong( v._i16 );
         ty = _MDDPY_INT;
         break;
      case mddFld_int32:
         py = PyInt_FromLong( v._i32 );
         ty = _MDDPY_INT;
         break;
      case mddFld_int64:
         py = PyInt_FromLong( v._i64 );
         ty = _MDDPY_INT;
         break;
      case mddFld_float:
         py = PyFloat_FromDouble( v._r32 );
         ty = _MDDPY_DBL;
         break;
      case mddFld_time:
         py = PyFloat_FromDouble( v._r64 );
         ty = _MDDPY_TM; // _MDDPY_DBL;
         break;
      case mddFld_double:
         py = PyFloat_FromDouble( v._r64 );
         ty = _MDDPY_DBL;
         break;
      case mddFld_date:
         r64 = v._r64 / 1000000.0;
         ymd = (int)r64;
         py  = PyInt_FromLong( ymd );
         ty  = _MDDPY_DT; // _MDDPY_INT;
         break;
      case mddFld_timeSec:
         ymd = (int)v._r64;
         py  = PyInt_FromLong( ymd );
         ty  = _MDDPY_TMSEC; // _MDDPY_INT;
         break;
      default:
         py = PyString_FromString( "Not Supported" );
         ty = _MDDPY_INT;
         break;
   }
   return py;
}


///////////////////////////////
// Mutator
///////////////////////////////
void Field::Update( mddField &f )
{
   mddValue &v = f._val;
   mddBuf   &b = v._buf;

   _fld = f;
   if ( f._type == mddFld_string ) {
      _InitBuf( b._dLen );
      _bufcpy( _b._data, b );
   }
}

void Field::ClearUpd()
{
   _bUpd = false;
}


///////////////////////////////
// Helpers
///////////////////////////////
char *Field::_InitBuf( int dLen )
{
   _bUpd    = true;
   _b._dLen = gmax( dLen, 0 );
   if ( (int)_b._dLen > _nAlloc ) {
      _nAlloc = _b._dLen+4;
      if ( _b._data )
         delete[] _b._data;
      _b._data = new char[_nAlloc];
   }
   return _b._data;
}

int Field::_bufcpy( char *bp, mddBuf b )
{
   if ( bp ) {
      if ( b._dLen )
         ::memcpy( bp, b._data, b._dLen );
      bp[b._dLen] = '\0';
   }
   return b._dLen;
}



////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      R e c o r d
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
Record::Record( const char   *pSvc, 
                const char   *pTkr, 
                int           userReqID,
                int           oid ) :
   _svc( pSvc ),
   _tkr( pTkr ),
   _flds(),
   _upds(),
   _oid( oid ),
   _userReqID( userReqID ),
   _nUpd( 0 ),
   _tUpd( 0.0 )
{
}

Record::~Record()
{
   FldMap::iterator it;

   for ( it=_flds.begin(); it!=_flds.end(); it++ )
      delete (*it).second;
   _flds.clear();
}


///////////////////////////////
// Access
///////////////////////////////
const char *Record::pSvc()
{
   return _svc.data();
}

const char *Record::pTkr()
{
   return _tkr.data();
}

int Record::oid()
{
   return _oid;
}

int Record::nFld()
{
   return _flds.size();
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

int Record::TouchAllFields()
{
   FldMap          &v = _flds;
   Field           *fld;
   FldMap::iterator it;

   // _cacheMtx-protected

   for ( it=v.begin(); it!=v.end(); it++ ) {
      fld = (*it).second;
      if ( !fld->_bUpd )
         _upds.push_back( fld );
      fld->_bUpd = true;
   }
   return nFld();
}

int Record::GetUpds( PyObjects &u )
{
   Field    *fld;
   PyObject *pyF, *pyV, *pyT;
   char     *pc;
   int       i, sz, ty;

   // Field Type : { _MDDPY_INT, _MDDPY_DBL, _MDDPY_STR }

   sz = _upds.size();
   for ( i=0; i<sz; i++ ) {
      fld = _upds[i];
      pc  = fld->data();
      pyF = PyInt_FromLong( fld->Fid() );
      pyV = fld->GetValue( ty );
      pyT = PyInt_FromLong( ty );
      u.push_back( PyTuple_Pack( 3, pyF, pyV, pyT ) );
      Py_DECREF( pyF );
      Py_DECREF( pyV );
      fld->ClearUpd();
   }
   _upds.clear();
   return u.size();
}


///////////////////////////////
// Real-Time Data
///////////////////////////////
void Record::Update( mddFieldList fl )
{
   mddField  f;
   mddField *fdb = fl._flds;
   Field    *fld;
   int       i, fid;

   // _cacheMtx-protected

   _tUpd = fl._nFld ? dNow() : _tUpd;
   for ( i=0; i<fl._nFld; i++ ) {
      f     = fdb[i];
      fid   = f._fid;
      if ( !(fld=GetField( fid )) ) {
         fld        = new Field( f );
         _flds[fid] = fld;
         _upds.push_back( fld );
      }
      else {
         if ( !fld->_bUpd )
            _upds.push_back( fld );
         fld->Update( f );
      }
   }
   _nUpd += 1;
}



////////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      S c h e m a
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
Schema::Schema() :
   _mtx(),
   _flds()
{
}

Schema::~Schema()
{
   Clear();
}


///////////////////////////////
// Access / Operations
///////////////////////////////
int Schema::GetUpds( PyObjects &u )
{
   RTEDGE::Locker l( _mtx );
   MDDPY::Field  *fld;
   PyObject      *pyF, *pyV,*pyT;
   char          *pc;
   int            i, sz, fid;
   mddFldType     ty;

   sz = _flds.size();
   for ( i=0; i<sz; i++ ) {
      fld = _flds[i];
      fid = fld->Fid();
      pc  = fld->data();
      ty  = fld->type();
      pyF = PyInt_FromLong( fid );
      pyV = PyString_FromString( pc );
      pyT = PyInt_FromLong( ty );
      u.push_back( PyTuple_Pack( 3, pyF, pyV, pyT ) );
      Py_DECREF( pyF );
      Py_DECREF( pyV );
      Py_DECREF( pyT );
   }
   return u.size();

}

void Schema::Update( rtEdgeData &d )
{
   RTEDGE::Locker l( _mtx );
   MDDPY::Record *rec;
   mddField      *fdb;
   int            i, nf;

   // Clear out existing; Add anew       

   Clear();
   rec = (MDDPY::Record *)0;
   fdb = (mddField *)d._flds;
   nf  = d._nFld;
   for ( i=0; i<nf; _flds.push_back( new Field( fdb[i++], true ) ) );
}

void Schema::Clear()
{
   RTEDGE::Locker    l( _mtx );
   FldList::iterator it;

   for( it=_flds.begin(); it!=_flds.end(); delete (*it),it++ );
   _flds.clear();
}


