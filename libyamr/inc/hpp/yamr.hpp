/******************************************************************************
*
*  yamr.hpp
*     libyamr base Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_yamr_H
#define __YAMR_yamr_H
#include <map>
#include <string>
#include <vector>
#undef K
#if !defined(hash_map)
#if !defined(_GLIBCXX_UNORDERED_MAP) && !defined(WIN32)
#include <tr1/unordered_map>
#define hash_map std::tr1::unordered_map
#else
#include <unordered_map>
#define hash_map std::unordered_map
#endif // !defined(_GLIBCXX_UNORDERED_MAP)
#endif // !defined(hash_map)
#define K 1024

/*
 * Can't use w/ CLI : Contention w/ array
 *
 * using namespace std;
 */

namespace YAMR
{

// Forwards

class Reader;
class Writer;
namespace Data
{
class StringDict;
} // namespace Data

/**
 * \enum DumpType
 * \brief Body dump type
 *
 * \see yamr::Dump()
 */
typedef enum {
   /** \brief Do not dump body */
   dump_none    = 0,
   /** \brief Verbose dumping */
   dump_verbose = 1,
   /** \brief Comma Separated Value, if applicable */
   dump_CSV     = 2,
   /** \brief JavaScript Object Notation (JSON) */
   dump_JSON    = 3,
   /** \brief Compressed JSON : All on one line */
   dump_JSONmin = 4
} DumpType;


////////////////////////////////////////////////
//
//        c l a s s    y a m r
//
////////////////////////////////////////////////

/**
 * \class yamr
 * \brief yamRecorder base class
 */
class yamr
{

	////////////////////////////////////
	// Class-wide public methods - libyamr
	////////////////////////////////////
public:
	/**
	 * \brief No-Op
	 */
	static void breakpoint()
	{
	}

	/**
	 * \brief Returns the library build version.
	 *
	 * \return Build version of the library
	 */
	static const char *Version()
	{
	   return ::yamr_Version();
	}

	/**
	 * \brief Sets the library debug level; Initiates logging. 
	 *
	 * \param pLog - Log filename
	 * \param dbgLvl - Debug level
	 */
	static void Log( const char *pLog, int dbgLvl )
	{
	   ::yamr_Log( pLog, dbgLvl );
	}

	/**
	 * \brief Returns clock counter to nanosecond granularity
	 *
	 * \return Clock counter to nanosecond granularity
	 */
	static double TimeNs()
	{
	   return ::yamr_TimeNs();
	}

	/**
	 * \brief Returns current time in Unixtime
	 *
	 * \return Current time in Unixtime
	 */
	static time_t TimeSec()
	{
	   return ::yamr_TimeSec();
	}

	/**
	 * \brief Returns time Unix time in Nanos from time string formatted
	 * as YYYY-MM-DD HH:MM:SS.uuuuuu
	 *
	 * Any combination of the following are supported:
	 * Type | Supported | Value
	 * --- | --- | ---
	 * Date | YYYY-MM-DD | As is
	 * Date | YYYYMMDD | As is
	 * Date | \<empty\> | Current Date
	 * Time | HH:MM:SS | As is
	 * Time | HHMMSS | As is
	 * Sub-Second | uuuuuu | Micros (6 digits)
	 * Sub-Second | mmm | Millis (3 digits)
	 * Sub-Second | \<empty\> | 0
	 *
	 * \param pTime - String-formatted time
	 * \return Unix Time in nanos
	 */
	static u_int64_t TimeFromString( const char *pTime )
	{
	   return ::yamr_TimeFromString( pTime );
	}

	/**
	 * \brief Returns  time in YYYY-MM-DD HH:MM:SS.mmm
	 *
	 * \see ::yamr_pDateTimeMs()
	 * \param rtn - std::string to hold return value
	 * \param tNano - Time in nanos; Set to 0 to return current time
	 * \return Current time in YYYY-MM-DD HH:MM:SS.mmm 
	 */
	static const char *pDateTimeMs( std::string &rtn, u_int64_t tNano=0 )
	{
	   char buf[K];

	   rtn = ::yamr_pDateTimeMs( buf, tNano );
	   return rtn.data();
	}

	/**
	 * \brief Returns current time in HH:MM:SS.mmm
	 *
	 * \see ::yamr_pTimeMs()
	 * \param rtn - std::string to hold return value
	 * \param tNano - Time in nanos; Set to 0 to return current time
	 * \return Current time in HH:MM:SS.mmm 
	 */
	static const char *pTimeMs( std::string &rtn, u_int64_t tNano=0 )
	{
	   char buf[K];

	   rtn = ::yamr_pTimeMs( buf, tNano );
	   return rtn.data();
	}

