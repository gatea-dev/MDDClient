/******************************************************************************
*
*  Correlate.cpp
*     Correlate time series from ChartDB
*     ASSUME : All time series of same interval
*
*  REVISION HISTORY:
*      2 AUG 2013 jcs  Created
*
*  (c) 1994-2013 Gatea Ltd.
*******************************************************************************/
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
extern "C" {
#include <librtEdge.h>
}

using namespace std;

#define BID  22  // BID field (FID 22) for now

////////////////
// Result Set
////////////////
typedef struct {
   const char *_tkr1;
   const char *_tkr2;
   double      _u1;
   double      _u2;
   double      _stDev1;
   double      _stDev2;
   double      _coVar;
   double      _correl;
   double      _tCalc;
} Correlate;

/////////////////////////
// Time-Series Class
/////////////////////////
class GLtsSeries
{
private:
   CDBData *_d;
   float   *_v;
   int      _nv;
   double   _avg;
   double   _stDev;

   ///////////////////////
   // Constructor / Destructor
   ///////////////////////
public:
   GLtsSeries( CDBData &d, float *v, int nv ) :
      _d( &d ),
      _v( v ),
      _nv( nv ),
      _avg( 0.0 ),
      _stDev( 0.0 )
   { ; }

   ~GLtsSeries()
   {
      ::CDB_Free( _d );
   }


   ///////////////////////
   // Access / Operations
   ///////////////////////
   float *v()
   {
      return _v;
   }

   int nv()
   {
      return _nv;
   }

   double mean()
   {
      int i;

      // Once

      if ( _avg == 0.0 ) {
         for ( i=0,_avg=0.0; i<nv(); _avg+=_v[i++] );
         _avg /= (double)gmax( nv(), 1 );
      }
      return _avg;
   }

   double stDev()
   {
      double du, dd, var;
      int    i;    

      // Once

      if ( _stDev == 0.0 ) { 
         du = mean();
         for ( i=0,var=0.0; i<nv(); i++ ) {
            dd   = ( _v[i] - du );
            var += ( dd*dd );
         }
         var   /= (double)gmax( nv(), 1 );
         _stDev = ::sqrt( var );
      }
      return _stDev;
   }

   double coVariance( GLtsSeries &t2 )
   {
      GLtsSeries &t1 = *this;
      float      *d1 = t1.v();
      float      *d2 = t2.v();
      double      cv, u1, u2;
      int         i, n;

      // Pre-condition

      if ( (n=t1.nv()) != t2.nv() )
         return 0.0;
      u1 = t1.mean();
      u2 = t2.mean();
      for ( i=0,cv=0.0; i<n; i++ )
         cv += ( ( d1[i] - u1 ) * ( d2[i] - u2 ) );
      cv /= (double)gmax( n-1, 1 );
      return cv;
   }
};


/////////////////////////
// Correlation Class
/////////////////////////
class GLtsCorrelation
{
private:
   GLtsSeries &_t1;
   GLtsSeries &_t2;
   double      _coVar;

   ///////////////////////
   // Constructor / Destructor
   ///////////////////////
public:
   GLtsCorrelation( GLtsSeries &t1, GLtsSeries &t2 ) :   
      _t1( t1 ),
      _t2( t2 ),
      _coVar( 0.0 )
   { ; }


   ///////////////////////
   // Access / Operations
   ///////////////////////
   double coVar()
   {
      _coVar = ( _coVar == 0.0 ) ? _t1.coVariance( _t2 ) : _coVar;
      return _coVar;
   }

   double operator()()
   {
      double den;

      den = _t1.stDev() * _t2.stDev();
      return ( den == 0.0 ) ? den :  coVar() / den;
   }
};


/////////////////////////////////////////
//
//     c l a s s      c T h r e a d
//
/////////////////////////////////////////
#ifdef WIN32
#ifdef u_int
#undef u_int
#endif // u_int
#include <process.h>
#include <windows.h>
#define THR_ARG        void
#define THR_TID        HANDLE
#else
#define THR_ARG        void *
#define THR_TID        pthread_t
#include <pthread.h>
#endif // WIN32

