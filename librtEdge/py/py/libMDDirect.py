## \cond
#################################################################
#
#  libMDDirect.py
#     MDDirect add-in driver
#
#  REVISION HISTORY:
#      3 APR 2019 jcs  Created.
#     18 NOV 2020 jcs  rtEdgeSchema
#      1 DEC 2020 jcs  Renamed to libMDDirect
#     20 JAN 2022 jcs  doxygen
#     26 JAN 2022 jcs  Bug fixes from ZB
#      3 FEB 2022 jcs  MDDirect.LVCSnap() : Returns tUpd
#     16 FEB 2022 jcs  Dump( bFldTy=false ); rtEdgeField.TypeName()
#     16 MAR 2022 jcs  _MDDPY_INT64
#     24 MAY 2022 jcs  _MDDPY_UNXTM
#     11 JUL 2022 jcs  rtEdgeData._tDead; LVC.IsDead()
#     15 JUL 2022 jcs  LVC.IsDead() if !_tUpd
#     19 JUL 2022 jcs  LVCAdmin
#     14 AUG 2023 jcs  NONE
#     21 AUG 2023 jcs  MDDirectException; SnapAll()
#      4 SEP 2023 jcs  EVT_BDS; DelTickers()
#     20 SEP 2023 jcs  BBDailyStats
#     27 SEP 2023 jcs  Tape debug
#     16 OCT 2023 jcs  MemFree()
#      6 DEC 2023 jcs  GetField( bDeepCopy )
#     16 JAN 2024 jcs  rtEdgeData.UserArg()
#     28 MAR 2024 jcs  MDDirect311
#     30 APR 2024 jcs  LVC _SetData() arg mismatch
#      6 JAN 2025 jcs  list( keys() )
#     21 JAN 2025 jcs  Build 13: EdgMon
#     29 JAN 2025 jcs  Build 13: LVCAdmin : schema=None means no argument
#      5 FEB 2025 jcs  Build 14: __del__; LVCAdmin.OnConnect()
#
#  (c) 1994-2025, Gatea Ltd.
#################################################################
import gc, math, os, sys, time, threading
import mmap, struct

_UNDEF = 'Undefined'

def Log( msg ):
   sys.stdout.write( msg + '\n' )
   sys.stdout.flush()

## Newest to oldest

try:
   import MDDirect311 as MDDirect
except:
   try: 
      import MDDirect39 as MDDirect
   except: 
      try: 
         import MDDirect36 as MDDirect
      except: 
         try: 
            import MDDirect27 as MDDirect
         except: 
            Log( 'MDDirect not found; Exitting ...' )
            sys.exit()
pass

## \endcond

## @mainpage
#
# ### librtEdge C Extension in Python
#
# The librtEdge library is a C/C++ library for accessing services from the 
# Gatea MDDirect platform, including:
# + Streaming data from rtEdgeCache3 data distributor
# + Snapped data from the Last Value Cache (LVC)
# 
# The library is wrapped as a C extension to Python depending on the 
# version of Python linked via the following:
#
# Python Ver | Win64 | Linux64
# --- | --- | ---
# 2.7 | MDDirect27.pyd | MDDirect27.so
# 3.6 | MDDirect36.pyd | MDDirect36.so
# 3.9 | MDDirect39.pyd | MDDirect39.so
# 3.11 | MDDirect311.pyd | MDDirect311.so
# 
# The libMDDirect.py module 'hunts' for and transparently loades the proper 
# C extension library based on the version of Python you are running. 
#
# 
# ### MDDirect Services in Python
#
# The MDDirect C extension is packaged with the libMDDirect.py file, which 
# contain helpful classes for accessing MDDirect services including:
# 
# class | Description 
# --- | --- 
# rtEdgeSubscriber | Subscription channel from rtEdgeCache3 or tape
# LVC | Snapshot data from Last Value Cache (LVC)
# LVCAdmin | LVC Admin Channel : Add, Refresh Ticker(s)
# rtEdgeData | Snapped or streaming data from rtEdgeSubscriber or LVC
# rtEdgeField | A single field from rtEdgeData
# 
# The objective is to have the same 'look and feel' as the MDDirect .NET API. 
#
# ### Compatibility with Different Python Interpreter Versions
#
# This libMDDirect.py module implements the import code shown above to load 
# the correct MDDirectxy module based on the version of the Python interpreter 
# you are using.  As such, your code written exclusively to libbMDDirect.py 
# will remain portable across Python 2.7 and Python 3.9 interpreters.
#

##########################
# Return True if Python 3.x
#
# @return True if Python 3.x
##########################
def IsPY3():
   return ( sys.version_info.major >= 3 )

#################################
# Returns version and build info
#
# @return version and build info
#################################
def Version():
   return MDDirect.Version()

#################################
# Returns CPU usage of this process
#
# @return CPU usage of this process
#################################
def CPU():
   return MDDirect.CPU()

#################################
# Returns process memory size in KB
#
# @return process memory size in KB
#################################
def MemSizeKb():
   return MDDirect.MemSize()

#################################
# Clears internal Python FreeLists - int, float, etc.
#
# @return Number of blocks freed
#################################
def MemFree():
   return MDDirect.MemFree()

#################################
# Returns number of Python objects
#
# Useful for checking object leaks, if any
#
# @return Number of Python objects
#################################
def NumPyObjects():
   return len( gc.get_objects() )

#################################
# Returns True if running on Windows
#
# @return True if running on Windows
#################################
def IsWindows():
   return ( os.name == 'nt' )


