/******************************************************************************
*
*  Chain.h
*
*  REVISION HISTORY:
*      7 JAN 2015 jcs  Created
*
*  (c) 1994-2015 Gatea, Ltd.
******************************************************************************/
#pragma once

#ifndef DOXYGEN_OMIT
#include <rtEdge.h>
#include <Data.h>
#endif // DOXYGEN_OMIT

#ifndef DOXYGEN_OMIT

namespace librtEdgePRIVATE
{
////////////////////////////////////////////////
//
//         c l a s s   I C h a i n
//
////////////////////////////////////////////////
	/**
	 * \class IChain
	 * \brief Abstract class to handle the 2 Chain Events:
	 * + OnLink()
	 * + OnData()
	 */
	public interface class IChain
	{
		// IChain Interface
	public:
		virtual void OnLink( String ^, int, librtEdge::rtEdgeData ^ ) abstract;
		virtual void OnData( String ^, int, int, librtEdge::rtEdgeData ^ ) abstract;
	};


////////////////////////////////////////////////
//
//        c l a s s   C h a i n C
// 
////////////////////////////////////////////////

	/**
	 * \class ChainC
	 * \brief RTEDGE::Chain sub-class to hook 4 virtual methods
	 * from native librtEdge library and dispatch to .NET consumer. 
	 */ 
	class ChainC : public RTEDGE::Chain
	{
	private:
		gcroot < IChain^ >                _cli;
		gcroot < librtEdge::rtEdgeData^ > _upd;

		// Constructor
	public:
		/**
		 * \brief Constructor for class to hook native events from 
		 * native librtEdge library and pass to .NET consumer via
		 * the IChain interface.
		 *
		 * \param cli - Event receiver - IChain::OnChainData(), etc.
		 * \param svc - Service supplying this Chain
		 * \param tkr - Published name of this Chain
		 */
		ChainC( IChain ^cli, const char *svc, const char *link1 );
		~ChainC();

		// Asynchronous Callbacks
	protected:
		virtual void OnChainLink( const char *, int, RTEDGE::Message & );
		virtual void OnChainData( const char *, int, int, RTEDGE::Message & );
	};

} // namespace librtEdgePRIVATE

#endif // DOXYGEN_OMIT



namespace librtEdge
{

////////////////////////////////////////////////
//
//       c l a s s     C h a i n
//
////////////////////////////////////////////////
	/**
	 * \class Chain
	 * \brief This class encapsulates a Chain
	 *
	 * When consuming you receive asynchronous notifications as follows:
	 * + OnLink() - A link in the chain has updated; This happens
	 * when the Chain "changes shape" - i.e., add, modifies, or removes 
	 * a record from the chain.
	 * + OnData() - Market data has updated for a record in the chain.
	 *
	 * \include Chain_example.h
	 */
	public ref class Chain : public librtEdgePRIVATE::IChain 
	{
	private: 
		librtEdgePRIVATE::ChainC *_chn;

		/////////////////////////////////
		// Constructor / Destructor
		/////////////////////////////////
	public:
		/**
		 * \brief Constructor.
		 *
		 * \param svc - Service supplying this Chain if Subscribe()
		 * \param tkr - Published name of this stream
		 */
		Chain( String ^tkr, String ^svc );

		/** \brief Destructor.  Cleans up internally.  */
		~Chain();


		/////////////////////////////////
		// Access
		/////////////////////////////////
	public:
		/**
		 * \brief Returns underlying RTEDGE::Chain object 
		 *
		 * \return Underlying RTEDGE::Chain object 
		 */
		 RTEDGE::Chain &chn();

		/**
		 * \brief Returns Service Name of this Byte Stream
		 *
		 * \return Service Name of this Byte Stream
		 */
		String ^svc();

		/**
		 * \brief Returns Name of 1st link in this Chain
		 *
		 * \return Name of 1st link in this Chain
		 */
		String ^name();


		/////////////////////////////////
		// IChain interface
		/////////////////////////////////
	public:
		/**
		 * \brief Called asynchronously when a market data update arrives for
		 * a ChainLink.  A ChainLink updates when the Chain "changes shape" -
		 * i.e., add, modifies or removes a ChainRecord.
		 *
		 * Override this method in your application to take action.
		 *
		 * \param name = Link Name
		 * \param nLnk - Link Number
		 * \param msg - Market data update in an rtEdgeData object
		 */
		virtual void OnLink( String ^name, int nLnk, rtEdgeData ^msg )
		{ ; }

		/**
		 * \brief Called asynchronously when a market data update arrives for
		 * a ChainRecord.
		 *
		 * Override this method in your application to take action.
		 *
		 * \param name - Record Name
		 * \param pos - Position of record in chain
		 * \param nUpd - Number of updates
		 * \param msg - Market data update in an rtEdgeData object
		 */
		virtual void OnData( String ^name, int pos, int nUpd, rtEdgeData ^msg )
		{ ; }

	};  // class rtEdgeSubscriber

} // namespace librtEdge
