/******************************************************************************
*
*  OptUtil.cpp
*     Utilities for Options
*
*  REVISION HISTORY:
*     17 SEP 2023 jcs  Created (from GreekServer.cpp)
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <set>

using namespace RTEDGE;
using namespace std;

typedef vector<int>                           Ints;
typedef map<u_int64_t, int, less<u_int64_t> > SortedInt64Map;
typedef set<u_int64_t, less<u_int64_t> >      SortedInt64Set;

#define _DSPLY_NAME    3
#define _TRDPRC_1      6
#define _BID          22
#define _ASK          25
#define _STRIKE_PRC   66
#define _EXPIR_DATE   67
#define _UN_SYMBOL  4200

////////////////////////////////////////////////
//
//           U t i l i t i e s
//
////////////////////////////////////////////////
static const char *_pBool( bool b )
{
   return b ? "true" : "false";
}

static bool _IsTrue( const char *p )
{
   return( !::strcmp( p, "YES" ) || !::strcmp( p, "true" ) );
}


////////////////////////////////////////////////
//
//      c l a s s   O p t i o n s B a s e
//
////////////////////////////////////////////////

/**
 * \class OptionsBase
 * \brief LVC-fed Options Utility Base Class
 */
class OptionsBase : public LVC
{
private:
   struct tm _lt;

   ////////////////////////////////////
   // Constructor / Destructor
   ////////////////////////////////////
public:
   OptionsBase( const char *svr ) :
      LVC( svr )
   {
      time_t now = TimeSec();

      ::localtime_r( &now, &_lt );
   }


   ////////////////////////////////////
   // Access
   ////////////////////////////////////
   /**
    * \brief Return Bid-Ask Mid Point
    *
    * \param msg - Message snapped from LVC
    * \return Bid-Ask Mid Point
    */
   double MidQuote( Message &msg )
   {
      double bid, ask;

      // Lame (but effective) attempt to handle one-sided quotes

      bid = _GetAsDouble( msg, _BID );
      ask = _GetAsDouble( msg, _ASK );
      bid = ( bid == 0.0 ) ? ask : bid;
      ask = ( ask == 0.0 ) ? bid : ask;
      return( 0.5 * ( bid+ask ) );
   }

   /**
    * \brief Return Last Price
    *
    * \param msg - Message snapped from LVC
    * \return Last Price
    */
   double Last( Message &msg )
   {
      return _GetAsDouble( msg, _TRDPRC_1 );
   }

   /**
    * \brief Return Strike Price
    *
    * \param msg - Message snapped from LVC
    * \return Strike Price
    */
   double StrikePrice( Message &msg )
   {
      return _GetAsDouble( msg, _STRIKE_PRC );
   }

   /**
    * \brief Return Expiration as JulNum or YYYYMMDD
    *
    * \param msg - Message snapped from LVC
    * \param bJulNu - true for julNum; false for YYYYMMDD
    * \return Expiration as JulNum
    */
   u_int64_t Expiration( Message &msg, bool bJulNum=true )
   {
      Field    *fld;
      u_int64_t rc;

      rc = 0;
      if ( (fld=msg.GetField( _EXPIR_DATE )) ) {
         if ( bJulNum )
            rc = julNum( fld->GetAsDate() );
         else {
            rc  = (u_int64_t)fld->field()._val._r64;
            rc /= 1000000;
         }
      }
      return rc;
   }



   ////////////////////////////////////
   // Expiration Time Stuff
   ////////////////////////////////////
   /**
    * \brief Convert string-ified date YYYYMMDD or into number of days since Epoch
    *
    * \param ymd : String-ified date as YYYYMMDD or YYYY-MM-DD
    * \param bJulNum : true for num days since Epoch; false for YYYYMMDD
    * \return  Number of days since Jan 1, 1970; 0 if error
    */
   u_int64_t ParseDate( const char *ymd, bool bJulNum )
   {
      string    s( ymd );
      struct tm lt;
      size_t    sz;
      time_t    unx;
      u_int64_t rc;

      unx = 0;
      lt  = _lt;
      sz  = s.length();
      switch( sz ) {
         case  8: // YYYYMMDD
            lt.tm_year  = atoi( s.substr( 0,4 ).data() );
            lt.tm_mon   = atoi( s.substr( 4,2 ).data() );
            lt.tm_mday  = atoi( s.substr( 6,2 ).data() );
            lt.tm_year -= 1900;
            lt.tm_mon  -= 1;
            lt.tm_hour  = 0;
            lt.tm_min   = 0;
            lt.tm_sec   = 0;
            unx         = ::mktime( &lt );
            break;
         case 10: // YYYY-MM-DD
            lt.tm_year  = atoi( s.substr( 0,4 ).data() );
            lt.tm_mon   = atoi( s.substr( 5,2 ).data() );
            lt.tm_mday  = atoi( s.substr( 8,2 ).data() );
            lt.tm_year -= 1900;
            lt.tm_mon  -= 1;
            lt.tm_hour  = 0;
            lt.tm_min   = 0;
            lt.tm_sec   = 0;
            unx         = ::mktime( &lt );
            break;
      }
      if ( bJulNum )
         rc = unx / 86400;
      else {
         rc  = 10000 * ( lt.tm_year + 1900 );
         rc +=   100 * ( lt.tm_mon + 1 );
         rc += lt.tm_mday;
      }
      return rc;
   }

