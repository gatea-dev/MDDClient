/******************************************************************************
*
*  CDBTest.cs
*     librtEdge .NET interface test - ChartDB
*
*  REVISION HISTORY:
*     20 OCT 2012 jcs  Created ( from LVCTest)
*     12 OCT 2015 jcs  Build 32: CDBData-centric
*
*  (c) 1994-2015 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using librtEdge;

class CDBTest 
{
   ////////////////////////////////
   // Helpers
   ////////////////////////////////
   private static void Show_OBSOLETE( CDBData ld ) 
   {
      float  dv;
      uint   i, nf;
      double dUs, dAge;
      string hdr, pUs, tm;

      hdr  = "[" + ld._pSvc + "," + ld._pTkr + "," + ld._fid.ToString() + "]";
      dUs  = ( ld._dSnap * 1000000.0 );
      dAge = ld._dAge * 1000.0;
      pUs  = dUs.ToString( "F1" );
      nf   = ld._curTick;
      Console.WriteLine( "{0} in {1}uS : {2} of {3} ticks; Age={4}mS", 
         hdr, pUs, nf, ld._numTick, dAge.ToString("F2") );
      for ( i=0; i<nf; i++ ) {
         tm = ld.SeriesTime( (int)i );
         if ( (dv=ld._flds[i]) != 0 )
            Console.WriteLine( "   {0} : {1}", tm, dv.ToString("F4") );
       }
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args) 
   {
      try {
         ChartDB   cdb;
         CDBData   d;
         CDBTable  t;
         CDBQuery  q;
         CDBRecRef r;
         int       i, argc, fid, nt;
         string    pCht, pAdm, pSvc, pTkr, pFld;
         string[]  tkrs;

         // [ <file> <Service> <Tickers> <FID> ]

         argc  = args.Length;
         pCht = "C:\\Gatea\\RealTime\\chart.db";
         pAdm = "localhost:8775";
         pSvc = "ULTRAFEED";
         pTkr = "AAPL,MHFI";
         pFld = "22";
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: pCht = args[i]; break;
               case 1: pSvc = args[i]; break;
               case 2: pTkr = args[i]; break;
               case 3: pFld = args[i]; break;
            }
         }
         tkrs = pTkr.Split(',');
         fid  = Convert.ToInt32( pFld );
         Console.WriteLine( rtEdge.Version() );

         // Snap and Dump

         cdb = new ChartDB( pCht, pAdm );
         nt  = tkrs.Length;
/*
         for ( i=0; i<tkrs.Length; i++ ) {
            d = cdb.View( pSvc, tkrs[i], fid );
            Show_OBSOLETE( d );
         }
 */
         if ( nt == 1 ) {
            d = cdb.View( pSvc, tkrs[0], fid );
            Console.Write( d.DumpNonZero() );
         }
         else {
            t = cdb.ViewTable( pSvc, tkrs, fid );
            Console.Write( t.DumpByTime() );
         }

         // Dump DB

         Console.WriteLine( "Hit <ENTER> to dump DB ..." );
         Console.ReadLine();
         q = cdb.Query();
         for ( i=0; i<q._nRec; i++ ) {
            r = q._recs[i];
            Console.WriteLine( "[{0},{1},{2}]", r._pSvc, r._pTkr, r._fid );
         }
         cdb.Destroy();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 
}
