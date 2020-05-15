/******************************************************************************
*
*  ygrep.cpp
*     grep yamr tape
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*      4 NOV 2019 jcs  Build  3: OnStringDict() - DUH!!
*
*  (c) 1994-2019, Gatea Ltd.
******************************************************************************/
#include <libyamr.h>
#include <bespoke/Usage.hpp>

using namespace std;
using namespace YAMR;
using namespace YAMR::Data;

typedef hash_map<u_int64_t, int>  QueryMap;

////////////////////////////////////////////////
//
//     c l a s s    y g r e p L i s t e n e r
//
////////////////////////////////////////////////
class ygrepListener : YAMR::Data::DataListener
{
public:
   YAMR::DumpType _dmpBody;
   u_int32_t      _addr;
   u_int16_t      _sess;
   u_int16_t      _pro;
   int            _NumQuery;

   ////////////////////////////////////
   // Constructor / Destructor
   ////////////////////////////////////
   /**
    * \brief Constructor
    *
    * \param reader - Tape Reader driving messages into us
    */
   ygrepListener( YAMR::Reader &reader ) :
      YAMR::Data::DataListener( reader ),
      _dmpBody( YAMR::dump_none ),
      _addr( 0 ),
      _sess( 0 ),
      _pro( 0 ),
      _NumQuery( 0 )
   { ; }


   /////////////////////////////////////
   // Access / Operations
   /////////////////////////////////////
   const char *ygrepID()
   {
      static string s;
      const char   *sccsid;

      // Once

      if ( !s.length() ) {
         char bp[K], *cp;

         cp  = bp;
         cp += sprintf( cp, "@(#)ygrep Build 3 " );
         cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
         s   = bp;
      }
      sccsid = s.data();
      return sccsid+4;
   }

   void ProgressBar( int i )
   {
      if ( i && !(i%1000000) )
         printf( "%d", i/1000000 );
      else if ( i && !(i%500000) )
         printf( "+" );
      else if ( i && !(i%100000) )
         printf( "."  );
      fflush( stdout );
   }


   ////////////////////////////////////
   // IReadListener Interface
   ////////////////////////////////////
private:
   virtual void OnMessage( Reader &reader, yamrMsg &msg, Data::Codec *codec )
   {
      u_int16_t proto;

      // 1) Unknown Protocol??

      if ( !codec ) {
         OnUnknown( msg );
         return;
      }

      // 2) Currently Supported Protocols

      proto = msg._MsgProtocol;
      switch( proto ) {
         case _PROTO_USAGE:
            if ( !_IsFiltered( msg ) )
               _Dump( "USG", msg );
            break;
         default:
            DataListener::OnMessage( reader, msg, codec );
            break;
      }
   }

   ////////////////////////////////////
   // DataListener Interface
   ////////////////////////////////////
public:
   virtual void OnFieldList( yamrMsg &msg, FieldList *codec )
   {
      if ( !_IsFiltered( msg ) )
         _Dump( "FLD", msg );
   }

   virtual void OnFloatList( yamrMsg &msg, FloatList *codec )
   {
      if ( !_IsFiltered( msg ) )
         _Dump( "FLT", msg );
   }

   virtual void OnStringDict( yamrMsg &msg, StringDict *codec )
   {
      if ( !_IsFiltered( msg ) )
         _Dump( "DCT", msg );
   }

   virtual void OnStringMap( yamrMsg &msg, StringMap *codec )
   {
      if ( !_IsFiltered( msg ) )
         _Dump( "SM", msg );
   }

   virtual void OnUnknown( yamrMsg &msg )
   {
      if ( !_IsFiltered( msg ) )
         _Dump( "UNK", msg );
   }


   ////////////////////////////////////
   // Helpers
   ////////////////////////////////////
private:
   void _Dump( const char *ty, yamrMsg &y )
   {
      string hdr, body;

      // Pre-condition

      if ( _IsFiltered( y ) )
         return;

      // OK to Dump

      hdr  = reader().DumpHeader( y );
      body = "";
      if ( _dmpBody != YAMR::dump_none )
         body = reader().DumpBody( y, _dmpBody );
      switch( _dmpBody ) {
         case YAMR::dump_CSV:
         case YAMR::dump_JSON:
         case YAMR::dump_JSONmin:
            ::fprintf( stdout, body.data() );
            break;
         default:
            ::fprintf( stdout, "{%s} %s %s\n", ty, hdr.data(), body.data() );
      }
   }

   bool _IsFiltered( yamrMsg &y )
   {
      if ( _NumQuery )
         return true;
      if ( _pro && ( y._MsgProtocol != _pro ) )
         return true;
      if ( _addr && ( y._Host != _addr ) )
         return true;
      if ( _sess && ( y._SessionID != _sess ) )
         return true;
      return false;
   }

}; // class ygrepListener


