/******************************************************************************
*
*  PubCache.cs
*     librtEdge .NET interactive Publisher w/ Cache
*
*  REVISION HISTORY:
*     24 MAR 2022 jcs  Created.
*
*  (c) 1994-2022, Gatea, Ltd.
******************************************************************************/
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using librtEdge;


////////////////////////////////////////////////
//
//        c l a s s   C a c h e F i e l d
//
////////////////////////////////////////////////
/*
 * 4 supported field types (for now)
 */
enum FieldType {
   type_Undef,
   type_Int64,
   type_Int32,
   type_Double,
   type_String
} // enum FieldType

/**
 * \class CacheField
 * \brief Polymorphic cache'ed field
 */
class CacheField
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private int       _Fid;
   private FieldType _Type;
   /*
    * No union's in C#
    */
   private long      _i64;
   private int       _i32;
   private double    _d64;
   private string    _str;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public CacheField( int fid )
   {
      _Fid  = fid;
      _Type = FieldType.type_Undef;
      _i64  = 0;
      _i32  = 0;
      _d64  = 0.0;
      _str  = null;
   }

   ////////////////////////////////
   // Cache Update
   ////////////////////////////////
   public void AddInt64( long i64 )
   {
      _Type = FieldType.type_Int64; 
      _i64  = i64; 
   }

   public void AddInt32( int i32 )
   {
      _Type = FieldType.type_Int32; 
      _i32  = i32; 
   }

   public void AddDouble( double d64 )
   {
      _Type = FieldType.type_Double; 
      _d64  = d64; 
   }

   public void AddString( string str )
   {
      _Type = FieldType.type_String; 
      _str  = str; 
   }

   ////////////////////////////////
   // Publication
   ////////////////////////////////
   public void Publish( rtEdgePubUpdate u )
   {
      switch( _Type ) {
         case FieldType.type_Undef:
            break;
         case FieldType.type_Int64:
            u.AddFieldAsInt64( _Fid, _i64 );
            break;
         case FieldType.type_Int32:
            u.AddFieldAsInt32( _Fid, _i32 ); 
            break;
         case FieldType.type_Double:
            u.AddFieldAsDouble( _Fid, _d64 ); 
            break;
         case FieldType.type_String:
            u.AddFieldAsString( _Fid, _str ); 
            break;
      }
   }

} // class CacheField


////////////////////////////////////////////////
//
//        c l a s s   C a c h e R e c o r d
//
////////////////////////////////////////////////
/**
 * \class CacheRecord
 * \brief A cached record w/ fields that may be  
 *
 */
class CacheRecord
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private PubCache                  _pub;
   private string                    _tkr;
   private IntPtr                    _StreamID;
   private Dictionary<int, CacheField> _flds;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public CacheRecord( PubCache pub, string tkr )
   {
      _pub      = pub;
      _tkr      = tkr;
      _flds     = new Dictionary<int, CacheField>();
      _StreamID = (IntPtr)null;
   }

   ////////////////////////////////
   // Access
   ////////////////////////////////
   public string Symbol()   { return _tkr; }
   public IntPtr StreamID() { return _StreamID; }

   public bool IsWatched()
   {
      return( _StreamID != null );
   }

   ////////////////////////////////
   // WatchList
   ////////////////////////////////
   public void AddWatch( IntPtr streamID )
   {
      _StreamID = streamID;
      Publish( true );
   }

   public void RemoveWatch()
   {
      _StreamID = (IntPtr)null;
   }

   ////////////////////////////////
   // Cache Updates
   ////////////////////////////////
   public void AddFieldAsInt64( int fid, long i64 )
   {
      CacheField fld;

      if ( !_flds.TryGetValue( fid, out fld ) ) {
         fld = new CacheField( fid );
         _flds.Add( fid, fld );
      }
      fld.AddInt64( i64 );
   }

   public void AddFieldAsInt32( int fid, int i32 )
   {
      CacheField fld;

      if ( !_flds.TryGetValue( fid, out fld ) ) {
         fld = new CacheField( fid );
         _flds.Add( fid, fld );
      }
      fld.AddInt32( i32 );
   }

   public void AddFieldAsDouble( int fid, double d64 )
   {
      CacheField fld;

      if ( !_flds.TryGetValue( fid, out fld ) ) {
         fld = new CacheField( fid );
         _flds.Add( fid, fld );
      }
      fld.AddDouble( d64 );
   }

   public void AddFieldAsString( int fid, string str )
   {
      CacheField fld;

      if ( !_flds.TryGetValue( fid, out fld ) ) {
         fld = new CacheField( fid );
         _flds.Add( fid, fld );
      }
      fld.AddString( str );
   }

   ////////////////////////////////
   // Publish
   ////////////////////////////////
   public void Publish( bool bImg )
   {
      rtEdgePubUpdate u;
      CacheField        fld;

      // Pre-condition

      if ( !IsWatched() )
         return;

      // OK to publish

      u = new rtEdgePubUpdate( _pub, _tkr, _StreamID, bImg );
      foreach ( var item in _flds ) {
         fld = (CacheField)item.Value;
         fld.Publish( u );
      }
      u.Publish();
   }

} // class CacheRecord

class PubCache : rtEdgePublisher
{
   ////////////////////////////////
   // Members
   ////////////////////////////////
   private Dictionary<string, CacheRecord> _cache;
   private Timer                           _tmr;
   private TimerCallback                   _cbk;
   private int                             _rtl;
   private string                          _bds;
   private int                             _bdsStreamID;

