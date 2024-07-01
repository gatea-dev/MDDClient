/******************************************************************************
*
*  LVCPerfCLI.cs
*     C# LVC Performance Test
*
*  REVISION HISTORY:
*     27 JUN 2024 jcs  Created.
*
*  (c) 1994-2024, Gatea, Ltd.
******************************************************************************/
#include <librtEdge.h>

using namespace RTEDGE;


/////////////////////////////////////
// Version
/////////////////////////////////////
const char *LVCPerfID()
{
   static std::string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)LVCPerf Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.\n", __DATE__, __TIME__ );
      cp += sprintf( cp, rtEdge::Version() );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}

// Forwards / Collections

class TestCfg;
class TestStat;

typedef std::vector<int>          FidSet;
typedef hash_map<int, int>        FieldDict;
typedef std::vector<const char *> Services;
typedef hash_set<string>          StringSet;
typedef std::vector<TestCfg *>    TestCfgs;
typedef std::vector<TestStat *>   TestStats;


/////////////////////////////////////
//
//    c l a s s   T e s t C f g
//
/////////////////////////////////////
class TestCfg
{
public:
   Strings     svcs;
   std::string flds;
   FidSet      fids;
   bool        bSvcFltr;
   bool        bFldFltr;
   bool        bFldType;

   /////////////////
   // Constructor
   /////////////////
public:
   TestCfg() :
      svcs(),
      flds(),
      fids(),
      bSvcFltr( false ),
      bFldFltr( false ),
      bFldType( false )
   { ; }

   /////////////////
   // Access
   /////////////////
public:
   std::string Descr()
   {
      char   bp[K], *cp;
      size_t i, n;

      // Services,Fields,Filter,FldType,

      /*
       * Services
       */
      cp = bp;
      if ( !(n=svcs.size()) ) {
         cp      += sprintf( cp, "All" );
         bSvcFltr = false;
      }
      else
         for ( i=0; i<n; cp += sprintf( cp, "%s;", svcs[i].data() ), i++ );
      cp += sprintf( cp, "," );
      /*
       * Fields
       */
      if ( !(n=flds.size()) ) {
         cp      += sprintf( cp, "All" );
         bFldFltr = false;
      }
      else
         cp += sprintf( cp, flds.data() );
      cp += sprintf( cp, "," );
      /*
       * Filter
       */
      if ( bSvcFltr || bFldFltr ) {
         cp += bSvcFltr ? sprintf( cp, "SVC " ) : 0;
         cp += bFldFltr ? sprintf( cp, "FLD" )  : 0;
      }
      else
         cp += sprintf( cp, "None" );
      cp += sprintf( cp, "," );
      /*
       * Field Type
       */
      cp += sprintf( cp, bFldType ? "Native," : "String," );
      return std::string( bp );
   }

   /////////////////
   // Field Value
   /////////////////
   int GetValue( Field &f )
   {
      const char *pf;
      ::int64_t   i64;
      double      r64;

      // Just Dump

      if ( !bFldType ) {
         pf = f.GetAsString();
         return f.Fid();
      }

      // By Type

      switch( f.TypeFromMsg() ) {
         case rtFld_int:     i64 = f.GetAsInt32();   break;
         case rtFld_double:  r64 = f.GetAsDouble();  break;
         case rtFld_date:
         case rtFld_time:
         case rtFld_timeSec: i64 = f.GetAsInt64();   break;
//            case rtFld_timeSec: dtTm = f.GetAsDateTime();   break;
         case rtFld_float:   r64 = f.GetAsFloat();   break;
         case rtFld_int8:    i64 = f.GetAsInt8();    break;
         case rtFld_int16:   i64 = f.GetAsInt16();   break;
         case rtFld_int64:   i64 = f.GetAsInt64();   break;
         case rtFld_string:
         default:            break;
      }
      pf = f.GetAsString();
      return f.Fid();
   }

}; // class TestCfg


/////////////////////////////////////
//
//    c l a s s   T e s t S t a t
//
/////////////////////////////////////
class TestStat
{
public:
   TestCfg    &_cfg;
   std::string Descr;
   int         tLibSnap;
   int         tSnap;
   int         tPull;
   int         tAll;
   int         NumTkr;
   int         NumFld;

   ////////////////
   // Constructor
   ////////////////
public:
   TestStat( TestCfg &cfg ) :
      _cfg( cfg ),
      Descr( cfg.Descr() ),
      tLibSnap( 0 ),
      tSnap( 0 ),
      tPull( 0 ),
      tAll( 0 ),
      NumTkr( 0 ),
      NumFld( 0 )
   { ; }

   ////////////////
   // Operations
   ////////////////
public:
   std::string Dump()
   {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, Descr.data() );
      cp += sprintf( cp, "%d,", tLibSnap );
      cp += sprintf( cp, "%d,", tSnap );
      cp += sprintf( cp, "%d,", tPull );
      cp += sprintf( cp, "%d,", tAll );
      cp += sprintf( cp, "%d,", NumTkr );
      cp += sprintf( cp, "%d\n", NumFld );
      return std::string( bp );
   }

}; // class TestStat


