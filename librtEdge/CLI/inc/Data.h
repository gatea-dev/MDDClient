/******************************************************************************
*
*  Data.h
*
*  REVISION HISTORY:
*     17 SEP 2014 jcs  Created.
*      8 JAN 2015 jcs  Build 29: mt() / MsgType().
*     23 FEB 2015 jcs  Build 30: array<Byte> _raw, not String
*      6 JUL 2015 jcs  Build 31: rtBuF
*     11 FEB 2016 jcs  Build 32: LVCDataAll.IsBinary; _InitHeap()
*     15 OCT 2017 jcs  Build 36: Dump()
*     11 JAN 2018 jcs  Build 39: Leak : _FreeHeap()
*     10 DEC 2018 jcs  Build 41: VS2017
*      9 FEB 2020 jcs  Build 42: GetColumnAsXxx()
*      3 SEP 2020 jcs  Build 44: _MsgTime; _StreamID
*
*  (c) 1994-2020 Gatea Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#include <Field.h>
#endif // DOXYGEN_OMIT

namespace librtEdge 
{

////////////////////////////////////////////////
//
//       c l a s s   r t E d g e D a t a
//
////////////////////////////////////////////////

/**
 * \class rtEdgeData
 * \brief A single market data update from the rtEdgeSubscriber 
 * channel.
 *
 * This class is reused by rtEdgeSubscriber.  When you receive it
 * in rtEdgeSubscriber::OnData(), it is volatile and only valid 
 * for the life of the callback.
 */
