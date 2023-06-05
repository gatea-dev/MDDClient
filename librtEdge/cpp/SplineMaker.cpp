/******************************************************************************
*
*  SplineMaker.cpp
*
*  REVISION HISTORY:
*     17 DEC 2022 jcs  Created (from SplineMaker.cs)
*     22 DEC 2022 jcs  Build 61: SubChannel or LVC
*      9 FEB 2023 jcs  Build 62: Curve-specific service; _fidKnot
*
*  (c) 1994-2023, Gatea Ltd.
******************************************************************************/
#include "inc/SplineMaker.h"
#include <stdarg.h>
#include <set>

static DTD _dtd;

typedef set<string, less<string> >   SortedStringSet;

static DoubleGrid A;

/////////////////////////////////////
// Handy-dandy Logger
/////////////////////////////////////
static void LOG( char *fmt, ... )
{
   va_list ap;
   char    bp[8*K], *cp;

   va_start( ap,fmt );
   cp  = bp;
   cp += vsprintf( cp, fmt, ap );
   cp += sprintf( cp, "\n" );
   va_end( ap );
   ::fwrite( bp, 1, cp-bp, stdout );
   ::fflush( stdout );
}


/////////////////////////////////////
// Version
/////////////////////////////////////
const char *SplineMakerID()
{
   static string s;
   const char   *sccsid;
   char          bp[K], *cp;

   // Once

   if ( !s.length() ) {
      cp  = bp;
      cp += sprintf( cp, "@(#)SplineMaker Build %s ", _MDD_LIB_BLD );
      cp += sprintf( cp, "%s %s Gatea Ltd.", __DATE__, __TIME__ );
      s   = bp;
   }
   sccsid = s.data();
   return sccsid+4;
}


////////////////////////////////////////
//
//         K n o t W a t c h
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
KnotWatch::KnotWatch( Knot &knot, Curve &curve, double X ) :
   _knot( knot ),
   _curve( curve ),
   _X( X )
{ ; }

////////////////////////////////
// Access
////////////////////////////////
double KnotWatch::X() 
{ 
   return _X; 
}

double KnotWatch::Y() 
{ 
   return _knot.Y();
}



////////////////////////////////////////
//
//            K n o t
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
Knot::Knot( Curve &crv, string &tkr, int fid ) :
   _curve( crv ),
   _Ticker( tkr ),
   _fid( fid ),
   _wl(),
   _Y( 0.0 ),
   _StreamID( 0 )
{ ; }


////////////////////////////////
// Access / Mutator
////////////////////////////////
int Knot::Subscribe( KnotSource &src )
{
   const char *svc = _curve.svc();
   const char *tkr = _Ticker.data();

   _StreamID = src.IOpenKnot( svc, tkr, arg_ );
   return StreamID();
}

KnotWatch *Knot::AddWatch( Curve &c, double intvl )
{
   KnotWatch *w;

   w = new KnotWatch( *this, c, intvl );
   _wl.push_back( w );
   return w;
}


////////////////////////////////
// Operations
////////////////////////////////
void Knot::OnData( const char *tm, Field *f )
{
   size_t      i, nw;
   const char *tkr;

   // Pre-condition(s)

   if ( !f )
      return;

   // Set Value / On-pass to Curve(s)

   _Y  = f->GetAsDouble();
   tkr = _Ticker.data();
   nw  = _wl.size();
   LOG( "[%s] %s = %s", tm, tkr, f->GetAsString() );
   for ( i=0; i<nw; _wl[i++]->_curve.OnData( *this ) );
}