   ////////////////////////////////
   // Constructor
   ////////////////////////////////
   public PubCache( string svr, string svc, string tkrs, int tTmr ) :
      base( svr, svc, true, false, true)
   {
      string[] csv = tkrs.Split(',');
      int      i, nt;

      // Fill watchlist with some stuff

      _cache = new Dictionary<string, CacheRecord>();
      _rtl   = 0;
      nt     = csv.Length;
      for ( i=0; i<nt; i++ ) {
         _cache.Add( csv[i], new CacheRecord( this, csv[i] ) );
          Console.WriteLine( "Publishing {0} to {1}@{2}", csv[i], svc, svr );
      }

      // Supported Symbol List (BDS)

      _bds         = "SymList";
      _bdsStreamID = 0;

      // Binary Protocol / Timer Callback

      SetBinary( true );
      _cbk = new TimerCallback( ProcessKafkaMsg );
      _tmr = new Timer( _cbk, this, tTmr, tTmr );
   }

   ////////////////////////////////
   // Timer Handler
   ////////////////////////////////
   public void ProcessKafkaMsg( object notUsed )
   {
      CacheRecord rec;
      string      tkr  = "NEW." + _rtl;
      string[]    tkrs = { tkr };

      /*
       * This is your Kafka entry point; We just publish the cache
       */
      lock( _cache ) {
         _cache.Add( tkr, new CacheRecord( this, tkr ) );
         if ( _bdsStreamID != 0 )
            PublishBDS( _bds, _bdsStreamID, tkrs );
         foreach ( var item in _cache ) {
            rec = (CacheRecord)item.Value;
            UpdateRecord( rec, false );
         }
      }
   }

   ////////////////////////////////
   // Operations
   ////////////////////////////////
   public void UpdateRecord( CacheRecord rec, bool bImg )
   {
      /*
       * Update Values in CacheRecord
       */
      rec.AddFieldAsString( 3, rec.Symbol() );
      rec.AddFieldAsInt32( 6, ++_rtl );
      rec.AddFieldAsDouble( 14263, rtEdge.TimeNs() );
      /*
       * Pubish if watched
       */
      rec.Publish( false );
   }


   ////////////////////////////////
   // rtEdgePublisher Interface
   ////////////////////////////////
   public override void OnConnect( string pConn, rtEdgeState state )
   {
      bool   bUp = ( state == rtEdgeState.edg_up );
      string pUp = bUp ? "UP" : "DOWN";

      Console.WriteLine( "[{0}] CONN {1} : {2}", DateTimeMs(), pConn, pUp );
   }

   public override void OnOpen( string tkr, IntPtr arg )
   {
      CacheRecord rec;
      rtEdgePubUpdate u;

      Console.WriteLine( "[{0}] OPEN {1}", DateTimeMs(), tkr );
      lock( _cache ) {
         if ( _cache.TryGetValue( tkr, out rec ) )
            rec.AddWatch( arg ); 
         else {
            u = new rtEdgePubUpdate( this, tkr, arg, false );
            u.PubError( tkr + " not found" );
         }
      }
   }

   public override void OnClose( string tkr )
   {
      CacheRecord rec;

      Console.WriteLine( "[{0}] CLOSE {1}", DateTimeMs(), tkr );
      lock( _cache ) {
         if ( _cache.TryGetValue( tkr, out rec ) )
            rec.RemoveWatch();
      }
   }

   public override void OnOpenBDS( string tkr, IntPtr arg )
   {
      rtEdgePubUpdate u;
      string          err;
      string[]        tkrs;

      Console.WriteLine( "[{0}] OPEN.BDS {1}", DateTimeMs(), tkr );
      lock( _cache ) {
         if ( tkr == _bds ) {
            _bdsStreamID = (int)arg;
            tkrs = _cache.Keys.ToArray();
            PublishBDS( _bds, _bdsStreamID, tkrs );
         }
         else {
            err  = "Unsupported BDS " + tkr;
            err += "; Request " + _bds + " instead";
            u = new rtEdgePubUpdate( this, tkr, arg, false );
            u.PubError( err );
         }
      }
   }

   public override void OnCloseBDS( string tkr )
   {
      Console.WriteLine( "[{0}] CLOSE.BDS {1}", DateTimeMs(), tkr );
      _bdsStreamID = 0;
   }



   ////////////////////////////////
   // main()
   ////////////////////////////////
   public static int Main(String[] args)
   {
      try {
         PubCache pub;
         int      i, tTmr, argc;
         string   svr, svc, tkr;

         // Quickie Check

         argc = args.Length;
         if ( argc > 0 && ( args[0] == "--version" ) ) {
            Console.WriteLine( rtEdge.Version() );
            return 0;
         }
         if ( argc > 0 && ( args[0] == "--config" ) ) {
            Console.WriteLine( "<host:port> <Svc> <Tickers> <pubTmr>" );
            return 0;
         }

         // [ <host:port> <Svc> <pubTmr> <ChainFile> <tHbeat> ]

         svr  = "localhost:9003";
         svc  = "bcast-svc";
         tkr  = "A,B,C,D,E";
         tTmr = 1000;
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: svr = args[i]; break;
               case 1: svc = args[i]; break;
               case 2: tkr = args[i]; break;
               case 3:
                  tTmr = Convert.ToInt32( args[i], 10 );
                  break;
            }
         }
         Console.WriteLine( rtEdge.Version() );
         pub = new PubCache( svr, svc, tkr, tTmr );
         pub.SetHeartbeat( 60 );
         pub.PubStart();
         Console.WriteLine( pub.pConn() );
         Console.WriteLine( "Hit <ENTER> to terminate..." );
         Console.ReadLine();
         pub.Stop();
      }
      catch( Exception e ) {
         Console.WriteLine( "Exception: " + e.Message );
      }
      return 0;
   }
}

