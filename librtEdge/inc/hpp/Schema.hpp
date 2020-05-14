/******************************************************************************
*
*  Schema.hpp
*     librtEdge subscription Schema
*
*  REVISION HISTORY:
*     15 SEP 2014 jcs  Created.
*      4 MAY 2015 jcs  Build 31: Fully-qualified std:: (compiler)
*      8 FEB 2016 jcs  Build 33: mddFieldList *Get()
*     15 JAN 2018 jcs  Build 39: Leak - 
*     29 SEP 2018 jcs  Build 41: public field()
*
*  (c) 1994-2018 Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Schema_H
#define __RTEDGE_Schema_H
#include <string>
#include <map>
#include <vector>

#if !defined(InRange)
#define InRange( a,b,c )     ( ((a)<=(b)) && ((b)<=(c)) )
#endif // !defined(InRange)


namespace RTEDGE
{

// Forward declarations

class Field;
class FieldDef;
class Schema;
class SubChannel;

typedef std::map<int, FieldDef *>         FldDefByIdMap;
typedef std::map<std::string, FieldDef *> FldDefByNameMap;
typedef std::vector<std::string>          VecString;


////////////////////////////////////////////////
//
//         c l a s s   F i e l d D e f
//
////////////////////////////////////////////////

/**
 * \class FieldDef
 *
 * \brief A single field definition in the Schema.
 */
class FieldDef
{
friend class Schema;
friend class SubChannel;
private:
	Schema     &_schema;
	rtFIELD     _mdd;
	std::string _name;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
protected:
	FieldDef( Schema &schema, rtFIELD f ) :
	   _schema( schema ),
	   _name( f._name )
	{
	   rtBUF &b = _mdd._val._buf;

	   _mdd    = f;
	   b._data = (char *)_name.c_str();
	}

	~FieldDef() { ; }


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return Field ID
	 *
	 * \return Field ID
	 */
	int Fid()
	{
	   return _mdd._fid;
	}

	/**
	 * \brief Return Field Name
	 *
	 * \return Field Name
	 */
	const char *pName()
	{
	   rtBUF &b = _mdd._val._buf;

	   return b._data;
	}

	/**
	 * \brief Return Field Type
	 *
	 * \return Field Type
	 */
	rtFldType fType()
	{
	   return _mdd._type;
	}


	////////////////////////////////////
	// Access - protected
	////////////////////////////////////
#ifndef DOXYGEN_OMIT
protected:
	mddField mdd()
	{
	   mddField *rtn;

	   rtn = (mddField *)&_mdd;
	   return *rtn;
	}
#endif // DOXYGEN_OMIT
public:
	rtFIELD &field()
	{
	   return _mdd;
	}

}; // class FieldDef



////////////////////////////////////////////////
//
//         c l a s s   S c h e m a
//
////////////////////////////////////////////////

/**
 * \class Schema
 * \brief This class contains the SubChannel data dictionary
 */
class Schema
{
friend class LVC;
friend class PubChannel;
friend class SubChannel;
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
protected:
	/**
	 * \brief The (one) Schema associated with a Channel
	 *
	 * SubChannel calls Initialize() to build the Schema
	 */
	Schema() :
	   _ddb( (FieldDef **)0 ),
	   _minFid( 0x7fffffff ),
	   _maxFid( 0 ),
	   _fld(),
	   _gfifId(),
	   _it(),
	   _gfifStr()
	{
	   ::memset( &_fl, 0, sizeof( _fl ) );
	}

	~Schema()
	{
	   _Clear();
	}