////////////////////////////////////////
//
//            C u r v e
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
Curve::Curve( KnotSource &src, XmlElem &xe ) :
   _src( src ),
   _svc( xe.getAttrValue( _dtd._attr_svc, src.svc() ) ),
   _name( xe.getAttrValue( _dtd._attr_name, "" ) ),
   _fid( xe.getAttrValue( _dtd._attr_fid, 0 ) ),
   _Xmax( 1.0 ),
   _X(),
   _Y(),
   _kdb(),
   _splines()
{
   XmlElemVector &edb = xe.elements();
   double         X;
   const char    *tkr;
   size_t         i, nw;
   int            fid;

   /*
    * <Curve Name="Swaps" Service="velocity">
    *    <Knot Ticker="RATES.SWAP.USD.PAR.3M"  FID="6" Interval="3" />
    *    <Knot Ticker="RATES.SWAP.USD.PAR.6M"  FID="6" Interval="6" />
    * </Curve Name="Swaps">
    */
   if ( !strlen( Name() ) )
      return;
   for ( i=0; i<edb.size(); i++ ) {
      if ( !(tkr=edb[i]->getAttrValue( _dtd._attr_tkr, (const char *)0 )) )
         continue; // for-i
      if ( (X=edb[i]->getAttrValue( _dtd._attr_intvl, 0.0 )) == 0.0 )
         continue; // for-i
      if ( !(fid=edb[i]->getAttrValue( _dtd._attr_fid, 0 )) )
         fid = _fid;
      if ( fid )
         _kdb.push_back( _src.AddWatch( *this, tkr, fid, X ) );
   }
   if ( !(nw=_kdb.size()) )
      return;
   for ( i=0; i<nw; _X.push_back( _kdb[i]->X() ), i++ );
   for ( i=0; i<nw; _Y.push_back( _kdb[i]->Y() ), i++ );
   for ( i=0; i<nw; _Xmax = gmax( _Xmax, _kdb[i]->X() ), i++ );
}


////////////////////////////////
// Mutator / Operations
////////////////////////////////
void Curve::AddSpline( Spline *s )
{
   _splines.push_back( s );
}

void Curve::OnData( Knot &k )
{
   size_t i, nw, ns;

   // 1) Reset Y Values

   nw = _kdb.size();
   for ( i=0; i<nw; _Y[i] = _kdb[i]->Y(), i++ );

   // 2) On-pass to Spines

   ns = _splines.size();
   for ( i=0; i<ns; _splines[i++]->Calc( _X, _Y, _Xmax ) );
}


////////////////////////////////////////
//
//            S p l i n e
//
////////////////////////////////////////

int Spline::_fidX    = -8001;
int Spline::_fidY    = -8002;
int Spline::_fidInc  =     6;
int Spline::_fidKnot = -8101;

////////////////////////////////
// Constructor
////////////////////////////////
Spline::Spline( SplinePublisher &pub, 
                const char      *tkr,
                double           dInc, 
                Curve           &curve ) :
   string( tkr ),
   _pub( pub ),
   _dInc( dInc ),
   _curve( curve ),
   _StreamID( (void *)0 ),
   _X(),
   _Z()
{
   _curve.AddSpline( this );
}


////////////////////////////////
// Access / Operations
////////////////////////////////
const char *Spline::tkr()
{
   return data();
}

void Spline::AddWatch( void *StreamID )
{
   _StreamID = StreamID;
}

void Spline::ClearWatch()
{
   _StreamID = (void *)0;
}

void Spline::Calc( DoubleList &X, DoubleList &Y, double xn )
{
   CubicSpline cs( X, Y );
   string      pd;
   double      x, d0, dd;
   int         i, nx;

   /*
    * Use Vector class as:
    *  1) !bPubFld : Container and Publisher
    *  2) bPubFld  : Container only
    */
   d0 = _pub.TimeNs();
   nx = (int)( xn / _dInc );
   _X.clear();
   _Z.clear();
   for ( i=0,x=0.0; i<nx && x<xn; i++, x+=_dInc ) {
      _X.push_back( x );
      _Z.push_back( cs.Spline( x ) );
   }
   dd = 1000.0 * ( _pub.TimeNs() - d0 );
   LOG( "Spline %s Cal'ed in %.2fmS", tkr(), dd );
   Publish();
}

void Spline::Publish()
{
   Update        &u = _pub.upd();
   KnotWatchList &wl = _curve.wl();
   size_t         i, n;

   if ( _StreamID && _pub.XON() ) {
      n = _fidKnot ? wl.size() : 0;
      u.Init( tkr(), _StreamID, true );
      u.AddField( _fidInc, _dInc );
      for ( i=0; i<n; u.AddField( _fidKnot-i, wl[i]->Y() ), i++ );
      if ( _fidX )
         u.AddVector( _fidX, _X, 2 );
      if ( _fidY )
         u.AddVector( _fidY, _Z, _dtd._precision );
      u.Publish();
   }
}



////////////////////////////////////////
//
//       K n o t S o u r c e
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
KnotSource::KnotSource( SplinePublisher &pub, XmlElem &xe ) :
   _pub( pub ),
   _xe( xe ),
   _svc( xe.getAttrValue( _dtd._attr_svc, "velocity" ) ),
   _byId(),
   _byName(),
   _curves()
{ ; }


