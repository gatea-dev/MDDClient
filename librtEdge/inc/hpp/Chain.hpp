/******************************************************************************
*
*  Chain.hpp
*     librtEdge Chain
*
*  REVISION HISTORY:
*      6 JAN 2015 jcs  Created.
*      4 MAY 2015 jcs  Build 31: Fully-qualified  (compiler)
*      2 JUL 2017 jcs  Build 34: _bLinkOnly
*     16 MAR 2019 jcs  Build 42: OnChainLink( ..., Strings )
*     21 DEC 2024 jcs  Build 74: IsListOnly()
*
*  (c) 1994-2024, Gatea Ltd.
******************************************************************************/
#ifndef __RTEDGE_Chain_H
#define __RTEDGE_Chain_H
#include <hpp/rtEdge.hpp>

namespace RTEDGE
{

// Forwards

class Chain;
class ChainLink;
class ChainRecord;
class SubChannel;

typedef std::vector<ChainRecord *>           ChainRecords;
typedef std::vector<ChainLink *>             ChainLinks;
typedef hash_map<int, ChainLink *>           ChainLinksById;
typedef hash_map<int, ChainRecord *>         ChainRecordsById;
typedef hash_map<std::string, ChainRecord *> ChainRecordsByName;;


////////////////////////////////////////////////
//
//           c l a s s   C h a i n L i n k
//
////////////////////////////////////////////////

/**
 * \class ChainLink
 * \brief This class encapsulates a market data chain link such 0\#DOW30
 */
class ChainLink : public rtEdge
{
friend class Chain;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor
	 *
	 * \param chn - Chain we associate with
	 * \param name - Link Name
	 * \param nLnk - Link number - 0, 1, 2, ...
	 */
	ChainLink( Chain &chn, const char *name, int nLnk ) :
	   _chn( chn ),
	   _name( name ),
	   _nLnk( nLnk ),
	   _StreamID( 0 )
	{
	}

