/******************************************************************************
*
*  Subscribe.cpp
*
*  REVISION HISTORY:
*     15 SEP 2014 jcs  Created (from test.c)
*      3 JUL 2015 jcs  Build 31: OnDead()
*     11 AUG 2015 jcs  Build 33: IsSnapshot()
*      5 FEB 2016 jcs  Build 32: Field.Dump()
*     13 OCT 2017 jcs  Build 36: Tape
*     13 JAN 2019 jcs  Build 41: -bytestream; -csv; -csvF
*     20 FEB 2019 jcs  Build 42: -renko
*     30 APR 2019 jcs  Build 43: -bds
*     10 SEP 2020 jcs  Build 44: -tapeDir -query
*      1 DEC 2020 jcs  Build 47: -ti, -s0, -sn
*      6 OCT 2021 jcs  Build 50: -table
*     16 AUG 2022 jcs  Build 55: stdout formatting buggies
*     22 OCT 2022 jcs  Build 58: MyVector
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <math.h>
#include <vector>

// http://ascii-table.com/ansi-escape-sequences.php

#define _TICKER      "TICKER"
#define _TKR_LEN     10
#define _TKR_FMT     "%-10s"
#define ANSI_CLEAR   "\033[H\033[m\033[J"
#define ANSI_HOME    "\033[2;1H\033[K"
#define ANSI_POS     "\033[%ld;%ldf"   // ( Row, Col )

using namespace RTEDGE;
using namespace std;

static void *arg_ = (void *)"User-supplied argument";

class Renko
{
public:
   int     _Fid;
   double  _BlockSz;
   double  _Last;
   double  _Mul;
   char   *_Lean;

};  // class Renko

static Renko _zRenko = { 0, 0.0, 0.0, 1.0, (char *)"Black" };

typedef hash_map<int, string>     ColFmtMap;
typedef hash_map<int, size_t>     FidPosMap;
typedef hash_set<int>             FidSet;
typedef vector<int>               Fids;
typedef hash_map<int, mddField>   Fields;
typedef hash_map<string, Renko *> Renkos;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *SubscribeID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)Subscribe Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


/////////////////////////////////////
//
//   c l a s s   M y V e c t o r
//
/////////////////////////////////////
class MyVector : public Vector
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyVector( const char *svc, const char *tkr ) :
      Vector( svc, tkr )
   { ; }

   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnData( DoubleList &img )
   {
      string s = Dump();

      ::fprintf( stdout, "IMG (%s,%s) ", Service(), Ticker() );
      ::fwrite( s.data(), 1, s.length(), stdout );
      ::fflush( stdout );
   }

   virtual void OnData( VectorUpdate &upd )
   {
      string s = Dump( upd );

      ::fprintf( stdout, "UPD (%s,%s) ", Service(), Ticker() );
      ::fwrite( s.data(), 1, s.length(), stdout );
      ::fflush( stdout );
   }

   virtual void OnError( const char *err )
   {
      ::fprintf( stdout, "Vector.OnError() : %s\n", err );
      ::fflush( stdout );
   }

}; // class MyVector


/////////////////////////////////////
//
//   c l a s s   M y S t r e a m
//
/////////////////////////////////////
class MyStream : public ByteStream
{
   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyStream( const char *svc, const char *tkr ) :
      ByteStream( svc, tkr )
   { ; }


   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnData( rtBUF buf )
   {
      ::fprintf( stdout, "BSTR.OnData() : %d bytes\n", buf._dLen );
      ::fflush( stdout );
   }

   virtual void OnError( const char *err )
   {
      ::fprintf( stdout, "BSTR.OnError() : %s\n", err );
      ::fflush( stdout );
   }

   virtual void OnSubscribeComplete()
   {
      ::fprintf( stdout, "BSTR.Done : %d bytes\n", subBufLen() );
      ::fflush( stdout );
   }

}; // class MyStream


/////////////////////////////////////
//
//  c l a s s   M y C h a n n e l
//
/////////////////////////////////////
class MyChannel : public SubChannel
{
public:
   RTEDGE::Field   _uFld;
   bool            _bDump;
   bool            _bCSV;
   bool            _bds;
   bool            _bHdr;
   bool            _bSnap;
   int             _nUpd;
   Fids            _csvFids;
   RTEDGE::Strings _colFmt;
   FidSet          _colFidSet;
   FidPosMap       _fidPosMap;
   ColFmtMap       _colFmtMap;
   int             _eolFid;
   char           *_pTbl;
   Renko           _Renko;
   Renkos          _rdb;

   ///////////////////////////////////
   // Constructor / Destructor
   ///////////////////////////////////
public:
   MyChannel() :
      _uFld(),
      _bDump( true ),
      _bCSV( false ),
      _bds( false ),
      _bHdr( true ),
      _bSnap( false ),
      _nUpd( 0 ),
      _csvFids(),
      _colFmt(),
      _colFidSet(),
      _fidPosMap(),
      _colFmtMap(),
      _eolFid( 0 ),
      _pTbl( (char *)0 ),
      _Renko( _zRenko ),
      _rdb()
   {
   }

   ~MyChannel()
   {
      if ( _pTbl )
         delete[] _pTbl;
   }

   ///////////////////////////////////
   // Access / Mutator
   ///////////////////////////////////
public:
   bool IsTable()
   {
      return( _colFmt.size() > 0 );
   }

   void LoadCSVFids( char *fids )
   {
      Fids  &v = _csvFids;
      string s( fids );
      char  *tok, *rp;
      int    fid;

      _bCSV = true;
      tok   = (char *)s.data();
      tok   = ::strtok_r( tok, ",", &rp );
      for ( ; tok ; tok = ::strtok_r( NULL, ",", &rp ) ) {
         if ( (fid=atoi( tok )) )
            v.push_back( fid );
      }
   }


   ///////////////////////////////////
   // Operations
   ///////////////////////////////////
   void LoadTable( char *fids )
   {
      RTEDGE::Strings sdb;
      string          s( fids );
      char           *t1, *t2, fmt[K];
      int             fid, wid;
      size_t          pos;

      // -table fid:colW,fid:colW,...,eolFid

      t1 = (char *)s.data();
      t1 = ::strtok( t1, "," );
      for ( ; t1 ; t1=::strtok( NULL, "," ) )
         sdb.push_back( string( t1 ) );
      pos  = 1;
      pos += _TKR_LEN;
      for ( size_t i=0; !_eolFid && i<sdb.size(); i++ ) {
         t1  = (char *)sdb[i].data();
         t1  = ::strtok( t1, ":" );
         t2  = ::strtok( NULL, ":" );
         fid = atoi( t1 );
         wid = t2 ? atoi( t2 ) : 0;
         if ( !fid )
            break; // for
         if ( wid ) {
            _csvFids.push_back( fid );
            _colFidSet.insert( fid );
            sprintf( fmt, "%%%ds", wid );
            _colFmt.push_back( string( fmt ) );
            _fidPosMap[fid] = pos;
            _colFmtMap[fid] = string( fmt );
            pos += ::abs( wid );
            pos += 1; // " "
         }
         else
            _eolFid = fid;
      }

      // All OK??

      if ( IsTable() )
         _pTbl = new char[K*K];
   }

   ///////////////////////////////////
   // RTEDGE::SubChannel Notifications
   ///////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bOK )
   {
      ::fprintf( stdout, "OnConnect( %s,%sOK )\n", msg, bOK ? "" : "NOT " );
      _flush();
   }

   virtual void OnService( const char *msg, bool bOK )
   {
      ::fprintf( stdout, "OnService( %s,%sOK )\n", msg, bOK ? "" : "NOT " );
      _flush();
   }

   virtual void OnData( Message &msg )
   {
      RTEDGE::Schema   &sch  = schema();
      Fids             &fids =_csvFids;
      mddField         *flds = (mddField *)msg.Fields();
      RTEDGE::FieldDef *def;
      mddField          f;
      mddField         *w;
      Fields            fdb;
      Fields::iterator  it;
      string            s, st;
      const char       *svc, *tkr, *pm, *tm;
      int               i, nf, fid;

      // Table?? Renko??

      if ( IsTable() ) {
         _OnTable( msg );
         return;
      }
      else if ( _Renko._Fid ) {
         OnRenko( msg );
         return;
      }
      /*
       * 1) One Time : Header, if CSV
       */
      svc = msg.Service();
      tkr = msg.Ticker();
      nf  = msg.NumFields();
      if ( !_nUpd && _bCSV ) {
         s = "Date,Type,Service,Ticker,NumFld,";
         /*
          * Initial Image defined -csvF if empty
          */
         if ( !fids.size() )
            for ( i=0; i<nf; fids.push_back( flds[i++]._fid ) );
         for ( i=0; i<(int)fids.size(); i++ ) {
            pm = (def=sch.GetDef( fids[i] )) ? def->pName() : "Undefined";
            s += pm;
            s += ",";
         }
         ::fprintf( stdout, "%s\n", s.data() );
      }
      /*
       * 2) Dump; If CSV, header and build hash_map of fields 1st
       */
      _nUpd += 1;
      if ( _bCSV ) {
         tm = pDateTimeMs( st, msg.MsgTime() );
         pm = msg.IsImage() ? "IMG" : "UPD";
         ::fprintf( stdout, "%s,%s,%s,%s,%d,", tm, pm, svc, tkr, nf );
         for ( i=0; i<nf; i++ ) {
            f        = flds[i];
            fid      = f._fid;
            fdb[fid] = f;
         }
         s = "";
         for ( i=0; i<(int)fids.size(); i++ ) {
            fid = fids[i];
            pm  = "-";
            if ( (it=fdb.find( fid )) != fdb.end() ) {
               f  = (*it).second;
               w  = (mddField *)&f;
               _uFld.Set( *w );
               pm = _uFld.GetAsString();
            }
            s += pm;
            s += ",";
        }
        ::fprintf( stdout, "%s\n", s.data() );
      }
      else
         ::fprintf( stdout, msg.Dump() );
      _flush();
   }

   virtual void OnRecovering( Message &msg )
   {
      OnDead( msg, msg.Error() );
   }

   virtual void OnStreamDone( Message &msg )
   {
      OnDead( msg, msg.Error() );
   }

   virtual void OnDead( Message &msg, const char *err )
   {
      string      s;
      u_int64_t   off;
      const char *tm, *svc, *tkr, *mt;

      tm  = pDateTimeMs( s, msg.MsgTime() );
      mt  = msg.MsgType();
      svc = msg.Service();
      tkr = msg.Ticker();
      ::fprintf( stdout, "[%s] %s ( %s,%s ) :%s", tm, mt, svc, tkr, err );
      if ( (off=msg.TapePos()) )
         ::fprintf( stdout, "; Pos=%ld", off );
      ::fprintf( stdout, "\n" );
      _flush();
   }

   virtual void OnSymbol( Message &msg, const char *sym )
   {
      string      s;
      const char *tm;

      tm  = pDateTimeMs( s, msg.MsgTime() );
      ::fprintf( stdout, "[%s] SYM-ADD : %s\n", tm, sym );
      _flush();
   }

   virtual void OnSchema( Schema &sch )
   {
      string      s;
      const char *tm;
      int         nf;

      tm  = pDateTimeMs( s );
      nf  = sch.Size();
      ::fprintf( stdout, "[%s] OnSchema() nFld=%d\n", tm, nf );
      _flush();
   }

   //////////////////////////
   // Private helpers
   //////////////////////////