## \cond
######################################
#                                    #
#           E d g M o n              #
#                                    #
######################################
## \endcond
## @class EdgMon
#
# Stripped-down MDD.MONITOR.EDGE.exe to sample object counts
#
# Member | Description
# --- | ---
# _name | Stats Filename
# _off | Offset in stats file to count metrics
# _fp | open()
# _Size | File Size
# _map | mmap'ed view of the file
# _tStart | Start Time in UnixTime from stats file
# _GLedgRequest | Object count : GLedgRequest
# _GLedgRecord | Object count : GLedgRecord
# _GLedgField | Object count : GLedgField
# _GLedgEvent | Object count : GLedgEvent
#
class EdgMon:
   ###########################
   # Constructor
   #
   # @param filename - stats file
   ###########################
   def __init__( self, filename ):
      self._name         = filename
      self._off          = 2216
      self._inc          = 8     ## sizeof( long ) == 8
      if IsWindows():
         self._off       = 2176  ## Not pack'ed
         self._inc       = 4     ## sizeof( long ) == 4
      self._fp           = None
      self._Size         = None
      self._map          = None
      self._tStart       = 0
      self._GLedgRequest = 0
      self._GLedgRecord  = 0
      self._GLedgField   = 0
      self._GLedgEvent   = 0
      try:    self._fp = open( filename, 'rb' ) 
      except: return
      try:    self._Size = os.stat( filename ).st_size
      except: return
      try:    self._map = mmap.mmap( fileno = self._fp.fileno(),
                                     length = self._Size,
                                     access = mmap.ACCESS_READ )
      except: pass

   ###########################
   # Snap current Values
   ###########################
   def Snap( self ):
      off = self._off
      inc = self._inc
      sz  = struct.calcsize( 'i' )
      m   = self._map
      if m:
         self._tStart       = struct.unpack( 'i', m[8:8+sz] )[0]
         self._GLedgRequest = struct.unpack( 'i', m[off:off+sz] )[0]
         off               += inc
         self._GLedgRecord  = struct.unpack( 'i', m[off:off+sz] )[0]
         off               += inc
         self._GLedgField   = struct.unpack( 'i', m[off:off+sz] )[0]
         off               += inc
         off               += inc
         off               += inc
         self._GLedgEvent   = struct.unpack( 'i', m[off:off+sz] )[0]
      return

   ###########################
   # Snap and Dump to stdout
   #
   # @param bCSV : True to dump as CSV
   ###########################
   def SnapAndDump( self, bCSV=False ):
      self.Snap()
      if bCSV:
         vdb = [ str( self._tStart ),
                 str( self._GLedgRequest ),
                 str( self._GLedgRecord ),
                 str( self._GLedgField ),
                 str( self._GLedgEvent )
               ]
         rc = ','.join( vdb )
      else:
         vdb = [ 'tStart       = %d' % self._tStart,
                 'GLedgRequest = %d' % self._GLedgRequest,
                 'GLedgRecord  = %d' % self._GLedgRecord,
                 'GLedgField   = %d' % self._GLedgField,
                 'GLedgEvent   = %d' % self._GLedgEvent
               ]
         rc = '\n'.join( vdb )
      return rc

   ###########################
   # Close File
   ###########################
   def Close( self ):
      fp  = self._fp
      map = self._map
      if map: map.close()
      if fp:  fp.close()
      self._fp           = None
      self._Size         = None
      self._map          = None
      self._tStart       = 0
      self._GLedgRequest = 0
      self._GLedgRecord  = 0
      self._GLedgField   = 0
      self._GLedgEvent   = 0


## \cond
######################################
#                                    #
# M D D i r e c t E x c e p t i o n  #
#                                    #
######################################
## \endcond
## @class MDDirectException
#
# Abstract base Exception class for all MDDirect Exceptions raised here
#
class MDDirectException( Exception ):
   ###########################
   # Constructor
   #
   # @param value - Exception Value
   ###########################
   def __init__( self, value ):
      Exception.__init__( self )
      self.value = value

## \cond
   def __str__(self):
      return repr(self.value)
## \endcond


## \cond
######################################
#                                    #
# M D D i r e c t V a l u e E x c . .#
#                                    #
######################################
## \endcond
## @class MDDirectValueException
#
# Bad MDDirect value exception
#
class MDDirectValueException( MDDirectException ):
   ###########################
   # Constructor
   #
   # @param value - Exception Value
   ###########################
   def __init__( self, value ):
      MDDirectException.__init__( self, 'Invalid value %s' % str( value ) )


