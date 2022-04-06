/******************************************************************************
*
*  rtMessage.hpp
*     librtEdge subscription Message
*
*  REVISION HISTORY:
*     15 SEP 2014 jcs  Created.
*     11 APR 2015 jcs  Build 31: arg(); rawData() / rawLen()
*      5 MAR 2016 jcs  Build 32: Dump(); edg_permQuery
*     12 OCT 2017 jcs  Build 36
*      8 FEB 2020 jcs  Build 42: IntMap for _dataLVC
*      3 SEP 2020 jcs  Build 44: IsRecovering(); StreamID()
*     17 SEP 2020 jcs  Build 45: SetParseOnly
*      3 DEC 2020 jcs  Build 47: TapePos()
*      5 APR 2022 jcs  Build 52: core : MsgTime() w/ LVC
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Message_H
#define __RTEDGE_Message_H
#include <hpp/rtEdge.hpp>

typedef hash_map<int, int> IntMap;

namespace RTEDGE
{

// Forward Declarations

class LVC;
class LVCAll;
class PubChannel;
class SubChannel;

////////////////////////////////////////////////
//
//         c l a s s   M e s s a g e
//
////////////////////////////////////////////////

/**
 * \class Message
 * \brief This class encapsulates the rtEdgeData message structure.
 *
 * This is a single market data update from the SubChannel subscription 
 * channel from rtEdgeCache3.
 */
class Message : public rtEdge
{
friend class LVC;
friend class LVCAll;
friend class PubChannel;
friend class SubChannel;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
protected:
	/**
	 * \brief The (one) Message associated with a SubChannel.
	 *
	 * \param cxt - Context for our SubChannel
	 * \param bLVC - true if sourced from LVC; false if from Edge2 channel
	 */
	Message( rtEdge_Context cxt, bool bLVC=false ) :
	   _cxt( cxt ),
	   _fld(),
	   _itr( -1 ),
	   _data( (rtEdgeData *)0 ),
	   _dataLVC( (LVCData *)0 ),
	   _offsLVC(),
	   _schema( (mddFieldList *)0 ),
	   _dump(),
	   _parseFids( (IntMap *)0 )
	{
	   rtFIELD _z;

	   ::memset( &_z, 0, sizeof( _z ) );
	   _fld.Set( *this, IsLVC(), _cxt, _z, rtFld_undef );
	}

	virtual ~Message()
	{
	   _ClearFidMap();
	}

	/**
	 * \brief Called by SubChannel to reuse this message
	 *
	 * \param data - New rtEdgeData struct
	 * \param schema - New rtEdgeData struct
	 */ 
	Message &Set( rtEdgeData *data, mddFieldList *schema )
	{
	   _data    = data;
	   _dataLVC = (LVCData *)0;
	   _schema  = schema;
	   _offsLVC.clear();
	   _BuildFidMap();
	   reset();
	   return *this;
	}

	/**
	 * \brief Called by LVC to reuse this message
	 *
	 * \param data - New LVCData struct
	 * \param schema - (Optional) Schema in a Field List
	 */ 
	Message &Set( LVCData *data, mddFieldList *schema=(mddFieldList *)0 )
	{
	   _data    = (rtEdgeData *)0;
	   _dataLVC = data;
	   _schema  = schema;
	   _offsLVC.clear();
	   reset();
	   return *this;
	}

	/**
	 * \brief Called by Tape-based SubChannel.Parse() 
	 *
	 * \param bParseOnly - true to set; false to disable 
	 */ 
	void SetParseOnly( bool bParseOnly )
	{
	   if ( _parseFids )
	      delete _parseFids;
	   _parseFids = bParseOnly ? new IntMap() : (IntMap *)0;
	}


	////////////////////////////////////
	// Access - Channel / Message
	////////////////////////////////////
public:
	/**
	 * \brief Returns rtEdge_Context from SubChannel supplying this Message
	 *
	 * \return rtEdge_Context from SubChannel supplying this Message
	 */
	rtEdge_Context cxt()
	{
	   return _cxt;
	}

