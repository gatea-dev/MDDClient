/******************************************************************************
*
*  MDD2DataDog.cpp
*     Consume MDD Data : Pump to DataDog
*
*  REVISION HISTORY:
*      3 MAY 2022 jcs  Created
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#define xml
#include <librtEdge.h>

using namespace RTEDGE;
using namespace std;

static XmlElem _zXml( (XmlElem *)0, "empty" );

// DTD

static const char *_dtdLogUpd = "LogUpds";
static const char *_dtdUser   = "User";
static const char *_dtdHost   = "Host";
static const char *_dtdPort   = "Port";
static const char *_dtdSvc    = "Service";
// static const char *_dtdTkr    = "Ticker";
static const char *_dtdField  = "Field";
static const char *_dtdName   = "Name";
static const char *_dtdMetric = "Metric";
/*
 * Dawg
 */ 
static const char *_dtdDawg   = "DataDog";
static const char *_dtdPfx    = "Prefix";
static const char *_dtdIntvl  = "Interval";
static const char *_dtdTags   = "DogTags";
static const char *_SEP       = "|";

// Forwards

class DawgMetric;
class DawgRecord;

// Collections

typedef hash_map<int, DawgMetric *>    MetricsByFID;
typedef hash_map<int, DawgRecord *>    RecordsByID;
typedef hash_map<string, DawgRecord *> RecordsByName;
typedef hash_map<string, string>       StringMap;