   /**
    * \brief Convert rtDate to number of days since Jan 1, 1970 
    *
    * \param dt : rtDate
    * \return  Number of days since Jan 1, 1970; 0 if error
    */
   u_int64_t julNum( rtDate dt )
   {
      rtDateTime dtTm;
      time_t     unx;

      ::memset( &dtTm, 0, sizeof( dtTm ) );
      dtTm._date = dt;
      unx        = (time_t)rtEdgeDateTime2unix( dtTm );
      return( unx / 86400 );
   }


   ////////////////////////////////////
   // Operations
   ////////////////////////////////////
public:
   /**
    * \brief Dump one Message from LVC
    *
    * The bHdr parameter controls title line:
    * bHdr | Title Line
    * --- | ---
    * true | Index,Time,Ticker,Active,Age, NumUpd
    * false | Ticker
    *
    * \param msg - LVC Message to dump
    * \param fids - List of FID's to dump
    * \param bHdr - true to dump header; false for ticker only
    * \return string w/ Message Dump
    */
   string DumpOne( Message &msg, Ints &fids, bool bHdr=true )
   {
      LVCData    &ld  = msg.dataLVC();
      const char *act = ld._bActive ? "ACTIVE" : "DEAD";
      const char *tkr = msg.Ticker();
      const char *pt;
      Field      *fld;
      string tm, sm;
      char        hdr[4*K], *cp;
      size_t      i;

      /*
       * 1) Header
       */
      pt  = msg.pTimeMs( tm, msg.MsgTime() );
      cp  = hdr;
      if ( bHdr ) {
         cp += sprintf( cp, "%s,%s,%s,", pt, tkr, act );
         cp += sprintf( cp, "%.2f,%d,", ld._dAge, ld._nUpd );
      }
      else
         cp += sprintf( cp, "%s,", tkr );
      sm  = hdr;
      /*
       * 2) Fields
       */
      for ( i=0; i<fids.size(); i++ ) {
         fld = msg.GetField( fids[i] );
         sm += fld ? fld->GetAsString() : "-";
         sm += ",";
      }
      /*
       * 3) Dump
       */
      sm += "\n";
      return string( sm );
   }

   /**
    * \brief Return sorted list of indices for ( Underlyer, PutOrCall )
    *
    * The list is sorted by Expiration, then Strike Price
    *
    * \param all - LVCAll w/ snapped values from LVC
    * \param und - Underlyer
    * \param bPut - true for put; false for call
    * \return Sort LVC index list containing ( Underlyer, PutOrCall )
    */
   Ints GetUnderlyer( LVCAll &all, const char *und, bool bPut )
   {
      Messages      &msgs = all.msgs();
      SortedInt64Map pdb, cdb;
      const char    *val;
      Message       *msg;
      Field         *fld;
      rtVALUE        v;
      Ints           idxs;
      u_int64_t      exp;
      bool           put;

      for ( size_t i=0; i<msgs.size(); i++ ) {
         msg = msgs[i];
         if ( !(fld=msg->GetField( _UN_SYMBOL )) )
            continue; // for-i
         val = fld->GetAsString();
         if ( ::strcmp( val, und ) )
            continue; // for-i
         /*
          * Sorted by ( PutOrCall, Expire, Strike )
          */
         put = false; 
         exp = 0;
         if ( (fld=msg->GetField( _DSPLY_NAME )) ) {
            string s( fld->GetAsString() );
            char   ch;

            ch  = s.length() ? s.data()[s.length()-1] : '?';
            put = ( ch == 'P' );
         }
         if ( (fld=msg->GetField( _EXPIR_DATE )) ) {
            v   = fld->field()._val;
            exp = (u_int64_t)v._r64;
         }
         exp += (fld=msg->GetField( _STRIKE_PRC )) ? fld->GetAsInt32() : 0;
         if ( put )
            pdb[exp] = i;
         else
            cdb[exp] = i;
      }
      SortedInt64Map          &rdb  = bPut ? pdb : cdb;
      SortedInt64Map::iterator st;

      for ( st=rdb.begin(); st!=rdb.end(); idxs.push_back( (*st).second ), st++ );
      return Ints( idxs );
   }

   ////////////////////////////////////
   // (private) Helpers
   ////////////////////////////////////
private:
   /**
    * \brief Find and return Field Value as double
    *
    * \param msg - Message snapped from LVC
    * \param fid - Field
    * \return Field value as double
    */
   double _GetAsDouble( Message &msg, int fid )
   {
      Field  *fld;

      return( (fld=msg.GetField( fid )) ? fld->GetAsDouble() : 0.0 );
   }

}; // class OptionsBase
