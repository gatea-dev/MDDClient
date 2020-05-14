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
*
*  (c) 1994-2019 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <vector>

using namespace RTEDGE;
using namespace std;

static void *arg_ = (void *)"User-supplied argument";

#if defined(K)
#undef K
#endif // K
#if !defined(hash_map)
#if !defined(_GLIBCXX_UNORDERED_MAP) && !defined(WIN32)
#include <tr1/unordered_map>
#define hash_map tr1::unordered_map
#else
#include <unordered_map>
#define hash_map unordered_map
#endif // !defined(_GLIBCXX_UNORDERED_MAP)
#endif // defined(hash_map)

#define K 1024

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

typedef vector<int>               Fids;
typedef hash_map<int, rtFIELD>    Fields;
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
      cp += sprintf( cp, "@(#)Subscribe Build 44 " );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


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
   RTEDGE::Field _uFld;
   bool          _bDump;
   bool          _bCSV;
   bool          _bds;
   bool          _bHdr;
   bool          _bSnap;
   int           _nUpd;
   Fids          _csvFids;
   Renko         _Renko;
   Renkos        _rdb;

   // Constructor
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
      _Renko( _zRenko ),
      _rdb()
   {
   }


   ///////////////////////////////////
   // Mutator
   ///////////////////////////////////
public:
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
      rtFIELD          *flds = msg.Fields();
      RTEDGE::FieldDef *def;
      rtFIELD           f;
      mddField         *w;
      Fields            fdb;
      Fields::iterator  it;
      string            s, st;
      const char       *svc, *tkr, *pm, *tm;
      int               i, nf, fid;

      // Renko?

      if ( _Renko._Fid ) {
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
         tm = pDateTimeMs( st );
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

   virtual void OnDead( Message &msg, const char *err )
   {
      string      s;
      const char *tm, *svc, *tkr, *mt;

      tm  = pDateTimeMs( s, msg.MsgTime() );
      mt  = msg.MsgType();
      svc = msg.Service();
      tkr = msg.Ticker();
      ::fprintf( stdout, "[%s] %s ( %s,%s ) :%s\n", tm, mt, svc, tkr, err );
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
         tm  = pDateTimeMs( st );
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
int main( int argc, char **argv )
{
   MyChannel   ch;
   Renko      &r = ch._Renko;
   const char *pc;
   FILE       *fp;
   const char *pt, *svr, *usr, *svc, *tkr, *t0, *t1, *r0, *r1, *r2;
   char       *pa, *cp, *rp, sTkr[K];
   bool        bCfg, aOK, bBin, bStr;
   string      s;
   int         i, nt, tRun;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", SubscribeID() );
      printf( "%s\n", ch.Version() );
      return 0;
   }
   svr  = "localhost:9998";
   usr  = argv[0];
   svc  = "bloomberg";
   tkr  = NULL;
   t0   = NULL;
   t1   = NULL;
   tRun = 0;
   bBin = true;
   bCfg = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   bStr = false;
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -h  <Source : host:port or TapeFile> ] \\ \n";
      s += "       [ -u  <Username> ] \\ \n";
      s += "       [ -s  <Service> ] \\ \n";
      s += "       [ -t  <Ticker : CSV or Filename> ] \\ \n";
      s += "       [ -t0 <TapeSliceStartTime> ] \\ \n";
      s += "       [ -t1 <TapeSliceEndTime> ] \\ \n";
      s += "       [ -r  <AppRunTime> ] \\ \n";
      s += "       [ -p  <Protocol MF|BIN> ] \\ \n";
      s += "       [ -bstr <If included, bytestream> ] \\ \n";
      s += "       [ -csv true ] \\ \n";
      s += "       [ -csvF <csv list of FIDs> \\ \n";
      s += "       [ -renko <FID,BoxSize[,Mult]>\n";
      s += "       [ -bds true ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -h     : %s\n", svr );
      printf( "      -u     : %s\n", usr );
      printf( "      -s     : %s\n", svc );
      printf( "      -t     : <empty>\n" );
      printf( "      -t0    : <empty>\n" );
      printf( "      -t1    : <empty>\n" );
      printf( "      -r     : %d\n", tRun );
      printf( "      -p     : %s\n", bBin ? "BIN" : "MF" );
      printf( "      -bstr  : %s\n", bStr ? "YES" : "NO" );
      printf( "      -csv   : %s\n", ch._bCSV ? "YES" : "NO" );
      printf( "      -csvF  : <empty>\n" );
      printf( "      -renko : <empty>\n" );
      printf( "      -bds   : %s\n", ch._bds ? "YES" : "NO" );
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
      else if ( !::strcmp( argv[i], "-r" ) )
         tRun = atoi( argv[++i] );
      else if ( !::strcmp( argv[i], "-p" ) )
         bBin = ::strcmp( argv[++i], "MF" );
      else if ( !::strcmp( argv[i], "-csv" ) ) {
         pa        = argv[++i];
         ch._bCSV |= !::strcmp( pa, "YES" ) || !::strcmp( pa, "true" );
      }
      else if ( !::strcmp( argv[i], "-csvF" ) )
         ch.LoadCSVFids( argv[++i] );
      else if ( !::strcmp( argv[i], "-bstr" ) ) {
         pa    = argv[++i];
         bStr |= !::strcmp( pa, "YES" ) || !::strcmp( pa, "true" );
      }
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
      else if ( !::strcmp( argv[i], "-bds" ) ) {
         pa       = argv[++i];
         ch._bds |= !::strcmp( pa, "YES" ) || !::strcmp( pa, "true" );
      }
   }
   printf( "%s\n", ch.Version() );
   ch.SetBinary( bStr || bBin );
   pc = ch.Start( svr, usr );
   printf( "%s\n", pc ? pc : "" );
   if ( !ch.IsValid() )
      return 0;

   // Open Items; Run until user kills us

   vector<string *> tkrs;
   MyStream        *str;

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
            tkrs.push_back( new std::string( sTkr ) );
      }
      ::fclose( fp );
   }
   else if ( tkr )
      tkrs.push_back( new string ( tkr ) );
   nt = (int)tkrs.size();
   if ( bStr ) {
      ch.Sleep( 1.0 ); // Wait for protocol negotiation to finish
      for ( i=0; i<nt; i++ ) {
         str = new MyStream( svc, tkrs[i]->data() );
         ch.Subscribe( *str );
         printf( "ByteStream.Subscribe( %s,%s )\n", str->svc(), str->tkr() );
      }
   }
   else if ( ch._bds )
      for ( i=0; i<nt; ch.OpenBDS( svc, tkrs[i++]->data(), arg_ ) );
   else
      for ( i=0; i<nt; ch.Subscribe( svc, tkrs[i++]->data(), arg_ ) );
   pt = ch.IsSnapshot() ? "snap again" : "stop";
   if ( ch.IsTape() ) {
      if ( t0 && t1 )
         ch.StartTapeSlice( t0, t1 );
      else
         ch.StartTape();
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
