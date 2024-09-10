/******************************************************************************
*
*  LVCStatMon.hpp
*     LVC Run-time Stats Monitor
*
*  REVISION HISTORY:
*      9 SEP 2024 jcs  Created.
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __LVCStatMon_H
#define __LVCStatMon_H


namespace RTEDGE
{

#ifndef DOXYGEN_OMIT
/////////////////////////////////
// 64-bit
/////////////////////////////////
typedef struct timeval GLtimeval;

/////////////////////////////////////////
// Sink channel stats
/////////////////////////////////////////
class GLmdSinkStats
{
public:
	u_int64_t _nMsg;      // Num msgs read
	u_int64_t _nByte;     // Num bytes read
	GLtimeval _lastMsg;   // Timestamp of last msg
	int       _nOpen;     // Num item open reqs sent
	int       _nClose;    // Num item close reqs sent
	int       _nImg;      // Num image msgs received
	int       _nUpd;      // Num update msgs received
	int       _nDead;     // Num inactive received
	int       _nInsAck;
	int       _nInsNak;
	time_t    _lastConn;
	time_t    _lastDisco;
	int       _nConn;
	long      _iVal[20];
	double    _dVal[20];
	char      _objname[200];
	char      _dstConn[200];
	char      _bUp;
	char      _pad[7];

}; // class GLmdSinkStats

/////////////////////////////////
// GLmdStatsHdr
/////////////////////////////////
class GLmdStatsHdr
{
public:
	u_int     _version;
	u_int     _fileSiz;
	GLtimeval _tStart;
	char      _exe[K];
	char      _build[K];
	int       _nSnk;
	int       _nSrc;

}; // class GLmdStatsHdr

///////////////////////
// MDDirectMon
///////////////////////
class GLlvcStats : public GLmdStatsHdr
{
public:
	GLmdSinkStats _snk;
	GLmdSinkStats _src; // Not Used : Required for FeedMon

}; // class GLlvcStats

class StatSnap
{
public:
	u_int64_t _NumMsg;
	double    _Time;

}; // StatSnap

typedef std::vector<StatSnap> StatSnaps;

#endif  // DOXYGEN_OMIT


////////////////////////////////////////////////
//
//       c l a s s   L V C S t a t M o n
//
////////////////////////////////////////////////

/**
 * \class LVCStatMon
 * \brief LVC Stats File Monitor
 */
class LVCStatMon : public RTEDGE::Channel
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor.
	 *
	 * \param statFile - LVC Stats File to Monitor
	 */
	LVCStatMon( const char *statFile ) :
	   Channel( false ),
	   _statFile( statFile ),
	   _mm( MapFile( statFile, false ) ),
	   _lvc( (GLlvcStats *)0 ),
	   _snkRt()
	{
	   StatSnap z = { 0, 0.0 };

	   if ( IsValidFile() ) {
	      _lvc = (GLlvcStats *)_mm._data;
	      z._NumMsg = _lvc->_snk._nMsg;
	      z._Time   = TimeNs();
	   }
	   for ( int i=0; i<900; _snkRt.push_back( z ), i++ );
	   StartThread();
	}

	/** \brief Destructor */