	/**
	 * \brief Sleeps for requested period of time.
	 *
	 * \param dSlp - Number of \<seconds\>.\<microseconds\> to sleep
	 */
	static void Sleep( double dSlp )
	{
	   ::yamr_Sleep( dSlp );
	}

	/**
	 * \brief Hex dump a message into an output buffer
	 *
	 * \param msg - Message to dump
	 * \param len - Message length
	 * \return Hex dump of msg in std::string
	 */
	static std::string HexMsg( char *msg, int len )
	{
	   std::string tmp;
	   char       *obuf;
	   int         sz;

	   obuf     = new char[len*8];
	   sz       = ::yamr_hexMsg( msg, len, obuf );
	   obuf[sz] = '\0';
	   tmp      = obuf;
	   delete[] obuf;
	   return std::string( tmp );
	}

	/**
	 * \brief Returns total CPU seconds used by application
	 *
	 * \return Total CPU seconds used by application
	 */
	static double CPU()
	{
	   return ::yamr_CPU();
	}

	/**
	 * \brief Returns total memory size of this process
	 *
	 * \return Total memory size for this process
	 */
	static int MemSize()
	{
	   return ::yamr_MemSize();
	}

	/**
	 * \brief Creates a memory-mapped view of the file
	 *
	 * Call UnmapFile() to unmap the view.
	 *
	 * \param pFile - Name of file to map
	 * \param bCopy - true to make copy; false for view-only
	 * \return Mapped view of a file in an rtBUF struct
	 */
	static yamrBuf MapFile( const char *pFile, bool bCopy=true )
	{
	   return ::yamr_MapFile( (char *)pFile, bCopy ? 1 : 0 );
	}

	/**
	 * \brief Re-maps view of file previously mapped via MapFile()
	 *
	 * This works as follows:
	 * + Current view passed in as 1st argument must be a view, not copy
	 * + File is re-mapped if the file size has grown
	 * + If file size has not changed, then view is returned
	 *
	 * \param view - Current view of file from MapFile()
	 * \return Mapped view of a file in an rtBUF struct
	 * \see MapFile()
	 */
	static yamrBuf RemapFile( yamrBuf view )
	{
	   return ::yamr_RemapFile( view );
	}

	/**
	 * \brief Unmaps a memory-mapped view of a file
	 *
	 * \param bMap - Memory-mapped view of file from MapFile()
	 */
	static void UnmapFile( yamrBuf bMap )
	{
	   ::yamr_UnmapFile( bMap );
	}


	////////////////////////////////////
	// Class-wide public utilities
	////////////////////////////////////
	/**
	 * \brief Trim leading / trailing spaces from a string
	 *
	 * \param str - String to trim
	 * \return Trimmed string
	 */
	static char *TrimString( char *str )
	{
	   char *cp, *rtn;
	   int   i, len;

	   // Pre-condition

	   len = str ? strlen( str ) : 0;
	   if ( !len )
	      return str;

	   // Trim leading fleh ...

	   rtn = cp = str;
	   for ( i=0; !InRange( '!',*cp,'~') && i<len; i++,cp++ );
	   if ( i == len ) {   // All fleh??
	      *str = '\0';
	      return str;
	   }
	   rtn = cp;

	   // ... and trailing fleh

	   cp = str+len-1;
	   for ( i=len-1; !InRange(  '!',*cp,'~') && i>=0; i--,cp-- );
	   *(cp+1) = '\0';

	   return rtn;
	}

};  // class yamr


////////////////////////////////////////////////
//
//    c l a s s    S h a r a b l e 
//
////////////////////////////////////////////////

/**
 * \class Sharable
 * \brief Abstract base class for sharable read / write stuff
 */
class Sharable : public yamr
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
protected:
	/** \brief Constructor
	 */
	Sharable() :
	   _dict( (Data::StringDict *)0 )
	{ ; }


	////////////////////////////////////
	// Singletons : StringDict
	////////////////////////////////////
public:
	/**
	 * \brief Return true if Data::StringDict singleton has been registered
	 *
	 * \return true if Data::StringDict singleton has been registered
	 */
	bool HasStringDict()
	{
	   return( _dict != (Data::StringDict *)0 );
	}

	/**
	 * \brief Return the Data::StringDict singleton associated w/ this 
	 * Sharable
	 *
	 * \return Data::StringDict singleton associated w/ this Sharable
	 */
	Data::StringDict &strDict()
	{
	   return *_dict;
	}

	/**
	 * \brief Register the StringDict singleton
	 *
	 * \param dict - StringDict instance to register
	 * \return true if registered; false if already registered
	 */
	bool RegisterStringDict( Data::StringDict *dict )
	{
	   if ( !_dict ) {
	      _dict = dict;
	      return true;
	   }
	   return false;
	}


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
protected:
	Data::StringDict *_dict;

};  // class Sharable


