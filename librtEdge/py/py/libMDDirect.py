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
#
#  (c) 1994-2022, Gatea Ltd.
#################################################################
import math, sys, time, threading

_UNDEF = 'Undefined'

def Log( msg ):
   sys.stdout.write( msg + '\n' )
   sys.stdout.flush()

try:
   import MDDirect27 as MDDirect
except:
   try: 
      import MDDirect39 as MDDirect
   except: 
      Log( 'MDDirect not found; Exitting ...' )
      sys.exit()

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
# OS | Python Version | Loadable Module
# --- | --- | ---
# Linux64 | 2.7 | MDDirect27.so
# WIN64 | 3.9 | MDDirect39.pyd
# Linux64 | 2.7 | MDDirect27.so
# WIN64 | 3.9 | MDDirect39.pyd
# 
# You import the module depending on the version of your Python interpreter
# as follows: 
# \code
#   import sys
#
#   try:
#      import MDDirect27 as MDDirect
#   except:
#      try:
#         import MDDirect39 as MDDirect
#      except:
#         sys.stdout.write( 'MDDirect not found; Exitting ...\n' )
#         sys.exit()
#
#    sys.stdout.write( MDDirect.Version() )
# \endcode
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

#################################
# Returns version and build info
#
# @return version and build info
#################################
def Version():
   return MDDirect.Version()

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
      self._cxt    = None
      self._schema = None
      self._msg    = rtEdgeData()
      self._ready  = threading.Event()

   ########################
   # Returns Version and Build info
   #
   # @return Version and Build info
   ########################
   def Version( self ):
      return MDDirect.Version()

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
      self.join()
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
      return MDDirect.PumpTape( self._cxt, t0, t1 )

   ########################
   # Open a subscription stream for the ( svc, tkr ) data stream.  
   #
   # Market data updates are returned in the OnData() asynchronous call.
   #
   # @param svc - Service Name (e.g., bloomberg)
   # @param tkr - Ticker Name (e.g., AAPL US EQUITY)
   # @param uid - Optional unique user ID
   # @return Unique non-zero stream ID on success
   # 
   # @see Unsubscribe()
   # @see OnData()
   ########################
   def Subscribe( self, svc, tkr, uid ):
      return MDDirect.Open( self._cxt, svc, tkr, uid )

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
   #################################
   def run( self ):
      self._ready.set()
      #
      # Drain until stopped
      #
      msg = self._msg
      while self._run:
         rd = MDDirect.Read( self._cxt, 0.25 )
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
            self.OnData( msg._SetData( svc, tkr, flds ) )
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
            self.OnDead( msg._SetError( svc, tkr, sts ), sts )
         elif mt == MDDirectEnum.EVT_RECOV:
            msg._tUpd = blob[0]
            uoid      = blob[1]
            svc       = blob[2]
            tkr       = blob[3]
            sts       = blob[4]
            self.OnRecovering( msg._SetData( svc, tkr, [] ), sts )
         elif mt == MDDirectEnum.EVT_DONE:
            tUpd = blob[0]
            uoid = blob[1]
            svc  = blob[2]
            tkr  = blob[3]
            sts  = blob[4]
            self.OnStreamDone( sts )
         elif mt == MDDirectEnum.EVT_SVC:
            kv = blob.split('|')
            self.OnService( kv[1], kv[0].strip() )
         elif mt == MDDirectEnum.EVT_CONN:
            ( ty, pMsg )  = blob.split('|')
            self.OnConnect( pMsg, ty.strip() )
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
   # Returns Field ID from Field Name in Schema
   #
   # @param name : Field Name
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
   # @param file : LVC filename
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
      rtn  = None
      if blob:
         rtn = rtEdgeData( self._schema )
         rtn._SetData( blob[0], blob[1], blob[2:] )
      return rtn

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
      self._svc    = ''
      self._tkr    = ''
      self._flds   = []
      self._byFid  = {}
      self._itr    = -1
      self._err    = ""
      self._fld    = rtEdgeField()

   ########################
   # Returns number of fields in the message
   #
   # @return Number of fields in the message
   ########################
   def NumFields( self ):
      return len(  self._flds )

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
   # Advances the iterator to the next field, returning field()
   #
   # @return field()
   ########################
   def forth( self ):
      self._itr += 1
      return field()

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
      if ( 0 <= _itr ) and ( _itr < nf ):
         ( fid, val, ty ) = fdb[itr]
         rc               = self._fld
         rc._Set( fid, val, ty, self._FieldName( fid ) )
      return rc

   ########################
   # Returns specific rtEdgeField by Field ID, or None if not found
   #
   # @return Specific rtEdgeField by Field ID, or None if not found
   # @see rtEdgeField
   ########################
   def GetField( self, reqFid ):
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
      if idb.has_key( reqFid ):
         fld         = self._fld
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
   # Return message contents as a string, one field per line
   #
   # @return Message contents as a string, one field per line
   ########################
   def Dump( self ):
      fdb  = self._flds
      fld  = self._fld
      s    = []
      for ( fid, val, ty ) in fdb:
         fld._Set( fid, val, ty, self._FieldName( fid ) )
         pn = fld.Name()
         pv = fld.GetAsString()
         s += [ '   [%04d] %-14s : %s' % ( fld.Fid(), pn, pv ) ]
      return '\n'.join( s )

