/******************************************************************************
*
*  Correlate.cs
*     Correlate time series from ChartDB
*     ASSUME : All time series of same interval
*
*  REVISION HISTORY:
*      2 AUG 2013 jcs  Created
*
*  (c) 1994-2013 Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;

////////////////
// Result Set
////////////////
public struct Correlate
{
   public string _tkr1;
   public string _tkr2;
   public double _u1;
   public double _u2;
   public double _stDev1;
   public double _stDev2;
   public double _coVar;
   public double _correl;
   public double _tCalc;
};

/////////////////////////
// Time-Series Class
/////////////////////////
class TimeSeries
{
   //////////////
   // Members
   //////////////
   private CDBData _d;
   private float[] _v;
   private double  _avg;
   private double  _stDev;

   ///////////////////////
   // Constructor / Destructor
   ///////////////////////
   public TimeSeries( CDBData d, float[] v )
   {
      _d     = d;
      _v     = v;
      _avg   = 0.0;
      _stDev = 0.0;
   }


   ///////////////////////
   // Properties
   ///////////////////////
   public float[] v
   {
      get{ return _v; }
   }

   public int nv
   {
      get{ return _v.Length; }
   }


   ///////////////////////
   // Access / Operations 
   ///////////////////////
   public double mean()
   {
      int i;

      // Once

      if ( _avg == 0.0 ) {
         for ( i=0,_avg=0.0; i<nv; _avg+=_v[i++] );
         _avg /= (double)Math.Max( nv, 1 );
      }
      return _avg;
   }

   public double stDev()
   {
      double du, dd, var;
      int    i;    

      // Once

      if ( _stDev == 0.0 ) { 
         du = mean();
         for ( i=0,var=0.0; i<nv; i++ ) {
            dd   = ( _v[i] - du );
            var += ( dd*dd );
         }
         var   /= (double)Math.Max( nv, 1 );
         _stDev = Math.Sqrt( var );
      }
      return _stDev;
   }

   public double coVariance( TimeSeries t2 )
   {
      TimeSeries t1 = this;
      float[]    d1 = t1.v;
      float[]    d2 = t2.v;
      double     cv, u1, u2;
      int        i, n;

      // Pre-condition

      if ( (n=t1.nv) != t2.nv )
         return 0.0;
      u1 = t1.mean();
      u2 = t2.mean();
      for ( i=0,cv=0.0; i<n; i++ )
         cv += ( ( d1[i] - u1 ) * ( d2[i] - u2 ) );
      cv /= (double)Math.Max( n-1, 1 );
      return cv;
   }
}


/////////////////////////
// Correlation Class
/////////////////////////
class Correlation
{
   private TimeSeries _t1;
   private TimeSeries _t2;
   private double     _coVar;

   ///////////////////////
   // Constructor / Destructor
   ///////////////////////
   public Correlation( TimeSeries t1, TimeSeries t2 )
   {
      _t1    = t1;
      _t2    = t2;
      _coVar = 0.0;
   }


   ///////////////////////
   // Access / Operations
   ///////////////////////
   public double coVar()
   {
      _coVar = ( _coVar == 0.0 ) ? _t1.coVariance( _t2 ) : _coVar;
      return _coVar;
   }

   public double correl()
   {
      double den;

      den = _t1.stDev() * _t2.stDev();
      return ( den == 0.0 ) ? den :  coVar() / den;
   }
}

///////////////////////
// Worker Bee Class
///////////////////////
class CorrelateThread
{
   //////////////
   // Members
   //////////////
   private CDBQuery         _q;
   private List<TimeSeries> _ts;
   private int              _i0;
   private int              _i1;
   private List<Correlate>  _v;
   private Thread           _thr;

   ///////////////////////
   // Constructor
   ///////////////////////
   public CorrelateThread( CDBQuery         q,
                           List<TimeSeries> ts, 
                           int              i0, 
                           int              nq )
   {
      ThreadStart beg;

      // Members

      _q  = q;
      _ts = ts;
      _i0 = i0;
      _i1 = i0+nq;
      _v  = new List<Correlate> ();

      // Thread

      beg  = new ThreadStart( this.Run );
      _thr = new Thread( beg );
      _thr.Start();
   }


   ///////////////////////
   // Properties
   ///////////////////////
   public List<Correlate> v
   {
      get{ return _v; }
   }


   ///////////////////////
   // Thread Shit
   ///////////////////////
   public void Join()
   {
      _thr.Join();
   }

   private void Run()
   {
      CDBRecRef   r1, r2;
      TimeSeries  t1, t2;
      double      t0;
      int         i, j, nr;
      Correlate   c;
      Correlation cor;

      nr = _ts.Count;
      for ( i=_i0; i<_i1; i++ ) {
         r1 = _q._recs[i];
         t1 = _ts[i];
         for ( j=i+1; j<nr; j++ ) {
            t0  = rtEdge.TimeNs();
            r2  = _q._recs[j];
            t2  = _ts[j];
            cor = new Correlation( t1, t2 );
            c._correl = cor.correl();
            c._tCalc  = ( rtEdge.TimeNs() - t0 ) * 1000000.0;
            c._tkr1   = r1._pTkr;
            c._tkr2   = r2._pTkr;
            c._u1     = t1.mean();
            c._u2     = t2.mean();
            c._stDev1 = t1.stDev();
            c._stDev2 = t2.stDev();
            c._coVar  = cor.coVar();
            _v.Add( c );
         }
      }
      return;
   }
}


///////////////////////
//
//  main()
//
///////////////////////
class CorrelateMain
{
   private static int BID = 22;

   ///////////////////////
   // Class-wide
   ///////////////////////
   static public int Offset( string tm, uint interval )
   {
      string[] kv;
      int      h, m, s, ts, nk;

      // 09:30:00 interval = 60 : Offset = 570

      kv = tm.Split(':');
      nk = kv.Length;
      h  = 0;
      m  = 0;
      s  = 0;
      switch( nk ) {
         case 1:
            Int32.TryParse( kv[0], out h );
            break;
         case 2:
            Int32.TryParse( kv[0], out h );
            Int32.TryParse( kv[1], out m );
            break;
         case 3:
            Int32.TryParse( kv[0], out h );
            Int32.TryParse( kv[1], out m );
            Int32.TryParse( kv[2], out s );
            break;
      }
      ts = ( h*3600 ) + ( m*60 ) + s;
      return ts / Math.Max( 1, (int)interval );
   }

   static public void Dump( Correlate c )
   {
      string msg, comma;

      comma = ",";
      msg   = c._tkr1 + comma + 
              c._u1.ToString("F4") + comma + 
              c._stDev1.ToString("F4") + comma + comma;
      msg  += c._tkr2 + comma +
              c._u2.ToString("F4") + comma +
              c._stDev2.ToString("F4") + comma + comma; 
      msg  += c._coVar.ToString("F4") + comma +
              c._correl.ToString("F4") + comma +
              c._tCalc.ToString("F1");
      Console.WriteLine( msg );
   }

   ////////////////////////////////
   // main()
   ////////////////////////////////
   static public int Main( String[] args )
   {
      ChartDB               cdb;
      CDBData               d1;
      CDBQuery              q;
      CDBRecRef             r1;
      float[]               f1;
      double                t0, tt, tc, td;
      int                   i, j, i0, np, nThr, o0, o1, nq, vs, argc;
      uint                  nr, ri;
      bool                  bDmp;
      string                msg;
      List<TimeSeries>      ts;
      List<Correlate>       vt;
      List<CorrelateThread> tdb;

      // Pre-condition

      argc = args.Length;
      if ( argc < 3 ) {
         msg  = "Usage : <file> <HH:MM:SS> <HH:MM:SS> ";
         msg += "[<nThr=1> [<bDump=true>]]";
         Console.WriteLine( msg );
         return 0;
      }
      nThr = 1;
      if ( argc > 3 ) {
         if ( !Int32.TryParse( args[3], out nThr ) )
            nThr = 1;
      }
      bDmp = ( argc > 4 ) ? !args[4].Equals( "false" ) : false;

      // Rock and Roll

      Console.WriteLine( rtEdge.Version() );
      Console.WriteLine( nThr.ToString("D1") + " threads" );
      cdb = new ChartDB( args[0], null );
      tt  = rtEdge.TimeNs();
      q   = cdb.Query();
      nr  = q._nRec;
      if ( nr == 0 )
         return 0;
      ri = q._recs[0]._interval;
      o0 = Math.Min( Offset( args[1], ri ), Offset( args[2], ri ) );
      o1 = Math.Max( Offset( args[2], ri ), Offset( args[2], ri ) );

      // Std Deviations

      t0 = rtEdge.TimeNs();
      ts = new List<TimeSeries> ();
      for ( i=0; i<nr; i++ ) {
         r1 = q._recs[i];
         d1 = cdb.View( r1._pSvc, r1._pTkr, BID );
         f1 = new float[o1-o0];
         Array.Copy( d1._flds, o0, f1, 0, o1-o0 );
         ts.Add( new TimeSeries( d1, f1 ) );
      }
      td = ( rtEdge.TimeNs() - t0 ) * 1000.0;

      // Correlations

      nq  = (int)nr / nThr;
      tdb = new List<CorrelateThread> ();
      for ( i=0,i0=0; i<nThr; i++,i0+=nq )
         tdb.Add( new CorrelateThread( q, ts, i0, nq ) );
      for ( i=0,np=0; i<nThr; i++ ) {
         tdb[i].Join();
         vs  = tdb[i].v.Count;
         np += vs;
Console.WriteLine( vs.ToString("D1") );
      }

      // Dump

      tc   = rtEdge.TimeNs() - tt;
      msg  = nr.ToString("D1") + " recs; ";
      msg += np.ToString("D1") + " pts in " + tc.ToString("F3") + "s; ";
      msg += "StDev in " + td.ToString("F1") + "mS";
      Console.WriteLine( msg );
      if ( bDmp ) {
         msg = "Tkr1,u1,stDev1,,Tkr2,u2,stDev2,,CoVar,Correl,,tCalc (uS),";
         Console.WriteLine( msg );
         for ( i=0; i<nThr; i++ ) {
            vt = tdb[i].v;
            for ( j=0; j<vt.Count; Dump( vt[j++] ) );
         }
      }

      // Done

      cdb.FreeQry();
      cdb.Destroy();
      return 0;
   }
}