	/**
	 * \brief Returns true if sourced from LVC
	 *
	 * \return true if source from LVC
	 */
	bool IsLVC()
	{
	   return( _dataLVC != (LVCData *)0 );
	}

	/**
	 * \brief Returns Message time
	 *
	 * \return Message Time
	 */
	double MsgTime()
	{
	   double rc;

	   if ( IsLVC() ) {
	      rc  = dataLVC()._tUpdUs;
	      rc *= 0.000001;
	      rc += dataLVC()._tUpd;
	   }
	   else
	      rc = data()._tMsg;
	   return rc;
	}

	/**
	 * \brief Returns Stream ID
	 *
	 * \return Stream ID
	 */
	int StreamID()
	{
	   return data()._StreamID;
	}

	/**
	 * \brief Returns Tape Position, if pumping from tape
	 *
	 * \return Tape Position, if pumping from tape
	 */
	u_int64_t TapePos()
	{
	   return data()._TapePos;
	}

	/**
	 * \brief Returns service associated with this message
	 *
	 * \return Service associated with this message
	 */
	const char *Service()
	{
	   return IsLVC() ? dataLVC()._pSvc : data()._pSvc;
	}

	/**
	 * \brief Returns ticker associated with this message
	 *
	 * \return Ticker associated with this message
	 */
	const char *Ticker()
	{
	   return IsLVC() ? dataLVC()._pTkr : data()._pTkr;
	}

	/**
	 * \brief Returns error associated with this message
	 *
	 * \return Error associated with this message
	 */
	const char *Error()
	{
	   return IsLVC() ? dataLVC()._pErr : data()._pErr;
	}

	/**
	 * \brief Returns the user-supplied argument for this message
	 *
	 * \return The user-supplied argument for this message
	 */
	void *arg()
	{
	   return IsLVC() ? (void *)0 : data()._arg;
	}

	/**
	 * \brief Returns Message type - edg_image, etc.
	 *
	 * \return Message type - edg_image, etc.
	 */
	rtEdgeType mt()
	{
	   return IsLVC() ? dataLVC()._ty : data()._ty;
	}

	/**
	 * \brief  Returns textual name of this message
	 *
	 * \return Textual name of this message
	 */
	const char *MsgType()
	{
	   rtEdgeType ty;

	   ty = mt();
	   switch( ty ) {
	      case edg_image:      return "IMAGE  ";
	      case edg_update:     return "UPDATE ";
	      case edg_stale:      return "STALE  ";
	      case edg_recovering: return "RECOVER";
	      case edg_dead:       return "DEAD   ";
	      case edg_permQuery:  return "QUERY  ";
	      case edg_streamDone: return "DONE   ";
	   }
	   return "UNKNOWN";
	}

	/**
	 * \brief Returns Field List in this message
	 *
	 * \return Field Field List in this message
	 */
	rtFIELD *Fields()
	{
	   return IsLVC() ? dataLVC()._flds : data()._flds;
	}

	/**
	 * \brief Returns size of Field List in this message
	 *
	 * \return Size of Field List in this message
	 */
	int NumFields()
	{
	   return IsLVC() ? dataLVC()._nFld : data()._nFld;
	}

	/**
	 * \brief Returns raw size of this message on the wire
	 *
	 * \return Raw size of this message on the wire
	 */
	int Size()
	{
	   return rawData()._dLen;
	}

	/**
	 * \brief Returns the raw data contents
	 *
	 * \return The user-supplied argument for this message
	 */
	rtBUF rawData()
	{
	   rtBUF b;

	   ::memset( &b, 0, sizeof( b ) );
	   if ( !IsLVC() ) {
	      b._data = (char *)data()._rawData;
	      b._dLen = data()._rawLen;
	   }
	   return b;
	}

