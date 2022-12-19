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
class Knot;
class KnotWatch;
class Spline;
class SplinePublisher;
class SplineSubscriber;

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
	int         Subscribe( SplineSubscriber &sub, string &svc );
	KnotWatch  *AddWatch( Curve &c, double intvl );

	// Operations
public:
	void OnData( SplineSubscriber &sub, Field * );

}; // class Knot


////////////////////////////////////////
//
//            C u r v e
//
////////////////////////////////////////
class Curve
{
private:
	SplineSubscriber &_sub;
	string            _name;
	double            _Xmax;
	DoubleList        _X;
	DoubleList        _Y;
	KnotWatchList     _kdb;
	SplineList        _splines;

	// Constructor
public:
	Curve( SplineSubscriber &sub, XmlElem &xe );

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
//    S p l i n e S u b s c r i b e r
//
////////////////////////////////////////
class SplineSubscriber : public RTEDGE::SubChannel  
{
private:
	XmlElem   &_xe;
	string     _svc;
	KnotById   _byId;
	KnotByName _byName;
	CurveMap   _curves;

	// Constructor
public:
	SplineSubscriber( XmlElem &xe );

	// Access / /Operations
public:
	const char *StartMD();
	size_t      LoadCurves();
	Curve      *GetCurve( string & );
	KnotWatch  *AddWatch( Curve &, const char *, int, double );
	size_t      Size();
	void        OpenAll();

	// RTEDGE::SubChannel Interface
protected:
	virtual void OnConnect( const char *, bool );
	virtual void OnService( const char *, bool );
	virtual void OnData( Message & );

}; // class SplineSubscriber


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
	size_t      OpenSplines( SplineSubscriber & );
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