/**
 * \brief Structured data Marshalling / De-marshalling Classes
 */
namespace Data
{

////////////////////////////////////////////////
//
//        c l a s s    C o d e c
//
////////////////////////////////////////////////

/**
 * \class Codec
 * \brief Encoder / Decoder Interface
 */
class Codec
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
protected:
	/** 
	 * \brief Constructor - DeMarshall 
	 *
	 * \param reader - Reader we DeMarshall messages from
	 */
	Codec( Reader &reader ) :
	   _sharable( (Sharable *)&reader ),
	   _reader( &reader ),
	   _writer( (Writer *)0 )
	{ ; }

	/** 
	 * \brief Constructor - Marshall 
	 *
	 * \param writer - Writer we Marshall messages to
	 */
	Codec( Writer &writer ) :
	   _sharable( (Sharable *)&writer ),
	   _reader( (Reader *)0 ),
	   _writer( &writer )
	{ ; }

	/** \brief Destructor. */
public:
	virtual ~Codec()
	{ ; }


	////////////////////////////////////
	// Access
	////////////////////////////////////
	/**
	 * \brief Return Reader associated w/ this Codec
	 *
	 * \return Reader associated w/ this Codec
	 */
	Reader &reader()
	{
	   return *_reader;
	}

	/**
	 * \brief Return Writer associated w/ this Codec
	 *
	 * \return Writer associated w/ this Codec
	 */
	Writer &writer()
	{
	   return *_writer;
	}


	////////////////////////////////////
	// Encode Interface
	////////////////////////////////////
public:
	/**
	 * \brief Encode and send message based on protocol
	 *
	 * \param MsgProto - Message Protocol; 0 = same as Wire Protocol
	 * \return true if message consumed by channel; false otherwise
	 */
	virtual bool Send( u_int16_t MsgProto=0 ) = 0;


	////////////////////////////////////
	// Decode Interface
	////////////////////////////////////
public:
	/**
	 * \brief Decode message based on protocol
	 *
	 * \param msg - Parsed unstructured YAMR message
	 * \return true if our protocol; false otherwise
	 */
	virtual bool Decode( yamrMsg &msg ) = 0;

	/**
	 * \brief Dump Message based on protocol
	 *
	 * \param msg - Parsed unstructured YAMR message
	 * \param dmpTy - DumpType : Verbose, CSV, etc.
	 * \return Viewable Message Contents
	 */
	virtual std::string Dump( yamrMsg &msg, DumpType dmpTy=dump_verbose ) = 0;


	////////////////////////////////////
	// Private Members
	////////////////////////////////////
protected:
	Sharable *_sharable;
	Reader   *_reader;
	Writer   *_writer;

};  // class Codec

} // namespace Data


////////////////////////////////////////////////
//
//   c l a s s    I W r i t e L i s t e n e r
//
////////////////////////////////////////////////

/**
 * \class IWriteListener
 * \brief Abstract "listener" for Writer events
 */
class IWriteListener
{
#ifndef DOXYGEN_OMIT
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	virtual ~IWriteListener() { ; }
#endif // DOXYGEN_OMIT

	////////////////////////////////////
	// IWriteListener Interface
	////////////////////////////////////
public:
	/** 
	 * \brief Called asynchronously when we connect or disconnect from
	 * the yamRecorder.
	 *
	 * \param chan - Writer driving the event
	 * \param bUP - true if connected; false if connection lost
	 */
	virtual void OnConnect( Writer &chan, bool bUP )
	{ ; }

	/** 
	 * \brief Called asynchronously when Writer status updates
	 *
	 * \param chan - Writer driving the event
	 * \param sts - Writer status
	 */
	virtual void OnStatus( Writer &chan, yamrStatus sts )
	{ ; }

}; // class IWriteListener


////////////////////////////////////////////////
//
//      c l a s s    I R e a d L i s t e n e r
//
////////////////////////////////////////////////

/**
 * \class IReadListener
 * \brief Abstract "listener" for Reader events
 */
class IReadListener
{
friend class Reader;
#ifndef DOXYGEN_OMIT
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	virtual ~IReadListener() { ; }
#endif // DOXYGEN_OMIT

	////////////////////////////////////
	// IReadListener Interface
	////////////////////////////////////
private:
	/** 
	 * \brief Called asynchronously when Reader reads message
	 *
	 * \param reader - Reader driving the event
	 * \param msg - Unstructured Message from tape
	 * \param codec - Decoded message, if protocol known; NULL if unknown
	 */
	virtual void OnMessage( Reader &reader, yamrMsg &msg, Data::Codec *codec ) = 0;

}; // class IReadListener

} // namespace YAMR

#endif // __YAMR_yamr_H 
