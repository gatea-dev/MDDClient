/******************************************************************************
*
*  CSVPublish.cpp
*     Publish field list from file
*
*  REVISION HISTORY:
*     22 APR 2016 jcs  Created.
*
*  (c) 1994-2016 Gatea Ltd.
******************************************************************************/
#include <librtEdge.h>
#include <stdarg.h>

static void breakpoint() { ; }

using namespace RTEDGE;
using namespace std;

static bool _bBin   = false;

// Forward

class Record;
class MyChannel;
class CField;

typedef map<string, Record *> Cache;
typedef vector<CField *>      Fields;
typedef vector<string>        Strings;
typedef vector<int>           Fids;
typedef vector<rtFldType>     Types;


////////////////////
//
//  C F i e l d
//
////////////////////
class CField
{
public:
   int       _fid;
   rtFldType _ty;
   string    _val;

   ///////////////////
   // Constructor
   ///////////////////
public:
   CField( int fid, rtFldType ty, string &val ) :
      _fid( fid ),
      _ty( ty ),
      _val( val )
   { ; }

   CField( const CField &c ) :
      _fid( c._fid ),
      _ty( c._ty ),
      _val( c._val )
   { ; }

   ~CField()
   { ; }


   ///////////////////
   // Access / Operations
   ///////////////////
   const char *pVal()
   {
      return _val.data();
   }

   bool IsEqual( const CField &c )
   {
      return ( ( _fid == c._fid ) &&
               ( _ty  == c._ty  ) &&
               ( _val == c._val ) );
   }

   void AddField( Update &u )
   {
      rtDateTime dtTm;
      int        i32;
      double     r64;
      float      r32;

      i32 = ::atoi( pVal() );
      r64 = _atof( pVal() );
      r32 = (float)r64;
      switch( _ty ) {
         case rtFld_string:
            u.AddField( _fid, pVal() );
            break;
         case rtFld_int:
         case rtFld_int8:
         case rtFld_int16:
         case rtFld_int64:
            u.AddField( _fid, i32 );
            break;
         case rtFld_double:
         case rtFld_real:
            u.AddField( _fid, r64 );
            break;
         case rtFld_date:
         case rtFld_time:
         case rtFld_timeSec:
            dtTm = _DateTime( pVal() );
            u.AddField( _fid, dtTm );
            break;
         case rtFld_float:
            u.AddField( _fid, r32 );
            break;
         default:
            u.AddField( _fid, pVal() ); // TODO
            break;
      }
   }

   ///////////////////
   // Helpers
   ///////////////////
private:
   rtDateTime _DateTime( const char *pf )
   {
      rtDateTime dtTm;
      rtDate    &dt = dtTm._date;
      rtTime    &tm = dtTm._time;
      string     s( pf );
      char      *rp, *pn, *pd, *pt;

      /*
       * 2016-04-25 19:48:00
       * 2016-04-25
       * 19:48:00
       */
      ::memset( &dtTm, 0, sizeof( rtDateTime ) );
      pn = (char *)s.data();
      pd = ::strtok_r( pn, " ", &rp );
      pt = ::strtok_r( NULL, " ", &rp );
      if ( pt ) {
         _Date( pd, dt );
         _Time( pt, tm );
      }
      else if ( !_Date( pd, dt ) )
         _Time( pd, tm );
      return dtTm;
   }

   bool _Date( const char *pf, rtDate &dt )
   {
      char       *py, *pm, *pd, *rp;
      const char *sep = "-";

      // 2016-04-25

      ::memset( &dt, 0, sizeof( rtDate ) );
      py = ::strtok_r( (char *)pf, sep, &rp );
      if ( !(pm=::strtok_r( NULL, sep, &rp )) )
         return false;
      if ( !(pd=::strtok_r( NULL, sep, &rp )) )
         return false;
      dt._year  = atoi( py );
      dt._month = atoi( pm );
      dt._mday  = atoi( pd );
      return true;
   }

   bool _Time( const char *pf, rtTime &tm )
   {
      char       *ph, *pm, *ps, *rp;
      const char *sep = ":";

      // 19:48:00

      ::memset( &tm, 0, sizeof( rtTime ) );
      ph = ::strtok_r( (char *)pf, sep, &rp );        
      if ( !(pm=::strtok_r( NULL, sep, &rp )) )
         return false;
      if ( !(ps=::strtok_r( NULL, sep, &rp )) )
         return false;
      tm._hour   = atoi( ph );
      tm._minute = atoi( pm );
      tm._second = atoi( ps );
      tm._micros = 0;
      return true; 
   }

