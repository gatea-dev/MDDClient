/******************************************************************************
*
*  Data.h
*
*  REVISION HISTORY:
*     19 JUN 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea, Ltd.
******************************************************************************/
#pragma once
#include <yamr.h>

namespace libyamr 
{

////////////////////////////////////////////////
//
//       c l a s s   y a m r M s g
//
////////////////////////////////////////////////

/**
 * \class yamrMsg
 * \brief A single unstructured message to yamRecorder or from tape
 */
public ref class yamrMsg
{
private: 
	::yamrMsg   *_msg;
	array<Byte> ^_raw;

	/////////////////////////////////
	// Constructor / Destructor
	/////////////////////////////////
public:
	yamrMsg() :
	   _msg( (::yamrMsg *)0 ),
	   _raw( nullptr )
	{ ; }

	~yamrMsg()
	{
	   _msg = (::yamrMsg *)0;
	   _raw  = nullptr;
	}


	/////////////////////////////////
	//  Operations
	/////////////////////////////////
#ifndef DOXYGEN_OMIT
	/**
	 * \brief Returns reference to native C ::yamrMsg struct
	 *
	 * \return Reference to native C ::yamrMsg struct
	 */
	::yamrMsg &cpp()
	{
	   return *_msg;
	}
#endif // DOXYGEN_OMIT

	/**
	 * \brief Called by yamrSubscriber before yamrSubscriber::OnData()
	 * to set the internal state of this reusable messsage.
	 *
	 * \param msg - Current message contents from yamrSubscriber channel.
	 */
	void Set( ::yamrMsg &msg )
	{
	   _msg = &msg;
	   _raw  = nullptr;
	}



	/////////////////////////////////
	// Properties
	/////////////////////////////////
public:
	/** \brief Returns Message time in Nanos since epoch */
	property long _Timestamp
	{
	   long get() { return _msg->_Timestamp; }
	}

	/** \brief Returns IP address of sending client in network byte order */
	property int _Host
	{
	   int get() { return _msg->_Host; }
	}

	/** \brief Returns Client Session ID */
	property short _SessionID
	{
	   short get() { return _msg->_SessionID; }
	}

	/** \brief Returns Message Data Type */
	property short _DataType
	{
	   short get() { return _msg->_DataType; }
	}

	/** \brief Returns Message Message Protocol */
	property short _MsgProtocol
	{
	   short get() { return _msg->_MsgProtocol; }
	}

	/** \brief Returns Message Wire Protocol
	 *
	 * For example, StringMap is IntList on wire)
	 */
	property short _WireProtocol
	{
	   short get() { return _msg->_WireProtocol; }
	}

	/** \brief Returns Message SeqNum */
	property long _SeqNum
	{
	   long get() { return _msg->_SeqNum; }
	}

	property array<byte> ^_Data
	{
	   array<byte> ^get() {
	       _raw   = yamr::_memcpy( _msg->_Data );
	      return _raw;
	   }
	}

};  // class yamrMsg

} // namespace libyamr
