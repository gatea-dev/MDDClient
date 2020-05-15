/******************************************************************************
*
*  Channel.h
*     yamrCache subscription channel
*
*  REVISION HISTORY:
*     11 MAY 2019 jcs  Created
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_CHANNEL_H
#define __YAMR_CHANNEL_H
#include <Internal.h>

#define MAX_FLD 128*K

namespace YAMR_PRIVATE
{

////////////////////////
// Forward declarations
////////////////////////
class Logger;
class Socket;


/////////////////////////////////////////
// yamRecorder Client Channel
/////////////////////////////////////////
class Channel : public Socket
{
protected:
	yamrAttr     _attr;
	yamr_Context _cxt;
	string       _con;
	u_int64_t    _SeqNum;
	u_int64_t    _SessID;

	// Constructor / Destructor
public:
	Channel( yamrAttr, yamr_Context );
	virtual ~Channel();

	// Access / Operations

	yamrAttr     attr();
	yamr_Context cxt();
	int          Send( yamrBuf, u_int16_t, u_int16_t );

	// Socket Interface
protected:
	virtual void OnQLoMark();
	virtual void OnQHiMark();

	// Thread Notifications
protected:
	virtual void OnConnect( const char * );
	virtual void OnDisconnect( const char * );
	virtual void OnRead();

	// TimerEvent Notifications
protected:
	virtual void On1SecTimer();

}; // class Channel

} // namespace YAMR_PRIVATE

#endif // __YAMR_CHANNEL_H
