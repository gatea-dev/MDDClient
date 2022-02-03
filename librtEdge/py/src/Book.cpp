/******************************************************************************
*
*  Book.cpp
*     Book.py equivalent in MDDirect
*
*  TODO : Preserve None-ness
*
*  REVISION HISTORY:
*     22 APR 2011 jcs  Created.
*      . . .
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*      3 FEB 2022 jcs  Build  5: De-lint
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
#include <MDDirect.h>



////////////////////////////////////////////////////////////////////////////
//
//                c l a s s      B o o k S c h e m a
//
////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
BookSchema::BookSchema() :
   _fidBid( 4201 ),
   _fidBSz( 4401 ),
   _fidBMm( 4001 ),
   _fidBQid( 4601 ),
   _fidBTim( 4801 ),
   _fidBCnd( 4651 ),
   _fidAsk( 4301 ),
   _fidASz( 4501 ),
   _fidAMm( 4101 ),
   _fidAQid( 4701 ),
   _fidATim( 4901 ),
   _fidACnd( 4751 )
{
}

BookSchema::~BookSchema()
{
}



////////////////////////////////////////////////////////////////////////////
//
//                   c l a s s      B o o k R o w
//
////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
BookRow::BookRow( Book &bk, int nRow, bool bBid ) :
   _bk( bk ),
   _nRow( nRow ),
   _bBid( bBid ),
   _fPrc( (MDDPY::Field *)0 ),
   _fQty( (MDDPY::Field *)0 ),
   _fECN( (MDDPY::Field *)0 ),
   _fQteID( (MDDPY::Field *)0 ),
   _fQteTm( (MDDPY::Field *)0 )
{
   BookSchema &ss = bk._schema;
   int         fid;

   // Set all field references

   fid     = bBid ? ss._fidBid  : ss._fidAsk;
   _fPrc   = bk.GetField( fid+nRow );
   fid     = bBid ? ss._fidBSz  : ss._fidASz;
   _fQty   = bk.GetField( fid+nRow );
   fid     = bBid ? ss._fidBMm  : ss._fidAMm;
   _fECN   = bk.GetField( fid+nRow );
   fid     = bBid ? ss._fidBQid : ss._fidAQid;
   _fQteID = bk.GetField( fid+nRow );
   fid     = bBid ? ss._fidBTim : ss._fidATim;
   _fQteTm = bk.GetField( fid+nRow );
}

BookRow::~BookRow()
{
}


///////////////////////////////
// Access
///////////////////////////////
double BookRow::GetPrc()
{
   return _fPrc ? atof( _fPrc->data() ) : 0.0;
}

int BookRow::GetQty()
{
   return _fQty ? atoi( _fQty->data() ) : 0;
}

char *BookRow::GetECN()
{
   return _fECN ? _fECN->data() : (char *)"";
}

char *BookRow::GetQuoteID()
{
   return _fQteID ? _fQteID->data() : (char *)"";
}

char *BookRow::GetQteTime()
{
   return _fQteTm ? _fQteTm->data() : (char *)"";
}

double BookRow::QuoteAge()
{
   double dq;

   dq = _Qte2LclTime();
   return( dNow() - dq );
}

bool BookRow::RateQuantity_InValidForProduction( int    minQty, 
                                                 double minPrc )
{
   return( ( GetQty() <= minQty ) || ( GetPrc() <= minPrc ) );
}



///////////////////////////////
// Helpers
///////////////////////////////
double BookRow::_Qte2LclTime()
{
   struct tm lt, qt;
   char      buf[K], *pf;
   char     *pYMD, *pHMS, *pMs;
   int       qSz, tMs, nMs;
   time_t    tq, tNow;
   double    dq;

   // Pre-condition

   if ( !_fQteTm )
      return 0.0;

   // Suck from string

   tNow = (time_t)dNow();
   ::localtime_r( &tNow, &lt );
   pf   = _fQteTm->data();
   qSz  = strncpyz( buf, pf, _fQteTm->dLen() );
   pYMD = buf;
   pHMS = (char *)::memchr( buf, '-', qSz );
   pMs  = (char *)::memchr( buf, '.', qSz );
   nMs  = pMs ? qSz - ( pMs+1-buf ) : 0;
   qt   = lt;
   if ( !pHMS ) {         // HH.MM.SS.mmm
      pHMS = pYMD;
      // Use date from lt, already in qt
   }
   else {                 // YYYYMMDD-HH:MM:SS.mmm
      pHMS      += 1; // '-'
      qt.tm_year = atoin( pYMD+0, 4 ) - 1900;
      qt.tm_mon  = atoin( pYMD+4, 2 ) - 1;
      qt.tm_mday = atoin( pYMD+6, 2 );
   }
   qt.tm_hour = atoin( pHMS+0, 2 );
   qt.tm_min  = atoin( pHMS+3, 2 );
   qt.tm_sec  = atoin( pHMS+6, 2 );
   tMs        = pMs ? atoin( pMs+1, nMs ) : 0;

   // mktime()

   tq = ::mktime( &qt );
   dq = ( 0.001 * tMs ) + tq;
   return dq;
}



////////////////////////////////////////////////////////////////////////////
//
//                     c l a s s      B o o k
//
////////////////////////////////////////////////////////////////////////////

///////////////////////////////
// Constructor / Destructor
///////////////////////////////
Book::Book( const char   *pSvc,
            const char   *pTkr,
            int           userReqID,
            int           oid,
            int           bkSz ) :
   Record( pSvc, pTkr, userReqID, oid ),
   _bkSz( bkSz ),
   _schema(),
   _bids(),
   _asks(),
   _bidBL(),
   _askBL()
{
}

Book::~Book()
{
   BookRows::iterator it;

   for ( it=_bids.begin(); it!=_bids.end(); delete (*it), it++ );
   for ( it=_asks.begin(); it!=_asks.end(); delete (*it), it++ );
   _bids.clear();
   _asks.clear();
   _bidBL.clear();
   _askBL.clear();
}


///////////////////////////////
// Access / Mutator
///////////////////////////////
BookRtn Book::GetCleanBook( double dInversion,
                            double dMaturity,
                            char **ecnKO )
{
   BookRtn  rtn;
   BookRow *row;
   int      i;

   row = get_suspicious_row( dInversion, dMaturity, ecnKO );
   for ( i=0; row && i<_bkSz; i++ ) {
//      ecnKO = [] + list( sets.Set( ecnKO + [ row._mmid ] ) ) // TODO
      row = get_suspicious_row( dInversion, dMaturity, ecnKO );
   }
   rtn = GetTopOfBook( ecnKO, true ); // TODO : Add ecnKO to return
   rtn._nItr = i;
   return rtn;
}

BookRtn Book::GetTopOfBook( char **ecnKO, bool bKO )
{
   BookRtn rtn;
   char   *bmmid, *ammid, *ecn;
   bool    bidOK, askOK;
   int     i, j, br, ar;

   // 1) Calc Bid/Ask Row Numbers

   br = -1;
   ar = -1;
   if ( ecnKO[0] && !bKO ) {    // First that matches
      for ( i=0; _bkSz; i++ ) {
         bmmid = _bids[i]->GetECN();
         ammid = _asks[i]->GetECN();
         for ( j=0; ecnKO[j]; j++ ) {
            ecn = ecnKO[j];
            if ( ( br == -1 ) && !::strcmp( bmmid, ecn ) ) br = i;
            if ( ( ar == -1 ) && !::strcmp( ammid, ecn ) ) ar = i;
         }
      }
   }
   else if ( ecnKO[0] && bKO ) {   // First that doesn't match ALL ecnKO's
      for ( i=0; _bkSz; i++ ) {
         bidOK = true;
         askOK = true;
         bmmid = _bids[i]->GetECN();
         ammid = _asks[i]->GetECN();
         for ( j=0; ecnKO[j]; j++ ) {
            ecn = ecnKO[j];
            if ( bidOK && ( br == -1 ) )
               bidOK = ( ::strcmp( bmmid, ecn ) != 0 );
            if ( askOK && ( ar == -1 ) )
               askOK = ( ::strcmp( ammid, ecn ) != 0 );
         }
         if ( bidOK && ( br == -1 ) ) br = i;
         if ( askOK && ( ar == -1 ) ) ar = i;
      }
   }
   else {   // Top of Book
      br = 0;
      ar = 0;
   }

   // 2) Return Values

   rtn._bid = ( br != -1 ) ? _bids[br] : (BookRow *)0;
   rtn._ask = ( ar != -1 ) ? _asks[ar] : (BookRow *)0;
   return rtn;
}


///////////////////////////////
// Real-Time Data
///////////////////////////////
void Book::Update( mddFieldList fl )
{
   bool bFirst;
   int  i;

   // 1) Default

   Record::Update( fl );

   // 2) Us  - BookRow's

   bFirst = ( _bids.size() == 0 );
   for ( i=0; bFirst && i<_bkSz; i++ ) {
      _bids.push_back( new BookRow( *this, i, true ) );
      _asks.push_back( new BookRow( *this, i, false ) );
   }
}


///////////////////////////////
// Blacklist Management
///////////////////////////////
bool Book::IsBlacklisted( BookRow & )
{
return false;
}

void Book::AddBlacklist( BookRow & )
{
m_breakpoint();
}

void Book::DelBlacklist( BookRow & )
{
m_breakpoint();
}

void Book::UpdBlacklist( BookRow & )
{
m_breakpoint();
}


///////////////////////////////
// Helpers
///////////////////////////////
BookRow *Book::get_suspicious_row( double dInversion, 
                                   double dMaturity, 
                                   char **ecnKO )
{
   BookRtn br;
   double  bid_rate, ask_rate, bid_age, ask_age;
   int     bid_qty, ask_qty;
   double  dVal, age_diff;

   // 1) Find first row 2/ ECN that is NOT on knockout list

   br = GetTopOfBook( ecnKO, true );
   if ( !br._bid )
      return (BookRow *)0;
   if ( !br._ask )
      return (BookRow *)0;

   // 2) Return, if either GetQty() or GetPrc() <= 0.0

   if ( br._bid->RateQuantity_InValidForProduction() )
      return br._bid;
   if ( br._ask->RateQuantity_InValidForProduction() )
      return br._ask;

   // 3) OK, check for inversion

   bid_qty  = br._bid->GetQty();
   ask_qty  = br._ask->GetQty();
   if ( bid_qty >= ask_qty )
      m_breakpoint();
   bid_rate = br._bid->GetPrc();
   ask_rate = br._ask->GetPrc();
   bid_age  = br._bid->QuoteAge();
   ask_age  = br._ask->QuoteAge();
   if ( ask_rate )
      dVal = bid_rate / ask_rate;
   else
      return br._ask;
   if ( dVal < dInversion )
      return (BookRow *)0;

   // 4) OK, we have inversion; If young, we're all right

   age_diff = ask_age - bid_age;
   if ( ::fabs( age_diff ) <= dMaturity )
      return (BookRow *)0;
   else if ( age_diff > 0 )
       return br._ask;
   return br._bid;
}