	/**
	 * \brief Initialize (or re-initialize) the Schema
	 *
	 * \param msg - Schema definition in a Message object
	 */
	void Initialize( Message &msg )
	{
	   Field      *fld;
	   const char *pn;
	   FieldDef   *def;
	   char       *rp;
	   std::string s;
	   int         i, fid;

	   _Clear();
	   for ( msg.reset(); (msg)(); ) {
	      fld  = msg.field();
	      def  = new FieldDef( *this, fld->field() );
	      pn   = def->pName();
	      fid  = def->Fid();
	      s    = def->pName();
	      if ( !GetDef( pn ) && !GetDef( fid ) ) {
	         _gfifId[fid] = def;
	         _gfifStr[s]  = def;
	         _minFid      = gmin( _minFid, fid );
	         _maxFid      = gmax( _maxFid, fid );
	      }
	      else
	         delete def;
	   }

	   // Array / FieldList

	   FldDefByIdMap           &vdb = _gfifId;
	   FldDefByIdMap::iterator  it;
	   int                      rng, sz, rFid;

	   ::mddFieldList_Free( _fl );
	   ::memset( &_fl, 0, sizeof( _fl ) );
	   rng = ( _maxFid - _minFid );
	   if ( rng <= 0 )
	      return;
	   _fl  = ::mddFieldList_Alloc( Size() );
	   rng += 1;
	   sz   = rng * sizeof( FieldDef * );
	   rp   = new char[sz];
	   _ddb = (FieldDef **)rp;
	   ::memset( rp, 0, sz );
	   for ( i=0,it=vdb.begin(); it!=vdb.end(); i++,it++ ) {
	      fid          = (*it).first;
	      rFid         = fid - _minFid;
	      def          = (*it).second;
	      _ddb[rFid]   = def;
	      _fl._flds[i] = def->mdd();
	   }
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	int Size()
	{
	   return _gfifId.size();
	}

	mddFieldList *Get()
	{
	   return &_fl;
	}

	FieldDef *GetDef( int fid )
	{
	   FldDefByIdMap          &v = _gfifId;
	   FldDefByIdMap::iterator it;
	   FieldDef               *def;
	   int                     rFid;

	   // Quickest : Array; Next quickest : std::map

	   def  = (FieldDef *)0;
	   rFid = fid - _minFid;
	   if ( _ddb && InRange( _minFid, fid, _maxFid ) )
	      def = _ddb[rFid];
	   else if ( (it=v.find( fid )) != v.end() )
	      def = (*it).second;
	   return def;
	}

	FieldDef *GetDef( const char *pFld )
	{
	   FldDefByNameMap          &v = _gfifStr;
	   FldDefByNameMap::iterator it;
	   std::string               s( pFld );
	   FieldDef                 *def;

	   def = (FieldDef *)0;
	   if ( (it=v.find( s )) != v.end() )
	      def = (*it).second;
	   return def;
	}


	////////////////////////
	// Iteration
	////////////////////////
	void reset()
	{
	   _it = _gfifId.begin();
	}

	Field *operator()()
	{
	   _it++;
	   return field();
	}

	Field *field()
	{
	   rtFIELD   f;
	   FieldDef *def;

	   if ( _it != _gfifId.end() ) {
	      def = (*_it).second;
	      f   = def->field();
	      return &_fld.Set( *this, f );
	   }
	   return (Field *)0;
	}


	////////////////////////
	// Helpers
	////////////////////////
private:
	void _Clear()
	{
	   FldDefByIdMap          &v = _gfifId;
	   FldDefByIdMap::iterator ft;
	   char                   *bp;

	   for ( ft=v.begin(); ft!=v.end(); delete (*ft).second,ft++ );
	   _gfifId.clear();
	   _gfifStr.clear();
	   if ( (bp=(char *)_ddb) )
	      delete[] bp;
	   ::mddFieldList_Free( _fl );
	   ::memset( &_fl, 0, sizeof( _fl ) );
	   _ddb = (FieldDef **)0;
	}

	////////////////////////
	// private Members
	////////////////////////
private:
	mddFieldList    _fl;
	FieldDef      **_ddb;
	int             _minFid;
	int             _maxFid;
	Field           _fld;
	FldDefByIdMap   _gfifId;
	FldDefByIdMap::iterator _it;
	FldDefByNameMap _gfifStr;

};  // class Schema

} // namespace RTEDGE

#endif // __LIBRTEDGE_Schema_H 