/////////////////////////////////////
// Version
/////////////////////////////////////
const char *MDD2DataDogID()
{
   static string s;
   const char   *sccsid;

   // Once

   if ( !s.length() ) {
      char bp[K], *cp;

      cp  = bp;
      cp += sprintf( cp, "@(#)MDD2DataDog Build 53 " );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


/////////////////////////////////////
//
//  c l a s s   D a w g R e c o r d
//
/////////////////////////////////////
class DawgRecord
{
private:
   MetricsByFID _metrics;
   string       _key; // Svc|Tkr
   string       _svc;
   string       _tkr;
   int          _StreamID;

   ///////////////////////////////////
   // Constructor
   ///////////////////////////////////
public:
   DawgRecord( const char *svc, const char *tkr, int StreamID ) :
      _metrics(),
      _key( svc ),
      _svc( svc ),
      _tkr( tkr ),
      _StreamID( StreamID )
   {
      _key += _SEP;
      _key += _tkr;
   }

   ///////////////////////////////////
   // Access
   ///////////////////////////////////
   MetricsByFID &metrics()  { return _metrics; }
   const char   *svc()      { return _svc.data(); }
   const char   *tkr()      { return _tkr.data(); }
   int           StreamID() { return _StreamID; }

   ///////////////////////////////////
   // Mutator
   ///////////////////////////////////
   void AddMetric( DawgMetric *metric, int fid )
   {
      _metrics[fid] = metric;
   }

}; // class DawgRecord


/////////////////////////////////////
//
//  c l a s s   D a w g M e t r i c
//
/////////////////////////////////////
class DawgMetric
{
private:
   DawgRecord &_rec;
   int         _fid;
   string      _DawgName;

   ///////////////////
   // Constructor
   ///////////////////
public:
   DawgMetric( DawgRecord &rec, XmlElem &xe, int fid ) :
      _rec( rec ),
      _fid( fid ),
      _DawgName()
   {
      char dflt[K];

      sprintf( dflt, "%s.%d", tkr(), _fid );
      _DawgName = xe.getAttrValue( _dtdMetric, dflt );
   }

   ///////////////////
   // Access
   ///////////////////
   const char *svc()  { return _rec.svc(); }
   const char *tkr()  { return _rec.tkr(); }
   int         fid()  { return _fid; }
   const char *dawg() { return _DawgName.data(); }

}; // class DawgMetric



/////////////////////////////////////
//
//  c l a s s   M y C h a n n e l
//
/////////////////////////////////////
class MyChannel : public SubChannel
{
public:
   DataDog      &_dawg;
   double        _dIntvl;
   time_t        _t0;
   bool          _bLog;
   RecordsByName _recByName;
   RecordsByID   _recByID;
   StringMap     _lvc;

   ///////////////////////////////////
   // Constructor / Destructor
   ///////////////////////////////////
public:
   MyChannel( DataDog &dawg, double dIntvl, bool bLog ) :
      _dawg( dawg ),
      _dIntvl( dIntvl ),
      _t0( TimeSec() ),
      _bLog( bLog ),
      _recByName(),
      _recByID(),
      _lvc()
   {
      SetIdleCallback( true );
   }

   ///////////////////////////////////
   // Operations
   ///////////////////////////////////
public:
   size_t WoofWoof()
   {
      RecordsByName          &ndb = _recByName;
      RecordsByName::iterator it;
      DawgRecord             *rec;

      for ( it=ndb.begin(); it!=ndb.end(); it++ ) {
         rec = (*it).second;
         Subscribe( rec->svc(), rec->tkr(), (void *)(VOID_PTR)rec->StreamID() );
      }
      return ndb.size();
   }

   void AddMetric( const char *svc, int fid, XmlElem &xe )
   {
      RecordsByName          &ndb = _recByName;
      RecordsByID            &rdb = _recByID;
      RecordsByName::iterator it;
      DawgRecord             *rec;
      string                  k( svc );
      const char             *tkr;
      size_t                  rid;
      int                     fidX;

      // Pre-condition

      if ( !(tkr=xe.getAttrValue( _dtdName, (const char *)0 )) )
         return;

      // Allow Field Override; Then Add / Store

      fid = (fidX=xe.getAttrValue( _dtdField, 0 )) ? fidX : fid;
      k  += _SEP;
      k  += tkr;
      rec = ( (it=ndb.find( k )) != ndb.end() ) ? (*it).second : (DawgRecord *)0;
      if ( !rec ) {
         rid      = ndb.size() + 1;
         rec      = new DawgRecord( svc, tkr, rid );
         ndb[k]   = rec;
         rdb[rid] = rec;
      }
      rec->AddMetric( new DawgMetric( *rec, xe, fid ), fid );
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
      RecordsByID          &rdb = _recByID;
      int                   rid = (size_t)msg.arg();
      RecordsByID::iterator it;
      DawgRecord           *rec;

      if ( (it=rdb.find( rid )) != rdb.end() ) {
         rec = (*it).second;
         _OnData( *rec, msg );
      }
   }

   virtual void OnIdle()
   {
      StringMap::iterator it;
      string              k, v, tm;
      time_t              now, age;

      // Every dIntvl seconds

      now = TimeSec();
      age = now - _t0;
      if ( age < _dIntvl )
         return;
      _t0 = now;

      // Rock on

      if ( _bLog )
         printf( "[%s] : %ld metrics\n", pDateTimeMs( tm ), _lvc.size() );
      for ( it=_lvc.begin(); it!=_lvc.end(); it++ ) {
         k = (*it).first;
         v = (*it).second;
         _dawg.Gauge( k, v );
         if ( _bLog )
            printf( "   %-20s = %s\n", k.data(), v.data() );
      }
      _flush();
   }

   //////////////////////////
   // Private helpers
   //////////////////////////
private:
   void _OnData( DawgRecord &rec, Message &msg )
   {
      MetricsByFID          &mdb = rec.metrics();
      MetricsByFID::iterator it;
      DawgMetric            *mon;
      Field                 *fld;
      string                 k, v;

      for ( it=mdb.begin(); it!= mdb.end(); it++ ) {
         mon = (*it).second;
         if ( !(fld=msg.GetField( mon->fid() )) )
            continue; // for-it
         k       = mon->dawg();
         v       = fld->GetAsString();
         _lvc[k] = v;
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
   XmlParser   x;
   XmlElem    *xe, *xd;
   Strings     sdb;
   const char *pc, *svc;
   char       *kv, *k, *v;
   int         fid;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      printf( "%s\n", MDD2DataDogID() );
      printf( "%s\n", MyChannel::Version() );
      return 0;
   }
   if ( !x.Load( argv[1] ) ) {
      printf( "Invalid XML file %s; Exitting ...\n", argv[1] );
      return 0;
   } 
   XmlElem &root = *x.root();

   // Parse Config

   xd = (xd=root.find( _dtdDawg )) ? xd : &_zXml;
   XmlElemVector &edb  = root.elements();
   const char   *svr  = root.getAttrValue( _dtdHost, "localhost:9998" ); 
   const char   *usr  = root.getAttrValue( _dtdUser, MDD2DataDogID() );
   string        tags( xd->getAttrValue( _dtdTags, "" ) );
   DataDog       dawg( xd->getAttrValue( _dtdHost, "localhost" ),
                       xd->getAttrValue( _dtdPort, 8125 ),
                       xd->getAttrValue( _dtdPfx, "" ) );
   MyChannel     ch( dawg, 
                     root.getAttrValue( _dtdIntvl, 5.0 ),
                     root.getAttrValue( _dtdLogUpd, false ) );

   kv = ::strtok( (char *)tags.data(), "," );
   for ( ; kv; kv=::strtok( NULL, "," ) )
      sdb.push_back( string( kv ) );
   for ( size_t i=0; i<sdb.size(); i++ ) {
      k = ::strtok( (char *)sdb.data(), "-" );
      v = ::strtok( NULL, "-" );
      dawg.AddTag( k, v );
   }
   ch.SetBinary( true );
   pc = ch.Start( svr, usr );
   printf( "%s\n", pc ? pc : "" );
   for ( size_t i=0; i<edb.size(); i++ ) {
      XmlElemVector &tdb = edb[i]->elements();

      if ( ::strcmp( _dtdSvc, edb[i]->name() ) )
         continue; // for-i
      if ( !(svc=edb[i]->getAttrValue( _dtdName, (const char *)0 )) )
         continue; // for-i
      if ( !(fid=edb[i]->getAttrValue( _dtdField, 0 )) )
         continue; // for-i
      for ( size_t j=0; j<tdb.size(); j++ ) {
         xe = tdb[j];
         ch.AddMetric( svc, fid, *xe );
      }
   }
   printf( "%ld Tickers Subscribed\n", ch.WoofWoof() );
   printf( "Hit <ENTER> to QUIT ...\n"  ); getchar();

   // Clean up

   printf( "Cleaning up ...\n" );
   ch.Stop();
   printf( "Done!!\n " );
   return 1;
}