   double _atof( const char *pf )
   {
      string s( pf );
      char  *pt, *pn, *pd;
      double dv, df;
      int    dec, num, den;

      /*
       * Fraction:
       *     99 24/32
       *     12/32
       */
      pt = (char *)s.data();
      pd = (char *)0;
      if ( (pn=::strchr( pt, ' ' )) ) {
         *pn = '\0';
         pn += 1;
         pd  = ::strchr( pn, '/' );
         if ( pd ) {
            *pd = '\0';
            pd += 1;
         }
      }
      else if ( (pd=::strchr( pt, '/' )) ) {
         pn  = pt;
         pt  = (char *)0;
         *pd = '\0';
         pd += 1;
      }
      if ( pd ) {
         dec = pt ? ::atoi( pt ) : 0;
         num = ::atoi( pn );
         den = ::atoi( pd );
         dv  = 1.0 * dec;
         df  = den ? num / ( 1.0 * den ) : 0.0;
         dv += df;
// printf( "%d %d/%d = %d + %.6f = ", dec, num, den, dec, df );
      }
      else
         dv = ::atof( pf );
// printf( "%.6f\n", dv );
      return dv;
   }
};


////////////////////
//
//  R e c o r d
//
////////////////////
class Record
{
private:
   string _tkr;
   Fields _flds;
   void  *_arg;
   int    _rtl;
   bool   _bPub;

   ///////////////////
   // Constructor
   ///////////////////
public:
   Record( const char *tkr ) :
      _tkr( tkr ),
      _flds(),
      _arg( (void *)0 ),
      _rtl( 1 ),
      _bPub( true )
   { ; }

   ~Record()
   {
      _Clear();
   }


   ///////////////////
   // Access
   ///////////////////
public:
   bool IsWatched()
   {
      return( _arg != (void *)0 );
   }

   const char *tkr()
   {
      return _tkr.c_str();
   }

   Fields &flds()
   {
      return _flds;
   }


   ///////////////////
   // Mutator
   ///////////////////
   void SetWatch( void *arg )
   {
      _arg  = arg;
      _bPub = true;
   }

   void ClearWatch()
   {
      SetWatch( (void *)0 );
      _bPub = false;
   }

   void SetFields( Fields &flds )
   {
      string s;
      size_t i, n, nf;

      // OK to continue

      n     = flds.size();
      nf    = _flds.size();
      _bPub = ( n != nf );
      for ( i=0; !_bPub && i<n; i++ )
         _bPub |= !_flds[i]->IsEqual( *flds[i] );
      _Clear();
      for ( i=0; i<n; _flds.push_back( new CField( *flds[i++] ) ) );
   }


   /////////////////////
   // Operations
   /////////////////////
   bool Publish( Update &u )
   {
      size_t i;

      // ( Ticker, Rate ) now

      if ( IsWatched() && _bPub ) {
         u.Init( tkr(), _arg, true );
         for ( i=0; i<_flds.size(); _flds[i++]->AddField( u ) );
         u.Publish();
         _bPub = false;
         return true;
      }
      return false;
   }


   /////////////////////
   // Helpers
   /////////////////////
private:
   void _Clear()
   {
      size_t i;

      for ( i=0; i<_flds.size(); delete _flds[i++] );
      _flds.clear();
   }
};


////////////////////////
//
//     M y P u b
//
////////////////////////

class MyChannel : public PubChannel
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
private:
   string _file;
   Cache  _wl;
   Mutex  _mtx;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
public:
   MyChannel( const char *pPub, const char *pFile ) :
      PubChannel( pPub ),
      _file( pFile ),
      _wl(),
      _mtx()
   {
      SetUserPubMsgTy( true );
      SnapCSV();
   }


   ////////////////////////////////
   // Operations
   ////////////////////////////////
