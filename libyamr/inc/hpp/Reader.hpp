/******************************************************************************
*
*  Reader.hpp
*     libyamr tape reader Class
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*     29 JUL 2019 jcs  Build  2: Sharable; DumpCSV()
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#ifndef __YAMR_Reader_H
#define __YAMR_Reader_H
#include <hpp/yamr.hpp>

namespace YAMR
{


////////////////////////////////////////////////
//
//        c l a s s    R e a d e r
//
////////////////////////////////////////////////

typedef hash_map<u_int16_t, Data::Codec *> CodecMap;
typedef hash_map<u_int16_t, std::string>   NameMap;
typedef hash_map<std::string, u_int16_t>   ProtoMap;

typedef std::vector<IReadListener *>       IReadListeners;

/**
 * \class Reader
 * \brief Unstructured tape reader class.
 *
 * All structured data transfer derive from and utilize the services
 * of the unstructured writer and reader classes.
 * \see Writer
 */      
class Reader : public Sharable
{
	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/** \brief Constructor. */
	Reader() :
	   _file(),
	   _cxt( (yamrTape_Context)0 ),
	   _codecMap(),
	   _nameMap(),
	   _protoMap(),
	   _undefName(),
	   _lsn()
	{
	}

	/** \brief Destructor. */
	virtual ~Reader()
	{
	   Close();
	}


	////////////////////////////////////
	// Debug : Message Dump
	////////////////////////////////////
	/**
	 * \brief Dump message header
	 *
	 * \param y - Parsed YAMR messae
	 * \return Viewable Message Header
	 */
	std::string DumpHeader( yamrMsg y )
	{
	   yamrBuf       &b = y._Data;
	   std::string    s;
	   struct in_addr a;
	   char           hdr[K], *cp;

	   a.s_addr = y._Host;
	   cp  = hdr;
	   cp += sprintf( cp, "[%s] ", pDateTimeMs( s, y._Timestamp ) );
	   cp += sprintf( cp, yamr_PRId64 " %s ", y._SeqNum, ProtoName( y ) );
	   cp += sprintf( cp, "%s:%d", inet_ntoa( a ), y._SessionID );
	   cp += sprintf( cp, " [" yamr_PRId64 " bytes]", b._dLen );
	   return std::string( hdr );
	}

	/**
	 * \brief Dump message based on protocol
	 *
	 * This class calls out to the Data::Codec::Dump() based on the 
	 * protocol in the message.
	 *
	 * \param msg - Parsed YAMR messae
	 * \param dmpTy - Dump Type
	 * \return Viewable Message Contents
	 * \see Data::Codec::Dump()
	 */
	std::string DumpBody( yamrMsg &msg, DumpType dmpTy=dump_verbose )
	{
	   CodecMap          &v = _codecMap;
	   CodecMap::iterator it;
	   u_int16_t          pro;
	   Data::Codec       *codec;

	   // Find based on MsgProtocol; Dispatch

	   pro = msg._MsgProtocol;
	   if ( (it=v.find( pro )) != v.end() ) {
	      codec = (*it).second;
	      return codec->Dump( msg, dmpTy );
	   }
	   return std::string();
	}

	/**
	 * \brief Return protocol name associated w/ message
	 * 
	 * \param msg - Parsed YAMR message
	 * \return Protocol name
	 */ 
	const char *ProtoName( yamrMsg msg )
	{
	   return ProtoName( msg._MsgProtocol );
	}

	/**
	 * \brief Return protocol name from protocol ID
	 * 
	 * \param proto - Protocol ID
	 * \return Protocol name
	 */ 
	const char *ProtoName( u_int16_t proto )
	{
	   NameMap          &ndb = _nameMap;
	   const char       *name;
	   char              buf[K];
	   NameMap::iterator it;

	   if ( (it=ndb.find( proto )) != ndb.end() )
	      name = (*it).second.data();
	   else {
	      sprintf( buf, "<<%04x>>", proto );
	      _undefName = buf;
	      name       = _undefName.data();
	   }
	   return name;
	}

	/**
	 * \brief Return Codec from protocol ID
	 * 
	 * \param proto - Protocol ID
	 * \return Data::Codec
	 */ 
	Data::Codec *GetCodec( u_int16_t proto )
	{
	   CodecMap          &cdb = _codecMap;
	   CodecMap::iterator it;
	   Data::Codec       *codec;

	   codec = (Data::Codec *)0;
	   if ( (it=cdb.find( proto )) != cdb.end() )
	      codec = (*it).second;
	   return codec;
	}

