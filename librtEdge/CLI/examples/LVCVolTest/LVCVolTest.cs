/******************************************************************************
*
*  LVCVolTest.cs
*     librtEdge .NET interface test - Last Value Cache(LVC)
*
*  REVISION HISTORY:
*     11 MAY 2011 jcs  Created.
*     31 JUL 2011 jcs  Build  6: GetSchema() / LVCSnap
*     17 JAN 2012 jcs  Build  9: SnapAll() / ViewAll() on 64-bit 
*     20 JAN 2012 jcs  Build 10: rtEdgeField
*
*  (c) 1994-2012 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using librtEdge;

class rtElapsed
{
   ////////////////////
   // Instance Members
   ////////////////////
   private string _msg;
   private double _d0;

   ////////////////////
   // Constructor
   ////////////////////
   public rtElapsed( string msg )
   {
      Reset( msg );
   }

   ////////////////////
   // Operations
   ////////////////////
   public void Reset( string msg )
   {
      _msg = msg;
      _d0  = rtEdge.TimeNs();
   }

   public void usElapsed()
   {
      double dd;

      dd = 1E6 * ( rtEdge.TimeNs() - _d0 );
      Console.WriteLine( _msg + " in {0}uS", dd.ToString("F1") );
   }

   public void msElapsed()
   {
      double dd;

      dd = 1E3 * ( rtEdge.TimeNs() - _d0 );
      Console.WriteLine( _msg + " in {0}mS", dd.ToString("F3") );
   }
}
 
class LVCVolTest 
{
   ////////////////////
   // Class-wide Members
   ////////////////////
   static string _pFld = "BID";

   ////////////////////
   // Instance Members
   ////////////////////
   private LVC _lvc;


   ////////////////////
   // Constructor
   ////////////////////
   private LVCVolTest( LVC lvc )
   {
      _lvc = lvc;
   }


   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private void Show( LVCData ld, string pSvc, string pTkr )
   {
      LVCSnap     s = new LVCSnap( _lvc, ld );
      rtEdgeField f;
      int         i;
      double      dUs, dAge;
      string      pUs;

      dUs  = ( ld._dSnap * 1000000.0 );
      dAge = ld._dAge * 1000.0;
      pUs = dUs.ToString( "F1" );
      Console.WriteLine( "{0},{1} in {2}uS : nUpd={3}; Age={4}mS", 
         pSvc, pTkr, pUs, ld._nUpd, dAge.ToString("F2") );
      if ( s.HasField( _pFld ) )
         Console.WriteLine( _pFld + " = " + s.GetField( _pFld ) );
      else {
         for ( i=0; i<ld._flds.Length; i++ ) {
            f = ld._flds[i];
            Console.WriteLine( "[{0},{1}] {2}", f.Fid(), f.Name(), f.Data() );
         }
       }
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args) 
   {
      try {
         LVCVolatile vol;
         LVCData     ld;
         LVCVolTest  ut;
         rtElapsed   e0;
         int         i, j, idx, argc;
         string      pLVC, pAdm, pSvc, pTkr, pEdg;
         string[]    tkrs;

         // [ <file> <Service> <Tickers> <edgHost:edgPort> ]

         argc  = args.Length;
         pLVC = "C:\\Gatea\\RealTime\\cache.lvc";
         pAdm = "localhost:8775";
         pSvc = "BBO";
         pTkr = "EUR/USD,GBP/USD";
         pEdg = "localhost:9998";
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pLVC = args[i]; break;
               case 1: pSvc = args[i]; break;
               case 2: pTkr = args[i]; break;
               case 3: pEdg = args[i]; break;
            }
         }
         tkrs = pTkr.Split(',');
         Console.WriteLine( rtEdge.Version() );
         rtEdge.Log( "C:\\TEMP\\LVCVolTest.txt", 0 );

         // LVCVolatile - All Tickers; Dump BID if found ...

         e0  = new rtElapsed( "LVCVolatile()" );
         vol = new LVCVolatile( pLVC, pAdm );
         e0.msElapsed();
         ut  = new LVCVolTest( vol );
         for ( i=0; i<tkrs.Length; i++ ) {
            e0.Reset( "LVCVolatile.GetIdx( pSvc, " + tkrs[i] + " )" );
            idx = vol.GetIdx( pSvc, tkrs[i] );
            e0.usElapsed();
            if ( idx == -1 )
               continue; // for-i
            e0.Reset( "LVCVolatile.ViewByIdx( " + idx.ToString() + " )" );
            ld = vol.ViewByIdx( idx );
            e0.usElapsed();
         }
         e0.Reset( "LVCVolatile.Destroy()" );
         vol.Destroy();
         e0.msElapsed();
         Console.WriteLine( "Hit <ENTER> to continue..." );
         Console.ReadLine();

         // Dump all fields from 1st ticker 3 times

         vol = new LVCVolatile( pLVC, pAdm );
         ut  = new LVCVolTest( vol );
         for ( j=0; j<3; j++ ) {
            for ( i=0; i<tkrs.Length; i++ ) {
               idx = vol.GetIdx( pSvc, tkrs[i] );
               ld  = vol.ViewByIdx( idx );
               ut.Show( ld, ld._pSvc, ld._pTkr );
            }
            Console.WriteLine( "Hit <ENTER> to continue..." );
            Console.ReadLine();
         }
         vol.Destroy();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 
}