## \cond
######################################
#                                    #
#   r t E d g e S u b s c r i b e r  #
#                                    #
######################################
## \endcond
## @class rtEdgeSubscriber
#
# Subscription channel from an MD-Direct data source - rtEdgeCache3 or 
# Tape File.
#
# The 1st argument to Start() defines your data source as follows:
#
# 1st Arg    | Data Source   | Data Type
# ---------- | ------------- | ----------------------
# host:port  | rtEdgeCache3  | Streaming real-time data
# filename   | Tape File     | Recorded market data from tape
#
# This class ensures that data from both sources - rtEdgeCache3 and Tape 
# File - is streamed into your application in the exact same manner using 
# the following API calls:
# + Subscribe()
# + OnData()
# + OnDead()
# + Unsubscribe()
#
# The Tape File data source is specifically driven from this class as follows:
#
# API              | Action
# ---------------- | -------------------------
# StartTape()      | Pump data for Subscribe()'ed tkrs until end of file
# StartTapeSlice() | Pump data for Subscribe()'ed tkrs in a time interval
# StopTape()       | Stop Tape Pump
#
class rtEdgeSubscriber( threading.Thread ):
   ########################
   # Constructor
   ########################
   def __init__( self ):
      threading.Thread.__init__( self )
      self._run    = True
      self._tid    = None
      self._cxt    = None
      self._schema = None
      self._bThrMD = False
      self._msg    = rtEdgeData()
      self._ready  = threading.Event()

   ########################
   # Destructor : Called at garbage collection time 
   # 
   # @see Stop()
   ########################
   def __del__( self ):
      self.Stop()

   ########################
   # Returns Version and Build info
   #
   # @return Version and Build info
   ########################
   def Version( self ):
      return MDDirect.Version()

   ########################
   # Returns True if dispatching from this thread
   #
   # @return True if dispatching from this thread
   ########################
   def IsOurThread( self ):
      return self._bThrMD

   ########################
   # Connect and Start session to MD-Direct rtEdgeCache3 server
   #
   # @param svr - host:port of rtEdgeCache3 server to connect to
   # @param usr - rtEdgeCache3 username
   # @param bBin - True for binary protocol
   ########################
   def Start( self, svr, usr, bBin ):
      if not self._cxt:
         self._cxt = MDDirect.Start( svr, usr, bBin )
      self.start()
      self._ready.wait()

   ########################
   # Stop session and disconnect from MD-Direct rtEdgeCache3 server 
   ########################
   def Stop( self ):
      self._run = False
      if self._tid:
         self.join()
      self._tid = None
      if self._cxt:
         MDDirect.Stop( self._cxt )
      self._cxt = None

   ########################
   # Returns True if fed from tape; False if from rtEdgeCache3
   #
   # @return True if fed from tape; False if from rtEdgeCache3
   ########################
   def IsTape( self ):
      return MDDirect.IsTape( self._cxt )

   ########################
   # Sets tape direction based on bTapeDir
   #
   # The tape is a linked-list of updates in REVERSE order.  As such, 
   # the tape direction is reverse as follows:
   # bTapeDir | Chronological Order??
   # --- | ---
   # True | NO
   # False | YES
   #
   # @param bTapeDir - False to pump in chronological order
   # @return False if chronological order; True if tape order
   ########################
   def SetTapeDirection( self, bTapeDir ):
      if bTapeDir: iDir = 1
      else:        iDir = 0
      return MDDirect.SetTapeDir( self._cxt, iDir )

   ########################
   # Snaps full tape or slice of tape for a single ( Service, Ticker ) 
   # stream.  If slice, the Tape Slice Start / End times is formatted 
   # as [YYYY-MM-DD] HH:MM[:SS.mmm]
   #
   # @param svc : Service Name
   # @param tkr : Ticker Name
   # @param flds : CSV list of field ID's or names
   # @param maxRow : Max number rows to return
   # @param tmout : Timeout in secs
   # @param t0 : Tape Slice Start; None (default) for all ticks
   # @param t1 : Tape Slice End; None (default) for all ticks
   # @param tSample : Sample time in seconds; 0 (default) for all ticks
   # @return [ [ ColHdr1, ColHdr2, ... ], [ row1 ], [ row2 ], ... ]
   ########################
   def SnapTape( self, 
                 svc, 
                 tkr, 
                 flds, 
                 maxRow, 
                 tmout   = 2.5, 
                 t0      = None, 
                 t1      = None,
                 tSample = 0 ):
      cxt = self._cxt
      if t0 and t1:
         return MDDirect.SnapTape( cxt, 
                                   svc, 
                                   tkr, 
                                   flds, 
                                   maxRow, 
                                   tmout, 
                                   t0, 
                                   t1, 
                                   tSample )
      return MDDirect.SnapTape( cxt, svc, tkr, flds, maxRow, tmout )

   ########################
   # Query for list of all tickers on the tape
   #
   # @return [ [ Svc1, Tkr1, NumMsg1 ], [ Svc2, Tkr2, NumMsg2 ], ... ]
   ########################
   def QueryTape( self ):
      return MDDirect.QueryTape( self._cxt )

   ########################
   # Pumps full tape or slice of tape
   #
   # For tape slice, t0 and t1 formatted as [YYYY-MM-DD] HH:MM[:SS.mmm]
   #
   # @param t0 - Tape Slice Start; None (default) for start of tape
   # @param t1 - Tape Slice End; None (default) for end of tape
   ########################
   def PumpTape( self, t0=None, t1=None ):
      if type( t0 ) != type( 'abc' ): t0 = '00:00:00'
      if type( t1 ) != type( 'abc' ): t1 = '23:59:59'
      return MDDirect.PumpTape( self._cxt, t0, t1 )

   ########################
   # Open a subscription stream for the ( svc, tkr ) data stream.  
   #
   # Market data updates are returned in the OnData() asynchronous call.
   #
   # @param svc - Service Name (e.g., bloomberg)
   # @param tkr - Ticker Name (e.g., AAPL US EQUITY)
   # @param arg - Optional user data returned in rtEdgeData.UserArg()
   # @return Unique non-zero stream ID on success
   # 
   # @see Unsubscribe()
   # @see OnData()
   # @see rtEdgeData.UserArg()
   ########################
   def Subscribe( self, svc, tkr, arg ):
      return MDDirect.Open( self._cxt, svc, tkr, arg )

   ########################
   # Closes a subscription stream for the ( svc, tkr ) data stream that was 
   # opened via Subscribe().  Market data updates are stopped.
   #
   # @param svc - Service Name (e.g., bloomberg)
   # @param tkr - Ticker Name (e.g., AAPL US EQUITY)
   #
   # @see Subscribe()
   ########################
   def Unsubscribe( self, svc, tkr ):
      return MDDirect.Close( self._cxt, svc, tkr )

   ########################
   # Open a subscription stream to a Broadcast Data Stream (BDS) from a service.
   #
   # Market data updates are returned in the OnSymbol() asynchronous call.
   #
   # @param svc - Service Name (e.g., factset)
   # @param bds - BDS Name (e.g., NYSE)
   # @param arg - Optional user data returned in rtEdgeData.UserArg()
   # @return Unique non-zero stream ID on success
   # 
   # @see CloseBDS()
   # @see OnSymbol()
   ########################
   def OpenBDS( self, svc, bds, arg ):
      return MDDirect.OpenBDS( self._cxt, svc, bds, arg )

   ########################
   # Closes a BDS stream that we opened via OpenBDS().  Market data 
   # updates are stopped.
   #
   # @param svc - Service Name (e.g., factset)
   # @param bds - BDS Name (e.g., NYSE)
   #
   # @see OpenBDS()
   ########################
   def CloseBDS( self, svc, bds ):
      return MDDirect.CloseBDS( self._cxt, svc, bds )

   ########################
   # Called asynchronously when we connect or disconnect from rtEdgeCache3.
   #
   # Override this method to take action when you connect or disconnect 
   # from rtEdgeCache3.
   #
   # @param msg - Textual description of connect event
   # @param bUP - True if UP; False if DOWN
   ########################
   def OnConnect( self, msg, bUP ):
      pass

   ########################
   # Called asynchronously when real-time publisher changes state (goes UP 
   # or DOWN) in the rtEdgeCache3.
   #
   # Override this method in your application to take action when a new 
   # publisher goes online or offline. The library transparently re-subscribes 
   # to any and all streams you have Subscribe()'ed to when the service comes 
   # back UP.  from rtEdgeCache3.
   #
   # @param svc - Service name (e.g., bloomberg)
   # @param bUP - True if UP; False if DOWN
   #
   # @see Subscribe()
   ########################
   def OnService( self, svc, bUP ):
      pass

   ########################
   # Called asynchronously when market data arrives from either the 
   # rtEdgeCache3 server (real-time) or tape (recorded).
   #
   # Override this method in your application to consume market data.
   #
   # @param mddMsg - rtEdgeData
   #
   # @see rtEdgeData
   ########################
   def OnData( self, mddMsg ):
      pass

   ########################
   # Called asynchronously when the real-time stream opened via Subscribe()
   # becomes DEAD
   #
   # Override this method in your application to handle market data stream
   # becoming DEAD.
   #
   # @param mddMsg - rtEdgeData
   # @param msg - Error message
   #
   # @see rtEdgeData
   ########################
   def OnDead( self, mddMsg, msg ):
      pass

   ########################
   # Called asynchronously when the real-time stream opened is recovering
   #
   # Override this method in your application to process when a market data
   # stream is recovering.
   #
   # @param mddMsg - rtEdgeData
   # @param sts - Status message
   #
   # @see rtEdgeData
   ########################
   def OnRecovering( self, mddMsg, sts ):
      pass

   ########################
   # Called asynchronously when the data stream from the tape is complete
   #
   # Override this method in your application to process when the data pumped 
   # from tape is complete.
   #
   # @param sts - Status message
   ########################
   def OnStreamDone( self, sts ):
      pass

   ########################
   # Called asynchronously when the data stream from the tape is complete
   #
   # Override this method in your application to process when the data pumped 
   # from tape is complete.
   #
   # @param tkr - Ticker Name
   # @see OpenBDS()
   ########################
   def OnSymbol( self, tkr ):
      pass

   ########################
   # Called asynchronously when the data dictionary for this subscription 
   # channel arrives from rtEdgeCache3
   #
   # Override this method in your application to process the schema
   # from tape is complete.
   #
   # @param schema - Data Dictionary as rtEdgeSchema
   #
   # @see rtEdgeSchema
   ########################
   def OnSchema( self, schema ):
      pass