public ref class rtEdgeData
{
private: 
	RTEDGE::Message      *_msg;
	rtEdgeType            _mt;
	u_int                 _NumFld;
	::rtEdgeData         *_data;
	String               ^_svc;
	String               ^_tkr;
	String               ^_err;
	array<Byte>          ^_raw;
	array<rtEdgeField ^> ^_fdb;
	rtEdgeField          ^_fld;
	array<rtEdgeField ^> ^_heap;
	Hashtable            ^_cachedFields;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/**
	 * \brief Reusable message object constructor
	 *
	 * This (reusable) object is created once by the rtEdgeSubscriber
	 * and reused on every market data update sent to you via  
	 * rtEdgeSubscriber::OnData() as follows:
	 *
	 * \include rtEdgeData_OnData.h
	 */
public:
	rtEdgeData();

	/**
	 * \brief Copy constructor
	 *
	 * Makes deep copy of data; Useful for crossing thread boundaries
	 * \param src - Source rtEdgeData to copy
	 */
public:
	rtEdgeData( rtEdgeData ^src );

	/** \brief Destructor */
	~rtEdgeData();


	////////////////////////////////////
	// Access - Iterate All Fields
	////////////////////////////////////
public:
	/**
	 * \brief Advances the iterator to the next field.
	 *
	 * \include rtEdgeData_iterate.h
	 *
	 * \return true if not end of message; false if EOM.
	 */
	bool forth();

	/**
	 * \brief Return Field at the current iterator position
	 *
	 * \include rtEdgeData_iterate.h
	 *
	 * \return Field at the current iterator position
	 */
	rtEdgeField ^field();


	////////////////////////////////////
	// Access - Misc
	////////////////////////////////////
public:
	/**
	 * \brief Return RTEDGE::Message populating this object
	 *
	 * \return RTEDGE::Message populating this object
	 */
	RTEDGE::Message *msg();

	/**
	 * \brief Retrieve and return requested field by FID
	 *
	 * \param fid - Requested Field ID 
	 * \return rtEdgeField if found; null otherwise
	 */
	rtEdgeField ^GetField( int fid );

	/**
	 * \brief Returns Message Type
	 *
	 * \return Message type
	 */
	rtEdgeType mt()
	{
	   return (rtEdgeType)_msg->mt();
	}

	/**
	 * \brief Returns textual name of this Message
	 *
	 * \return Textual name of this Message
	 */
	String ^MsgType()
	{
	   return gcnew String( _msg->MsgType() );
	}

	/**
	 * \brief Dumps message contents as string
	 *
	 * \return Message contents as string
	 */
	String ^Dump()
	{
	   return gcnew String( _msg->Dump() );
	}


	/////////////////////////////////
	//  Operations
	/////////////////////////////////
	/**
	 * \brief Called by rtEdgeSubscriber before rtEdgeSubscriber::OnData()
	 * to set the internal state of this reusable messsage.
	 *
	 * \param msg - Current message contents from rtEdgeSubscriber channel.
	 */
	void Set( RTEDGE::Message &msg );

	/**
	 * \brief Clear out internal state so this message may be reused.
	 */
	void Clear();



	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns name of service supplying this update */
	property DateTime ^_MsgTime
	{
	   DateTime ^get() {
	      double r64, mike;
	      long   tv_sec;

	      r64    = _msg->MsgTime();
	      tv_sec = (long)r64;
	      mike   = ( r64  - tv_sec ) * 1000000.0;
	      return rtEdge::FromUnixTime( tv_sec, (long)mike );
	   }
	}

	/** \brief Returns unique Stream ID */
	property int _StreamID
	{
	   int get() { return _msg->StreamID(); }
	}

	/** \brief Returns name of service supplying this update */
	property String ^_pSvc
	{
	   String ^get() {
	      if ( _svc == nullptr )
	         _svc = gcnew String( _msg->Service() );
	      return _svc;
	   }
	}

	/** \brief Returns ticker name of this update */
	property String ^_pTkr
	{
	   String ^get() {
	      if ( _tkr == nullptr )
	          _tkr = gcnew String( _msg->Ticker() );
	      return _tkr;
	   }
	}

	/** \brief Returns error (if any) from this update */
	property String ^_pErr
	{
	   String ^get() {
	      if ( _err == nullptr )
	          _err = gcnew String( _msg->Error() );
	      return _err;
	   }
	}

	/** \brief Returns user-supplied argument from this update */
	property u_int _arg
	{
	   u_int get() { return (u_int)(size_t)_data->_arg; } 
	}

	/** \brief Returns message type */
	property rtEdgeType _ty
	{
	   rtEdgeType get() { return (rtEdgeType)_data->_ty; }
	}

	/** \brief Returns Field List from this update */
	property array<rtEdgeField ^> ^_flds
	{
	   array<rtEdgeField ^> ^get() {
	      rtEdgeField f;
	      u_int       i, nf;

	      // Create once per message; Grow _heap as required ...

	      nf = _nFld;
	      if ( _fdb == nullptr ) {
	         _CheckHeap( nf );
	         _fdb = gcnew array<rtEdgeField ^>( nf );
	         _msg->reset();
	         for ( i=0; i<nf && forth(); i++ ) {
	            _heap[i]->Copy( _msg->field() );
	            _fdb[i] = _heap[i];
	         }
	      } 
	      return _fdb;
	   }
	}

	/** \brief Returns number of fields in this update */
	property u_int _nFld
	{
	   u_int get() { return _NumFld; }
	}

	/** \brief Returns raw message */
	property array<byte> ^_rawData
	{
	   array<byte> ^get() {
	      if ( _raw == nullptr ) {
	         ::rtBUF       b;
	          ::rtEdgeData &d = _msg->data();

	         b._data = (char *)d._rawData;
	         b._dLen = d._rawLen;
	          _raw    = rtEdge::_memcpy( b );
	      }
	      return _raw;
	   }
	}


	/////////////////////////////////
	//  Helpers
	/////////////////////////////////
private:
	void _CheckHeap( int nf );
	void _FreeHeap();

};  // class rtEdgeData


////////////////////////////////////////////////
//
//         c l a s s   L V C D a t a
//
////////////////////////////////////////////////

/**
 * \class LVCData
 * \brief View of a single market data record in the Last Value Cache (LVC)
 *
 * This class is reused by LVC via LVC::Snap() and LVC::View().
 */
