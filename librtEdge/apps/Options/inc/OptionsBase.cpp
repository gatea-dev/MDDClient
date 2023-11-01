/******************************************************************************
*
*  OptUtil.cpp
*     Utilities for Options
*
*  REVISION HISTORY:
*     17 SEP 2023 jcs  Created (from GreekServer.cpp)
*     15 OCT 2023 jcs  Build 65: _ymd2julNum()
*     31 OCT 2023 jcs  Build 66: quant.hpp
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <quant.hpp>
#include <stdarg.h>
#include <set>

using namespace RTEDGE;
using namespace QUANT;
using namespace std;

// RTEDGE Only

#define _DoubleGrid   RTEDGE::DoubleGrid
#define _DoubleList   RTEDGE::DoubleList

// Templatized Collections

typedef vector<int>                           Ints;
typedef map<u_int64_t, int, less<u_int64_t> > SortedInt64Map;
typedef set<u_int64_t, less<u_int64_t> >      SortedInt64Set;
typedef set<string, less<string> >            SortedStringSet;
typedef map<string, string, less<string> >    SortedStringMap;

// Inbound Record Template

#define _DSPLY_NAME    3
#define _TRDPRC_1      6
#define _HST_CLOSE    21
#define _BID          22
#define _ASK          25
#define _STRIKE_PRC   66
#define _EXPIR_DATE   67
#define _STOCK_RIC  1026
#define _UN_SYMBOL  4200

#define _MIL     1000000
#define _SECPERDAY 86400

// Enum

typedef enum {
   spline_put  = 0,
   spline_call = 1,
   spline_both = 2,
} SplineType;

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

/////////////////////////////////////
// Handy-dandy Logger
/////////////////////////////////////
static FILE *_log = stdout;

static void LOG( const char *fmt, ... )
{
   va_list ap;
   char    bp[8*K], *cp;
   string  tm;

   va_start( ap,fmt );
   cp  = bp;
   cp += sprintf( cp, "[%s] ", rtEdge::pDateTimeMs( tm ) );
   cp += vsprintf( cp, fmt, ap );
   cp += sprintf( cp, "\n" );
   va_end( ap );
   ::fwrite( bp, 1, cp-bp, _log );
   ::fflush( _log );
}

static void LOGRAW( const char *fmt, ... )
{
   va_list ap;
   char    bp[8*K], *cp;
   string  tm;

   va_start( ap,fmt );
   cp  = bp;
   cp += vsprintf( cp, fmt, ap );
   cp += sprintf( cp, "\n" );
   va_end( ap );
   ::fwrite( bp, 1, cp-bp, _log );
   ::fflush( _log );
}



////////////////////////////////////////////////
//
//      c l a s s   I n d e x C a c h e
//
////////////////////////////////////////////////

/**
 * \class IndexCache
 * \brief A store of indices for specific underlyer
 */
class IndexCache
{
public:
   Ints _puts;
   Ints _calls;
   Ints _both;

   ////////////////////////////////////
   // Constructor / Destructor
   ////////////////////////////////////
public:
   IndexCache() :
      _puts(),
      _calls(),
      _both()
   { ; }
   
}; // IndexCache



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
   struct tm      _lt;
   SortedInt64Map _julNumMap;

   ////////////////////////////////////
   // Constructor / Destructor
   ////////////////////////////////////
