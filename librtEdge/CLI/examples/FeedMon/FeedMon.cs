/******************************************************************************
*
*  FeedMon.cs
*     Feed Monitor
*
*  REVISION HISTORY:
*      2 MAY 2014 jcs  Created.
*
*  (c) 1994-2014 Gatea, Ltd.
******************************************************************************/
using System;
using System.IO;
using System.Collections;
using System.ServiceProcess;
using System.Threading;
using System.Runtime.InteropServices;
using librtEdge;

namespace FeedMon
{   
   #region stat structs
   ////////////////////////////////
   // struct
   ////////////////////////////////
   [StructLayout(LayoutKind.Explicit, Size=2072)]
   public struct GLmdStatsHdr
   {
       [FieldOffset(0)]
       public int    _version;
       [FieldOffset(4)]
       public int    _fileSiz;
       [FieldOffset(8)]
       public int    _tStart_sec;
       [FieldOffset(12)]
       public int    _tStart_micros;
       [FieldOffset(16)]
       public string _exe;
       [FieldOffset(1040)]
       public string _build;
       [FieldOffset(2064)]
       public int    _nSub;
       [FieldOffset(2068)]
       public int    _nPub;
   }
   
   [StructLayout(LayoutKind.Explicit, Size=712)]
   public struct GLmdChanStats
   {
       [FieldOffset(0)]
       public long    _nMsg;
       [FieldOffset(8)]
       public long    _nByte;
       [FieldOffset(16)]
       public int     _lastMsg_sec;
       [FieldOffset(20)]
       public int     _lastMsg_micros;
       [FieldOffset(24)]
       public int     _nOpen;
       [FieldOffset(28)]
       public int     _nClose;
       [FieldOffset(32)]
       public int     _nImg;
       [FieldOffset(36)]
       public int     _nUpd;
       [FieldOffset(40)]
       public int     _nDead;
       [FieldOffset(44)]
       public int     _nInsAck;
       [FieldOffset(48)]
       public int     _nInsNak;
       [FieldOffset(52)]
       public int     _lastConn;
       [FieldOffset(56)]
       public int     _lastDisco;
       [FieldOffset(60)]
       public int     _nConn;
       [FieldOffset(64)]
       public int     _iVal00;
       [FieldOffset(68)]
       public int     _iVal01;
       [FieldOffset(72)]
       public int     _iVal02;
       [FieldOffset(76)]
       public int     _iVal03;
       [FieldOffset(80)]
       public int     _iVal04;
       [FieldOffset(84)]
       public int     _iVal05;
       [FieldOffset(88)]
       public int     _iVal06;
       [FieldOffset(92)]
       public int     _iVal07;
       [FieldOffset(96)]
       public int     _iVal08;
       [FieldOffset(100)]
       public int     _iVal09;
       [FieldOffset(104)]
       public int     _iVal10;
       [FieldOffset(108)]
       public int     _iVal11;
       [FieldOffset(112)]
       public int     _iVal12;
       [FieldOffset(116)]
       public int     _iVal13;
       [FieldOffset(120)]
       public int     _iVal14;
       [FieldOffset(124)]
       public int     _iVal15;
       [FieldOffset(128)]
       public int     _iVal16;
       [FieldOffset(132)]
       public int     _iVal17;
       [FieldOffset(136)]
       public int     _iVal18;
       [FieldOffset(140)]
       public int     _iVal19;
       [FieldOffset(144)]
       public double   _dVal00;
       [FieldOffset(152)]
       public double   _dVal01;
       [FieldOffset(160)]
       public double   _dVal02;
       [FieldOffset(168)]
       public double   _dVal03;
       [FieldOffset(176)]
       public double   _dVal04;
       [FieldOffset(184)]
       public double   _dVal05;
       [FieldOffset(192)]
       public double   _dVal06;
       [FieldOffset(200)]
       public double   _dVal07;
       [FieldOffset(208)]
       public double   _dVal08;
       [FieldOffset(216)]
       public double   _dVal09;
       [FieldOffset(224)]
       public double   _dVal10;
       [FieldOffset(232)]
       public double   _dVal11;
       [FieldOffset(240)]
       public double   _dVal12;
       [FieldOffset(248)]
       public double   _dVal13;
       [FieldOffset(256)]
       public double   _dVal14;
       [FieldOffset(264)]
       public double   _dVal15;
       [FieldOffset(272)]
       public double   _dVal16;
       [FieldOffset(280)]
       public double   _dVal17;
       [FieldOffset(288)]
       public double   _dVal18;
       [FieldOffset(296)]
       public double   _dVal19;
       [FieldOffset(304)]
       public string   _objname;
       [FieldOffset(504)]
       public string   _dstConn;
       [FieldOffset(704)]
       public char     _bUp;
       [FieldOffset(705)]
       public char     _pad1;
       [FieldOffset(706)]
       public char     _pad2;
       [FieldOffset(707)]
       public char     _pad3;
       [FieldOffset(708)]
       public char     _pad4;
       [FieldOffset(709)]
       public char     _pad5;
       [FieldOffset(710)]
       public char     _pad6;
       [FieldOffset(711)]
       public char     _pad7;
   }
   