////////////////////////////////
// Main Functions
////////////////////////////////
static TestStat *RunIt( LVC &lvc, TestStat *st )
{
   LVCAll       la( lvc, lvc.GetSchema() );
   Messages    &mdb  = la.msgs();
   TestCfg     &cfg  = st->_cfg;
   Message     *ld;
   Field       *fldP, fld;
   mddField    *flds;
   double       d0, d1, d2;
   Services     svcs;
   StringSet    svcSet;
   FieldDict    fdb;
   std::string  s;
   const char **svcFltr;
   const char  *fldFltr;
   size_t       i, j, nf, ns, nl;
   int          fid;

   /*
    * 1) Set Filter
    */
   ns = cfg.bSvcFltr ? cfg.svcs.size() : 0;
   nf = cfg.bFldFltr ? cfg.flds.size() : 0;
   for ( i=0; i<ns; svcSet.insert( cfg.svcs[i] ), i++ );
   for ( i=0; i<ns; svcs.push_back( cfg.svcs[i].data() ), i++ );
   if ( ns )
      svcs.push_back( (const char *)0 );
   svcFltr = ns ? svcs.data()     : (const char **)0;
   fldFltr = nf ? cfg.flds.data() : (const char *)0;
   lvc.SetFilter( fldFltr, svcFltr );
   /*
    * 2) SnapAll()
    */
   d0          = lvc.TimeNs();
   lvc.ViewAll_safe( la );
   d1           = lvc.TimeNs();
   st->tSnap    = (int)( 1000.0 * ( d1 - d0 ) );
   st->tLibSnap = (int)( 1000.0 * la.dSnap() );
   st->NumTkr   = la.Size();
   st->NumFld   = 0;
   nl           = (size_t)la.Size();
   for ( i=0; i<nl; st->NumFld += mdb[i++]->NumFields() );
   /*
    * 3) Walk / Dump
    */
   d1       = lvc.TimeNs();
   for ( i=0; i<nl; i++ ) {
      ld = mdb[i];
      s  = ld->Service();
      if ( svcFltr && ( svcSet.find( s ) == svcSet.end() ) )
         continue;
      /*
       * All, else specifics
       */
      fdb.clear();
      if ( !(nf=cfg.fids.size()) ) {
         flds = (mddField *)ld->Fields();
         nf   = (size_t)ld->NumFields();
         for ( j=0; j<nf; j++ ) {
            fld.Set( flds[j] );
            fid      = fld.Fid();
            fdb[fid] = cfg.GetValue( fld );
         }
      }
      else {
         for ( j=0; j<nf; j++ ) {
            if ( (fldP=ld->GetField( cfg.fids[j] )) ) {
               fid      = fldP->Fid();
               fdb[fid] = cfg.GetValue( *fldP );
            }
         }
      }
   }
   d2        = lvc.TimeNs();
   st->tPull = (int)( 1000.0 * ( d2 - d1 ) );
   st->tAll  = (int)( 1000.0 * ( d2 - d0 ) );
   return st;

} // RunIt()


//////////////////////////
// main()
//////////////////////////
int main( int argc, char **argv )
{
   std::string svr, s, tmp;
   TestCfg     cfg;
   bool        aOK;
   char       *tok;
   size_t      i, j, k;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", LVCPerfID() );
      return 0;
   }
   svr = "./cache.lvc";
   if ( ( argc < 2 ) || !::strcmp( argv[1], "--config" ) ) {
      s  = "Usage: %s \\ \n";
      s += "       [ -db <LVC d/b file> ] \\ \n";
      s += "       [ -s  <CSV Service List> ] \\ \n";
      s += "       [ -f  <CSV Field ID List> ] \\ \n";
      printf( s.data(), argv[0] );
      printf( "   Defaults:\n" );
      printf( "      -db : %s\n", svr.data() );
      printf( "      -s  : <empty>\n" );
      printf( "      -f  : <empty>\n" );
      return 0;
   }

   /////////////////////
   // cmd-line args
   /////////////////////
   for ( i=0; i<(size_t)argc; i++ ) {
      aOK = ( i+1 < (size_t)argc );
      if ( !aOK )
         break; // for-i
      if ( !::strcmp( argv[i], "-db" ) )
         svr = argv[++i];
      else if ( !::strcmp( argv[i], "-s" ) ) {
         tmp = argv[++i];
         tok = ::strtok( (char *)tmp.data(), "," );
         for ( ; tok; tok=::strtok( NULL, "," ) )
            cfg.svcs.push_back( std::string( tok ) );
      }
      else if ( !::strcmp( argv[i], "-f" ) )
         cfg.flds = argv[++i];
   }
   /*
    * 1) Create LVC; Get Field List from Schema
    */
   LVC       lvc( svr.data() );
   Schema   &sch = lvc.GetSchema();
   FieldDef *def;
   TestStats sdb;
   Strings   flds;

   tmp = cfg.flds;
   tok = ::strtok( (char *)tmp.data(), "," );
   for ( ; tok; tok=::strtok( NULL, "," ) ) {
      if ( (def=sch.GetDef( tok )) )
         cfg.fids.push_back( def->Fid() );
   }
   /*
    * 2) Tests : Original, etc.
    */
   printf( "Services,Fields,Filter,FldType," );
   printf( "tSnap-C,tSnap-C#,tPull,tAll,NumTkr,NumFld\n" );
   RunIt( lvc, new TestStat( cfg ) ); // 'Warp up' LVC datafile
   for ( i=0; i<2; i++ ) {
      cfg.bSvcFltr = ( i != 0 );
      if ( cfg.bSvcFltr && !cfg.svcs.size() )
         continue; // for-i
      for ( j=0; j<2; j++ ) {
         cfg.bFldFltr = ( j != 0 );
         if ( cfg.bFldFltr && !cfg.fids.size() )
            continue; // for-i
         for ( k=0; k<2; k++ ) {
            cfg.bFldType = ( k != 0 );
            sdb.push_back( RunIt( lvc, new TestStat( cfg ) ) );
         }
      }
   }
   /*
    * 3) Clean-up
    */
   for ( i=0; i<sdb.size(); printf( sdb[i]->Dump().data() ), i++ );
   for ( i=0; i<sdb.size(); delete sdb[i], i++ );
   sdb.clear();
   printf( "Done!!\n" );
   return 0;
} // main()