////////////////////////
//
//     main()
//
////////////////////////
int main( int argc, char **argv )
{
   YAMR::Reader  rd;
   ygrepListener lsn( rd );
   const char   *eol;
   char         *arg, *p1, *p2, *rp;
   int           i, nMsg;
   u_int16_t     sess, pro;
   yamrMsg       y;
   yamrBuf       yb;

   // cmd-line args

   eol = " \\\n   ";
   arg = ( argc > 1 ) ? argv[1] : (char *)0;
   if ( arg && !::strcmp( arg, "--version" ) ) {
      printf( "%s\n", lsn.ygrepID() );
      printf( "%s\n", rd.Version() );
      return 0;
   }
   if ( ( argc < 2 ) || !::strcmp( "-help", arg ) || !::strcmp( "-h", arg ) ) {
      printf( "Usage : %s <tape_filename> %s", argv[0], eol );
      printf( "-query [<NumMsgMil>]%s", eol );
      printf( "-session <ipAddr>:<SessID>%s", eol );
      printf( "-protocol <Proto ID or Name>%s", eol );
      printf( "-dump%s", eol );
      printf( "-dumpCSV%s", eol );
      printf( "-dumpJSON%s", eol );
      printf( "-dumpJSONmin\n" );
      return 0;
   }

   // Validate File

   yb = rd.MapFile( argv[1], false );
   if ( !yb._data ) {
      printf( "Invalid tape %s; Exitting ...\n", argv[1] );
      return 0;
   } 
   rd.UnmapFile( yb );

   // Supported Protocols

   YAMR::Data::FieldList md( rd );
   YAMR::Data::StringMap sm( rd );
   YAMR::bespoke::Usage  prm( rd );

   // Parse 'em up

   for ( i=2; i<argc; ) {
      arg = argv[i++];
      if ( !::strcmp( "-query", arg ) )
         lsn._NumQuery = ( i < argc ) ? atoi( argv[i++] ) : K;
      else if ( !::strcmp( "-session", arg ) && ( i < argc ) ) {
         string      s( argv[i++] );
         const char *_SEP = ":";

         p1        = ::strtok_r( (char *)s.data(), _SEP, &rp );
         p2        = ::strtok_r( NULL, _SEP, &rp );
         lsn._addr = (u_int32_t)inet_addr( p1 );
         lsn._sess = p2 ? strtol( p2, NULL, 0 ) : 0;
      }
      else if ( !::strcmp( "-protocol", arg ) && ( i < argc ) ) {
         p1  = argv[i++];
         if ( !(pro=strtol( p1, NULL, 0 )) )
            lsn._pro = rd.ProtoNumber( p1 );
      }
      else if ( !::strcmp( "-dump", arg ) )
         lsn._dmpBody = YAMR::dump_verbose;
      else if ( !::strcmp( "-dumpCSV", arg ) )
         lsn._dmpBody = YAMR::dump_CSV;
      else if ( !::strcmp( "-dumpJSON", arg ) )
         lsn._dmpBody = YAMR::dump_JSON;
      else if ( !::strcmp( "-dumpJSONmin", arg ) )
         lsn._dmpBody = YAMR::dump_JSONmin;
   }

   // Query?

   QueryMap           qdb;
   QueryMap::iterator qt;
   u_int64_t          stream;
   u_int32_t          u32, l32;
   struct in_addr     ip;

   if ( lsn._NumQuery ) {
      /*
       * Read up to lsn._NumQuery Million Msgs, idx by ( Addr, SessID, Proto )
       */
      lsn._NumQuery *= K;
      lsn._NumQuery *= K;
      rd.Open( argv[1] );
      printf( "Querying %s\n", argv[1] );
      for ( i=0,rd.Rewind(); i<lsn._NumQuery && rd.Read( y ); i++ ) {
         lsn.ProgressBar( i );
         rd.Decode( y );
         stream      = y._Host;
         stream    <<= 32;
         l32         = y._SessionID;
         l32       <<= 16;
         l32        += y._MsgProtocol;
         stream     += l32;
         qt          = qdb.find( stream );
         nMsg        = ( qt != qdb.end() ) ? (*qt).second : 0;
         qdb[stream] = nMsg+1;
      }
      /*
       * Dump Msg Count by ( Addr, SessID, Proto )
       */
      printf( "\n" );
      printf( "Count    IP Address      SessID Proto  ProtoName     \n" );
      printf( "-------- --------------- ------ ------ --------------\n" );
      for ( qt=qdb.begin(); qt!=qdb.end(); qt++ ) {
         stream    = (*qt).first;
         nMsg      = (*qt).second;
         u32       = (u_int32_t)( stream >> 32 );
         l32       = (u_int32_t)stream;
         sess      = (u_int16_t)( l32 >> 16 );
         pro       = (u_int16_t)l32;
         ip.s_addr = u32;
         p1        = inet_ntoa( ip );
         p2        = (char *)rd.ProtoName( pro );
         printf( "%8d %-15s %6d 0x%04x %s\n", nMsg, p1, sess, pro, p2 );
      }
      ::fflush( stdout );
      rd.Close();
      return 0;
   }

   // Rock on

   ::fprintf( stdout, "%s\n", lsn.ygrepID() );
   ::fprintf( stdout, "%s\n", rd.Version() );
   ::fprintf( stdout, "[Time] SeqNum <<Proto>> host:sessID data\n" );
   rd.Open( argv[1] );
   for ( i=0,rd.Rewind(); rd.Read( y ); rd.Decode( y ), i++ );
   rd.Close();
   printf( "Done!!\n " );
   return 1;
}