class cThread
{
protected:
   THR_TID _tid;

   ///////////////////////
   // Constructor / Destructor
   ///////////////////////
public:
   cThread() :
      _tid( (THR_TID)0 )
   {
      Start();
   }

    
   ///////////////////////
   // Thread Shit ...
   ///////////////////////
   void Start()
   {
      if ( !_tid )
#ifdef WIN32
         _tid = (THR_TID)::_beginthread( _thrProc, 0, (void *)this );
#else
         ::pthread_create( &_tid, NULL, _thrProc, this );
#endif // WIN32
   }

   void Join()
   {
      // Safe to stop ...

      if ( _tid )
#ifdef WIN32
         ::WaitForSingleObject( _tid, INFINITE );
#else
         ::pthread_join( _tid, 0 );
#endif // WIN32
      _tid = 0;
   }

   virtual void *Run()
   {
      // Derived Classes Implement

      return (void *)0;
   }


   ////////////////////////////////////////////
   // Class wide
   ////////////////////////////////////////////
   static THR_ARG _thrProc( void *arg )
   {
      cThread *us;

      us = (cThread *)arg;
      us->Run();
#ifndef WIN32
      return (void *)0;
#endif // WIN32
   }
};


///////////////////////
//  Utility Fcns
///////////////////////
#ifdef WIN32
#define STRTOK(a,b,c)  ::strtok( (a),(b) )
#else
#define STRTOK(a,b,c)  ::strtok_r( (a),(b),(c) )
#endif // WIN32

int Offset( char *tm, int interval )
{
   char *ph, *pm, *ps, *rp;
   char  buf[K];
   int   h, m, s, ts;

   // 09:30:00 interval = 60 : Offset = 570

   strcpy( buf, tm );
   ph = STRTOK( buf, ":",&rp );
   pm = STRTOK( NULL,":",&rp );
   ps = STRTOK( NULL,":",&rp );
   h  = ph ? atoi( ph ) : 0;
   m  = pm ? atoi( pm ) : 0;
   s  = ps ? atoi( ps ) : 0;
   ts = ( h*3600 ) + ( m*60 ) + s;
   return ts / gmax( 1, interval );
}

void Dump( Correlate &c )
{
   printf( "%s,%.4f,%.4f,,%s,%.4f,%.4f,,%.4f,%.4f,,%.1f\n",
         c._tkr1, c._u1, c._stDev1,
         c._tkr2, c._u2, c._stDev2,
         c._coVar, c._correl, c._tCalc );
}


///////////////////////
//
//  Correlation Thread
//
///////////////////////
class CorrelationThread : public cThread
{
private:
   CDBQuery             &_q;
   vector<GLtsSeries *> &_ts;
   int                   _i0;
   int                   _i1;
public:
   vector<Correlate>     _v;

   ///////////////////////
   // Constructor
   ///////////////////////
public:
   CorrelationThread( CDBQuery             &q,
                      vector<GLtsSeries *> &ts, 
                      int                   i0, 
                      int                   nq ) :
      _q( q ),
      _ts( ts ),
      _i0( i0 ),
      _i1( i0+nq )
   {
      Start();
   }


   ///////////////////////
   // Thread Shit
   ///////////////////////
   virtual void *Run()
   {
      CDBRecDef   r1, r2;
      GLtsSeries *t1, *t2;
      double      t0;
      int         i, j, nr;
      Correlate   c;

      nr = _ts.size();
      for ( i=_i0; i<_i1; i++ ) {
         r1 = _q._recs[i];
         t1 = _ts[i];
         for ( j=i+1; j<nr; j++ ) {
            t0 = ::rtEdge_TimeNs();
            r2 = _q._recs[j];
            t2 = _ts[j];

            GLtsCorrelation cor( *t1, *t2 );

            c._correl = (cor)();
            c._tCalc  = ( ::rtEdge_TimeNs() - t0 ) * 1000000.0;
            c._tkr1   = r1._pTkr;
            c._tkr2   = r2._pTkr;
            c._u1     = t1->mean();
            c._u2     = t2->mean();
            c._stDev1 = t1->stDev();
            c._stDev2 = t2->stDev();
            c._coVar  = cor.coVar();
            _v.push_back( c );
         }
      }
      return (void *)0;
   }
};