	/**
	 * \brief Returns underlying rtEdgeData struct
	 *
	 * \return Underlying rtEdgeData struct
	 */
	rtEdgeData &data()
	{
	   return *_data;
	}

	/**
	 * \brief Returns underlying LVCData struct
	 *
	 * \return Underlying LVCData struct
	 */
	LVCData &dataLVC()
	{
	   return *_dataLVC;
	}

	/**
	 * \brief Returns true if this mesage is an IMAGE
	 *
	 * \return true if Message is an IMAGE
	 */
	bool IsImage()
	{
	   rtEdgeType ty;

	   ty = IsLVC() ? dataLVC()._ty : data()._ty;
	   return( ty == edg_image );
	}

	/**
	 * \brief Returns true if this mesage is an UPDATE
	 *
	 * \return true if Message is an UPDATE
	 */
	bool IsUpdate()
	{
	   rtEdgeType ty;

	   ty = IsLVC() ? dataLVC()._ty : data()._ty;
	   return( ty == edg_update );
	}

	/**
	 * \brief Returns true if the stream is RECOVERING
	 *
	 * \return true if the stream is RECOVERING
	 */
	bool IsRecovering()
	{
	   rtEdgeType ty;

	   ty = IsLVC() ? dataLVC()._ty : data()._ty;
	   return( ty == edg_recovering );
	}

	/**
	 * \brief Returns true if the stream is STALE
	 *
	 * \return true if the stream is STALE
	 */
	bool IsStale()
	{
	   rtEdgeType ty;

	   ty = IsLVC() ? dataLVC()._ty : data()._ty;
	   return( ty == edg_stale );
	}

	/**
	 * \brief Returns true if this mesage is DEAD
	 *
	 * \return true if Message is DEAD
	 */
	bool IsDead()
	{
	   rtEdgeType ty;

	   ty = IsLVC() ? dataLVC()._ty : data()._ty;
	   return( ty == edg_dead );
	}

	/**
	 * \brief Returns true if the stream is completed / closed
	 *
	 * \return true if the stream is completed / closed
	 */
	bool IsStreamDone()
	{
	   rtEdgeType ty;

	   ty = IsLVC() ? dataLVC()._ty : data()._ty;
	   return( ty == edg_streamDone );
	}


	////////////////////////////////////
	// Access - Iterate All Fields
	////////////////////////////////////
public:
	/**
	 * \brief Resets the iterator to the beginning
	 *
	 * \include Message_iterate.hpp
	 **/
	void reset()
	{
	   _itr = -1;
	}

	/**
	 * \brief Advances the iterator to the next field and returns it. 
	 *
	 * \include Message_iterate.hpp
	 *
	 * \return Next field in the message, else NULL if end of message. 
	 **/
	Field *operator()()
	{
	   _itr++;
	   return field();
	}

	/**
	 * \brief Return Field at the current iterator position
	 *
	 * \include Message_iterate.hpp
	 *
	 * \return Field at current iterator position; NULL if end of message.
	 **/
	Field *field()
	{
	   rtFIELD   *fdb;
	   mddField  *sdb;
	   rtFIELD    f;
	   mddFldType ty;

	   fdb = IsLVC() ? dataLVC()._flds : data()._flds;
	   sdb = _schema ? _schema->_flds : (mddField *)0;
	   if ( _itr < NumFields() ) {
	      f  = fdb[_itr];
	      ty = sdb ? sdb[_itr]._type : (mddFldType)f._type;
	      return &_fld.Set( *this, IsLVC(), _cxt, f, (rtFldType)ty );
	   }
	   return (Field *)0;
	}

	/**
	 * \brief Dumps message contents as string
	 *
	 * \return Field contents as string
	 */
	const char *Dump()
	{
	   char        buf[16*K], *cp;
	   const char *pd, *pm;
	   std::string dt;

	   cp  =  buf;
	   pd  = pDateTimeMs( dt, MsgTime() );
	   pm  = MsgType();
	   cp += sprintf( cp, "%s {%s} (%s,%s)\n", pd, pm, Service(), Ticker() ); 
	   for ( reset(); (*this)(); ) {
	      cp += field()->Dump( cp, true );
	      cp += sprintf( cp, "\n" );
	   }
	   cp   += sprintf( cp, "\n" );
	   _dump = buf;
	   return _dump.data();
	}


