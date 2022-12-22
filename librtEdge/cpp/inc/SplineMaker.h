/******************************************************************************
*
*  SplineMaker.h
*
*  REVISION HISTORY:
*     17 DEC 2022 jcs  Created (from SplineMaker.cs)
*
*  (c) 1994-2022, Gatea Ltd.
******************************************************************************/
#ifndef __SPLINE_MAKER_H
#define __SPLINE_MAKER_H
#include <librtEdge.h>

using namespace std;
using namespace RTEDGE;
using namespace QUANT;

static void *arg_ = (void *)"User-supplied argument";

// Forwards / Collections

class Curve;
class Edge3Source;
class Knot;
class KnotSource;
class KnotWatch;
class LVCSource;
class Spline;
class SplinePublisher;

typedef vector<KnotWatch *>        KnotWatchList;
typedef hash_map<int, Knot *>      KnotById;
typedef hash_map<string, Knot *>   KnotByName;
typedef hash_map<string, Curve *>  CurveMap;
typedef hash_map<string, Spline *> SplineMap;
typedef vector<Spline *>           SplineList;


////////////////////////////////////////
//
//            D T D
//
////////////////////////////////////////
class DTD
{
public:
	double _min_dInc;
	int    _fid_X;
	int    _fid_Y;
	int    _fidVal;
	int    _precision;

	////////////////////////
	// Elements
	////////////////////////
public:
	const char *_elem_sub;
	const char *_elem_pub;
	const char *_elem_knot;
	const char *_elem_spline;

	////////////////////////
	// Attributes
	////////////////////////
	const char *_attr_dataSrc;
	const char *_attr_enter;
	const char *_attr_svc;
	const char *_attr_usr;
	const char *_attr_svr;
	const char *_attr_bds;
	const char *_attr_tkr;
	const char *_attr_fid;
	const char *_attr_intvl;
	const char *_attr_name;
	const char *_attr_inc;
	const char *_attr_curve;

	// Constructor
public:
	DTD() :
	   _min_dInc( 0.001 ),
	   _fid_X( -8001 ),
	   _fid_Y( -8002 ),
	   _fidVal( 6 ),
	   _precision( 4 ),
	   _elem_sub( "Subscriber" ),
	   _elem_pub( "Publisher" ),
	   _elem_knot( "Knot" ),
	   _elem_spline( "Spline" ),
	   _attr_dataSrc( "DataSource" ),
	   _attr_enter( "RunInForeground" ),
	   _attr_svc( "Service" ),
	   _attr_usr( "Username" ),
	   _attr_svr( "Server" ),
	   _attr_bds( "BDS" ),
	   _attr_tkr( "Ticker" ),
	   _attr_fid( "FieldID" ),
	   _attr_intvl( "Interval" ),
	   _attr_name( "Name" ),
	   _attr_inc( "Increment" ),
	   _attr_curve( "Curve" )
	{ ; }

}; // DTD


////////////////////////////////////////
//
//         K n o t W a t c h
//
////////////////////////////////////////
class KnotWatch
{
public:
	Knot  &_knot;
	Curve &_curve;
	double _X;

	// Constructor
public:
	KnotWatch( Knot &, Curve &, double );

	// Access
public:
	double X();
	double Y();

}; // class KnotWatch


////////////////////////////////////////
//
//            K n o t
//
////////////////////////////////////////
class Knot
{
private:
	string        _Ticker;
	int           _fid;
	KnotWatchList _wl;
	double        _Y;
	int           _StreamID;

	// Constructor
public:
	Knot( string &, int );

	// Access / Mutator
public:
	const char *tkr()      { return _Ticker.data(); }
	int         fid()      { return _fid; }
	double      Y()        { return _Y; }
	int         StreamID() { return _StreamID; }
	int         Subscribe( KnotSource &, string & );
	KnotWatch  *AddWatch( Curve &c, double intvl );

	// Operations
public:
	void OnData( const char *, Field * );

}; // class Knot