private:
   void _OnTable( RTEDGE::Message &msg )
   {
      RTEDGE::Schema      &sch  = schema();
      RTEDGE::Strings     &cfmt = _colFmt;
      FidSet              &cols = _colFidSet;
      FidPosMap           &pos  = _fidPosMap;
      ColFmtMap           &cdb  = _colFmtMap;
      mddField            *fdb  = (mddField *)msg.Fields();
      FidPosMap::iterator it;
      ColFmtMap::iterator ct;
      RTEDGE::FieldDef   *def;
      mddField            f;
      Fields              row;
      char               *cp;
      const char         *pn, *fmt, *pf;
      int                 i, nc, nf, fid;
      size_t              nRow, nCol;

      // Initial Image?  Header

      cp   = _pTbl;
      nc   = (int)cfmt.size();
      nRow = (size_t)msg.arg();
      if ( !_nUpd++ ) {
         cp += sprintf( cp, ANSI_CLEAR );
         cp += nRow ? sprintf( cp, _TKR_FMT, _TICKER ) : 0;
         for ( i=0; i<nc; i++ ) {
            fid = _csvFids[i];
            def = sch.GetDef( fid );
            pn  = def ? def->pName() : "undefined";
            cp += sprintf( cp, cfmt[i].data(), pn );
            cp += sprintf( cp, " " );
         }
      }
      if ( nRow ) {
         cp += sprintf( cp, ANSI_POS, nRow, (size_t)1 );
         cp += sprintf( cp, _TKR_FMT, msg.Ticker() );
      }
      else
         cp += sprintf( cp, ANSI_HOME );
      nf  = msg.NumFields();

      // One row at a time

      for ( i=0; i<nf; i++ ) {
         f   = fdb[i];
         fid = f._fid;
         if ( nRow ) {
            if ( (it=pos.find( fid )) == pos.end() )
               continue; // for-i
            if ( (ct=cdb.find( fid )) == cdb.end() )
               continue; // for-i
            _uFld.Set( f );
            nCol = (*it).second;
            fmt  = (*ct).second.data();
            pf   = _uFld.GetAsString();
            cp  += sprintf( cp, ANSI_POS, nRow, nCol );
            cp  += sprintf( cp, fmt, pf );
            cp  += sprintf( cp, " " );
         }
         else {
            if ( fid == _eolFid )
               cp += _DumpRow( cp, row );
            else if ( cols.find( fid ) != cols.end() )
               row[fid] = f;
         }
      }
      cp += nRow ? 0 : _DumpRow( cp, row );

      // Dump it out

      ::fwrite( _pTbl, cp-_pTbl, 1, stdout );
      _flush();
   }

   int _DumpRow( char *bp, Fields &row )
   {
      RTEDGE::Strings &cfmt = _colFmt;
      mddField         f;
      Fields::iterator it;
      char            *cp;
      const char      *pf;
      int              fid;
      size_t           i, nc;

      // Pre-condition

      if ( !row.size() )
         return 0;

      // Rock on ...

      cp = bp;
      nc = cfmt.size();
      for ( i=0; i<nc; i++ ) {
         fid = _csvFids[i];
         pf  = "-";
         if ( (it=row.find( fid )) != row.end() ) {
            f = (*it).second;
            _uFld.Set( f );
            pf = _uFld.GetAsString();
         }
         cp += sprintf( cp, cfmt[i].data(), pf );
         cp += sprintf( cp, " " );
      }

      // CR, clear out row, return size

      cp += sprintf( cp, "\n" );
      row.clear();
      return( cp - bp );
   }
   void OnRenko( Message &msg )
   {
      Renko           *r;
      Renkos::iterator rt;
      Field           *f;
      double           dv, dr, blk;
      string           s, st;
      const char      *svc, *tkr, *tm;
      int              n, fid;

      // Renko?

      fid = _Renko._Fid;
      blk = _Renko._BlockSz;
      if ( (f=msg.GetField( fid )) ) {
         dv  = f->GetAsDouble();
         dv *= _Renko._Mul;
         svc = msg.Service();
         tkr = msg.Ticker();
         tm  = pDateTimeMs( st, msg.MsgTime() );
         /*
          * Find; Create if not found
          */
         s   = tkr;
         blk = _Renko._BlockSz;
         if ( (rt=_rdb.find( s )) == _rdb.end() ) {
            r           = new Renko;
            r->_Fid     = fid;
            r->_BlockSz = blk;
            r->_Last    = dv;
            dr          = ::fmod( dv, blk );
            r->_Last   -= ( dr < blk ) ? dr : 0.0;
            r->_Lean    = "Black";
            _rdb[s]     = r;
         }
         else
            r = (*rt).second;
         for ( n=0; !InRange( r->_Last, dv, r->_Last+blk ); ) {
            if ( dv < r->_Last ) {
               r->_Last -= blk;
               n        -= 1;
               r->_Lean  = "Red";
            }
            else {
               r->_Last += blk;
               n        += 1;
               r->_Lean  = "Green";
            }
         }
         ::fprintf( stdout, "%s,%s,%s,%.2f+", tm, svc, tkr, dv );
         ::fprintf( stdout, "%.2f,%d,%s\n", r->_Last, n, r->_Lean );
         _flush();
         _nUpd += 1;
      }
   }

   void _flush()
   {
      fflush( stdout );
   }

}; // class MyChannel



