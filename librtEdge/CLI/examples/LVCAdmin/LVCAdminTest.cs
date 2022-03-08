/******************************************************************************
*
*  LVCAdminTest.cs
*     librtEdge .NET LVCAdmin driver
*
*  REVISION HISTORY:
*      7 MAR 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using librtEdge;

class LVCAdminTest 
{
   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      LVCAdmin lvc;
      String   svr, svc, tkrs;
      int      argc;

      /////////////////////
      // Quickie checks
      /////////////////////
      argc = args.Length;
      if ( argc > 0 && ( args[0] == "--version" ) ) {
         Console.WriteLine( rtEdge.Version() );
         return 0;
      }
      tkrs = ( argc > 0 ) ? args[0] : "ABBV,ABC,ABT,ACE,ACN,ACT,ADBE";
      svc  = ( argc > 1 ) ? args[1] : "WARP_1";
      svr  = ( argc > 2 ) ? args[2] : "gatea.com:8775";
      Console.WriteLine( rtEdge.Version() );
      Console.WriteLine( "Connecting to {0} ...", svr );
      lvc = new LVCAdmin( svr );
      lvc.AddTickers( svc, tkrs.Split(',') );
      Console.WriteLine( "Hit <ENTER> to terminate ..." );
      Console.ReadLine();
      Console.WriteLine( "Done!!" );
      return 0;
   }
}