   [StructLayout(LayoutKind.Explicit, Size=3496)]
   public struct FeedStats
   {
       [FieldOffset(0)]
       public GLmdStatsHdr  _hdr;
       [FieldOffset(2072)]
       public GLmdChanStats _sub;
       [FieldOffset(2784)]
       public GLmdChanStats _pub;
   }
   #endregion


   ////////////////////////////////
   // .NET mmap is for children
   ////////////////////////////////
   class GLmmap
   {
      #region private Member variables
      //////////////////////////////
      // Private Members
      //////////////////////////////
      private string   _file;
      private IntPtr   _hFile;
      private IntPtr   _hMap;
      private IntPtr   _base;
      private FileInfo _fi;
      #endregion
   
      #region Constructor
      ////////////////////////
      // Constructor
      ////////////////////////
      public GLmmap( string file )
      {
         _file  = file;
         _hFile = IntPtr.Zero;
         _hMap  = IntPtr.Zero;
         _base  = IntPtr.Zero;
         _fi    = new FileInfo( _file );
         Open();
      }
   
      public void Dispose()
      {
         Close();
      }
      #endregion


      #region Access
      ////////////////////////
      // Access
      ////////////////////////
      public uint Size()
      {
         return (uint)_fi.Length;
      }

      public GLmdStatsHdr GetHdr()
      {
         GLmdStatsHdr h;
         IntPtr       oh;

         oh               = Marshal.OffsetOf( typeof( FeedStats ), "_hdr" );
         h                = new GLmdStatsHdr();
         h._version       = ReadInt32( oh, h, "_version" );
         h._fileSiz       = ReadInt32( oh, h, "_fileSiz" );
         h._tStart_sec    = ReadInt32( oh, h, "_tStart_sec" );
         h._tStart_micros = ReadInt32( oh, h, "_tStart_micros" );
         h._exe           = ReadString( oh, h, "_exe" );
         h._build         = ReadString( oh, h, "_build" );
         h._nSub          = ReadInt32( oh, h, "_nSub" );
         h._nPub          = ReadInt32( oh, h, "_nPub" );
         return h;
      }