## \cond
   #################################
   # (private) threading.Thread Interface
   #
   # MDDirect.Read() returns as follows:
   #    EVT_UPD    : [ tUpd, oid, Svc, Tkr, nAgg, fld1, fld2, ... ]
   #                    where fldN = [ fidN, valN, tyN ]
   #    EVT_BYSTR  : [ tUpd, oid, Svc, Tkr, bytestream ]  
   #    EVT_STS    : [ tUpd, oid, Svc, Tkr, sts ]
   #    EVT_RECOV  : [ tUpd, oid, Svc, Tkr, sts ]
   #    EVT_DONE   : [ tUpd, oid, Svc, Tkr, sts ]
   #    EVT_CONN   : msg
   #    EVT_SVC    : msg
   #    EVT_SCHEMA : [ fld1, fld2, ... ]
   #                    where fldN = [ fidN, valN, tyN ]
   #################################
   def run( self ):
      self._tid = threading.currentThread().ident
      self._ready.set()
      #
      # Drain until stopped
      #
      msg = self._msg
      while self._run:
         self._bThrMD = False
         rd = MDDirect.Read( self._cxt, 0.25 )
         self._bThrMD = True
         if not rd:
            continue ## while-run
         ( mt, blob ) = rd
         if mt == MDDirectEnum.EVT_UPD:
            msg._tUpd = blob[0]
            uoid      = blob[1]
            svc       = blob[2]
            tkr       = blob[3]
            sts       = ''
            nAgg      = blob[4]
            flds      = blob[5:]
            self.OnData( msg._SetData( svc, tkr, uoid, flds ) )
         elif mt == MDDirectEnum.EVT_BYSTR:
            msg._tUpd = blob[0]
            uoid      = blob[1]
            svc       = blob[2]
            tkr       = blob[3]
            sts       = ''
            data      = blob[4]
            dLen      = len( blob[4] )
            Log( 'OnByteStream( %s,%s )' % ( svc, tkr ) )
         elif mt == MDDirectEnum.EVT_STS:
            msg._tUpd = blob[0]
            uoid      = blob[1]
            svc       = blob[2]
            tkr       = blob[3]
            sts       = blob[4]
            self.OnDead( msg._SetError( svc, tkr, uoid, sts ), sts )
         elif mt == MDDirectEnum.EVT_RECOV:
            msg._tUpd = blob[0]
            uoid      = blob[1]
            svc       = blob[2]
            tkr       = blob[3]
            sts       = blob[4]
            self.OnRecovering( msg._SetData( svc, tkr, uoid, [] ), sts )
         elif mt == MDDirectEnum.EVT_DONE:
            tUpd = blob[0]
            uoid = blob[1]
            svc  = blob[2]
            tkr  = blob[3]
            sts  = blob[4]
            self.OnStreamDone( sts )
         elif mt == MDDirectEnum.EVT_BDS:
            tkr  = blob
            self.OnSymbol( tkr )
         elif mt == MDDirectEnum.EVT_SVC:
            kv  = blob.split('|')
            bUP = ( kv[0].strip() == 'UP' )
            self.OnService( kv[1], bUP )
         elif mt == MDDirectEnum.EVT_CONN:
            ( ty, pMsg ) = blob.split('|')
            bUP          = ( ty.strip() == 'UP' )
            self.OnConnect( pMsg, bUP )
         elif mt == MDDirectEnum.EVT_SCHEMA:
            self._schema = rtEdgeSchema( blob )
            self.OnSchema( self._schema )
            self._msg._schema = self._schema
      return
## \endcond



## \cond
######################################
#                                    #
#             L V C                  #
#                                    #
######################################
## \endcond
## @class LVC
#
# Read-only view on Last Value Cache (LVC) file
#
# Member | Description
# --- | ---
# _cxt    | LVC Context from MDDirect library
# _schema | rtEdgeSchema
#
class LVC:
   ########################
   # Constructor
   ########################
   def __init__( self ):
      self._cxt    = None
      self._schema = None

   ########################
   # Destructor : Called at garbage collection time 
   # 
   # @see Close()
   ########################
   def __del__( self ):
      self.Close()

   ########################
   # Returns Field ID from Field Name in Schema
   #
   # @param name : Field Name
   # @return Field ID from Field Name in Schema
   # @see rtEdgeSchema.GetFieldID()
   ########################
   def GetFieldID( self, name ):
      ddb = self._schema
      if ddb:
         return ddb.GetFieldID( name )
      return 0

   ########################
   # Returns Field Name from ID in Schema
   #
   # @param fid : Field FID
   # @return Field Name from ID in Schema
   # @see rtEdgeSchema.GetFieldName()
   ########################
   def GetFieldName( self, fid ):
      ddb = self._schema
      if ddb:
         return ddb.GetFieldName( fid )
      return 0

   ########################
   # Open read-only LVC file
   #
   # @param file : LVC filename
   ########################
   def Open( self, file ):
      ddb = self._schema
      if not self._cxt:
         self._cxt    = MDDirect.LVCOpen( file )
         self._schema = rtEdgeSchema( MDDirect.LVCSchema( self._cxt ) )
      return

   ########################
   # Return list of [ Svc, Tkr ] tuples in the LVC
   #
   # @return [ [ Svc1, Tkr1 ], [ Svc2, Tkr2 ], ... ]
   ########################
   def GetTickers( self ):
      return MDDirect.LVCGetTickers( self._cxt )

   ########################
   # Snap LVC data for ( svc, tkr )
   #
   # @param svc : Service Name
   # @param tkr : Ticker Name
   # @return rtEdgeData populated with data if found; None if not found
   #
   # @see rtEdgeData
   ########################
   def Snap( self, svc, tkr ):
      blob = MDDirect.LVCSnap( self._cxt, svc, tkr )
      if blob:
         return self._Build_rtEdgeData( blob )
      return None

   ########################
   # Snap ALL LVC data
   #
   # @return [ rtEdgeData1, rtEdgeData2, ... ]
   ########################
   def SnapAll( self ):
      #
      # [ blob1, blob2, ... ]
      #
      blobDB = MDDirect.LVCSnapAll( self._cxt )
      if not blobDB:
         return None
      return [ self._Build_rtEdgeData( blob ) for blob in blobDB ]

   ########################
   # Close read-only LVC file
   #
   # @see Open()
   ########################
   def Close( self ):
      if self._cxt:
         MDDirect.LVCClose( self._cxt )
      self._cxt = None

## \cond
   ########################
   # Marshall PyList blob from MDDirect pyd into rtEdgeData 
   #
   # @param blob : [ tUpd, tDead, Svc, Tkr, fld1, fld2, ... ]
   # @return rtEdgeData populated with data
   ########################
   def _Build_rtEdgeData( self, blob ):
      #
      # blob = [ tUpd, tDead, Svc, Tkr, fld1, fld2, ... ]
      #          where fldN = [ fidN, valN, tyN ]
      #
      msg        = rtEdgeData( self._schema )
      msg._tUpd  = blob[0]
      msg._tDead = blob[1]
      svc        = blob[2]
      tkr        = blob[3]
      flds       = blob[4:]
      return msg._SetData( svc, tkr, None, flds, True )