## \cond
   #################################
   # Helpers
   #################################
   def _SetData( self, svc, tkr, flds ):
      """rtEdgeData._SetData() is called by rtEdgeSubscriber to set the 
message data contents.
"""
      self._svc   = svc
      self._tkr   = tkr
      self._byFid = {}
      self._flds  = flds
      self._itr   = -1
      self._err   = ''
      return self

   def _SetError( self, svc, tkr, err ):
      """rtEdgeData._SetError() is called by rtEdgeSubscriber to set the 
message error contents
"""
      self._svc   = svc
      self._tkr   = tkr
      self._flds  = []
      self._byFid = {}
      self._itr   = -1
      self._err   = err
      return self

   def _FieldName( self, fid ):
      """rtEdgeData._FieldName() finds field name from the schema in the 
rtEdgeSubscriber feeding us.
"""
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
      self._val  = ''
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
   # Returns Field Name
   #
   # @return Field Name
   #################################
   def Name( self ):
      return self._name

   #################################
   # Returns Field Type from the following set:
   # Enum | Type
   # --- | ---
   # _MDDPY_INT | Integer
   # _MDDPY_DBL | Double
   # _MDDPY_STR | String
   #
   # @return Field Type
   #################################
   def Type( self ):
      return self._type

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
      try:    dv = float( self._val )
      except: dv = 0.0
      return dv

   #################################
   # Returns Field Value as string, regardless of (native) data type
   #
   # @return Field Value as string
   #################################
   def GetAsString( self, nDec=0 ):
      rc = str( self._val )
      if nDec and rc.count( '.' ):
         ix = rc.index( '.' )
         rc = rc[:ix+nDec+1]
      return rc

## \cond
   #################################
   # Helpers
   #################################
   def _Set( self, fid, val, ty, name ):
      """rtEdgeField._Set() is called by rtEdgeData to set the field 
contents
"""
      self._fid  = fid
      self._val  = val
      self._type = ty
      self._name = name
      #
      # Convert Time to String
      #
      r64 = None
      try:    r64 = float( val )
      except: pass
      if r64 == None:
         return
      i32 = int( r64 )
      h   = ( i32 / 10000 )
      m   = ( i32 % 10000 ) / 100
      s   = ( i32 % 100 )
      v   = str( val )
      return
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
      if schema:
         for kv in schema:
            fid       = kv[0] 
            name      = kv[1]
            try:    type = kv[2]
            except: type = None
            fdb[fid]  = name
            ndb[name] = fid
      self._byFid  = fdb
      self._byName = ndb

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
      rc = [] + self._byFid.keys()
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
   # @return Name from FID
   #################################
   def GetFieldName( self, fid ):
      try:    i32 = int( fid )
      except: i32 = 0
      fdb = self._byFid
      try:    name = fdb[i32]
      except: name = fid
      return name

## \cond
######################################
#                                    #
#      M D D i r e c t E n u m       #
#                                    #
######################################
class MDDirectEnum:
   """Hard-coded Enumerated Types from MDDirect addin library
"""
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
   EVT_CHAN   = ( EVT_CONN | EVT_SVC )
   EVT_ALL    = 0xffff
   IOCTL_RTD  = 0x0001
   #
   # Field Types
   #
   _MDDPY_INT   = 1
   _MDDPY_DBL   = 2
   _MDDPY_STR   = 3
   _MDDPY_DT    = 4    ## i32 = ( y * 10000 ) + ( m * 100 ) + d
   _MDDPY_TM    = 5    ## r64 = i32 + mikes
   _MDDPY_TMSEC = 6    ## i32 = ( h * 10000 ) + ( m * 100 ) + s
## \endcond