      public GLmdChanStats GetChanStats( bool bPub )
      {
         GLmdChanStats st;
         IntPtr        oh;
         string        fld;

         fld                = bPub ? "_pub" : "_sub";
         oh                 = Marshal.OffsetOf( typeof( FeedStats ), fld );
         st                 = new GLmdChanStats();
         st._nMsg           = ReadInt64( oh, st, "_nMsg" );
         st._nByte          = ReadInt64( oh, st, "_nByte" );
         st._lastMsg_sec    = ReadInt32( oh, st, "_lastMsg_sec" );
         st._lastMsg_micros = ReadInt32( oh, st, "_lastMsg_micros" );
         st._nOpen          = ReadInt32( oh, st, "_nOpen" );
         st._nClose         = ReadInt32( oh, st, "_nClose" );
         st._nImg           = ReadInt32( oh, st, "_nImg" );
         st._nUpd           = ReadInt32( oh, st, "_nUpd" );
         st._nDead          = ReadInt32( oh, st, "_nDead" );
         st._lastConn       = ReadInt32( oh, st, "_lastConn" );
         st._lastDisco      = ReadInt32( oh, st, "_lastDisco" );
         st._nConn          = ReadInt32( oh, st, "_nConn" );
         st._iVal00         = ReadInt32( oh, st, "_iVal00" );
         st._iVal01         = ReadInt32( oh, st, "_iVal01" );
         st._iVal02         = ReadInt32( oh, st, "_iVal02" );
         st._iVal03         = ReadInt32( oh, st, "_iVal03" );
         st._iVal04         = ReadInt32( oh, st, "_iVal04" );
         st._iVal05         = ReadInt32( oh, st, "_iVal05" );
         st._iVal06         = ReadInt32( oh, st, "_iVal06" );
         st._iVal07         = ReadInt32( oh, st, "_iVal07" );
         st._iVal08         = ReadInt32( oh, st, "_iVal08" );
         st._iVal09         = ReadInt32( oh, st, "_iVal09" );
         st._iVal10         = ReadInt32( oh, st, "_iVal10" );
         st._iVal11         = ReadInt32( oh, st, "_iVal11" );
         st._iVal12         = ReadInt32( oh, st, "_iVal12" );
         st._iVal13         = ReadInt32( oh, st, "_iVal13" );
         st._iVal14         = ReadInt32( oh, st, "_iVal14" );
         st._iVal15         = ReadInt32( oh, st, "_iVal15" );
         st._iVal16         = ReadInt32( oh, st, "_iVal16" );
         st._iVal17         = ReadInt32( oh, st, "_iVal17" );
         st._iVal18         = ReadInt32( oh, st, "_iVal18" );
         st._iVal19         = ReadInt32( oh, st, "_iVal19" );
         st._dVal00         = ReadDbl( oh, st, "_dVal00" );
         st._dVal01         = ReadDbl( oh, st, "_dVal01" );
         st._dVal02         = ReadDbl( oh, st, "_dVal02" );
         st._dVal03         = ReadDbl( oh, st, "_dVal03" );
         st._dVal04         = ReadDbl( oh, st, "_dVal04" );
         st._dVal05         = ReadDbl( oh, st, "_dVal05" );
         st._dVal06         = ReadDbl( oh, st, "_dVal06" );
         st._dVal07         = ReadDbl( oh, st, "_dVal07" );
         st._dVal08         = ReadDbl( oh, st, "_dVal08" );
         st._dVal09         = ReadDbl( oh, st, "_dVal09" );
         st._dVal10         = ReadDbl( oh, st, "_dVal10" );
         st._dVal11         = ReadDbl( oh, st, "_dVal11" );
         st._dVal12         = ReadDbl( oh, st, "_dVal12" );
         st._dVal13         = ReadDbl( oh, st, "_dVal13" );
         st._dVal14         = ReadDbl( oh, st, "_dVal14" );
         st._dVal15         = ReadDbl( oh, st, "_dVal15" );
         st._dVal16         = ReadDbl( oh, st, "_dVal16" );
         st._dVal17         = ReadDbl( oh, st, "_dVal17" );
         st._dVal18         = ReadDbl( oh, st, "_dVal18" );
         st._dVal19         = ReadDbl( oh, st, "_dVal19" );
         st._objname        = ReadString( oh, st, "_objname" );
         st._dstConn        = ReadString( oh, st, "_dstConn" );
         return st;
      }
      #endregion


      #region Marshalling Helpers
      ////////////////////////
      // Marshalling Helpers
      ////////////////////////
      private int ReadInt16( IntPtr oBase, Object o, string fieldName )
      {
         bool bOK;
         int  off;

         bOK = ( _base != IntPtr.Zero );
         off = _Offset32( oBase, o, fieldName );
         return bOK ? Marshal.ReadInt16( _base, off ) : 0;
      }

