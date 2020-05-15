/******************************************************************************
*
*  DataListener.hpp
*     libyamr Reader Listener Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*      4 NOV 2019 jcs  Build  3: protected OnMessage()
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_DataListener_H
#define __YAMR_DataListener_H
#include <hpp/data/Data.hpp>

namespace YAMR
{

namespace Data
{

////////////////////////////////////////////////
//
//      c l a s s    D a t a L i s t e n e r
//
////////////////////////////////////////////////

/**
 * \class DataListener
 * \brief Listener for Reader events of known structured data protocols
 *
 * The following pre-packaged structured data types are supported:
 * Class | Method
 * --- | ---
 * DoubleList | OnDoubleList()
 * FieldList | OnFieldList()
 * FloatList | OnFloatList()
 * IntList | OnIntList()
 * StringDict | OnStringDict()
 * StringList | OnStringList()
 * StringMap | OnStringMap()
 */
class DataListener : public IReadListener
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
	/**
	 * \brief Constructor
	 *
	 * \param reader - Tape Reader driving messages into us
	 */
public:
	DataListener( Reader &reader ) :
	   _reader( reader )
	{
	   reader.AddListener( *this );
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/** 
	 * \brief Return Reader associated with this DataListener
	 *
	 * \return Reader associated with this DataListener
	 */
	Reader &reader()
	{
	   return _reader;
	}


	////////////////////////////////////
	// DataListener Interface
	////////////////////////////////////
public:
	/** 
	 * \brief Called when Reader reads DoubleList
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded DoubleList
	 */
	virtual void OnDoubleList( yamrMsg &msg, DoubleList *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads FieldList
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded FieldList
	 */
	virtual void OnFieldList( yamrMsg &msg, FieldList *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads FloatList
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded FloatList
	 */
	virtual void OnFloatList( yamrMsg &msg, FloatList *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads IntList
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded IntList
	 */
	virtual void OnIntList( yamrMsg &msg, IntList *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads StringDict
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded StringDict
	 */
	virtual void OnStringDict( yamrMsg &msg, StringDict *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads StringList
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded StringList
	 */
	virtual void OnStringList( yamrMsg &msg, StringList *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads StringMap
	 *
	 * \param msg - Raw message from tape
	 * \param codec - Decoded StringMap
	 */
	virtual void OnStringMap( yamrMsg &msg, StringMap *codec )
	{ ; }

	/** 
	 * \brief Called when Reader reads yamrMsg w/ unknown protocol
	 *
	 * \param msg - Raw message from tape
	 */
	virtual void OnUnknown( yamrMsg &msg )
	{ ; }


	////////////////////////////////////
	// IReadListener Interface
	////////////////////////////////////
protected:
	/** 
	 * \brief Called when Reader reads message
	 *
	 * \param reader - Reader driving the event
	 * \param msg - Message
	 * \param codec - Decodec Message, if found; NULL if unknown protocol
	 */
	virtual void OnMessage( Reader &reader, yamrMsg &msg, Data::Codec *codec )
	{
	   u_int16_t proto;

	   // 1) Unknown Protocol??

	   if ( !codec ) {
	      OnUnknown( msg );
	      return;
	   }

	   // 2) Currently Supported Protocols

	   proto = msg._MsgProtocol;
	   switch( proto ) {
	      case _PROTO_STRINGDICT:
	         OnStringDict( msg, (StringDict *)codec );
	         break;
	      case _PROTO_STRINGLIST:
	         OnStringList( msg, (StringList *)codec );
	         break;
	      case _PROTO_STRINGMAP:
	         OnStringMap( msg, (StringMap *)codec );
	         break;
	      case _PROTO_INTLIST8:
	      case _PROTO_INTLIST16:
	      case _PROTO_INTLIST32:
	         OnIntList( msg, (IntList *)codec );
	         break;
	      case _PROTO_FLOATLIST:
	         OnFloatList( msg, (FloatList *)codec );
	         break;
	      case _PROTO_DOUBLELIST:
	         OnDoubleList( msg, (DoubleList *)codec );
	         break;
	      case _PROTO_FIELDLIST:
	         OnFieldList( msg, (FieldList *)codec );
	         break;
	   }
	}

	////////////////////////////////////
	// Private Members
	////////////////////////////////////
protected:
	Reader &_reader;

}; // class DataListener

} // namespace Data

} // namespace YAMR

#endif // __YAMR_DataListener_H