public:
	virtual ~LVCStatMon()
	{
	   UnmapFile( _mm );
	   StopThread();
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Return true if valid file
	 *
	 * \return Return true if valid file
	 */
	bool IsValidFile()
	{
	   return( _mm._dLen > 0 );
	}

	/**
	 * \brief Snap Stats
	 *
	 * \return rtEdgCacheStats
	 */
	rtEdgeChanStats *Snap( rtEdgeChanStats &rc )
	{
	   GLmdSinkStats &ls = _lvc->_snk;

	   ::memset( &rc, 0, sizeof( rc ) );
	   rc._nMsg      = ls._nMsg;
	   rc._nByte     = ls._nByte;
	   rc._lastMsg   = ls._lastMsg.tv_sec;
	   rc._lastMsgUs = ls._lastMsg.tv_usec;
	   rc._nOpen     = ls._nOpen;
	   rc._nClose    = ls._nClose;
	   rc._nImg      = ls._nImg;
	   rc._nUpd      = ls._nUpd;
	   rc._nDead     = ls._nDead;
	   return &rc;
	}

	/**
	 * \brief Return LVC Start Time
	 *
	 * \return LVC Start Time
	 */
	double tStart()
	{
	   double rc;

	   rc  = _lvc ? _lvc->_tStart.tv_usec : 0.0;
	   rc /= 1000000.0;
	   rc += _lvc ? _lvc->_tStart.tv_sec  : 0.0;
	   return rc;
	}

	/**
	 * \brief Return Executable Name
	 *
	 * \return Executable Name
	 */
	const char *ExeName()
	{
	   return _lvc ? _lvc->_exe : "";
	}

	/**
	 * \brief Return LVC Build Num
	 *
	 * \return LVC Build Num
	 */
	const char *BuildNum()
	{
	   return _lvc ? _lvc->_build : "";
	}

	/**
	 * \brief Return File Remaining in Kb
	 *
	 * \return File Remaining in Kb
	 */
	int FileLeftKb()
	{
	   return _lvc ? _lvc->_snk._iVal[13] : 0;
	}

	/**
	 * \brief Return File Size in Kb
	 *
	 * \return File Size in Kb
	 */
	int FileSizeKb()
	{
	   return _lvc ? _lvc->_snk._iVal[14] : 0;
	}

	/**
	 * \brief Return File Remaining in Pct { 0 ... 100.0 }
	 *
	 * \return File Remaining in Pct { 0 ... 100.0 }
	 */
	double FileLeftPct()
	{
	   return _lvc ? _lvc->_snk._dVal[13] : 0.0;
	}

	/**
	 * \brief Calc and Return Msg/Sec
	 *
	 * \brief numSec - Nubmer of seconds
	 * \return Msg/Sec over last numSec
	 */
	int UpdPerSec( int numSec )
	{
	   StatSnaps &sdb = _snkRt;
	   StatSnap   s0, s1;
	   double     nm, tm;
	   size_t     sz, off;

	   sz  = sdb.size() - 1;
	   off = WithinRange( 0, sz-numSec, sz );
	   s1 = sdb[sz];
	   s0 = sdb[off];
	   nm = s1._NumMsg - s0._NumMsg;
	   tm = s1._Time   - s0._Time;
	   return ( tm != 0.0 ) ? ( nm / tm ) : 0.0;
	}

	////////////////////////////////////
	// Asynchronous Callbacks - protected
	////////////////////////////////////
protected:
	/**
	 * \brief Called repeatedly by the library between the time you
	 * call StartThread() and StopThread()
	 *
	 * If your Worker Thread is an infinite loop, you must periodically
	 * call ThreadIsRunning() and return control if false
	 */
	virtual void OnWorkerThread()
	{
	   StatSnaps &sdb  = _snkRt;
	   u_int64_t &nMsg = _lvc->_snk._nMsg;
	   double     d0, d1, age;
	   StatSnap   s;

	   for ( d0=TimeNs(); ThreadIsRunning(); Sleep( 0.25 ) ) {
	      d1  = TimeNs();
	      age = d1 - d0;
	      if ( age >= 1.0 ) {
	         d0        = d1;
	         s._Time   = d0;
	         s._NumMsg = nMsg; 
	         sdb.erase( sdb.begin() );
	         sdb.push_back( s );
	      }
	   }
	}

	////////////////////////
	// Private Members
	////////////////////////
private:
	std::string _statFile;
	rtBuf64     _mm;
	GLlvcStats *_lvc;
	StatSnaps   _snkRt;

};  // class LVCStatMon

};  // namespace RTEDGE

#endif // __LVCStatMon_H 
