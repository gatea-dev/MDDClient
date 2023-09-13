/******************************************************************************
*
*  GreekServer.cpp
*
*  REVISION HISTORY:
*     13 SEP 2023 jcs  Created (from LVCDump.cpp)
*
*  (c) 1994-2023, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;
using namespace std;

typedef vector<int>                           Ints;
typedef map<u_int64_t, int, less<u_int64_t> > SortedInt64Map;

#define _STRIKE_PRC   66
#define _EXPIR_DATE   67
#define _UN_SYMBOL  4200

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *GreekServerID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)GreekServer Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

static void _DumpOne( Message &msg, Ints &fids )
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
   cp += sprintf( cp, "%s,%s,%s,", pt, tkr, act );
   cp += sprintf( cp, "%.2f,%d,", ld._dAge, ld._nUpd );
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
   ::fwrite( sm.data(), sm.size(), 1, stdout );
   ::fflush( stdout );
}

//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   Strings     tkrs;
   Ints        fids, idxs;
   string s;
   char        sTkr[4*K], *cp, *rp;
   bool        aOK, bCfg;
   size_t      nt;
   int         i, fid, ix;
   double      rate;
   const char *svr, *und, *flds, *val;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", GreekServerID() );
      return 0;
   }

   // cmd-line args

   svr  = "./cache.lvc";
   und  = "AAPL";
   flds = "3,6,22,25,32,66,67";
   rate = 1.0;
   bCfg = ( argc < 2 ) || ( argc > 1 && !::strcmp( argv[1], "--config" ) );
   if ( bCfg ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db      <LVC d/b filename> ] \\ \n";
      s += "       [ -u       <Underlyer> \\ \n";
      s += "       [ -r       <DumpRate> ] \\ \n";
      s += "       [ -f       <CSV Fids> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -db      : %s\n", svr );
      printf( "      -u       : %s\n", und );
      printf( "      -r       : %.2f\n", rate );
      printf( "      -f       : <empty>\n" );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=1; i<argc; i++ ) {
      aOK = ( i+1 < argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-db" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-u" ) )
         und = argv[++i];
      else if ( !::strcmp( argv[i], "-f" ) )
         flds = argv[++i];
      else if ( !::strcmp( argv[i], "-r" ) )
         rate = atof( argv[++i] );
   }

   /////////////////////
   // Do it
   /////////////////////
   LVC       lvc( svr );
   Message  *msg;
   Field    *fld;
   FieldDef *fd;
   Schema   &sch = lvc.GetSchema();

   /*
    * CSV : Field Name or FID
    */
   s   = flds;
   val = ::strtok_r( (char *)s.data(), ",", &rp );
   for ( ; val && strlen( val ); val=::strtok_r( NULL, ",", &rp ) ) {
      if ( (fid=atoi( val )) )
         fids.push_back( fid );
      else if ( (fd=sch.GetDef( val )) )
         fids.push_back( fd->Fid() );
   }
   if ( (nt=fids.size()) ) {
      cp  = sTkr;
      cp += sprintf( cp, "Index,Time,Ticker,Active,Age,NumUpd," );
      for ( size_t i=0; i<nt; i++ ) {
         if ( (fd=sch.GetDef( fids[i] )) )
            cp += sprintf( cp, "%s,", fd->pName() );
         else
            cp += sprintf( cp, "%d,", fids[i] );
      }
      cp += sprintf( cp, "\n" );
      ::fprintf( stdout, sTkr );
      ::fflush( stdout );
   }
   /*
    * First kiss : Walk all; build sorted list of indices by ( Expire, Strike )
    */
   double                   d0, age;
   SortedInt64Map           srt;
   SortedInt64Map::iterator st;
   rtVALUE                  v;
   u_int64_t                exp;

   d0 = lvc.TimeNs();
   {
      LVCAll   &all  = lvc.ViewAll();
      Messages &msgs = all.msgs();

      for ( size_t i=0; i<msgs.size(); i++ ) {
         msg = msgs[i];
         if ( !(fld=msg->GetField( _UN_SYMBOL )) )
            continue; // for-i
         val = fld->GetAsString();
         if ( ::strcmp( val, und ) )
            continue; // for-i
         /*
          * Sorted by ( Expire, Strike )
          */
         exp = 0;
         if ( (fld=msg->GetField( _EXPIR_DATE )) ) {
            v   = fld->field()._val;
            exp = (u_int64_t)v._r64;
         }
         exp     += (fld=msg->GetField( _STRIKE_PRC )) ? fld->GetAsInt32() : 0;
         srt[exp] = i;
      }
   }
   for ( st=srt.begin(); st!=srt.end(); idxs.push_back( (*st).second ), st++ );
   age = lvc.TimeNs() - d0;
   /*
    * Rock on
    */
   {
      LVCAll   &all  = lvc.ViewAll();
      Messages &msgs = all.msgs();

      for ( size_t i=0; i<idxs.size(); i++ ) {
         ix  = idxs[i];
         msg = msgs[ix];
         ::fprintf( stdout, "%d,", ix );
         _DumpOne( *msg, fids ); 
      }
      ::fprintf( stdout, "ViewAll() in %.2fs\n", all.dSnap() );
      ::fprintf( stdout, "Build List in %.2fs\n", age );
   }
   ::fprintf( stdout, "Done!!\n " );
   ::fflush( stdout );
   return 1;
}