## \endcond


## \cond
######################################
#                                    #
#         L V C A d m i n            #
#                                    #
######################################
## \endcond
## @class LVCAdmin
#
# Control channel to LVC : Add, Refresh Tickers
#
# You get notified of channel events via:
# + OnConnect()
# + OnAdminACK()
# + OnAdminNAK()
#
# Member | Description
# --- | ---
# _cxt    | LVCAdmin Context from MDDirect library
#
class LVCAdmin( threading.Thread ):
   ########################
   # Constructor : Create and Connect to LVCAdmin channel
   #
   # @param conn : host:port of LVC Admin Channel
   ########################
   def __init__( self, conn='localhost:8775' ):
      threading.Thread.__init__( self )
      self._cxt   = MDDirect.LVCAdminOpen( conn )
      self._run   = True
      self._tid   = None
      self._ready = threading.Event()
      self.start()
      self._ready.wait()

   ########################
   # Destructor : Called at garbage collection time
   #
   # @see Close()
   ########################
   def __del__( self ):
      self.Close()

   ########################
   # Add Broadcast Data Stream (BDS) to LVC
   #
   # A Broadcast Data Stream (BDS) is an updating stream of tickers from
   # a publisher.  When an LVC is seeded by the BDS, it subscribes to the
   # updating stream of tickers (e.g., List of NYSE exchange tickers), then
   # subscribes to market data from each individual ticker.
   #
   # In this manner, the LVC only needs to know a single name - i.e., the
   # name of the BDS - rather than the names of all tickers.
   #
   # @param svc : Service Name
   # @param bds : BDS Name from svc
   ########################
   def AddBDS( self, svc, bds ):
      MDDirect.LVCAdminAddBDS( self._cxt, svc, bds )

   ########################
   # Add single ticker to LVC
   #
   # @param svc : Service Name
   # @param tkr : Ticker name
   # @param schema : (Optional) Schema Name; Default is None
   ########################
   def AddTicker( self, svc, tkr, schema=None ):
      if schema: MDDirect.LVCAdminAddTicker( self._cxt, svc, tkr, schema )
      else:      MDDirect.LVCAdminAddTicker( self._cxt, svc, tkr )

   ########################
   # Add ticker list to LVC for specific service
   #
   # @param svc : Service Name
   # @param tkrs : [ Ticker1, Ticker2, ... ]
   # @param schema : (Optional) Schema Name; Default is None
   ########################
   def AddTickers( self, svc, tkrs, schema=None ):
      if schema: MDDirect.LVCAdminAddTickers( self._cxt, svc, tkrs, schema )
      else:      MDDirect.LVCAdminAddTickers( self._cxt, svc, tkrs )

   ########################
   # Delete single ticker to LVC
   #
   # @param svc : Service Name
   # @param tkr : Ticker name
   # @param schema : (Optional) Schema Name; Default is None
   ########################
   def DelTicker( self, svc, tkr, schema=None ):
      if schema: MDDirect.LVCAdminDelTicker( self._cxt, svc, tkr, schema )
      else:      MDDirect.LVCAdminDelTicker( self._cxt, svc, tkr )

   ########################
   # Delete ticker list to LVC for specific service
   #
   # @param svc : Service Name
   # @param tkrs : [ Ticker1, Ticker2, ... ]
   # @param schema : (Optional) Schema Name; Default is None
   ########################
   def DelTickers( self, svc, tkrs, schema=None ):
      if schema: MDDirect.LVCAdminDelTickers( self._cxt, svc, tkrs, schema )
      else:      MDDirect.LVCAdminDelTickers( self._cxt, svc, tkrs )

   ########################
   # Refresh single ticker to LVC
   #
   # @param svc : Service Name
   # @param tkr : Ticker name
   ########################
   def RefreshTicker( self, svc, tkr ):
      MDDirect.LVCAdminRefreshTicker( self._cxt, svc, tkr )

   ########################
   # Refresh ticker list to LVC for specific service
   #
   # @param svc : Service Name
   # @param tkrs : [ Ticker1, Ticker2, ... ]
   ########################
   def RefreshTickers( self, svc, tkrs ):
      MDDirect.LVCAdminRefreshTickers( self._cxt, svc, tkrs )

   ########################
   # Close LVCAdmin channel
   ########################
   def Close( self ):
      self._run = False
      if self._tid:
         self.join()
      self._tid = None
      if self._cxt:
         MDDirect.LVCAdminClose( self._cxt )
      self._cxt = None

   ########################
   # Called asynchronously when we connect or disconnect from LVCAdmin.
   #
   # Override this method to take action when you connect or disconnect
   # from LVCAdmin.
   #
   # @param msg - Textual description of connect event
   # @param bUP - True if UP; False if DOWN
   ########################
   def OnConnect( self, msg, bUP ):
      pass

   ########################
   # Called asynchronously when an NAK message arrives
   #
   # Override this method to take action on Admin ACK
   #
   # @param bAdd - true if ADD; false if DEL
   # @param svc - Service Name
   # @param tkr - Ticker Name
   ########################
   def OnAdminACK( self, bAdd, svc, tkr ):
      pass

   ########################
   # Called asynchronously when an NAK message arrives
   #
   # Override this method to take action on Admin ACK
   #
   # @param bAdd - true if ADD; false if DEL
   # @param svc - Service Name
   # @param tkr - Ticker Name
   ########################
   def OnAdminACK( self, bAdd, svc, tkr ):
      pass

## \cond
   #################################
   # (private) threading.Thread Interface
   #
   # MDDirect.Read() returns as follows:
   #    EVT_CONN   : UP|Message or 
   #                 DOWN|Mesasge
   #    EVT_SVC    : ACK|ADD|service|ticker or
   #                 NAK|ADD|service|ticker or
   #                 ACK|DEL|service|ticker or
   #                 NAK|DEL|service|ticker or
   #################################
   def run( self ):
      self._tid = threading.currentThread().ident
      self._ready.set()
      #
      # Drain until stopped
      #
      while self._run:
         rd = MDDirect.LVCAdminRead( self._cxt, 0.25 )
         if not rd:
            continue ## while
         ( mt, blob ) = rd
         if mt == MDDirectEnum.EVT_CONN:
            ( ty, pMsg ) = blob.split('|')
            bUP          = ( ty.strip() == 'UP' )
            self.OnConnect( pMsg, bUP )
         elif mt == MDDirectEnum.EVT_SVC:
            kv  = blob.split('|')
            try:    ack = ( kv[0] == 'ACK' )
            except: ack = False
            try:    add = ( kv[1] == 'ADD' )
            except: add = False
            try:    svc = kv[2]
            except: svc = 'undefined'
            try:    tkr = kv[3]
            except: tkr = 'undefined'
            if ack: self.OnAdminACK( add, svc, tkr )
            else:   self.OnAdminNAK( add, svc, tkr )
      return
## \endcond