      private int ReadInt32( IntPtr oBase, Object o, string fieldName )
      {
         bool bOK;
         int  off;

         bOK = ( _base != IntPtr.Zero );
         off = _Offset32( oBase, o, fieldName );
         return bOK ? Marshal.ReadInt32( _base, off ) : 0;
      }

      private long ReadInt64( IntPtr oBase, Object o, string fieldName )
      {
         bool bOK;
         int  off;

         bOK = ( _base != IntPtr.Zero );
         off = _Offset32( oBase, o, fieldName );
         return bOK ? Marshal.ReadInt64( _base, off ) : 0;
      }

      private double ReadDbl( IntPtr oBase, Object o, string fieldName )
      {
         // Not really used; But what the hell ...

         return (double)ReadInt64( oBase, o, fieldName );
      }

      private string ReadString( IntPtr oBase, Object o, string fieldName )
      {
         IntPtr off, ptr;

         if ( _base != IntPtr.Zero ) {
            off = _Offset( oBase, o, fieldName );
            ptr = new IntPtr( off.ToInt64() + _base.ToInt64() );
            return Marshal.PtrToStringAnsi( ptr );
         }
         return "";
      }
      #endregion
   
      #region Operations
      ////////////////////////
      // Operations
      ////////////////////////
      public void Open()
      {
         if ( _hFile != IntPtr.Zero )
            return;
         _hFile = CreateFile( _file,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              IntPtr.Zero,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              IntPtr.Zero );
      }
   
      public void Close()
      {
         unmap();
         _hFile = _Close( _hFile );
      }

      public IntPtr map()
      {
         // Pre-condition

         if ( _hFile == IntPtr.Zero )
            return _hFile;
         unmap();
         _hMap = CreateFileMapping( _hFile,
                                    IntPtr.Zero,
                                    PAGE_READONLY,
                                    0,
                                    Size(),
                                    null );
         if ( _hMap != IntPtr.Zero ) {
            _base = MapViewOfFileEx( _hMap,
                                     FILE_MAP_READ,
                                     0,
                                     0,
                                     Size(),
                                     IntPtr.Zero );
         }
         return _base;
      }

      public void unmap()
      {
         _hMap = _Close( _hMap );
         _base = IntPtr.Zero;
      }
      #endregion

   
      #region Imports
      //////////////////////////////
      // Helpers
      //////////////////////////////
      private IntPtr _Close( IntPtr handle )
      {
         bool rtn;

         // Pre-condition

         if ( handle == IntPtr.Zero )
            return handle;

         // Safe to close

         rtn = CloseHandle( handle );
         return IntPtr.Zero;
      }

      public int _Offset32( IntPtr oBase, Object o, string fieldName )
      {
         return _Offset( oBase, o, fieldName ).ToInt32();
      }

      public IntPtr _Offset( IntPtr oBase, Object o, string fieldName )
      {
         IntPtr oFld;

         oFld = Marshal.OffsetOf( o.GetType(), fieldName );
         return new IntPtr( oBase.ToInt64() + oFld.ToInt64() );
      }
      #endregion

      #region Imports
      //////////////////////////////
      // .NET mmap childishness
      //////////////////////////////
      private const uint FILE_SHARE_READ       = 0x0001;
      private const uint FILE_SHARE_WRITE      = 0x0002;
      private const uint FILE_ATTRIBUTE_NORMAL = 0x0080;
      private const uint FILE_MAP_WRITE        = 0x0002;
      private const uint FILE_MAP_READ         = 0x0004;
      private const uint INVALID_HANDLE_VALUE  = 0xffffffff;
      private const uint GENERIC_READ          = 0x80000000;
      private const uint GENERIC_WRITE         = 0x40000000;
      private const uint CREATE_NEW            = 1;
      private const uint CREATE_ALWAYS         = 2;
      private const uint OPEN_EXISTING         = 3;
      private const uint PAGE_READONLY         = 0x02;