public:
   OptionsBase( const char *svr ) :
      LVC( svr ),
      _julNumMap()
   {
      time_t now = TimeSec();

      ::localtime_r( &now, &_lt );
   }


   ////////////////////////////////////
   // Access - D/B
   ////////////////////////////////////
   /**
    * \brief Return sorted list of all underlyers
    *
    * \param all - LVCAll w/ snapped values from LVC
    * \return Sorted list of all underlyers
    */
   SortedStringMap Underlyers( LVCAll &all ) 
   {
      Messages       &msgs = all.msgs();
      Field          *fld;
      string          k;
      SortedStringMap rc;

      for ( size_t i=0; i<msgs.size(); i++ ) {
         if ( !(fld=msgs[i]->GetField( _UN_SYMBOL )) )
            continue; // for-i
         k = fld->GetAsString();
         if ( !(fld=msgs[i]->GetField( _STOCK_RIC )) )
            continue; // for-i
         if ( rc.find( k ) == rc.end() )
            rc[k] = string( fld->GetAsString() );
      }
      return SortedStringMap( rc );
   }


   ////////////////////////////////////
   // Access - Per Message
   ////////////////////////////////////
   /**
    * \brief Find and return Field Value as double
    *
    * \param msg - Message snapped from LVC
    * \param fid - Field
    * \return Field value as double
    */
   double GetAsDouble( Message &msg, int fid )
   {
      Field  *fld;

      return( (fld=msg.GetField( fid )) ? fld->GetAsDouble() : 0.0 );
   }

   /**
    * \brief Return Bid-Ask Mid Point
    *
    * \param msg - Message snapped from LVC
    * \return Bid-Ask Mid Point
    */
   double MidQuote( Message &msg )
   {
      double bid, ask, last, cls, rc;

      /*
       * First of the following that is non-zero:
       *    1) Bid-Ask Mid
       *    2) Last
       *    3) Close
       *    4) max( bid, ask )
       */
      last = GetAsDouble( msg, _TRDPRC_1 );
      cls  = GetAsDouble( msg, _HST_CLOSE );
      bid  = GetAsDouble( msg, _BID );
      ask  = GetAsDouble( msg, _ASK );
      if ( bid && ask )
         rc = 0.5 * ( bid+ask );
      else if ( last )
         rc = last;
      else if ( cls )
         rc = cls;
      else
         rc = gmax( bid,  ask );
      return rc;
   }

   /**
    * \brief Return Last Price
    *
    * \param msg - Message snapped from LVC
    * \return Last Price
    */
   double Last( Message &msg )
   {
      return GetAsDouble( msg, _TRDPRC_1 );
   }

   /**
    * \brief Return Strike Price
    *
    * \param msg - Message snapped from LVC
    * \return Strike Price
    */
   double StrikePrice( Message &msg )
   {
      return GetAsDouble( msg, _STRIKE_PRC );
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
      SortedInt64Map &jdb = _julNumMap;
      Field          *fld;
      u_int64_t       rc, ymd;

      rc = 0;
      if ( (fld=msg.GetField( _EXPIR_DATE )) ) {
         ymd  = (u_int64_t)fld->field()._val._r64;
         ymd /= _MIL;
         if ( bJulNum ) {
            /*
             * Query; Add if not there
             */
            if ( !(rc=_ymd2julNum( ymd )) ) {
               rc       = julNum( fld->GetAsDate() );
               jdb[ymd] = rc;
            }
         }
         else
            rc = ymd;
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

   /**
    * \brief Convert list of julNum's to Unix Time
    *
    * \param jul - IN] List of julNums
    * \param unx - [OUT] List of Unix Times
    * \return unx 
    */
   _DoubleList &julNum2Unix( _DoubleList &jul, _DoubleList &unx )
   {
      size_t i, n;

      unx.clear();
      n = jul.size(); 
      for ( i=0; i<n; unx.push_back( jul[i] * _SECPERDAY ), i++ );
      return unx;
   }

   /**
    * \brief Convert list of yyyymmdd's to Unix Time
    *
    * \param ymd - IN] List of yyyymdd
    * \param unx - [OUT] List of Unix Times
    * \return unx 
    */
   _DoubleList &ymd2Unix( _DoubleList &ymd, _DoubleList &unx )
   {
      _DoubleList jul;
      size_t     i, n;

      unx.clear();
      n = ymd.size(); 
      for ( i=0; i<n; jul.push_back( _ymd2julNum( ymd[i] ) ), i++ );
      return julNum2Unix( jul, unx );
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
    * \param splineType - Put, Call or Both
    * \param bByExp - true to sort by ( Expiration, Strike ); false for reverse
    * \return Sort LVC index list containing ( Underlyer, PutOrCall )
    */
   Ints GetUnderlyer( LVCAll     &all, 
                      const char *und, 
                      SplineType  splineType,
                      bool        bByExp=true )
   {
      Messages                &msgs = all.msgs();
      SortedInt64Map           idb;
      SortedInt64Map::iterator it;
      const char              *val;
      Message                 *msg;
      Field                   *fld;
      Ints                     idxs;
      u_int64_t                exp;
      bool                     put, bAdd;

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
         /*
          * How do we sort?
          */
         if ( bByExp ) {
            exp  = Expiration( *msg, false );
            exp *= _MIL;
            exp += (u_int64_t)StrikePrice( *msg );
         }
         else {
            exp  = (u_int64_t)StrikePrice( *msg );
            exp *= _MIL;
            exp += Expiration( *msg, false );
         }
         /*
          * Put / Call / Both??
          */
         switch( splineType ) {
            case spline_put:  bAdd =  put; break;
            case spline_call: bAdd = !put; break;
            case spline_both:
            {
               /*
                * Take the Contract w/ the Highest MidQuote()
                */
               bAdd = ( (it=idb.find( exp )) == idb.end() );
               if ( !bAdd ) {
                  size_t j  = (*it).second;
                  double v1 = MidQuote( *msg );
                  double v2 = MidQuote( *msgs[j] );

                  bAdd = ( v1 > v2 );
               }
               break;
            }
         }
         if ( bAdd )
            idb[exp] = i;
      }
      /*
       * Stuff 'em in
       */
      for ( it=idb.begin(); it!=idb.end(); idxs.push_back( (*it).second ), it++ );
      return Ints( idxs );
   }

   ////////////////////////////////////
   // (private) Helpers
   ////////////////////////////////////
private:
   /**
    * \brief Convert YYYYMMDD to julNum
    *
    * \param ymd : YYYYMMDD
    * \return  Number of days since Jan 1, 1970; 0 if error
    */
   u_int64_t _ymd2julNum( u_int64_t ymd )
   {
      SortedInt64Map          &jdb = _julNumMap;
      SortedInt64Map::iterator it;
      u_int64_t                rc;

      rc = 0;
      if ( (it=jdb.find( ymd )) != jdb.end() )
         rc = (*it).second;
      return rc;
   }

}; // class OptionsBase