## \cond
######################################
#                                    #
#      B B D a i l y S t a t s       #
#                                    #
######################################
## \endcond
## @class BBDailyStats
#
# Sampled BB Daily Stats from run-time stats file
#
# The following are supported:
# -# BBSubscribe
# -# BBUnSubscribe
# -# BBSubscribeElem
# -# BBRequest
# -# BBRequestElem
# -# BBRealTime
# -# BBRealTimeElem
# -# BBRefResponse
# -# BBRefResponseElem
#
# Member | Description
# --- | ---
# _stats | { 'BBStat1' : val1, 'BBStat2' : val2, ... }
#
class BBDailyStats:
   ########################
   # Constructor : Read BBDailyStats from run-time stats file
   #
   # @param statFile : host:port of LVC Admin Channel
   ########################
   def __init__( self, statFile ):
      lst = MDDirect.GetBBDailyStats( statFile )
      sdb = {}
      for ( k,v ) in lst:
         sdb[k] = v
      self._stats  = sdb

   ########################
   # Return string-ified list of stats
   #
   # @return String-ified list of stats
   ########################
   def Dump( self ):
      sdb = self._stats
      kdb = list( sdb.keys() )
      rc  = [ '%-20s = %d' % ( k, sdb[k] ) for k in kdb ]
      return '\n'.join( rc )

   ########################
   # Return string-ified list of stats
   #
   # @param statName : Name of stat
   # @return Value if found; None if not
   ########################
   def GetStat( self, statName ):
      sdb = self._stats
      val = None
      if statName in sdb:
         val = sdb[statName]
      return val


## \cond
######################################
#                                    #
#      r t E d g e D a t a           #
#                                    #
######################################
## \endcond
## @class rtEdgeData
#
# A container for data from rtEdgeSubscriber and LVC.
#
# This class is reused by rtEdgeSubscriber (and LVC). When you receive 
# it in rtEdgeSubscriber.OnData(), it is volatile and only valid for the 
# life of the callback.
#
class rtEdgeData:
   ########################
   # Constructor
   ########################
   def __init__( self, schema=None ):
      self._schema = schema
      self._tUpd   = 0.0
      self._tDead  = 0.0
      self._svc    = ''
      self._tkr    = ''
      self._arg    = None
      self._flds   = []
      self._byFid  = {}
      self._itr    = -1
      self._err    = ""
      self._fld    = rtEdgeField()

   ########################
   # Return Service Name
   #
   # @return Service Name
   ########################
   def Service( self ):
      return self._svc

   ########################
   # Return Ticker Name
   #
   # @return Ticker Name
   ########################
   def Ticker( self ):
      return self._tkr

   ########################
   # Return user-supplied argument
   #
   # @return User-supplied argument
   # @see rtEdgeSubscriber.Subscribe()
   ########################
   def UserArg( self ):
      return self._arg

   ########################
   # Return Message (Update) Time in UnixTime
   #
   # @return Message (Update) Time in UnixTime
   ########################
   def MsgTime( self ):
      return self._tUpd

   ########################
   # Returns number of fields in the message
   #
   # @return Number of fields in the message
   ########################
   def NumFields( self ):
      return len(  self._flds )

   ########################
   # Returns True if stream is Active
   #
   # @return True if stream is Active
   ########################
   def IsActive( self ):
      return self._tUpd and not self.IsDead()

   ########################
   # Returns True if stream is DEAD
   #
   # @return True if stream is DEAD
   ########################
   def IsDead( self ):
      if not self._tUpd:
         return True
      elif self._tDead:
         return( self._tDead > self._tUpd )
      return False

   ########################
   # Returns stringified message time
   #
   # @return stringified message time
   ########################
   def MsgTime( self ):
      t   = self._tUpd
      lt  = time.localtime( t )
      tMs = int( math.fmod( t * 1000, 1000 ) )
      rc  = '%04d-%02d-%02d'  % ( lt.tm_year, lt.tm_mon, lt.tm_mday )
      rc += ' %02d:%02d:%02d' % ( lt.tm_hour, lt.tm_min, lt.tm_sec )
      rc += '.%03d' % tMs
      return rc

   ########################
   # Returns stringified DEAD time
   #
   # @return stringified DEAD time
   ########################
   def DeadTime( self ):
      t   = self._tDead
      lt  = time.localtime( t )
      tMs = int( math.fmod( t * 1000, 1000 ) )
      rc  = '%04d-%02d-%02d'  % ( lt.tm_year, lt.tm_mon, lt.tm_mday )
      rc += ' %02d:%02d:%02d' % ( lt.tm_hour, lt.tm_min, lt.tm_sec )
      rc += '.%03d' % tMs
      return rc

   ########################
   # Reset the iterator
   ########################
   def reset( self ):
      self._itr = -1

   ########################
   # Advances the iterator to the next field, returning field()
   #
   # @return field()
   ########################
   def forth( self ):
      self._itr += 1
      return self.field()

   ########################
   # Returns current rtEdgeField in the iteration or None if end
   #
   # @return Current rtEdgeField in the iteration or None if end
   # @see rtEdgeField
   ########################
   def field( self ):
      fdb = self._flds
      nf  = len( fdb )
      rc  = None
      itr = self._itr
      if ( 0 <= itr ) and ( itr < nf ):
         ( fid, val, ty ) = fdb[itr]
         rc               = self._fld
         rc._Set( fid, val, ty, self._FieldName( fid ) )
      return rc

   ########################
   # Returns specific rtEdgeField by Field ID, or None if not found
   #
   # @param reqFid - Requested Field ID
   # @param bDeepCopy - True for deep copy into unique rtEdgeField
   # @return Specific rtEdgeField by Field ID, or None if not found
   # @see rtEdgeField
   ########################
   def GetField( self, reqFid, bDeepCopy=True ):
      # Build once / message
      #
      idb = self._byFid
      fdb = self._flds
      if len( fdb ) and not len( idb ):
         for ( fid, val, ty ) in fdb:
            idb[fid] = ( val, ty )
      #
      # OK to pull it out 
      #
      fld = None
      if reqFid in idb:
         if bDeepCopy: fld = rtEdgeField()
         else:         fld = self._fld
         ( val, ty ) = idb[reqFid]
         fld._Set( reqFid, val, ty, self._FieldName( reqFid ) )
      return fld

   ########################
   # Returns error string
   #
   # @return Error string
   ########################
   def GetError( self ):
      return self._err

   ########################
   # Return message header as one-line string
   #
   # @return Message header as one-line string
   ########################
   def DumpHdr( self ):
      tm  = self.MsgTime()
      svc = self._svc
      tkr = self._tkr
      nf  = self.NumFields()
      return '%s [%s,%s] : %d fields' % ( tm, svc, tkr, nf )

   ########################
   # Return message contents as a string, one field per line
   #
   # @param bHdr : True for DumpHdr(); False for fields only
   # @param bFldTy : True to show field type; False otherwise
   # @return Message contents as a string, one field per line
   #
   # @see DumpHdr()
   ########################
   def Dump( self, bHdr=True, bFldTy=False ):
      fdb  = self._flds
      fld  = self._fld
      s    = []
      if bHdr:
         s += [ self.DumpHdr() ]
      for ( fid, val, ty ) in fdb:
         fld._Set( fid, val, ty, self._FieldName( fid ) )
         pn = fld.Name()
         try:                           pv = fld.GetAsString()
         except MDDirectException as x: pv = 'MDDirectException : ' + x.value
         if bFldTy: ty = fld.TypeName()
         else:      ty = ''
         s += [ '   [%04d] %-14s : %s%s' % ( fld.Fid(), pn, ty, pv ) ]
      return '\n'.join( s )