      [DllImport("kernel32.dll", SetLastError=true)]
      static extern IntPtr CreateFile( string lpFileName, 
                                       uint dwDesiredAccess,
                                       uint dwShareMode, 
                                       IntPtr lpSecurityAttributes, 
                                       uint dwCreationDisposition,
                                       uint dwFlagsAndAttributes, 
                                       IntPtr hTemplateFile );
      [DllImport("kernel32.dll", SetLastError=true)]
      static extern bool CloseHandle(IntPtr hObject);
      [DllImport("kernel32.dll", SetLastError=true)]
      static extern IntPtr CreateFileMapping ( IntPtr hFile,
                                               IntPtr lpFileMappingAttributes,
                                               uint  flProtect,
                                               uint dwMaximumSizeHigh,
                                               uint dwMaximumSizeLow,
                                               string lpName );
      [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
      static extern IntPtr MapViewOfFileEx( IntPtr hFileMappingObject,
                                            uint dwDesiredAccess, 
                                            uint dwFileOffsetHigh, 
                                            uint dwFileOffsetLow,
                                            uint dwNumberOfBytesToMap, 
                                            IntPtr lpBaseAddress );
      #endregion
   }


   ////////////////////////////////
   // Feed Monitor
   ////////////////////////////////
   class FeedMon
   {
      #region private Member variables
      //////////////////////////////
      // Private Members
      //////////////////////////////
      private string                 _file;
      private int                    _iTmr;
      private TimerCallback          _cbk;
      private System.Threading.Timer _tmr;
      private FeedStats              _stat;
      #endregion
   
      #region Constructor / Operations
      ////////////////////////
      // Constructor
      ////////////////////////
      public FeedMon( string file, double tSnap )
      {
         _file      = file;
         _iTmr      = Convert.ToInt32( 1000.0 * tSnap );
         _cbk       = new TimerCallback( Snap );
         _tmr       = null;
         _stat      = new FeedStats();
         _stat._hdr = new GLmdStatsHdr();
         _stat._sub = new GLmdChanStats();
         _stat._pub = new GLmdChanStats();
   
         // Rock and Roll

         if ( _tmr == null )
            _tmr = new System.Threading.Timer( _cbk, null, _iTmr, _iTmr );
         Console.WriteLine( _file );
      }
   
      public FeedStats GetStats()
      {
         FeedStats rtn;
   
         rtn = new FeedStats();
         lock( this ) {
            rtn = _stat;
         }
         return rtn;
      }
      #endregion
   
      #region Timer Handler
      //////////////////////////////
      // Timer Handler
      //////////////////////////////
      /*
       * .NET mmap() absolutely sucks
       */
      private void Snap( object data )
      {
         GLmmap        mm;
         GLmdStatsHdr  hdr;
         GLmdChanStats pub, sub;
         DateTime      tBeg, tPub, tSub;

         // Snap All Stats

         mm = new GLmmap( _file );
         mm.map();
         lock( this ) {
            _stat._hdr = mm.GetHdr();
            _stat._sub = mm.GetChanStats( false );
            _stat._pub = mm.GetChanStats( true );
         }
         mm.Dispose();

         // Dump some stats

         hdr = _stat._hdr;
         pub = _stat._pub;
         sub = _stat._sub;
tBeg = rtEdge.FromUnixTime( hdr._tStart_sec, hdr._tStart_micros );
tPub = rtEdge.FromUnixTime( pub._lastMsg_sec, pub._lastMsg_micros );
tSub = rtEdge.FromUnixTime( sub._lastMsg_sec, sub._lastMsg_micros );
Console.WriteLine( _stat._hdr._exe );
Console.WriteLine( _stat._hdr._build );
Console.WriteLine( "tStart : {0}", tBeg );
Console.WriteLine( "SUB: nUpd={0} @ {1}", sub._nUpd, tSub );
Console.WriteLine( "PUB: nUpd={0} @ {1}", pub._nUpd, tPub );
      }
      #endregion
   }
   
   
   ////////////////////////
   // FeedStats Publisher
   ////////////////////////
   class FeedMonPub : rtEdgePublisher
   {
      //////////////
      // Members
      //////////////
      private FeedMon _stats;
   