////////////////////////////////////////
//
//            C u r v e
//
////////////////////////////////////////
class Curve
{
private:
	KnotSource   &_src;
	string        _name;
	double        _Xmax;
	DoubleList    _X;
	DoubleList    _Y;
	KnotWatchList _kdb;
	SplineList    _splines;

	// Constructor
public:
	Curve( KnotSource &, XmlElem & );

	// Access
public:
	bool        IsValid() { return _Y.size() > 0; }
	const char *Name()    { return _name.data(); }
	DoubleList &X()       { return _X; }
	DoubleList &Y()       { return _Y; }

	// Mutator / Operations
public:
	void AddSpline( Spline * );
	void OnData( Knot & );

}; // class Curve


////////////////////////////////////////
//
//            S p l i n e
//
////////////////////////////////////////
class Spline : public string
{
private:
	SplinePublisher &_pub;
	double           _dInc;
	Curve           &_curve;
	void            *_StreamID;
	DoubleList       _X;
	DoubleList       _Z;

	// Constructor
public:
	Spline( SplinePublisher &, const char *, double, Curve & );

	// Access / Operations
public:
	const char *tkr();
	void        AddWatch( void * );
	void        ClearWatch();
	void        Calc( DoubleList &X, DoubleList &Y, double );
	void        Publish();

}; // class Spline



////////////////////////////////////////
//
//        K n o t S o u r c e
//
////////////////////////////////////////
class KnotSource
{
protected:
	XmlElem   &_xe;
	string     _svc;
	KnotById   _byId;
	KnotByName _byName;
	CurveMap   _curves;

	// Constructor
public:
	KnotSource( XmlElem &xe );
	virtual ~KnotSource() { ; }

	// Access / /Operations
public:
	size_t      LoadCurves();
	Curve      *GetCurve( string & );
	KnotWatch  *AddWatch( Curve &, const char *, int, double );
	size_t      Size();
	void        OpenAll();

	// KnotSource Interface
public:
	virtual const char *StartMD() = 0;
	virtual void        StopMD() = 0;
	virtual int         IOpenKnot( const char *, const char *, void * ) = 0;
	virtual void        Pump() = 0;

}; // class KnotSource


////////////////////////////////////////
//
//       E d g e 3 S o u r c e
//
////////////////////////////////////////
class Edge3Source : public KnotSource,
                    public RTEDGE::SubChannel  
{
public:
	Edge3Source( XmlElem &xe );

	// KnotSource Interface
public:
	virtual const char *StartMD();
	virtual void        StopMD();
	virtual int         IOpenKnot( const char *, const char *, void * );
	virtual void        Pump() { ; }

	// RTEDGE::SubChannel Interface
protected:
	virtual void OnConnect( const char *, bool );
	virtual void OnService( const char *, bool );
	virtual void OnData( Message & );

}; // class Edge3Source


////////////////////////////////////////
//
//       L V C S o u r c e
//
////////////////////////////////////////
class LVCSource : public KnotSource,
                  public RTEDGE::LVC  
{
public:
	LVCSource( XmlElem &xe );

	// KnotSource Interface
public:
	virtual const char *StartMD() { return pFilename(); }
	virtual void        StopMD() { ; }
	virtual int         IOpenKnot( const char *, const char *, void * ) { return 0; }
	virtual void        Pump();

}; // class LVCSource



////////////////////////////////////////
//
//    S p l i n e P u b l i s h e r
//
////////////////////////////////////////
class SplinePublisher : public RTEDGE::PubChannel  
{
private:
	XmlElem  &_xp;
	string    _bds;
	SplineMap _splines;
	int       _bdsStreamID;

	// Constructor
public:
	SplinePublisher( XmlElem & );

	// Access / Operations
public:
	size_t      Size();
	size_t      OpenSplines( KnotSource & );
	const char *PubStart();

	// RTEDGE::PubChannel Interface
protected:
	virtual void    OnConnect( const char *, bool );
	virtual void    OnPubOpen( const char *, void * );
	virtual void    OnPubClose( const char * );
	virtual void    OnOpenBDS( const char *, void * );
	virtual void    OnCloseBDS( const char * );
	virtual Update *CreateUpdate();

}; // SplinePublisher

#endif // __SPLINE_MAKER_H