	/**
	 * \brief Return protocol number from registered name
	 * 
	 * \param name - Registered Protocol Name
	 * \return Protocol number; 0 if not found
	 */ 
	u_int16_t ProtoNumber( const char *name )
	{
	   ProtoMap          &pdb = _protoMap;
	   ProtoMap::iterator it;
	   std::string        s( name );

	   it = pdb.find( s );
	   return ( it != pdb.end() ) ? (*it).second : 0;
	}


	////////////////////////////////////
	// Protocol Registration
	////////////////////////////////////
	/**
	 * \brief Register De-marshalling class based on protocol
	 * 
	 * \param codec - Data::Codec de-marshalling class 
	 * \param proto - Protocol associated w/ de-marshalling class
	 * \param protoName - Protocol name
	 */ 
	void RegisterProtocol( Data::Codec &codec, 
	                       u_int16_t    proto,
	                       const char  *protoName )
	{
	   std::string s( protoName );

	   _codecMap[proto] = &codec;
	   _nameMap[proto]   = std::string( protoName );
	   _protoMap[s]      = proto;
	}


	////////////////////////////////////
	// IReadListener
	////////////////////////////////////
public:
	/**
	 * \brief Register interest to be notified of Reader message events
	 *
	 * \param lsn - Listener to be notified of Reader message events
	 */
	void AddListener( IReadListener &lsn )
	{
	   _lsn.push_back( &lsn );
	}


	////////////////////////////////////
	// Reader Operations
	////////////////////////////////////
public:
	/**
	 * \brief Opens yamRecorder tape for reading
	 *
	 * \param filename - Tape filenme
	 */
	void Open( const char *filename )
	{
	   // Pre-condition(s)

	   if ( !_cxt && filename ) {
	      _file = filename;
	      _cxt  = ::yamrTape_Open( _file.data() );
	   }
	}

	/** \brief Close yamRecorder tape file */
	void Close()
	{
	   if ( _cxt )
	      ::yamrTape_Close( _cxt );
	   _cxt = (yamrTape_Context)0;
	}

	/**
	 * \brief Rewind to beginning of tape
	 *
	 * \return Unix Time in Nanos of next message; 0 if empty tape
	 */
	u_int64_t Rewind()
	{
	   return ::yamrTape_Rewind( _cxt );
	}

	/**
	 * \brief Rewind tape to specific position
	 *
	 * \param pos - Rewind position in Nanos since epoch 
	 * \return Unix Time in Nanos of next message; 0 if empty tape
	 */
	u_int64_t RewindTo( u_int64_t pos )
	{
	   return ::yamrTape_RewindTo( _cxt, pos );
	}

	/**
	 * \brief Rewind tape to specific position
	 *
	 * \param pTime - String formatted time
	 * \return Unix Time in Nanos of next message; 0 if empty tape
	 */
	u_int64_t RewindTo( const char *pTime )
	{
	   return ::yamrTape_RewindTo( _cxt, TimeFromString( pTime ) );
	}

	/**
	 * \brief Read unstructured message from tape
	 *
	 * \param msg - Unstructured message
	 * \return true if successful read; false otherwise
	 */
	bool Read( yamrMsg &msg )
	{
	   return ::yamrTape_Read( _cxt, &msg ) ? true : false;
	}

	/**
	 * \brief Decode message based on protocol
	 *
	 * This class calls out to the Data::Codec::Decode() based on the 
	 * protocol in the message.
	 *
	 * \param msg - Parsed YAMR message
	 * \see Data::Codec::Decode()
	 */
	void Decode( yamrMsg &msg )
	{
	   CodecMap          &cdb = _codecMap;
	   IReadListeners    &ldb = _lsn;
	   CodecMap::iterator it;
	   u_int16_t           pro;
	   Data::Codec        *codec;
	   size_t              i;

	   // Find based on protocol; Dispatch

	   pro   = msg._MsgProtocol;
	   codec = (Data::Codec *)0;
	   if ( (it=cdb.find( pro )) != cdb.end() ) {
	      codec = (*it).second;
	      codec->Decode( msg );
	   }
	   for ( i=0; i<ldb.size(); ldb[i++]->OnMessage( *this, msg, codec ) );
	}


	////////////////////////////////////
	// Protected Members
	////////////////////////////////////
protected:
	std::string      _file;
	yamrTape_Context _cxt;
	CodecMap         _codecMap;
	NameMap          _nameMap;
	ProtoMap         _protoMap;
	std::string      _undefName;
	IReadListeners   _lsn;

};  // class Reader

} // namespace YAMR

#endif // __YAMR_Reader_H 