      ////////////////////////////////
      // Constructor
      ////////////////////////////////
      public FeedMonPub( FeedMon stats, string pSvr, string pPub ) :
         base( pSvr, pPub, true )
      {
         _stats = stats;
      }
   
   
      ////////////////////////////////
      // rtEdgePublisher Interface
      ////////////////////////////////
      public override void OnConnect( string pConn, rtEdgeState state )
      {
         bool   bUp = ( state == rtEdgeState.edg_up );
         string pUp = bUp ? "UP" : "DOWN";
   
         Console.WriteLine( "PUB-CONN {0} : {1}", pConn, pUp );
      }
   
      public override void OnOpen( string tkr, IntPtr arg )
      {
         Console.WriteLine( "OPEN {0}", tkr );
      }
   
      public override void OnClose( string tkr )
      {
         Console.WriteLine( "CLOSE {0}", tkr );
      }
   }
   
   
   ///////////////////////////////////
   // class FeedMain
   ///////////////////////////////////
   class FeedMain : ServiceBase
   {
      #region private Instance Members
      //////////////////////////////
      // Class-wide Members
      //////////////////////////////
      public static string _svc = "--service";
      public static string _us  = "FeedMon";
      public static string _ver = "FeedMon.cs Build 1";

      //////////////////////////////
      // Private Members
      //////////////////////////////
      private string     _file;
      private string     _pubHost;
      private string     _pubSvc;
      private double     _tSnap;
      private bool       _bSvc;
      private FeedMon    _mon;
      private FeedMonPub _pub;
      #endregion

      #region Constructor
      //////////////////////////////
      // Constructor
      //////////////////////////////
      public FeedMain( String[] args )
      {
         int i, argc;

         // Guts

         _file    = "./MDDirectMon.stats";
         _pubHost = "localhost:9995";
         _pubSvc  = "FeedMon";
         _tSnap   = 1.0;
         _bSvc    = false;
         _mon     = null;
         _pub     = null;

         // Config : [ <File> <Interval> <PubHost:port> <PubSvc> [ --service] ]

         argc = args.Length;
         Console.WriteLine( rtEdge.Version() );
         for ( i=0; i<argc; i++ ) {
            switch( i ) {
               case 0: _file    = args[i]; break;
               case 1: _tSnap   = Convert.ToDouble( args[i] ); break;
               case 2: _pubHost = args[i]; break;
               case 3: _pubSvc  = args[i]; break;
               case 4: _bSvc    = args[i] == _svc; break;
            }
        }

         // ServiceBase Requirements

         ServiceName         = FeedMain._us;
         EventLog.Log        = "Application";
         CanHandlePowerEvent = true;
         CanPauseAndContinue = true;
         CanShutdown         = true;
         CanStop             = true;
      }
      #endregion
  
      #region ServiceBase Interface
      //////////////////////////////
      // ServiceBase Interface
      //////////////////////////////
      protected override void OnStart( string[] args )
      {
         // Stats; Publication Channel

         try {
           _mon = new FeedMon( _file, _tSnap );
           _pub = new FeedMonPub( _mon, _pubHost, _pubSvc );

           // Do it

           Console.WriteLine( "PUB : {0}", _pub.pConn() );
           Console.WriteLine( "Hit <ENTER> to terminate..." );
         }
         catch( Exception e ) {
           Console.WriteLine( "Exception: " + e.Message );
         }
      }

      protected override void OnShutdown()
      {
         OnStop();
      }

      protected override void OnStop()
      {
         // Shutdown - Once

         if ( _mon != null ) {
//            GLlogger.LOG( "Shutting down ..." );
            _mon = null;
            _pub = null;
//            GLlogger.LOG( "Done!!" );
         }
      }
      #endregion

      //////////////////////////////
      // main()
      //////////////////////////////
      static void Main( string[] args )
      {
         FeedMain app;
         int      argc;
         bool     bSvc;

         // [ <File> <Interval> <PubHost:port> <PubSvc> [ -service] ]

         argc = args.Length;
         bSvc = ( argc < 5 ) ? false : args[5] == _svc;
         if ( bSvc )
            ServiceBase.Run( new FeedMain( args ) );
         else {
            app = new FeedMain( args );
            app.OnStart( args );
            Console.ReadLine();
            app.OnStop();
         }
      }
   }
}
