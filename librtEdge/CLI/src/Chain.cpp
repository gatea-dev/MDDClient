/******************************************************************************
*
*  Chain.cpp
*
*  REVISION HISTORY:
*      7 JAN 2015 jcs  Created
*
*  (c) 1994-2015 Gatea, Ltd.
******************************************************************************/
#include "StdAfx.h"
#include <Chain.h>


namespace librtEdgePRIVATE
{

////////////////////////////////////////////////
//
//       c l a s s     C h a i n C
//
////////////////////////////////////////////////

//////////////////////////
// Constructor
//////////////////////////
ChainC::ChainC( IChain     ^cli,
                const char *svc,
                const char *link1 ) :
   _cli( cli ),
   _upd( gcnew librtEdge::rtEdgeData() ),
   RTEDGE::Chain( svc, link1 )
{
}

ChainC::~ChainC()
{
   _upd = nullptr;
}


//////////////////////////
// Asynchronous Callbacks
//////////////////////////
void ChainC::OnChainLink( const char *name, int nLnk, RTEDGE::Message &msg )
{
   _upd->Set( msg );
   _cli->OnLink( gcnew String( name ), nLnk, _upd );
}

void ChainC::OnChainData( const char      *name, 
                          int              pos, 
                          int              nUpd, 
                          RTEDGE::Message &msg )
{
   _upd->Set( msg );
   _cli->OnData( gcnew String( name ), pos, nUpd, _upd );
}

} // namespace librtEdgePRIVATE


namespace librtEdge
{

////////////////////////////////////////////////
//
//         c l a s s   C h a i n
//
////////////////////////////////////////////////

/////////////////////////////////
// Constructor / Destructor
/////////////////////////////////
Chain::Chain( String ^svc, String ^tkr ) :
   _chn( new librtEdgePRIVATE::ChainC( this, 
                                        rtEdge::_pStr( svc ),
                                        rtEdge::_pStr( tkr ) ) )
{
}

Chain::~Chain()
{
   delete _chn;
   _chn = (librtEdgePRIVATE::ChainC *)0;
}


/////////////////////////////////
// Access
/////////////////////////////////
RTEDGE::Chain &Chain::chn()
{
   return *_chn;
}

String ^Chain::svc()
{
   return gcnew String( chn().svc() );
}

String ^Chain::name()
{
   return gcnew String( chn().name() );
}

} // namespace librtEdge