///////////////////////
//
//  main()
//
///////////////////////
int main( int argc, char **argv )
{
   CDB_Context          cxt;
   CDBData              d1, d2;
   CDBQuery             q;
   CDBRecDef            r1, r2;
   GLtsSeries          *t1, *t2;
   float               *f1, *f2;
   double               t0, tt, tc, td;
   int                  i, j, i0, nr, np, nThr, o0, o1, ri, nq, vs;
   bool                 bOK, bDmp;
   Correlate            c;
   vector<GLtsSeries *> ts;
   vector<Correlate>    v;

   // Pre-condition

   if ( argc < 4 ) {
      printf( "Usage : %s <file> <HH:MM:SS> <HH:MM:SS> ", argv[0] );
      printf( "[<nThr=1> [<bDump=true>]]\n" );
      return 0;
   }
   nThr = ( argc > 4 ) ? atoi( argv[4] ) : 1;
   bDmp = ( argc > 5 ) ? ::strcmp( argv[5], "false" ) : true;

   // Rock and Roll

   printf( "%s\n", ::rtEdge_Version() );
   printf( "%d threads\n", nThr );
   if ( !(cxt=::CDB_Initialize( argv[1], NULL )) ) {
      printf( "ERROR opening ChartDB file %s\n", argv[1] );
      return 0;
   }
   tt = ::rtEdge_TimeNs();
   q  = ::CDB_Query( cxt );
   nr = q._nRec;
   if ( !nr )
      return 0;
   ri = q._recs[0]._interval;
   o0 = gmin( Offset( argv[2], ri ), Offset( argv[3], ri ) );
   o1 = gmax( Offset( argv[2], ri ), Offset( argv[3], ri ) );

   // Std Deviations

   t0 = ::rtEdge_TimeNs();
   for ( i=0; i<nr; i++ ) {
      r1  = q._recs[i];
      d1  = ::CDB_View( cxt, r1._pSvc, r1._pTkr, BID );
      f1  = d1._flds;
      f1 += o0;
      ts.push_back( new GLtsSeries( d1, f1, o1-o0 ) );
   }
   td = ( ::rtEdge_TimeNs() - t0 ) * 1000.0;

   // Correlations

   vector<CorrelationThread *> thrs;

   nq = nr / nThr;
   for ( i=0,i0=0; i<nThr; i++,i0+=nq )
      thrs.push_back( new CorrelationThread( q, ts, i0, nq ) );
   for ( i=0,np=0; i<nThr; i++ ) {
      thrs[i]->Join();
      vs  = thrs[i]->_v.size();
printf( "===>>> %d\n",  vs );
      np += vs;
   }

   // Dump

   tc = ::rtEdge_TimeNs() - tt;
   printf( "\n%d recs; %d pts in %.3fs; StDev in %.1fmS\n", nr, np, tc, td );
   if ( bDmp ) {
      printf( "Tkr1,u1,stDev1,,Tkr2,u2,stDev2,,CoVar,Correl,,tCalc (uS),\n" );
      for ( i=0; i<nThr; i++ ) {
         vector<Correlate> &vt = thrs[i]->_v;

         for ( j=0; j<vt.size(); Dump( vt[j++] ) );
      }
      fflush( stdout );
   }

   // Done

   for ( i=0; i<ts.size(); delete ts[i++] );
   ::CDB_FreeQry( &q );
   ::CDB_Destroy( cxt );
   return 0;
}