public ref class LVCData
{
private: 
	RTEDGE::Message      *_msg;
	::LVCData            *_data;
	String               ^_svc;
	String               ^_tkr;
	String               ^_err;
	array<rtEdgeField ^> ^_fdb;
	rtEdgeField          ^_fld;
	array<rtEdgeField ^> ^_heap;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/**
	 * \brief Reusable message object constructor
	 *
	 * This (reusable) object is created once by the LVC and reused on 
	 * every LVC::View() / LVC::Snap() call.
	 *
	 * \include LVCData_OnData.h
	 */
public:
	LVCData();
	~LVCData();


	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/**
	 *\brief Returns true if LVC values are binary
	 *
	 *\return true if LVC values are binary
	 */
	property bool IsBinary
	{
	   bool get() { return _data ? _data->_bBinary : false; }
	}



	////////////////////////////////////
	// Access - Iterate All Fields
	////////////////////////////////////
public:
	/**
	 * \brief Advances the iterator to the next field.
	 *
	 * \include LVCData_iterate.h
	 *
	 * \return true if not end of message; false if EOM.
	 */
	bool forth();

	/**
	 * \brief Return Field at the current iterator position
	 *
	 * \include LVCData_iterate.h
	 *
	 * \return Field at the current iterator position
	 */
	rtEdgeField ^field();


	////////////////////////////////////
	// Access - Misc
	////////////////////////////////////
public:
	/**
	 * \brief Return RTEDGE::Message populating this object
	 *
	 * \return RTEDGE::Message populating this object
	 */
	RTEDGE::Message *msg();

	/**
	 * \brief Retrieve and return requested field by FID
	 *
	 * \param fid - Requested Field ID 
	 * \return rtEdgeField if found; null otherwise
	 */
	rtEdgeField ^GetField( int fid );


	/////////////////////////////////
	//  Operations
	/////////////////////////////////
	/**
	 * \brief Called by LVC::Snap() or LVC::View() to set the internal 
	 * state of this reusable messsage.
	 *
	 * \param msg - Current message contents from LVC.
	 */
	void Set( RTEDGE::Message &msg );

	/**
	 * \brief Clear out internal state so this message may be reused.
	 */
	void Clear();



	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns name of service supplying this update */
	property String ^_pSvc
	{
	   String ^get() {
	      if ( _svc == nullptr )
	         _svc = gcnew String( _msg->Service() );
	      return _svc;
	   }
	}

	/** \brief Returns ticker name of this update */
	property String ^_pTkr
	{
	   String ^get() {
	      if ( _tkr == nullptr )
	          _tkr = gcnew String( _msg->Ticker() );
	      return _tkr;
	   }
	}

	/** \brief Returns error (if any) from this update */
	property String ^_pErr
	{
	   String ^get() {
	      if ( _err == nullptr )
	          _err = gcnew String( _msg->Error() );
	      return _err;
	   }
	}

	/** \brief Returns message type */
	property rtEdgeType _ty
	{
	   rtEdgeType get() { return (rtEdgeType)_data->_ty; }
	}

	/** \brief Returns create time in LVC (Unix time) */
	property u_int _tCreate
	{
	   u_int get() { return (u_int)_data->_tCreate; } 
	}

	/** \brief Returns update time in LVC (Unix time) */
	property u_int _tUpd
	{
	   u_int get() { return (u_int)_data->_tUpd; } 
	}

	/** \brief Returns update time micros in LVC */
	property u_int _tUpdUs
	{
	   u_int get() { return (u_int)_data->_tUpdUs; } 
	}

	/** \brief Returns age in LVC in seconds */
	property double _dAge
	{
	   double get() { return _data->_dAge; } 
	}

	/** \brief Returns time ticker became DEAD in LVC (Unix time) */
	property u_int _tDead
	{
	   u_int get() { return (u_int)_data->_tDead; } 
	}

	/** \brief Returns num update in LVC */
	property u_int _nUpd
	{
	   u_int get() { return (u_int)_data->_nUpd; } 
	}

	/** \brief Returns snap time in seconds */
	property double _dSnap
	{
	   double get() { return _data->_dSnap; } 
	}

	/** \brief Returns Field List from this update */
	property array<rtEdgeField ^> ^_flds
	{
	   array<rtEdgeField ^> ^get() {
	      rtEdgeField f;
	      u_int       i, nf;

	      // Create once per message; Grow _heap as required ...

	      nf = _nFld;
	      if ( _fdb == nullptr ) {
	         _CheckHeap( nf );
	         _fdb = gcnew array<rtEdgeField ^>( nf );
	         _msg->reset();
	         for ( i=0; i<nf && forth(); i++ ) {
	            _heap[i]->Copy( _msg->field() );
	            _fdb[i] = _heap[i];
	         }
	      } 
	      return _fdb;
	   }
	}

	/** \brief Returns number of fields in this update */
	property u_int _nFld
	{
	   u_int get() { return (u_int)_msg->NumFields(); }
	}


	/////////////////////////////////
	//  Helpers
	/////////////////////////////////
private:
	void _InitHeap( int nf );
	void _CheckHeap( int nf );
	void _FreeHeap();

};  // class LVCData