////////////////////////////////
// Access / /Operations
////////////////////////////////
size_t KnotSource::LoadCurves()
{
   XmlElemVector &edb = _xe.elements();
   string         k;
   Curve         *crv;

   for ( size_t i=0; i<edb.size(); i++ ) {
      crv = new Curve( *this, *edb[i] );
      k   = crv->Name();
      if ( crv->IsValid() )
         _curves[k] = crv;
      else
         delete crv;
   }
   return Size();
}

const char *KnotSource::svc()
{
   return _svc.data();
}

Curve *KnotSource::GetCurve( string &k )
{
   CurveMap          &cdb = _curves;
   CurveMap::iterator it;
   Curve             *rc;

   rc = ( (it=cdb.find( k )) != cdb.end() ) ? (*it).second : (Curve *)0;
   return rc;
}

KnotWatch *KnotSource::AddWatch( Curve      &crv, 
                                 const char *tkr, 
                                 int         fid,
                                 double      X )
{
   KnotByName          &kdb = _byName;
   KnotByName::iterator it;
   Knot                *rc;
   string               k( tkr );

   // Create if necessary

   if ( (it=kdb.find( k )) == kdb.end() ) {
      rc     = new Knot( crv, k, fid );
      kdb[k] = rc;
   }
   else
      rc = (*it).second;
   return rc->AddWatch( crv, X );
}

size_t KnotSource::Size()
{
   return _curves.size();
}

void KnotSource::OpenAll()
{
   KnotByName          &kdb = _byName;
   KnotById            &idb = _byId;
   KnotByName::iterator it;
   string               tkr;
   Knot                *knot;
   int                  sid;

   for ( it=kdb.begin(); it!=kdb.end(); it++ ) {
      tkr      = (*it).first;
      knot     = (*it).second;
      sid      = knot->Subscribe( *this );
      idb[sid] = knot;
   }
}

////////////////////////////////////////
//
//      E d g e 3 S o u r c e
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
Edge3Source::Edge3Source( SplinePublisher &pub, XmlElem &xe ) :
   KnotSource( pub, xe ),
   SubChannel()
{ ; }


////////////////////////////////
// KnotSource InterfaceOperations
////////////////////////////////
const char *Edge3Source::StartMD()
{
   return Start( _xe.getAttrValue( _dtd._attr_svr, "localhost:9998" ),
                 _xe.getAttrValue( _dtd._attr_usr, "SwapSpline" ) );
}

void Edge3Source::StopMD()
{
   SubChannel::Stop();
}

int Edge3Source::IOpenKnot( const char *svc, const char *tkr, void *arg )
{
   return SubChannel::Subscribe( svc, tkr, arg );
}


////////////////////////////////
// rtEdgeSubscriber Interface
////////////////////////////////
void Edge3Source::OnConnect( const char *msg, bool bUp )
{
   const char *ty = bUp ? "UP" : "DOWN";
   string      tm;

   LOG( "[%s] SUB-CONN %s : %s", pDateTimeMs( tm ), msg, ty );
}

void Edge3Source::OnService( const char *svc, bool bUp )
{
   const char *ty = bUp ? "UP" : "DOWN";
   string      tm;

   if ( ::strcmp( "__GLOBAL__", svc ) )
      LOG( "[%s] SUB-SVC %s : %s", pDateTimeMs( tm ), svc, ty );
}

void Edge3Source::OnData( Message &msg )
{
   int                sid = msg.StreamID();
   KnotById          &idb = _byId;
   KnotById::iterator it;
   Knot              *k;
   const char        *tm;
   string             tt;

   if ( (it=idb.find( sid )) != idb.end() ) {
      k  = (*it).second;
      tm = pDateTimeMs( tt );
      k->OnData( tm, msg.GetField( k->fid() ) );
   }
}


////////////////////////////////////////
//
//         L V C S o u r c e
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
LVCSource::LVCSource( SplinePublisher &pub, XmlElem &xe ) :
   KnotSource( pub, xe ),
   LVC( _xe.getAttrValue( _dtd._attr_svr, "./cache.lvc" ) )
{ ; }


////////////////////////////////
// KnotSource Interface
////////////////////////////////
void LVCSource::Pump()
{
   KnotByName          &idb = _byName;
   KnotByName::iterator it;
   Knot                *k;
   Message             *msg;
   const char          *tm, *svc, *tkr;
   string               tt;

   _pub.SetXON( false );
   for ( it=idb.begin(); it!=idb.end(); it++ ) {
      k   = (*it).second;
      svc = _svc.data();
      tkr = k->tkr();
      if ( (msg=Snap( svc, tkr )) ) {
         tm = pDateTimeMs( tt );
         k->OnData( tm, msg->GetField( k->fid() ) );
      }
   }
   _pub.SetXON( true );
}