## \cond
   #################################
   # Set the message data contents from field list update:
   #
   # The real-time thread in MDDirect.pyd that services the real-time 
   # data stream puts all fields received into a field list that is 
   # returned to the Python thread calling MDDirect.Read() (and this method).
   #
   # To this end, the flds list in this method is ALL fields that have updated
   # for this data stream.  As such, the bLVC param controls how this list
   # of possibly duplicated fields is handled as follows:
   #
   # bLVC | Description
   # --- | ---
   # True | Use Last Field from list only
   # False | All Fields
   #
   # @param svc : Service name (bloomberg)
   # @param tkr : Ticker Name (AAPL US Equity)
   # @param arg : Optional User-supplied argument
   # @param flds : [ [ fid1, val1 ], [ fid2, val2 ], ... ]
   # @param bLVC : True last update; False for all fields
   # @return self
   #################################
   def _SetData( self, svc, tkr, arg, flds, bLVC=True ):
      self._svc   = svc
      self._tkr   = tkr
      self._arg   = arg
      self._itr   = -1
      self._err   = ''
      bdb         = { fid: (val, ty) for fid, val, ty in flds }
      self._byFid = bdb
      fids        = list( bdb.keys() )
      self._flds  = [ ( fid, bdb[fid][0], bdb[fid][1] ) for fid in fids ]
##      self._flds  = [ list( fld ) for fld in flds ] if bLVC else flds
      return self

   #################################
   # Set the message error contents
   #
   # @param svc : Service name (bloomberg)
   # @param tkr : Ticker Name (AAPL US Equity)
   # @param arg : Optional User-supplied argument
   # @param err : Error Message
   # @return self
   #################################
   def _SetError( self, svc, tkr, arg, err ):
      self._svc   = svc
      self._tkr   = tkr
      self._arg   = arg
      self._flds  = []
      self._byFid = {}
      self._itr   = -1
      self._err   = err
      return self

   #################################
   # Return field name for Field ID from rtEdgeSubscriber Schema feeding us
   #
   # @param fid : Field ID
   # @return rtEdgeSchema.GetFieldName()
   # @see rtEdgeSchema.GetFieldName()
   #################################
   def _FieldName( self, fid ):
      ddb = self._schema
      return ddb.GetFieldName( fid )
## \endcond



## \cond
######################################
#                                    #
#       r t E d g e F i e l d        #
#                                    #
######################################
## \endcond
## @class rtEdgeField
#
# A container for a single field of data from an rtEdgeData structure.
#
# When pulling an rtEdgeField from an incoming rtEdgeData, this object is 
# reused. The contents are volatile and only valid until the next call to 
# rtEdgeData.forth().
#
class rtEdgeField:
   #################################
   # Constructor
   #################################
   def __init__( self ):
      self._fid  = 0
      self._val  = None
      self._type = MDDirectEnum._MDDPY_STR
      self._name = _UNDEF

   #################################
   # Returns Field ID
   #
   # @return Field ID
   #################################
   def Fid( self ):
      return self._fid

   #################################
   # Returns True if empty field
   #
   # @return True if empty field
   #################################
   def IsEmpty( self ):
      return( self._type == MDDirectEnum._MDDPY_NONE )

   #################################
   # Returns Field Name
   #
   # @return Field Name
   #################################
   def Name( self ):
      return self._name

   #################################
   # Returns string-ified Field Type from the following set:
   # Enum | TypeName
   # --- | ---
   # _MDDPY_INT | (int)
   # _MDDPY_INT64 | (i64)
   # _MDDPY_DBL | (dbl)
   # _MDDPY_STR | <str)
   #
   # @return string-ified Field Type from the following set:
   #################################
   def TypeName( self ):
      return GetTypeSuffix( self._type ) + ' '

   #################################
   # Returns Field Type from the following set:
   # Enum | Type
   # --- | ---
   # _MDDPY_INT | Integer
   # _MDDPY_INT64 | int64_t
   # _MDDPY_DBL | Double
   # _MDDPY_STR | String
   #
   # @return Field Type
   #################################
   def Type( self ):
      return self._type

   #################################
   # Returns Raw Field Value
   #
   # @return Raw Field Value
   #################################
   def GetValue( self ):
      return self._val

   #################################
   # Returns Field Value as 64-bit Integer, regardless of (native) data type
   #
   # @return Field Value as 64-bit Integer
   #################################
   def GetAsInt64( self ):
      return self.GetAsInt()

   #################################
   # Returns Field Value as Integer, regardless of (native) data type
   #
   # @return Field Value as Integer
   #################################
   def GetAsInt( self ):
      return int( self.GetAsDouble() )

   #################################
   # Returns Field Value as double, regardless of (native) data type
   #
   # @return Field Value as double
   #################################
   def GetAsDouble( self ):
      try:
         dv = float( self._val )
      except ( TypeError, ValueError ):
         raise MDDirectValueException( self._val )
      return dv

   #################################
   # Returns Field Value as string, regardless of (native) data type
   #
   # @return Field Value as string
   #################################
   def GetAsString( self, nDec=0 ):
      if self.IsEmpty():
         raise MDDirectValueException( 'Empty Field' )
      rc = str( self._val )
      if nDec and rc.count( '.' ):
         ix = rc.index( '.' )
         rc = rc[:ix+nDec+1]
      return rc

## \cond
   #################################
   # Set field contents
   #
   # @param fid : Field ID
   # @param val : Field Value
   # @param ty : Field Type
   # @param name : Field Name
   #################################
   def _Set( self, fid, val, ty, name ):
      self._fid  = fid
      self._val  = val
      self._type = ty
      self._name = name
      #
      # Convert Time to String
      #
      timeFields = [ MDDirectEnum._MDDPY_DT, 
                     MDDirectEnum._MDDPY_TM, 
                     MDDirectEnum._MDDPY_TMSEC ]
      if not ty in timeFields:
         return
      r64 = None
      try:    r64 = float( val )
      except: pass
      if r64 == None:
         return
      try:                   i32 = int( r64 )
      except Exception as x: raise MDDirectValueException( '[' + val + '] ' + x.args[0] ) 
      h   = ( i32 / 10000 )
      m   = ( i32 % 10000 ) / 100
      s   = ( i32 % 100 )
      v   = str( val )
      return
      #
      # Don't need this code : Shown for completeness
      #
      if ty == MDDirectEnum._MDDPY_DT:      ## ( y * 10000 ) + ( m * 100 ) + d
         v = '%04d-%02d-%02d' % ( h, m, s )
      elif ty == MDDirectEnum._MDDPY_TM:    ## _MDDPY_TMSEC + mikes
         v  = '%02d:%02d:%02d' % ( h, m, s )
         v += '%.06d' % int( ( r64 - i32 ) * 1000000.0 )
      elif ty == MDDirectEnum._MDDPY_TMSEC: ## ( h * 10000 ) + ( m * 100 ) + s
         v  = '%02d:%02d:%02d' % ( h, m, s )
      self._val = v