public:
   int SnapCSV()
   {
      Schema     &sch = schema();
      rtBUF       csv;
      string      s, t;
      Strings     rows, colHdr, vals;
      Fields      flds;
      FieldDef   *def;
      rtFldType   fTy;
      Fids        fdb;
      Types       tdb;
      Record     *rec;
      const char *pf, *tkr;
      char       *cp;
      int         fid;
      size_t      r, c, nr, nc;
      const char *_rs = "\n"; 
      const char *_cs = ",";

      // 1) File Modified?

breakpoint(); // TODO

      // 2) Map File

      pf  = _file.data();
      csv = MapFile( pf );
      if ( !csv._dLen )
         return 0;

      // 3) Chop up lines
      {
         string tmp( csv._data, csv._dLen );

         s = tmp;
      }
      cp = (char *)s.data();
      for ( cp=::strtok( cp,_rs ); cp; cp=::strtok( NULL,_rs ) )
         rows.push_back( string( cp ) );
      UnmapFile( csv );
      if ( !(nr=rows.size()) )
         return nr;

      // 4) Column Headers : TODO : Field Type?

      cp = (char *)rows[0].data();
      for ( c=0,cp=::strtok( cp,_cs ); cp; cp=::strtok( NULL,_cs ), c++ ) {
         def = sch.GetDef( cp );
         fTy = def ? def->fType() : rtFld_string;
         fid = def ? def->Fid() : 0;
         fdb.push_back( fid );
         tdb.push_back( fTy );
         colHdr.push_back( string( cp ) );
      }

      // 5) Chop up rows / Publish : ( Ticker,Rate ) now

      for ( r=1; r<nr; r++ ) {
         cp = (char *)rows[r].data();
         for ( c=0,cp=::strtok( cp,_cs ); cp; cp=::strtok( NULL,_cs ), c++ )
            vals.push_back( string( cp ) );
         if ( (nc=vals.size()) > 1 ) {
            t   = vals[0];
            tkr = t.data();
            if ( !(rec=GetRecord( tkr )) ) {
               rec    = new Record( tkr );
               _wl[t] = rec;
            }
            for ( c=1; c<nc; c++ )
               flds.push_back( new CField( fdb[c], tdb[c], vals[c] ) );
            rec->SetFields( flds );
            if ( rec->Publish( upd() ) )
               _fprintf( "PUB %s", rec->tkr() );
            for ( c=0; c<flds.size(); delete flds[c++] );
            flds.clear();
         }
         vals.clear();
      }

      _fprintf( "Snapped %ld tkrs", nr-1 );
      return nr-1;
   }

   void _fprintf( const char *fmt, ... )
   {
      Locker  lck( _mtx );
      string  tm;
      char    sText[8*K];
      char   *sp;
      va_list ap;

      // varargs ... Tried and True

      sp  = sText;
      sp += sprintf( sp, "[%s] ", pDateTimeMs( tm ) );
      va_start( ap,fmt );
      sp += vsprintf( sp, fmt, ap );
      va_end( ap );
      sp += sprintf( sp, "\n" );

      // Flush ...

      ::fprintf( stdout, sText );
      ::fflush( stdout );
   } 

   ////////////////////////////////
   // Asynchronous Callbacks
   ////////////////////////////////
public:
   virtual void OnConnect( const char *msg, bool bUP )
   {
      const char *pUp = bUP ? "UP" : "DOWN";

      _fprintf( "CONN %s : %s", pUp, msg );
   }

   virtual void OnPubOpen( const char *tkr, void *arg )
   {
      Locker  lck( _mtx );
      Record *rec;

      _fprintf( "OPEN %s", tkr );
      if ( (rec=GetRecord( tkr )) ) {
         rec->SetWatch( arg );
         rec->Publish( upd() );
      }
      else {
breakpoint(); // TODO : Publish Dead
      }
   }

   virtual void OnPubClose( const char *tkr )
   {
      Locker  lck( _mtx );
      Record *rec;

      _fprintf( "CLOSE %s", tkr );
      if ( (rec=GetRecord( tkr )) )
         rec->ClearWatch();
   }

   virtual void OnSchema( Schema &s )
   {
      _fprintf( "SCHEMA %d lds", s.Size() );
   }


   ////////////////////////////////////
   // PubChannel Interface
   ////////////////////////////////////
protected:
   virtual Update *CreateUpdate()
   {
      return new Update( *this );
   }


   ////////////////////////////////////
   // Helpers
   ////////////////////////////////////
private:
   Record *GetRecord( const char *tkr )
   {
      Locker          lck( _mtx );
      Cache::iterator it;
      string          s( tkr );
      Record         *rec;

      rec = (Record *)0;
      if ( (it=_wl.find( s )) != _wl.end() )
         rec = (*it).second;
      return rec;
   }

};

int main( int argc, char **argv )
{
   const char *pSvr, *pPub, *pCSV, *pc;
   double      tSlp;
   int         i, nt;
   rtBUF       chn;
   mddProtocol pro;

   // Quickie check

   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", rtEdge::Version() );
      return 0;
   }

   // cmd-line args

   _bBin = ( ::getenv( "__JD_BIN" ) != (char *)0 );
   if ( argc < 5 ) {
      pc = "Usage: %s <hosts> <Svc> <pubTmr> <CSVfile>";
      printf( pc, argv[0] );
      printf( "; Exitting ...\n" );
      return 0;
   }
   pro  = _bBin ? mddProto_Binary : mddProto_MF;
   pSvr = argv[1];
   pPub = argv[2];
   tSlp = atof( argv[3] );
   pCSV = argv[4];

   MyChannel   pub( pPub, pCSV );
   string s;

   pub.SetBinary( _bBin );
   pub.SetHeartbeat( 15 );
// pub.SetPerms( true );
   pub._fprintf( pub.Version() );
   ::fprintf( stdout, "%s\n", pub.Start( pSvr ) );
   ::fprintf( stdout, "Publish every %.1fs\n", tSlp );
   ::fflush( stdout );
   for ( ; pub.GetProtocol() != pro; pub.Sleep( 0.25 ) );
   ::fprintf( stdout, "Protocol ready ...\n" );
   for ( i=0; ; pub.Sleep( tSlp ), i++ )
      nt = pub.SnapCSV();

   // Clean up

   ::fprintf( stdout, "Cleaning up ...\n" ); ::fflush( stdout );
   pub.Stop();
   pub.UnmapFile( chn );
   printf( "Done!!\n " );
   return 1;
}