////////////////////////////////////////
//
//    S p l i n e P u b l i s h e r
//
////////////////////////////////////////

////////////////////////////////
// Constructor
////////////////////////////////
SplinePublisher::SplinePublisher( XmlElem &xp ) :
   PubChannel( xp.getAttrValue( _dtd._attr_svc, "SwapSpline" ) ),
   _xp( xp ),
   _bds( xp.getAttrValue( _dtd._attr_bds, pPubName() ) ),
   _splines(),
   _bdsStreamID( 0 ),
   _XON( true )
{
   SetBinary( true );
}

////////////////////////////////
// Access / Operations
////////////////////////////////
size_t SplinePublisher::Size()
{
   return _splines.size();
}

size_t SplinePublisher::OpenSplines( KnotSource &src )
{
   XmlElemVector &edb = _xp.elements();
   SplineMap     &sdb = _splines;
   const char    *pc, *tkr;
   string         s;
   double         dInc;
   Curve         *crv;

   for ( size_t i=0; i<edb.size(); i++ ) {
      if ( !(tkr=edb[i]->getAttrValue( _dtd._attr_name, (const char *)0 )) )
         continue; // for-k
      if ( !(pc=edb[i]->getAttrValue( _dtd._attr_curve, (const char *)0 )) )
         continue; // for-k
      s = pc;
      if ( !(crv=src.GetCurve( s )) )
         continue; // for-k
      s      = tkr;
      dInc   = edb[i]->getAttrValue( _dtd._attr_inc, 1.0 );
      sdb[s] = new Spline( *this, tkr, dInc, *crv );
      LOG( "Spline %s added", tkr );
   }
   return Size();
}

const char *SplinePublisher::PubStart()
{
   return Start( _xp.getAttrValue( _dtd._attr_svr, "localhost:9995" ) );
}

bool SplinePublisher::XON()
{
   return _XON;
}

void SplinePublisher::SetXON( bool xon )
{
   SplineMap          &sdb = _splines;
   SplineMap::iterator it;

   // xon == true implies publish

   _XON = xon;
   for ( it=sdb.begin(); it!=sdb.end() && _XON; (*it).second->Publish(), it++ );
}


////////////////////////////////
// rtEdgePublisher Interface
////////////////////////////////
void SplinePublisher::OnConnect( const char *msg, bool bUp )
{
   const char *ty = bUp ? "UP" : "DOWN";
   string      tm; 

   LOG( "[%s] PUB-CONN %s : %s", pDateTimeMs( tm ), msg, ty );
}

void SplinePublisher::OnPubOpen( const char *tkr, void *arg )
{
   SplineMap          &sdb = _splines;
   Update             &u   = upd();
   SplineMap::iterator it;
   string              tm, s( tkr );
   Spline             *spl;

   LOG( "[%s] OPEN  %s", pDateTimeMs( tm ), tkr );
   if ( (it=sdb.find( s )) != sdb.end() ) {
      spl = (*it).second;
      spl->AddWatch( arg );
      spl->Publish();
   }
   else {
      u.Init( tkr, arg );
      u.PubError( "non-existent ticker" );
   }
}

void SplinePublisher::OnPubClose( const char *tkr )
{
   SplineMap          &sdb = _splines;
   SplineMap::iterator it; 
   string              tm, s( tkr );
   Spline             *spl;

   LOG( "[%s] CLOSE %s", pDateTimeMs( tm ), tkr );
   if ( (it=sdb.find( s )) != sdb.end() ) {
      spl = (*it).second;
      spl->ClearWatch();
   }
}