////////////////////////////////////////////////
//
//        c l a s s   L V C D a t a A l l
//
////////////////////////////////////////////////

/**
 * \class LVCDataAll
 * \brief View of ALL market data records in the Last Value Cache (LVC)
 *
 * This class is reused by LVC via LVC::SnapAll() and LVC::ViewAll().
 */
public ref class LVCDataAll
{
private:
	RTEDGE::LVCAll   *_all; 
	LVCData          ^_data;
	int               _itr;
	array<LVCData ^> ^_fdb;
	array<LVCData ^> ^_heap;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////

	/**
	 * \brief Reusable message object constructor
	 *
	 * This (reusable) object is created once by the LVC and reused on 
	 * every LVC::View() / LVC::Snap() call.
	 *
	 * \include LVCDataAll_OnData.h
	 */
public:
	LVCDataAll();
	~LVCDataAll();


	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/**
	 *\brief Returns true if LVC values are binary
	 *
	 *\return true if LVC values are binary
	 */
	property bool IsBinary
	{
	   bool get() { return _all ? _all->IsBinary() : false; }
	}


	////////////////////////////////////
	// Access - Specific Record
	////////////////////////////////////
public:
	/**
	 * \brief Returns specific database record
	 *
	 * \param idx - Record index
	 * \return LVCData containing record; nullptr if out of range
	 */
	LVCData ^GetRecord( int idx );


	////////////////////////////////////
	// Access - Iterate All Rows
	////////////////////////////////////
public:
	/** \brief Resets iterator.  */
	void reset();

	/**
	 * \brief Advances the iterator to the next field.
	 *
	 * \include LVCDataAll_iterate.h
	 *
	 * \return true if not end of message; false if EOM.
	 */
	bool forth();

	/**
	 * \brief Return Field at the current iterator position
	 *
	 * \include LVCDataAll_iterate.h
	 *
	 * \return Field at the current iterator position
	 */
	LVCData ^data();


	////////////////////////////////////
	// Access - Column-centric
	////////////////////////////////////
	/**
	 * \brief Query all rows for field, returning values as string
	 *
	 * \param fid Field ID to query
	 * \return Column of field values as string array
	 */
	array<String ^> ^GetColumnAsString( int fid );

	/**
	 * \brief Query all rows for field, returning values as int
	 *
	 * \param fid Field ID to query
	 * \return Column of field values as int array
	 */
	array<int> ^GetColumnAsInt32( int fid );

	/**
	 * \brief Query all rows for field, returning values as double
	 *
	 * \param fid Field ID to query
	 * \return Column of field values as double array
	 */
	array<double> ^GetColumnAsDouble( int fid );


	/////////////////////////////////
	//  Operations
	/////////////////////////////////
public:
	/**
	 * \brief Called by LVC::Snap() or LVC::View() to set the internal 
	 * state of this reusable messsage.
	 *
	 * \param res - Current contents of the LVC
	 */
	void Set( RTEDGE::LVCAll &res );

	/**
	 * \brief Clear out internal state so this message may be reused.
	 */
	void Clear();



	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns name of service supplying this update */
	property double _dSnap
	{
	   double get() { return _all->dSnap(); }
	}

	/** \brief Returns LVCData List from this update */
	property array<LVCData ^> ^_tkrs
	{
	   array<LVCData ^> ^get() {
	      RTEDGE::Messages &mdb = _all->msgs();
	      LVCData           f;
	      u_int             i, nf;

	      // Create once per message; Grow _heap as required ...

	      nf = mdb.size();
	      if ( _fdb == nullptr ) {
	         _CheckHeap( nf );
	         _fdb = gcnew array<LVCData ^>( nf );
	         _data->Clear();
	         for ( i=0; i<nf; i++ ) {
	            _heap[i]->Set( *mdb[i] );
	            _fdb[i] = _heap[i];
	         }
	      } 
	      return _fdb;
	   }
	}

	/** \brief Returns number of tickers in this snapshot */
	property u_int _nTkr
	{
	   u_int get() { return (u_int)_all->Size(); }
	}


	/////////////////////////////////
	//  Helpers
	/////////////////////////////////
private:
	void _InitHeap( int nf );
	void _CheckHeap( int nf );
	void _FreeHeap();

};  // class LVCDataAll

} // namespace librtEdge
