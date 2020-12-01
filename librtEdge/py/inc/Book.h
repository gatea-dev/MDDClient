/******************************************************************************
*
*  Book.h
*     Book.py equivalent in MD-Direct
*
*  REVISION HISTORY:
*     22 APR 2011 jcs  Created.
*      . . . 
*      3 APR 2019 jcs  Build 23: MD-Direct / VS2017.32
*     
*  (c) 1994-2019 Gatea, Ltd.  
******************************************************************************/
#ifndef _MDDPY_BOOK_H
#define _MDDPY_BOOK_H
#include <MDDirect.h>


/////////////////
// Forwards
/////////////////
class BookRow;
class MDDPY::Field;
class MDDPY::Record;

/////////////////
// Fixed Structs
/////////////////
class BookRowValue
{
public:
   double _dPrc;
   int    _iQty;
   double _qTim;
};

class BookRtn
{
public:
   BookRow *_bid;
   BookRow *_ask;
   int      _nItr;

}; // class BookRowValue


typedef map<string, BookRowValue> BlackList;
typedef vector<BookRow *>         BookRows;


/////////////////////////////////////////
// Book Schema
/////////////////////////////////////////
class BookSchema
{
public:
   int _fidBid;
   int _fidBSz;
   int _fidBMm;
   int _fidBQid;
   int _fidBTim;
   int _fidBCnd;
   int _fidAsk;
   int _fidASz;
   int _fidAMm;
   int _fidAQid;
   int _fidATim;
   int _fidACnd;

   // Constructor / Destructor
public:
   BookSchema();
   ~BookSchema();

}; // class BookSchema

/////////////////////////////////////////
// BBO Book Row ( Price, Qty, ECN, QuoteID, Time )
/////////////////////////////////////////
class BookRow
{
public:
   Book         &_bk;
   int           _nRow;
   bool          _bBid;
   MDDPY::Field *_fPrc;
   MDDPY::Field *_fQty;
   MDDPY::Field *_fECN;
   MDDPY::Field *_fQteID;
   MDDPY::Field *_fQteTm;

   // Constructor / Destructor
public:
   BookRow( Book &, int, bool );
   ~BookRow();

   // Access

   double GetPrc();
   int    GetQty();
   char  *GetECN();
   char  *GetQuoteID();
   char  *GetQteTime();
   double QuoteAge();
   bool   RateQuantity_InValidForProduction( int minQty=0, double minPrc=0.0 );

   // Helpers
private:
   double _Qte2LclTime();

}; // class BookRow


/////////////////////////////////////////
// BBO Book
/////////////////////////////////////////
class Book : public MDDPY::Record
{
public:
   int         _bkSz;
   BookSchema  _schema;
   BookRows    _bids;
   BookRows    _asks;
   BlackList   _bidBL;
   BlackList   _askBL;

   // Constructor / Destructor
public:
   Book( const char *, const char *, int, int, int bkSz=5 );
   virtual ~Book();

   // Access / Mutator

   BookRtn GetCleanBook( double, double, char ** );
   BookRtn GetTopOfBook( char **, bool bKO=false );

   // Real-Time Data

   virtual void Update( mddFieldList );

   // Blacklist Management

   bool IsBlacklisted( BookRow & );
   void AddBlacklist( BookRow & );
   void DelBlacklist( BookRow & );
   void UpdBlacklist( BookRow & );

   // Helpers
private:
   BookRow *get_suspicious_row( double, double, char ** );

}; // class Book

#endif // _MDDPY_BOOK_H