void SplinePublisher::OnOpenBDS( const char *bds, void *tag )
{
   Update                   &u   = upd();
   SplineMap                &sdb = _splines;
   SplineMap::iterator       it;
   SortedStringSet           srt;
   SortedStringSet::iterator st;
   string                    tm, k;
   char                      err[K], *tkr;
   vector<char *>            tkrs;

   LOG( "[%s] OPEN.BDS %s", pDateTimeMs( tm ), bds );
   if ( !::strcmp( bds, _bds.data() ) ) {
      _bdsStreamID =  (size_t)tag;
      for ( it=sdb.begin(); it!=sdb.end(); srt.insert( (*it).first ), it++ );
      for ( st=srt.begin(); st!=srt.end(); st++ ) {
         k   = (*st);
         if ( (it=sdb.find( k )) != sdb.end() ) {
            tkr = (char *)(*it).second->tkr();
            tkrs.push_back( tkr );
         }
      }
      tkrs.push_back( (char *)0 );
      PublishBDS( bds, _bdsStreamID, tkrs.data() );
   }
   else {
      sprintf( err, "Unsupported BDS %s; Request %s instead", bds, _bds.data() );
      u.Init( bds, tag );
      u.PubError( err );
   }
}

void SplinePublisher::OnCloseBDS( const char *bds )
{
   string tm;

   LOG( "[%s] CLOSE.BDS %s", pDateTimeMs( tm ), bds );
   _bdsStreamID = 0;
}

Update *SplinePublisher::CreateUpdate()
{
   return new Update( *this );
}



/////////////////////////////////////
//
//        main()
//
/////////////////////////////////////
int main( int argc, char **argv )
{
   XmlParser   x;
   XmlElem    *xs, *xp;

   /////////////////////
   // Quickie checks
   /////////////////////
   if ( argc > 1 && !::strcmp( argv[1], "--version" ) ) {
      LOG( "%s", SplineMakerID() );
      LOG( "%s", SplinePublisher::Version() );
      return 0;
   }
   if ( !x.Load( argv[1] ) ) {
      LOG( "Invalid XML file %s; Exitting ...", argv[1] );
      return 0;
   }
   XmlElem    &root   = *x.root();
   bool        bEnter = root.getAttrValue( _dtd._attr_enter, true );
   double      dPmp   = root.getAttrValue( _dtd._attr_pumpIntvl, 5.0 );
   const char *ty     = root.getAttrValue( _dtd._attr_dataSrc, "Edge3" );
   bool        bLVC   = !::strcmp( ty, "LVC" );

   if ( !(xs=root.find( _dtd._elem_sub )) ) {
      LOG( "%s not found; Exitting ...", _dtd._elem_sub );
      return 0;
   }
   if ( !(xp=root.find( _dtd._elem_pub )) ) {
      LOG( "%s not found; Exitting ...", _dtd._elem_pub );
      return 0;
   }

   SplinePublisher pub( *xp );
   KnotSource     *src;

pub.Log( "./mew.log", 4 );
   Spline::_fidX   = xp->getAttrValue( _dtd._attr_fidX,   Spline::_fidX ); 
   Spline::_fidY   = xp->getAttrValue( _dtd._attr_fidY,   Spline::_fidY ); 
   Spline::_fidInc = xp->getAttrValue( _dtd._attr_fidInc, Spline::_fidInc ); 
   if ( bLVC )
      src = new LVCSource( pub, *xs );
   else
      src = new Edge3Source( pub, *xs );
   if ( !src->LoadCurves() ) {
      LOG( "No Curves in %s; Exitting ...", _dtd._elem_sub );
      delete src;
      return 0;
   }
   if ( !pub.OpenSplines( *src ) ) {
      LOG( "No Splines in %s; Exitting ...", _dtd._elem_pub );
      return 0;
   }

   /////////////////////////////////////
   // Do it
   /////////////////////////////////////
   const char *ps, *pp;
   double      d0, age;

   LOG( "%s", pub.Version() );
   ps = src->StartMD();
   src->OpenAll();
   pp = pub.PubStart();
   LOG( "SUB : %s : %ld Curves", ps, src->Size() );
   LOG( "PUB : %s : %ld Splines", pp, pub.Size() );
   if ( bEnter && !bLVC ) {
      LOG( "Running; <Enter> to terminate ..." );
      getchar();
   }
   else {
      if ( bLVC )
         LOG( "Pumping from %s every %.1fs", ps, dPmp  );
      else
         LOG( "Pumping from %s", ps );
      LOG( "<CTRL>-C to terminate ..." );
      d0 = pub.TimeNs();
      for ( size_t i=0; true; pub.Sleep( 0.25 ), i++ ) {
         age = pub.TimeNs() - d0;
         if ( age > dPmp ) {
            src->Pump();
            d0 = pub.TimeNs();
         }
      }
   }
   LOG( "Shutting down ..." );
   src->StopMD();
   pub.Stop();
   delete src;
   LOG( "Done!!" );
   return 0;

} // PubSpline