	////////////////////////////////////
	// Access - Specific Field
	////////////////////////////////////
public:
	/**
	 * \brief Determine if this message contains requested field (by name)
	 *
	 * \param fieldName - Schema name of field to retrieve
	 * \return true if found
	 */
	bool HasField( const char *fieldName )
	{
	   return _fld.Has( fieldName );
	}

	/**
	 * \brief Determine if this message contains requested field (by ID)
	 *
	 * \param fid -  ID of field to retrieve 
	 * \return true if found
	 */
	bool HasField( int fid )
	{
	   return _fld.Has( fid );
	}

	/**
	 * \brief Retrieve and return requested field
	 *
	 * \param fieldName - Schema name of field to retrieve
	 * \return Field if found; Else null
	 */
	Field *GetField( const char *fieldName )
	{
	   bool bOK;

	   bOK = !_dataLVC && ( _fld.Get( fieldName ).Fid() != 0 );
	   return bOK ? &_fld : (Field *)0;
	}

	/**
	 * \brief Retrieve and return requested field
	 *
	 * \param fid -  ID of field to retrieve 
	 * \return Field if found; Else null
	 */
	Field *GetField( int fid )
	{
	   IntMap          &odb = _offsLVC;
	   IntMap::iterator it;
	   bool             bOK;
	   rtFIELD         *fdb, f;
	   size_t           no;
	   int              i, nf, off, ix;

	   // Build _offsLVC once, then search

	   bOK = false;
	   if ( _data ) {
	      if ( _parseFids ) {
	         IntMap &idb = *_parseFids;

	         bOK = ( (it=idb.find( fid )) != idb.end() );
	         if ( bOK ) {
	            off = (*it).second;
	            fdb = _data->_flds;
	            nf  = _data->_nFld;
	            f   = fdb[off];
	            _fld.Set( *this, false, _cxt, f, f._type );
	         }
	      }
	      else
	         bOK = ( _fld.Get( fid ).Fid() != 0 );
	   }
	   else if ( _dataLVC ) {
	      /*
	       * Once per message
	       */
	      fdb = _dataLVC->_flds;
	      nf  = _dataLVC->_nFld;
	      no  = odb.size();
	      for ( i=0; !no && i<nf; i++ ) {
	         ix      = fdb[i]._fid;
	         odb[ix] = i;
	      }
	      /*
	       * Query
	       */
	      bOK = ( (it=odb.find( fid )) != odb.end() );
	      if ( bOK ) {
	         off = (*it).second;
	         f   = fdb[off];
	         _fld.Set( *this, true, f );
	      }
	   }
	   return bOK ? &_fld : (Field *)0;
	}


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	void _BuildFidMap()
	{
	   int i, fid;

	   // Pre-condition

	   if ( !_parseFids || !_data )
	      return;

	   // OK to continue

	   IntMap     &idb = *_parseFids;
	   rtEdgeData &d  = *_data;

	   _ClearFidMap();
	   for ( i=0; i<d._nFld; i++ ) {
	      fid      = d._flds[i]._fid;
	      idb[fid] = i;
	   }
	}

	void _ClearFidMap()
	{
	   if ( _parseFids )
	      _parseFids->clear();
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	rtEdge_Context _cxt;
	Field          _fld;
	int            _itr;
	rtEdgeData    *_data;
	LVCData       *_dataLVC;
	IntMap         _offsLVC;
	mddFieldList  *_schema;
	std::string    _dump;
	IntMap        *_parseFids;

};  // class Message

} // namespace RTEDGE

#endif // __LIBRTEDGE_Message_H 