	virtual ~ChainLink()
	{
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Chain supplying this link
	 *
	 * \return Chain supplying this link
	 */
	Chain &chn()
	{
	   return _chn;
	}

	/**
	 * \brief Returns Name of this ChainLink
	 *
	 * \return Name of this ChainLink
	 */
	const char *name()
	{
	   return _name.c_str();
	}

	/**
	 * \brief Returns link number
	 *
	 * \return Link number
	 */
	int LinkNumber()
	{
	   return _nLnk;
	}

	/**
	 * \brief Returns Subscription Stream ID
	 *
	 * \return Subscription Stream ID
	 */
	int StreamID()
	{
	   return _StreamID;
	}


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	/**
	 * \brief Sets the unique Stream ID for this ChainLink
	 *
	 * \param StreamID - Unique subscription ID
	 */
	void SetStreamID( int StreamID )
	{
	   _StreamID = StreamID;
	}

	/**
	 * \brief Called by SubChannel when market data arrives 
	 * 
	 * \return new ChainLink if created
	 */
	ChainLink *_OnData( Message &msg, size_t nLnk, Strings &lnks )
	{
	   Field      *next, *lnk;
	   ChainLink  *nxt;
	   const char *pn, *pl;
	   int         i, fid;

	   /*
	    * Support for 2 Chain schemas:
	    *
	    *    Description  Schema1   Schema2
	    *    -----------  -------   -------
	    *    Prev Link    PREV_LR   LONGPREVLR
	    *    Next Link    NEXT_LR   LONGNEXTLR
	    *    Link 1       LINK_1    LONGLINK1
	    */
	   nxt  = (ChainLink *)0;
	   next = (Field *)0;
	   fid  = LINK_1; // Compiler warning
	   if ( msg.HasField( LINK_1 ) ) {
	      next = msg.GetField( NEXT_LR );
	      fid  = LINK_1;
	   }
	   else if ( msg.HasField( LONGLINK1 ) ) {
	      next = msg.GetField( LONGNEXTLR );
	      fid  = LONGLINK1;
	   }

	   // 1) Next Link

	   pn = next ? next->GetAsString() : (char *)0;
	   if ( pn && strlen( TrimString( (char *)pn ) ) )
	      nxt = new ChainLink( _chn, pn, nLnk );

	   // 2) Records

	   for ( i=0; i<_NUM_LINK; i++ ) {
	      lnk = msg.GetField( fid+i );
	      pl  = lnk ? lnk->GetAsString() : (char *)0;
	      if ( pl && strlen( TrimString( (char *)pl ) ) )
	         lnks.push_back( std::string( pl ) );
	   }

	   // 3) Return new ChainLink if created

	   return nxt;
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	Chain      &_chn;
	std::string _name;
	int         _nLnk;
	int         _StreamID;

};  // class ChainLink



////////////////////////////////////////////////
//
//         c l a s s   C h a i n R e c o r d
//
////////////////////////////////////////////////

/**
 * \class ChainRecord
 * \brief This class encapsulates a market data chain record
 *
 */
class ChainRecord
{
friend class Chain;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor
	 *
	 * \param chn - Chain we associate with
	 * \param name - Link Name
	 * \param pos - Position in Chain
	 */
	ChainRecord( Chain &chn, const char *name, int pos ) :
	   _chn( chn ),
	   _name( name ),
	   _pos( pos ),
	   _nUpd( 0 ),
	   _StreamID( 0 )
	{
	}

	virtual ~ChainRecord()
	{
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Chain supplying this link
	 *
	 * \return Chain supplying this link
	 */
	Chain &chn()
	{
	   return _chn;
	}

	/**
	 * \brief Returns Name of this ChainRecord
	 *
	 * \return Name of this ChainRecord
	 */
	const char *name()
	{
	   return _name.c_str();
	}

	/**
	 * \brief Returns Position in Chain
	 *
	 * \return Position in Chain
	 */
	int Position()
	{
	   return _pos;
	}

	/**
	 * \brief Returns number of updates
	 *
	 * \return Number of updates
	 */
	int NumUpd()
	{
	   return _nUpd;
	}

	/**
	 * \brief Returns Subscription Stream ID
	 *
	 * \return Subscription Stream ID
	 */
	int StreamID()
	{
	   return _StreamID;
	}


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	/**
	 * \brief Sets position in chain
	 *
	 * \param pos - Position in chain
	 */
	void SetPosition( int pos )
	{
	   _pos = pos;
	}

	/**
	 * \brief Sets the unique Stream ID for this ChainRecord
	 *
	 * \param StreamID - Unique subscription ID
	 */
	void SetStreamID( int StreamID )
	{
	   _StreamID = StreamID;
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	Chain      &_chn;
	std::string _name;
	int         _pos;
	int         _nUpd;
	int         _StreamID;

};  // class ChainRecord



////////////////////////////////////////////////
//
//           c l a s s   C h a i n
//
////////////////////////////////////////////////

/**
 * \class Chain
 * \brief This class encapsulates a market data chain.
 *
 * A Chain consists of 2 types of FieldList-based records:
 * + ChainLink - Record whose fields point to up to 14 other records
 * + ChainRecord - Record in chain containing pricing data
 *
 * For example the Dow Jones 30 Industrials could be put in a Chain as
 * follows:
 * + 30 ChainRecord's - AXP, BA, CAT, etc.
 * + 3 ChainLink's - 0\#DOW30, 1\#DOW30, 2\#DOW30; The 1st ChainLink contains
 * the 1st 14 Dow 30 components - AXP, BA, CAT, etc. 
 *
 * When consuming you receive asynchronous notifications as follows:
 * + OnChainLink() - When a ChainLink update is received
 * + OnChainData() - When a ChainRecord update is received
 * + OnChainListComplete() - The list of names is complete
 */
class Chain
{
friend class SubChannel;

	////////////////////////////////////
	// Constructor / Destructor
	////////////////////////////////////
public:
	/**
	 * \brief Constructor
	 *
	 * \param svc - Service supplying this Chain
	 * \param link1 - Name of 1st link in this Chain
	 * \param bListOnly - ChainLink's only; Do not subscribe to chain
	 */
	Chain( const char *svc, const char *link1, bool bListOnly=false ) :
	   _svc( svc ),
	   _bListOnly( bListOnly ),
	   _linkNames(),
	   _links(),
	   _linksById(),
	   _records(),
	   _recordsById(),
	   _recordsByName()
	{
	   _links.push_back( new ChainLink( *this, link1, 0 ) );
	}

	virtual ~Chain()
	{
	}


	////////////////////////////////////
	// Access
	////////////////////////////////////
public:
	/**
	 * \brief Returns Service Name of this Chain
	 *
	 * \return Service Name of this Chain
	 */
	const char *svc()
	{
	   return _svc.c_str();
	}

	/**
	 * \brief Returns Name of 1st link in this Chain
	 *
	 * \return Name of 1st link in this Chain
	 */
	const char *name()
	{
	   return _links[0]->name();
	}

	/**
	 * \brief Return true for ChainLink's only; Do not subscribe to chain
	 *
	 * \return true for ChainLink's only; Do not subscribe to chain
	 */
	bool IsListOnly()
	{
	   return _bListOnly;
	}

	/**
	 * \brief Returns array of ChainLink's
	 *
	 * \return array of ChainLink's
	 */
	ChainLinks &links()
	{
	   return _links;
	}

	/**
	 * \brief Returns array of ChainRecords's
	 *
	 * \return array of ChainRecords's
	 */
	ChainRecords &records()
	{
	   return _records;
	}


	////////////////////////////////////
	// Asynchronous Callbacks
	////////////////////////////////////
protected:
	/**
	 * \brief Called asynchronously when a market data update arrives for
	 * a ChainLink.  A ChainLink updates when the Chain "changes shape" - 
	 * i.e., add, modifies or removes a ChainRecord.
	 *
	 * Override this method in your application to take action.
	 *
	 * \param name = Link Name
	 * \param nLnk - Link Number
	 * \param lnks - Names in this link
	 */
	virtual void OnChainLink( const char *name, int nLnk, Strings &lnks )
	{ ; }

	/**
	 * \brief Called asynchronously when a market data update arrives for
	 * a ChainRecord.
	 *
	 * Override this method in your application to take action.
	 *
	 * \param name - Record Name
	 * \param pos - Position of record in chain
	 * \param nUpd - Number of updates received by record
	 * \param msg - Market data update in a Message object
	 */
	virtual void OnChainData( const char *name, int pos, int nUpd, Message &msg )
	{ ; }

	/**
	 * \brief Called asynchronously when the list of tickers in the chain
	 * is complete.
	 *
	 * Override this method in your application to take action.
	 *
	 * \param lst - List of links in the Chain
	 */
	virtual void OnChainListComplete( Strings &lst )
	{ ; }


	////////////////////////////////////
	// Helpers
	////////////////////////////////////
private:
	/**
	 * \brief Called by SubChannel when we Subscribe or Unsubscribe a
	 * subscription stream for a ChainLink
	 */
	void _SetStreamID( ChainLink *lnk, int StreamID )
	{
	   ChainLinksById          &ldb = _linksById;
	   ChainLinksById::iterator lt;
	   int                      sid;

	   // 1) Remove

	   sid = lnk->StreamID();
	   if ( sid && ( (lt=ldb.find( sid )) != ldb.end() ) )
	      ldb.erase( lt );

	   // 2) Add 

	   lnk->SetStreamID( StreamID );
	   if ( StreamID && ( (lt=ldb.find( StreamID)) == ldb.end() ) )
	      ldb[StreamID] = lnk;
	}

	/**
	 * \brief Called by SubChannel when we Subscribe or Unsubscribe a
	 * subscription stream for a ChainRecord
	 */
	void _SetStreamID( ChainRecord *rec, int StreamID )
	{
	   ChainRecordsById          &rdb = _recordsById;
	   ChainRecordsById::iterator rt;
	   int                        sid;

	   // 1) Remove

	   sid = rec->StreamID();
	   if ( sid && ( (rt=rdb.find( sid )) != rdb.end() ) )
	      rdb.erase( rt );

	   // 2) Add 

	   rec->SetStreamID( StreamID );
	   if ( StreamID && ( (rt=rdb.find( StreamID)) == rdb.end() ) )
	      rdb[StreamID] = rec;
	}

	/**
	 * \brief Called by SubChannel when market data arrives 
	 * 
	 * \return true if new ChainLink or ChainRecords is created
	 */
	bool _OnData( SubChannel &sub, Message &msg )
	{
	   ChainLinksById            &ldb = _linksById;
	   ChainRecordsById          &rdb = _recordsById;
	   ChainLinksById::iterator   lt;
	   ChainRecordsById::iterator rt;
	   ChainLink                 *lnk, *nxt;
	   ChainRecord               *rec;
	   Strings                    ndb;
	   bool                       bNew;
	   int                        StreamID;
	   const char                *pr;
	   size_t                     i, nr;

	   // Pre-condition

	   if ( !msg.NumFields() )
	      return false; // TODO : Chain.OnChainStatus() ??

	   /*
	    * Either of the following based on StreamID:
	    *   1) ChainLink
	    *   2) ChainRecord
	    */
	   bNew     = false;
	   StreamID = msg.data()._StreamID;
	   lt       = ldb.find( StreamID );
	   rt       = rdb.find( StreamID );
	   if ( lt != ldb.end() ) {
	      lnk  = (*lt).second;
	      nxt = lnk->_OnData( msg, _links.size(), ndb );
	      if ( nxt ) {
	         _links.push_back( nxt );
	         bNew = true;
	      }
	      /*
	       * TODO : Handle updating chain
	       *   1) Compare to _recordsByName
	       *   2) Move shit around; Re-position ChainRecords
	       */
	      for ( i=0; i<ndb.size(); i++ ) {
	         pr   = ndb[i].c_str();
	         nr   = _records.size() + i;
	         if ( !_bListOnly ) {
	            rec  = new ChainRecord( *this, pr, nr );
	            bNew = true;
	            _records.push_back( rec );
	         }
	         _linkNames.push_back( std::string( pr ) );
	      }
	      OnChainLink( lnk->name(), lnk->LinkNumber(), ndb );
	   }
	   else if ( rt != rdb.end() ) {
	      rec = (*rt).second;
	      rec->_nUpd += 1;
	      OnChainData( rec->name(), rec->Position(), rec->NumUpd(), msg );
	   }

	   // 3) Return true if new link or record created ...

	   if ( !bNew )
	      OnChainListComplete( _linkNames );
	   return bNew;
	}


	////////////////////////
	// private Members
	////////////////////////
private:
	std::string        _svc;
	bool               _bListOnly;
	Strings            _linkNames;
	ChainLinks         _links;
	ChainLinksById     _linksById;
	ChainRecords       _records;
	ChainRecordsById   _recordsById;
	ChainRecordsByName _recordsByName;

};  // class Chain


} // namespace RTEDGE

#endif // __LIBRTEDGE_Chain_H 