//////////////////////////
// main()
//////////////////////////
static bool _IsTrue( const char *p )
{
   return( !::strcmp( p, "YES" ) || !::strcmp( p, "true" ) );
}

int main( int argc, char **argv )
{
   MyChannel   ch;
   Renko      &r = ch._Renko;
   const char *pc;
   FILE       *fp;
   const char *pt, *svr, *usr, *svc, *tkr, *t0, *t1, *tf, *r0, *r1, *r2;
   char       *cp, *rp, sTkr[K];
   bool        bCfg, aOK, bBin, bStr, bVec, bTape, bQry;
   void       *arg;
   string      s;
   u_int64_t   s0;
   double      slp;
   int         i, nt, tRun, ti, sn;
   ::MDDResult res; 
   ::MDDRecDef rd;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", SubscribeID() );
      printf( "%s\n", ch.Version() );
      return 0;
   }
   svr   = "localhost:9998";
   usr   = argv[0];
   svc   = "bloomberg";
   tkr   = NULL;
   t0    = NULL;
   t1    = NULL;
   ti    = 0;
   tf    = NULL;
   s0    = 0;
   sn    = 0;
   tRun  = 0;
   bBin  = true;
   bCfg  = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   bStr  = false;
   bVec  = false;
   bTape = true;
   bQry  = false;
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -h  <Source : host:port or TapeFile> ] \\ \n";
      s += "       [ -u  <Username> ] \\ \n";
      s += "       [ -s  <Service> ] \\ \n";
      s += "       [ -t  <Ticker : CSV or Filename> ] \\ \n";
      s += "       [ -t0 <TapeSliceStartTime> ] \\ \n";
      s += "       [ -t1 <TapeSliceEndTime> ] \\ \n";
      s += "       [ -ti <TapeSlice Sample Interval> ] \\ \n";
      s += "       [ -tf <CSV TapeSlice Sample Fields> ] \\ \n";
      s += "       [ -s0 <TapeSlice Start Offset> ] \\ \n";
      s += "       [ -sn <TapeSlice NumMsg> ] \\ \n";
      s += "       [ -r  <AppRunTime> ] \\ \n";
      s += "       [ -p  <Protocol MF|BIN> ] \\ \n";
      s += "       [ -bstr <If included, bytestream> ] \\ \n";
      s += "       [ -vector <If included, bytestream> ] \\ \n";
      s += "       [ -csv true ] \\ \n";
      s += "       [ -csvF <csv list of FIDs> \\ \n";
      s += "       [ -renko <FID,BoxSize[,Mult]>\n";
      s += "       [ -bds true ] \\ \n";
      s += "       [ -tapeDir <true for to pump in tape (reverse) dir> ] \\ \n";
      s += "       [ -query <true to dump directory, if available> ] \\ \n";
      s += "       [ -table fid:colW,fid:colW,...,eolFid ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -h       : %s\n", svr );
      printf( "      -u       : %s\n", usr );
      printf( "      -s       : %s\n", svc );
      printf( "      -t       : <empty>\n" );
      printf( "      -t0      : <empty>\n" );
      printf( "      -t1      : <empty>\n" );
      printf( "      -ti      : %d\n", ti );
      printf( "      -tf      : <empty>\n" );
      printf( "      -s0      : %ld\n", s0 );
      printf( "      -sn      : %d\n", sn );
      printf( "      -r       : %d\n", tRun );
      printf( "      -p       : %s\n", bBin ? "BIN" : "MF" );
      printf( "      -bstr    : %s\n", bStr ? "YES" : "NO" );
      printf( "      -vector  : %s\n", bVec ? "YES" : "NO" );
      printf( "      -csv     : %s\n", ch._bCSV ? "YES" : "NO" );
      printf( "      -csvF    : <empty>\n" );
      printf( "      -renko   : <empty>\n" );
      printf( "      -bds     : %s\n", ch._bds ? "YES" : "NO" );
      printf( "      -tapeDir : %s\n", bTape   ? "YES" : "NO" );
      printf( "      -query   : %s\n", bQry    ? "YES" : "NO" );
      printf( "      -table   : <empty>\n" );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-h" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-u" ) )
         usr = argv[++i];
      else if ( !::strcmp( argv[i], "-s" ) )
         svc = argv[++i];
      else if ( !::strcmp( argv[i], "-t" ) )
         tkr = argv[++i];
      else if ( !::strcmp( argv[i], "-t0" ) )
         t0  = argv[++i];
      else if ( !::strcmp( argv[i], "-t1" ) )
         t1  = argv[++i];
      else if ( !::strcmp( argv[i], "-ti" ) )
         ti  = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-tf" ) )
         tf  = argv[++i];
      else if ( !::strcmp( argv[i], "-s0" ) )
         s0  = atol( argv[++i] );
      else if ( !::strcmp( argv[i], "-sn" ) )
         sn  = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-r" ) )
         tRun = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-p" ) )
         bBin = ::strcmp( argv[++i], "MF" );
      else if ( !::strcmp( argv[i], "-csv" ) )
         ch._bCSV = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-csvF" ) )
         ch.LoadCSVFids( argv[++i] );
      else if ( !::strcmp( argv[i], "-bstr" ) )
         bStr = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-vector" ) )
         bVec = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-renko" ) ) {
         string sr( argv[++i] );

         r0 = ::strtok_r( (char *)sr.data(), ",", &rp );
         r1 = ::strtok_r( NULL, ",", &rp );
         r2 = ::strtok_r( NULL, ",", &rp );
         if ( r0 && r1 ) {
            r._Fid     = atoi( r0 ); 
            r._BlockSz = atof( r1 ); 
            r._Mul     = r2 ? atof( r2 ) : r._Mul; 
            if ( !r._Fid || ( r._BlockSz == 0.0 ) )
               r = _zRenko;
         }
      }
      else if ( !::strcmp( argv[i], "-bds" ) )
         ch._bds = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-tapeDir" ) )
         bTape = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-query" ) )
         bQry = _IsTrue( argv[++i] );
      else if ( !::strcmp( argv[i], "-table" ) )
         ch.LoadTable( argv[++i] );
   }
   printf( "%s\n", ch.Version() );
   ch.SetBinary( bStr || bBin || bVec );
   ch.SetTapeDirection( bTape );
   pc = ch.Start( svr, usr );
   printf( "%s\n", pc ? pc : "" );
   if ( !ch.IsValid() )
      return 0;

   // Tape Query??

   if ( bQry ) {
      printf( "Service,Ticker,NumMsg,\n" );
      res = ch.Query();
      for ( i=0; i<res._nRec; i++ ) {
         rd = res._recs[i];
         printf( "%s,%s,%d\n", rd._pSvc, rd._pTkr, rd._interval );
      }
      ch.FreeResult();
   }

   // Open Items; Run until user kills us

   vector<string *> tkrs;
   MyStream        *str;
   MyVector        *vec;

   if ( tkr && (fp=::fopen( tkr, "r" )) ) {
      while( ::fgets( (cp=sTkr), K, fp ) ) {
         ::strtok( sTkr, "#" );
         cp += ( strlen( sTkr ) - 1 );
         for ( ; cp > sTkr; ) {
            if ( ( *cp == '\r' ) || ( *cp == '\n' ) ||
                 ( *cp == '\t' ) || ( *cp == ' ' ) ) {
               cp--;
               continue; // for-i
            }
            break; // for-cp
         }
         cp[1] = '\0';
         if ( strlen( sTkr ) )
            tkrs.push_back( new string( sTkr ) );
      }
      ::fclose( fp );
   }
   else if ( tkr )
      tkrs.push_back( new string ( tkr ) );
   nt  = (int)tkrs.size();
   slp = (r2=::getenv( "__JD_SLEEP" )) ? atof( r2 ) : 2.5; 
   if ( bStr ) {
      ch.Sleep( slp); // Wait for protocol negotiation to finish
      for ( i=0; i<nt; i++ ) {
         str = new MyStream( svc, tkrs[i]->data() );
         ch.Subscribe( *str );
         printf( "ByteStream.Subscribe( %s,%s )\n", svc, str->Ticker() );
      }
   }
   else if ( bVec ) {
      ch.Sleep( slp ); // Wait for protocol negotiation to finish
      for ( i=0; i<nt; i++ ) {
         vec = new MyVector( svc, tkrs[i]->data() );
         vec->Subscribe( ch );
         printf( "Vector.Subscribe( %s,%s )\n", svc, vec->Ticker() );
      }
   }
   else if ( ch._bds )
      for ( i=0; i<nt; ch.OpenBDS( svc, tkrs[i++]->data(), arg_ ) );
   else {
      arg = ch.IsTable() ? (void *)0 : arg_;
      for ( i=0; i<nt; ch.Subscribe( svc, tkrs[i++]->data(), arg ) );
   }
   pt = ch.IsSnapshot() ? "snap again" : "stop";
   if ( ch.IsTape() ) {
      if ( t0 && t1 ) {
         if ( ti && tf )
            ch.PumpTapeSliceSample( t0, t1, ti, tf );
         else 
            ch.PumpTapeSlice( t0, t1 );
      }
      else if ( sn )
         ch.PumpFullTape( s0, sn );
      else
         ch.PumpTape();
   }
   if ( tRun )
      ch.Sleep( tRun );
   else {
      printf( "Hit <ENTER> to %s ...\n", pt ); getchar();
   }

   // Clean up

   printf( "Cleaning up ...\n" );
   ch.Stop();
   printf( "Done!!\n " );
   return 1;
}
