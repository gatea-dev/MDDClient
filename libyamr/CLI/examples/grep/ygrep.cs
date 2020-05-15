/******************************************************************************
*
*  ygrep.cs
*     grep yamr tape
*
*  REVISION HISTORY:
*     10 MAY 2019 jcs  Created.
*
*  (c) 1994-2019, Gatea Ltd.
******************************************************************************/
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Net;
using libyamr;
using libyamr.Data;

class ygrep : Reader 
{
   ////////////////////////////////
   // Yeah Booooooooiii
   ////////////////////////////////
   public void ProgressBar( int i )
   {
      if ( i == 0 )
         return;
      if ( (i%1000000) == 0 )
         Console.Write( i/1000000 );
      else if ( ( i%500000 ) == 0 )
         Console.Write( "+" );
      else if ( ( i%100000 ) == 0 )
         Console.Write( "." );
   }


   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main( String[] args ) 
   {
      try {
         ygrep  grep;
         int    i, argc, nQ, addr;
         short  sess, pro;
         bool   bB;
         string arg, hdr, body, eol, p1, p2;

         /////////////////////
         // Quickie checks
         /////////////////////
         nQ   = 0;
         bB   = false;
         sess = 0;
         addr = 0;
         pro  = 0;
         eol  = " \\\n   ";
         argc = args.Length;
         arg  = ( argc > 0 ) ? args[0] : "";
         if ( arg == "--version" ) {
            Console.WriteLine( "ygrep Build 1" );
            Console.WriteLine( yamr.Version() );
            return 0;
         }
         if ( ( arg == "--config" ) || ( arg == "-help" ) || ( arg == "-h" ) ) {
            hdr  = "Usage : ygrep <tape_filename>" + eol;
            hdr += "-query [<NumMsgMil>]" + eol;
            hdr += "-session <ipAddr>:<SessID>" + eol;
            hdr += "-protocol <Proto ID or Name>" + eol;
            hdr += "-dump";
            Console.WriteLine( hdr );
            return 0;
         }

         /////////////////////
         // Validate File
         /////////////////////
         byte[] raw;

         grep = new ygrep();
         raw  = yamr.ReadFile( arg );
         if ( raw == null ) {
            hdr = "Invalid tape " + arg + "; Exitting ...";
            Console.WriteLine( hdr );
            return 0;
         }
         Console.WriteLine( "Tape " + arg + " = " + raw.Length + " bytes" );

         /////////////////////
         // Supported Protocols
         /////////////////////
         IPAddress  ip;
         FieldList  md;
         StringList sl;
         StringMap  sm;
//         bespoke.Usage  prm;

         md = new FieldList( grep );
         sl = new StringList( grep );
         sm = new StringMap( grep );
//         prm = new bespoke.Usage( grep );

         /////////////////////
         // cmd-line args
         /////////////////////
         for ( i=1; i<argc; ) {
            arg = args[i++];
            if ( ( arg == "-query" ) && ( i < argc ) )
               Int32.TryParse( args[i++], out nQ );
            else if ( ( arg == "-session" ) && ( i < argc ) ) {
               string[] kv = args[i++].Split( ':' );

               if ( kv.Length > 1 ) {
                  ip   = IPAddress.Parse( kv[0] );
                  addr = BitConverter.ToInt32( ip.GetAddressBytes(), 0 );
                  Int16.TryParse( kv[1], out sess );
               }
            }
            else if ( ( arg == "-protocol" ) && ( i < argc ) )
               Int16.TryParse( args[i++], out pro );
            else if ( arg == "-dump" )
               bB = true;
         }
         Console.WriteLine( ygrep.Version() );

         /////////////////////
         // Query??
         /////////////////////
         Dictionary<long, int> qdb;
         long                  stream;
         int                   u32, l32, nMsg;
         yamrMsg               ym;

         qdb = new Dictionary<long, int>();
         if ( nQ != 0 ) {
            /*
             * Read up to nQ Million Msgs, indexed by ( Addr, SessID, Proto )
             */
            nQ *= 1024;
            nQ *= 1024;
            grep.Open( args[0] );
            Console.WriteLine( "Querying " + args[0] );
            for ( i=0,grep.Rewind(); i<nQ; i++ ) {
               if ( (ym=grep.Read()) == null )
                  break; // for-i
               grep.ProgressBar( i );
               grep.Decode( ym );
               stream      = ym._Host;
               stream    <<= 32;
               l32         = ym._SessionID;
               l32       <<= 16;
               l32        += ym._MsgProtocol;
               stream     += l32;
               if ( !qdb.TryGetValue( stream, out nMsg ) )
                  nMsg = 0;
               qdb[stream] = nMsg+1;
            }
            /*
             * Dump Msg Count by ( Addr, SessID, Proto )
             */
            hdr  = "\n";
            hdr += "Count    IP Address      SessID Proto  ProtoName     \n";
            hdr += "-------- --------------- ------ ------ --------------\n";
            Console.Write( hdr );
            foreach( KeyValuePair<long, int> kv in qdb ) {
               stream = kv.Key;
               nMsg   = kv.Value;
               u32    = (int)( stream >> 32 );
               l32    = (int)stream;
               sess   = (short)( l32 >> 16 );
               pro    = (short)l32;
               ip     = new IPAddress( BitConverter.GetBytes( u32 ) );
               p1     = ip.ToString();
               p2     = grep.ProtoName( pro );
               hdr    = String.Format( "{0,8} ", nMsg );
               hdr   += String.Format( "{0,-15} ", p1 );
               hdr   += String.Format( "{0,6} ", sess );
               hdr   += "0x" + pro.ToString( "X4" ) + " " + p2;
               Console.WriteLine( hdr );
            }
            grep.Close();
            return 0;
         }

         /////////////////////
         // Rock on
         /////////////////////
         Console.WriteLine( "[Time] SeqNum <<Proto>> host:sessID data" );
         grep.Open( args[0] );
         for ( i=0,grep.Rewind(); (ym=grep.Read()) != null; i++ ) {
            grep.Decode( ym );
            if ( ( pro != 0 ) && ( ym._MsgProtocol != pro ) )
               continue; // for-i
            if ( ( addr != 0 ) && ( ym._Host != addr ) )
               continue; // for-i
            if ( ( sess != 0 ) && ( ym._SessionID != sess ) )
               continue; // for-i
            hdr  = grep.DumpHeader( ym );
            body = bB ? grep.DumpBody( ym ) : "";
            Console.WriteLine( hdr + " " + body );
         }
         grep.Close();
         Console.WriteLine( "Done!!" );
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   } 
}