## \endcond


## \cond
######################################
#                                    #
#       r t E d g e S c h e m a      #
#                                    #
######################################
## \endcond
## @class rtEdgeSchema
#
# Data Dictionry : FID-to-name and Name-to-FID
#
# Member | Description
# --- | ---
# _byFid  | { fid1 : name1, fid2 : name2, ... }
# _byName | { name1 : fid1, name2 : fid2, ... }
# _types  | { fid1 : type1, fid2 : type2, ... }
#
class rtEdgeSchema:
   #################################
   # Constructor
   #
   # @param schema [ [ fid1, name1 ], [ fid2, name2 ], ... ]
   #################################
   def __init__( self, schema ):
      fdb = {}
      ndb = {}
      tdb = {}
      if schema:
         for kv in schema:
            fid     = kv[0] 
            name    = kv[1]
            try:    type = kv[2]
            except: type = None
            fdb[fid]  = name
            ndb[name] = fid
            tdb[fid]  = type
      self._byFid  = fdb
      self._byName = ndb
      self._types  = tdb

   #################################
   # Return number of fields in the Schema
   #
   # @return Number of fields in the Schema
   #################################
   def Size( self ):
      return len( self._byFid )

   #################################
   # Return list of FID's, optionally sorted
   #
   # @param bSort : True for sorted; False for unsorted
   # @return list of FID's, optionally sorted
   #################################
   def GetFIDs( self, bSort=True ):
      rc = [] + list( self._byFid.keys() )
      if bSort:
         rc.sort()
      return rc

   #################################
   # Return FID from name, allowing for name to be stringified number
   #
   # @return FID from name
   #################################
   def GetFieldID( self, name ):
      # 1) Allow name to be string-ified numbber
      #
      try:    fid = int( name )
      except: fid = 0
      if fid:
         return fid
      #
      # 2) OK, lookup by name
      #
      ndb = self._byName
      try:    fid = ndb[name]
      except: fid = 0
      return fid

   #################################
   # Return Name from FID
   #
   # @param fid : Field ID
   # @return Name from FID
   #################################
   def GetFieldName( self, fid ):
      try:    i32 = int( fid )
      except: i32 = 0
      fdb = self._byFid
      try:    name = fdb[i32]
      except: name = fid
      return name

   ########################
   # Returns Field Type from ID in Schema
   #
   # @param fid : Field FID
   # @return Field Type from ID in Schema
   ########################
   def GetFieldType( self, fid ):
      tdb = self._types
      if fid in tdb:
         return tdb[fid]
      return MDDirectEnum._MDDPY_NONE

   ########################
   # Returns Field Type from ID in Schema
   #
   # @param fid : Field FID
   # @return Field Type from ID in Schema
   ########################
   def GetFieldType( self, fid ):
      tdb = self._types
      if fid in tdb:
         return tdb[fid]
      return MDDirectEnum._MDDPY_NONE

   ########################
   # Returns Field Type Suffix from ID
   #
   # @param fid : Field FID
   # @return Field Type Suffix
   ########################
   def GetFieldTypeSfx( self, fid ):
      ty = self.GetFieldType( fid )
      return GetTypeSuffix( ty )


## \cond
######################################
#                                    #
#      M D D i r e c t E n u m       #
#                                    #
######################################
## @class MDDirectEnum
#
# Hard-coded Enumerated Types from MDDirect addin library
#
class MDDirectEnum:
   #
   # Event Types
   #
   EVT_CONN   = 0x0001
   EVT_SVC    = 0x0002
   EVT_UPD    = 0x0004
   EVT_STS    = 0x0008
   EVT_SCHEMA = 0x0010
   EVT_OPEN   = 0x0100
   EVT_CLOSE  = 0x0200
   EVT_BYSTR  = 0x0400
   EVT_RECOV  = 0x0800
   EVT_DONE   = 0x1000
   EVT_BDS    = 0x2000
   EVT_CHAN   = ( EVT_CONN | EVT_SVC )
   EVT_ALL    = 0xffff
   IOCTL_RTD  = 0x0001
   #
   # Field Types
   #
   _MDDPY_UNDEF  =  0
   _MDDPY_INT    =  1
   _MDDPY_DBL    =  2
   _MDDPY_STR    =  3
   _MDDPY_DT     =  4    ## i32 = ( y * 10000 ) + ( m * 100 ) + d
   _MDDPY_TM     =  5    ## r64 = i32 + mikes
   _MDDPY_TMSEC  =  6    ## i32 = ( h * 10000 ) + ( m * 100 ) + s
   _MDDPY_INT64  =  7
   _MDDPY_UNXTM  =  8
   _MDDPY_NONE   =  9
   _MDDPY_VECTOR = 10
   #
   # Type Names /  Suffixes
   #
   _TypeNames = { _MDDPY_UNDEF  : 'undef',
                  _MDDPY_INT    : 'int',
                  _MDDPY_INT64  : 'int64',
                  _MDDPY_DBL    : 'double',
                  _MDDPY_STR    : 'string',
                  _MDDPY_DT     : 'date',
                  _MDDPY_TM     : 'time',
                  _MDDPY_TMSEC  : 'time-sec',
                  _MDDPY_UNXTM  : 'unixTime',
                  _MDDPY_NONE   : 'NaN',
                  _MDDPY_VECTOR : 'vector'
                }
   _TypeSfxs =  { _MDDPY_UNDEF  : '(n/a)',
                  _MDDPY_INT    : '(int)',
                  _MDDPY_INT64  : '(i64)',
                  _MDDPY_DBL    : '(dbl)',
                  _MDDPY_STR    : '(str)',
                  _MDDPY_DT     : '(dat)',
                  _MDDPY_TM     : '(tim)',
                  _MDDPY_TMSEC  : '(tms)',
                  _MDDPY_UNXTM  : '(unx)',
                  _MDDPY_NONE   : '(NaN)',
                  _MDDPY_VECTOR : '(vec)'
                }

def GetTypeSuffix( ty ):
   try:    rc = MDDirectEnum._TypeSfxs[ty]   
   except: rc = '(%03d)' % ty
   return rc

def GetTypeName( ty ):
   try:    rc = MDDirectEnum._TypeNames[ty]   
   except: rc = 'string'
   return rc

## \endcond
