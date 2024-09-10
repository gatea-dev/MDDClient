/******************************************************************************
*
*  LVCMonCLI.cs
*     librtEdge .NET LVCAdmin driver
*
*  REVISION HISTORY:
*      7 MAR 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections.Generic;
using librtEdge;

class LVCMonCLI
{
   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args )
   {
      try {
         int    i, ii, argc, num, M, S;
         double tSlp;
         bool   aOK;
         string s, file, upd;

         /////////////////////
         // Quickie checks
         /////////////////////
         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         file = "./MDDirectMon.stats";
         tSlp = 1.0;
         num  = 15;
         upd  = "15,30,60,300,900";
         if ( ( argc == 0 ) || ( args[0] == "--config" ) ) {
            s  = "Usage: " + args[0] + " \\ \n";
            s += "       [ -f <LVC stats file> ] \\ \n";
            s += "       [ -r <Stat Sample Rate> ] \\ \n";
            s += "       [ -n <Num Snaps> ] \\ \n";
            s += "       [ -u <CSV UpdRates> ] \\ \n";
            Console.WriteLine( s );
            Console.Write( "      -f  : {0}\n", file );
            Console.Write( "      -r  : {0}\n", tSlp );
            Console.Write( "      -n  : {0}\n", num );
            Console.Write( "      -u  : {0}\n", upd );
            return 0;
         }

         /////////////////////
         // cmd-line args
         /////////////////////
         for ( i=0; i<argc; i++ ) {
            aOK = ( i+1 < argc );
            if ( !aOK )
               break; // for-i
            if ( args[i] == "-f" )
               file = args[++i];
            else if ( args[i] == "-r" )
               tSlp = Convert.ToDouble( args[++i] );
            else if ( args[i] == "-n" )
               num = Convert.ToInt32( args[++i], 10 );
            else if ( args[i] == "-u" )
               upd = args[++i];
         }
         Console.WriteLine( rtEdge.Version() );
         tSlp = Math.Min( Math.Max( 0.1, tSlp ), 86400.0 );
         num  = Math.Min( Math.Max(   1,  num ),  3600 );

         ////////////////
         // Rock on
         ////////////////
         LVCStatMon      mon;
         List<int>       idb = new List<int>();
         string[]        tdb = upd.Split(',');
         rtEdgeChanStats st;

         for ( i=0; i<tdb.Length; i++ ) {
            try {
               idb.Add( Convert.ToInt32( tdb[i], 10 ) );
            } catch( Exception ) {
               Console.WriteLine( "Invalid Time " + tdb[i] );
            }
         }
         mon = new LVCStatMon( file );
         Console.WriteLine( "BUILD : %s", mon.BuildNum() );
         Console.WriteLine( "EXE   : %s", mon.ExeName() );
         s = "NumMsg,NumByte,";
         for ( i=0; i<idb.Count; i++ ) {
            M  = idb[i] / 60;
            S  = idb[i] % 60;
            s += M.ToString( "D2" ) + ":" + S.ToString( "D2" ) + ",";
         }
         Console.WriteLine( s );
         for ( i=0; i<num; i++ ) {
            st = mon.Snap();
            s  = st._nMsg.ToString() + ",";
            s += st._nByte.ToString() + ",";
            for ( ii=0; ii<idb.Count; ii++ ) {
               s += mon.UpdPerSec( idb[ii] );
               s += ",";
            }
            Console.WriteLine( s );
            rtEdge.Sleep( tSlp );
         }
         Console.WriteLine( "Done!!" );
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
 
   } // Main()
 
} // class LVCMonCLI
