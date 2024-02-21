/******************************************************************************
*
*  SuckCPU.cs
*     Suck CPU for a while
*
*  REVISION HISTORY:
*     17 FEB 2024 jcs  Created.
*     20 FEB 2024 jcs  Multiple threads
*
*  (c) 1994-2024, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;


/////////////////////////////////////
//
//   c l a s s   M y T h r e a d
//
/////////////////////////////////////
class MyThread
{
   private long   _num;
   private bool   _bRun;
   private Thread _thr;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public MyThread()
   {
      ThreadStart beg;

      beg   = new ThreadStart( this.Run );
      _num  = 0;
      _bRun = true;
      _thr  = new Thread( beg );
   }

   ///////////////////////
   // Access
   ///////////////////////
   public long num()
   {
      return _num;
   }

   public double cpuMillis()
   {
      Process  us  = Process.GetCurrentProcess();
      TimeSpan cpu = us.TotalProcessorTime;

      return cpu.TotalMilliseconds;
   }


   ///////////////////////
   // Thread Stuff
   ///////////////////////
   public void Start()
   {
      _thr.Start();
   }
   public void Stop()
   {
      _bRun = false;
      if ( _thr != null )
         _thr.Join();
      _thr = null;
   }

   private void Run()
   {
      double dv;

      for ( _num=0,dv=0.0; _bRun; _num++ ) {
         if ( _num == 0 )
            Console.WriteLine( "running ..." );
         dv += ( Math.PI * Math.E * (double)_num );
      }
   }

};  // class MyThread


/////////////////////////////////////
//
//   c l a s s   S u c k C P U
//
/////////////////////////////////////
class SuckCPU 
{
   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      List<MyThread> tdb = new List<MyThread>();
      string         fmt;
      double         cpu;
      long           num, mps;
      int            i, n, argc;

      argc = args.Length;
      n    = 1;
      if ( argc >= 1 )
         Int32.TryParse( args[0], out n );
      n    = Math.Min( 8, Math.Max( n, 1 ) );
      for ( i=0; i<n; tdb.Add( new MyThread() ), i++ );
      Console.WriteLine( "{0} threads; Hit <ENTER> to terminate ...", n );
      for ( i=0; i<tdb.Count; tdb[i++].Start() );
      Console.ReadLine();
      for ( i=0; i<tdb.Count; tdb[i++].Stop() );
      for ( i=0,num=0; i<tdb.Count; num+=tdb[i++].num() );
      cpu = tdb[0].cpuMillis();
      mps = (long) ( (double)num / cpu );
      fmt = "{0} in {1} ms = {2} mpms";
      Console.WriteLine( fmt, num, cpu.ToString( "F3" ), mps );
      return 0;

   } // main()

} // class SuckCPU
