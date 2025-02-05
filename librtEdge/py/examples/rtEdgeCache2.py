#################################################################
#
#  rtEdgeCache2.py
#     libMDDirect.py-driven rtEdgeCache Subscription Channel
#
#  REVISION HISTORY:
#     30 DEC 2007 jcs  Created.
#      . . .
#     26 APR 2024 jcs  libMDDirect-driven; No mo TS1 / RMDS
#
#  (c) 1994-2024, Gatea Ltd.
#################################################################
import threading, time
import libMDDirect

## @namespace rtEdgeCache2
#
# rtEdgeCache.py manages the subscription channel to the rtEdgeCache 
# real-time data distributor.  It is a wrapper class around libMDDirect
# and is supplied for backwards-compatibility.
#
# Two classes are exposed:
# -# GLrecord  : A stateful real-time market data record, and
# -# GLedgChan : The subscription channel to rtEdgeCache
#
# The GLrecord class is reference-counted such that one **or more** users 
# may reference and use the real-time object.  Users increment / decrement 
# the reference count via the GLrecord.AddRef() / GLrecord.Release() methods.
# When the reference count goes to 0, the object is deleted.
#

# Bind() function identifiers

## Bind() event : MDDirect connection UP
EDG_UP    = 0x10
## Bind() event : MDDirect connection DOWN
EDG_DOWN  = 0x11
## Bind() event : MDDirect Service UP
SVC_UP    = 0x12
## Bind() event : MDDirect Service DOWN
SVC_DOWN  = 0x13

################
# Global Fcns
################
def Version( bTrunc=True ):
   return libMDDirect.Version()

###############################
# Return default alignment based on field type
#
# Value | Description
# --- | ---
# 1 | Left
# 0 | Center
# -1 | Right
#
# @param ty - Field Type from libMDDirect.MDDirectEnum 
# @return Alignment - Left, Right or Center
# @see libMDDirect.MDDirectEnum 
###############################
def Alignment( ty ):
   _LEFT   =  1
   _CENTER =  0
   _RIGHT  = -1
   EDB     = libMDDirect.MDDirectEnum
   if ty == EDB._MDDPY_DBL:    return _RIGHT
   if ty == EDB._MDDPY_STR:    return _LEFT
   if ty == EDB._MDDPY_DT:     return _CENTER
   if ty == EDB._MDDPY_TM:     return _CENTER
   if ty == EDB._MDDPY_TMSEC:  return _CENTER
   if ty == EDB._MDDPY_INT64:  return _RIGHT
   if ty == EDB._MDDPY_UNXTM:  return _CENTER
   if ty == EDB._MDDPY_NONE:   return _CENTER
   if ty == EDB._MDDPY_VECTOR: return _LEFT
   return _CENTER

###############################
# Return descriptive name of field type
#
# @param ty - Field Type from libMDDirect.MDDirectEnum
# @return Descriptive name of field type
# @see libMDDirect.GetTypeName()
###############################
def FldType( ty ) :
   return libMDDirect.GetTypeName( ty )

###############################
# Return short descriptive name of field type
#
# @param ty - Field Type from libMDDirect.MDDirectEnum
# @return Short Descriptive name of field type
# @see libMDDirect.GetTypeSuffix()
###############################
def FldTypeShort( ty ) :
   return libMDDirect.GetTypeName( ty )

def FldTypeShort( ty ) :
   rc = libMDDirect.GetTypeSuffix( ty )
   return rc.replace( '(','' ).replace( ')','' )

## \cond
###############################
#                             #
#      I U n k n o w n        #
#                             #
###############################
## \endcond
## @class IUnknown
#
# Abstract reference-counting class.  It is named after the Microsoft 
# IUnknown class and contains 3 methods:
#
# Method | Description
# --- | ---
# AddRef()  | Add reference; Increment reference count
# Release() | Remove reference; Decrement reference count
# RefCount()  | Current reference count
#
# You are notified when RefCount() goes to 0 via OnRefCnt0(), allowing you to 
# clean up before the object is delete
#
class IUnknown:
   ###########################
   # Constructor
   ###########################
   def __init__( self ):
      self._refCnt = 0

   ########################
   # Increment RefCount()
   ########################
   def AddRef( self ):
      self._refCnt += 1

   ########################
   # Decrement RefCount()
   ########################
   def Release( self ):
      self._refCnt -= 1
      if not self.RefCount():
         self.OnRefCnt0()
      pass

   ########################
   # Return Reference Count
   #
   # @return Reference Count
   ########################
   def RefCount( self ):
      return max( self._refCnt, 0 )

   ########################
   # Called when IUnknown.RefCount() goes to 0
   #
   # Derived classes override to clean up
   ########################
   def OnRefCnt0( self ):
      del self


## \cond
###############################
#                             #
#       S c h e m a           #
#                             #
###############################
## \endcond
## @class Schema
#
# Backward-compatible Wrapper around libMDDirect.Schema
#
# Member | Description
# --- | ---
# _mdd | libMDDirect.rtEdgeSchema
#
class Schema:
   ###########################
   # Constructor
   #
   # @param mdd : libMDDirect.rtEdgeSchema
   ###########################
   def __init__( self, mdd ):
      self._mdd = mdd

   ###########################
   # Return list of fids, optionally sorted
   #
   # @param bSort : True for sorted list
   # @return [ fid1, fid2, ... ]
   ###########################
   def GetAllFids( self, bSort=True ):
      return self._mdd.GetFIDs( bSort )

   #################################
   # Return FID from name, allowing for name to be stringified number
   #
   # @return FID from name
   #################################
   def GetFID( self, name ):
      return self._mdd.GetFieldID( name )

   #################################
   # Return Name from FID
   #
   # @param fid : Field ID
   # @return Name from FID
   #################################
   def GetFieldName( self, fid ):
      return self._mdd.GetFieldName( fid )

   #################################
   # Return Field Type from FID
   #
   # @param fid : Field ID
   # @return Field Type from FID
   #################################
   def GetFieldType( self, fid ):
      return self._mdd.GetFieldType( fid )

   #################################
   # Return descriptive text for field based on bType:
   #
   # bType | Return
   # --- | ---
   # True | "[00109]   PUTCALLIND (int)"
   # False | "[00109]   PUTCALLIND"
   #
   # @param bType : True for type appendage
   # @return Descriptive text for field based on bType
   #################################
   def GetFullField( self, fid, bType=True ):
      pFld = self.GetFieldName( fid )
      rc   = '[%05d] %s' % ( fid, pFld )
      if bType:
         rc += ' %s' % self._mdd.GetFieldTypeSfx( fid )
      return rc

## \cond 
###############################
#                             #
#     G L r e c o r d         #
#                             #
###############################
## \endcond 
## @class GLrecord
#
# Stateful real-time market data record from a specific service such as 
# EUR Curncy from bloomberg.  The MDDirect platform supports the concept of 
# a stateful real-time data stream such as EUR Curncy from bloomberg.  As such, 
# EUR Curncy from another data source such as bidfx is a different, unique data 
# stream.  Thus, to subscribe to EUR Curncy from both bloomberg and bidfx, you
# would create 2 GLrecord instances - One from bloomberg and one from bidfx
#
# GLrecord's must be opened via GLedgChan.Open() to receive data.  Data Streams 
# are auto-rerequested to maintain proper state
#
# Member | Description
# --- | ---
# _edg | rtEdgeCache.GLedgChan
# _svc | Service Name such as bloomberg or bidfx
# _tkr | Ticker Name such as EUR Curncy
# _ReqID | Unique StreamID
# _err | Error message; None if OK
# _nUpd | Number of updates
#
class GLrecord( IUnknown ):
   ###########################
   # Constructor
   #
   # @param edg : rtEdgeCache.GLedgChan
   # @param svc : Service Name such as bloomberg or bidfx
   # @param tkr : Ticker Name such as EUR Curncy
   ###########################
   def __init__( self, edg, svc, tkr ):
      IUnknown.__init__( self )
      self._edg    = edg
      self._svc    = svc
      self._tkr    = tkr
      self._ReqID  = edg.ReqID()
      self._err    = None
      self._bOpn   = False
      self._nUpd   = 0

   ########################
   # Return True if we have updates
   #
   # @return True if we have updates
   ########################
   def HasData( self ):
      return( self._nUpd != 0 )

   ########################
   # @override
   #
   # We close down the data stream from GLedgChan
   ########################
   def OnRefCnt0( self ):
      self._edg.Close( self )
      IUnknown.OnRefCnt0( self )

   ########################
   # Called when Market Data Image is received
   #
   # Derived classes override to implement logic
   #
   # @param mddMsg - libMDDirect.rtEdgeData
   ########################
   def OnImage( self, mddMsg ):
      pass

   ########################
   # Called when Market Data Update is received
   #
   # Derived classes override to implement logic
   #
   # @param mddMsg - libMDDirect.rtEdgeData
   ########################
   def OnUpdate( self, mddMsg ):
      pass

   ########################
   # Called when DEAD Status is received
   #
   # Derived classes override to implement logic
   #
   # @param err - DEAD Status
   ########################
   def OnDead( self, err ):
      pass

## \cond
   ########################
   # Real-Time Data Dispatcher
   #
   # @param mddMsg - libMDDirect.rtEdgeData
   ########################
   def _OnData( self, mddMsg ):
      bUpd = self._nUpd 
      if bUpd: self.OnUpdate( mddMsg )
      else:    self.OnImage( mddMsg )
      self._nUpd += 1
## \endcond 



## \cond
###############################
#                             #
#     G L e d g C h a n       #
#                             #
###############################
## \endcond
## @class GLedgChan
#
# Subscription channel from an MD-Direct data source - rtEdgeCache3 or
# Tape File.
#
# Features:
# -# libMDMDirect-driven class
# -# GLrecord-centric design; Updates pumped into GLrecord and derivities
# -# Bind() your functions to receive MDDirect events such as Service Up / Down
# -# Auto-reconnect / re-subscribe built in
#
# Member | Description
# --- | ---
# _svr | MDDirect Server
# _user | MDDirect username
# _schema | Schema object
# _ReqID | Unique request ID
# _dstConn | String showing dest conn
# _wlName | { svc1|tkr1 : GLrecord1, svc2|tkr2 : GLreord2, ... }
# _wlOid | { rid1 : GLrecord1, rid2 : GLrecord2, ... }
# _wlMtx | thread.Lock protecting watchlist
# _nMsg | Num messages received on channel
# _tMsg | Time last msg received
# _nUpd | Num updates received on channel
# _tUpd | Time of last update
# _nFldUpd | Num fields received on channel
# _fcnMddUp | Bind()'ed functions for MDD_UP
# _fcnMddDown | Bind()'ed functions for MDD_DOWN
# _fcnSvcUp | Bind()'ed functions for SVC_UP
# _fcnSvcDown | Bind()'ed functions for SVC_DOWN
#
class GLedgChan( libMDDirect.rtEdgeSubscriber ):
   ###########################
   # Constructor
   #
   # @param svr - host:port of MDDirect platform
   # @param user - MDDirect username
   ###########################
   def __init__( self, svr, user ) : 
      libMDDirect.rtEdgeSubscriber.__init__( self )
      self._svr         = svr
      self._user        = user
      self._schema      = None
      self._dstConn     = None
      self._ReqID       = 1
      self._wlName      = {}
      self._wlOid       = [ 0 ] * 100
      self._wlMtx       = threading.Lock()
      self._nMsg        = 0
      self._tMsg        = 0.0
      self._nUpd        = 0
      self._tUpd        = 0.0
      self._nFldUpd     = 0
      self._fcnMddUp    = []
      self._fcnMddDown  = []
      self._fcnSvcUp    = []
      self._fcnSvcDown  = []

   ########################
   # Connect and Start session to MD-Direct rtEdgeCache3 server. 
   ########################
   def Start( self ):
      svr = self._svr
      usr = self._user
      libMDDirect.rtEdgeSubscriber.Start( self, svr, usr, True )

   ########################
   # Return Event Queue
   #
   # @return Event Queue
   ########################
   def EvtQ( self ):
      return None $# TODO - ELSE DEPRECATED

   ########################
   # Return Schema object
   #
   # @return Schema object
   ########################
   def Schema( self ):
      return self._schema

   ########################
   # Return Channel Protocol Name
   #
   # @return Channel Protocol Name
   ########################
   def Protocol( self ):
      return 'BINARY'

   ########################
   # Return CPU usage for the process
   #
   # @return CPU usage for the process
   ########################
   def CPU( self ):
      return libMDDirect.CPU()

   #################################
   # Returns process memory size in KB
   #
   # @return process memory size in KB
   #################################
   def MemSize( self ):
      return libMDDirect.MemSizeKb()

   ########################
   # Return a unique Request ID
   #
   # @return Unique Request ID
   ########################
   def ReqID( self ):
      mtx = self._wlMtx
      mtx.acqure()
      rtn          = self._ReqID
      self._ReqID += 1
      mtx.release()
      return rtn

   ########################
   # Open a subscription stream for the GLrecord data stream
   #
   # @param rec - GLrecord
   # @return Unique non-zero stream ID on success
   ########################
   def Open( self, rec ):
      rc = self.Subscribe( rec._svc, rec._ric, rec._ReqID )
      self.AddRecord( rec )
      return rc

   ########################
   # Closes a subscription stream for the GLrecord data stream that was 
   # opened via Open().  Market data updates are stopped.
   #
   # @param rec - GLrecord
   ########################
   def Close( self, rec ):
      self.Unsubscribe( rec._svc, rec._tkr )
      self.RemoveRecord( rec )

## \cond
   ########################
   # Return GLrecord by unique ReqID
   #
   # @param uoid - Unique Request ID
   # @return GLrecord; None if not found
   ########################
   def GetRecByOid( self, uoid ):
      mtx = self._wlMtx
      wlo = self._wlOid
      rec = None
      try:
         mtx.acquire()
         try:    rec = wlo[uoid]
         except: rec = None
      finally: mtx.release()
      return rec

   ########################
   # Return GLrecord by ( svc,tkr ) name
   #
   # @param svc - Service Name
   # @param tkr - Ticker Name
   # @return GLrecord; None if not found
   ########################
   def GetRecord( self, svc, tkr ):
      mtx = self._wlMtx
      rec = None
      wln = self._wlName
      try:
         mtx.acquire()
         key = "%s|%s" % ( svc, tkr )
         try:    rec = wln[key]
         except: rec = None
      finally: mtx.release()
      return rec

   ########################
   # Add GLrecord to internal watchlist
   #
   # @param rec - GLrecord
   ########################
   def AddRecord( self, rec ):
      mtx = self._wlMtx
      wlo = self._wlOid
      wln = self._wlName
      try:
         mtx.acquire()
         key  = "%s|%s" % ( rec._svc, rec._ric )
         uoid = rec._ReqID
         try:
            sz = len( wlo )
            while sz <= uoid:
               wlo += [ 0 ] * 100
               sz   = len( wlo )
            if not wln.has_key( key ): 
               wln[key] = rec
            if wlo[uoid] == 0:
               wlo[uoid] = rec
         except:  pass
      finally: mtx.release()

   ########################
   # Remove GLrecord from internal watchlist
   #
   # @param rec - GLrecord
   ########################
   def RemoveRecord( self, rec ):
      mtx = self._wlMtx
      wlo = self._wlOid
      wln = self._wlName
      try:
         mtx.acquire()
         try:
            key  = "%s|%s" % ( rec._svc, rec._ric )
            uoid = rec._ReqID
            if wln.has_key( key ):
               wln.pop( key )
            sz = len( wlo )
            if sz > uoid:
               wlo[uoid] = 0
         except:  pass
      finally: mtx.release()

## \endcond

   ########################
   # Bind callback functions for life-cycle events
   #
   # Event | Fcn Args | Description
   # --- | --- | ---
   # MDD_UP | ConnMsg | MDDirect connection established
   # MDD_DOWN | ConnMsg | MDDirect connection lost
   # SVC_UP | SvcName | rtEdgeCache quote service UP
   # SVC_DOWN | SvcName | rtEdgeCache quote service DOWN
   #
   # @param iEvt - Event to bind from above table
   # @param fcn - Event to bind from above table
   # @param evtQ - Event to bind from above table
   ########################
   def Bind( self, iEvt, fcn, evtQ=None ):
      if iEvt == MDD_UP:      FDB = self._fcnMddUp
      elif iEvt == MDD_DOWN:  FDB = self._fcnMddDown
      elif iEvt == SVC_UP:    FDB = self._fcnSvcUp
      elif iEvt == SVC_DOWN:  FDB = self._fcnSvcDown
      else:                   FDB = []
      FDB += [ [ fcn, evtQ ] ]

## \cond
   ########################
   # Called asynchronously when we connect or disconnect from rtEdgeCache3.
   #
   # We override this method to notify all functions registered via Bind()
   #
   # @param msg - Textual description of connect event
   # @param bUP - True if UP; False if DOWN
   # @see Bind()
   ########################
   def OnConnect( self, msg, bUP ):
      if bUP: FDB = self._fcnMddUp
      else:   FDB = self._fcnMddDown
      for ( fcn, evtQ ) in FDB:
         if evtQ: evtQ.AddEvent( msg, fcn )
         else:    fcn( msg )
      return

   ########################
   # Called asynchronously when real-time publisher changes state (goes UP
   # or DOWN) in the rtEdgeCache3.
   #
   # We override this method to notify all functions registered via Bind()
   #
   # @param svc - Service name (e.g., bloomberg)
   # @param bUP - True if UP; False if DOWN
   # @see Bind()
   ########################
   def OnService( self, svc, bUP ):
      if bUP: FDB = self._fcnSvcUp
      else:   FDB = self._fcnSvcDown
      for ( fcn, evtQ ) in FDB: 
         if evtQ: evtQ.AddEvent( svc, fcn )
         else:    fcn( svc )
      return

   ########################
   # Called asynchronously when market data arrives from MDDirect
   #
   # @param mddMsg - libMDDirect.rtEdgeData
   # @see libMDDirect.rtEdgeData
   ########################
   def OnData( self, mddMsg ):
      rec = self._GetRecord( mddMsg )
      if rec:
         rec.OnData( mddMsg )
      return

   ########################
   # Called asynchronously when the real-time data stream becomes DEAD
   #
   # @param mddMsg - libMDDirect.rtEdgeData
   # @param msg - Error message
   # @see libMDDirect.rtEdgeData
   ########################
   def OnDead( self, mddMsg, err ):
      rec = self._GetRecord( mddMsg )
      if rec:
         rec.OnDead( mddMsg )
      return

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
   # @param schema - libMDDirect.rtEdgeSchema
   # @see libMDDirect.rtEdgeSchema
   ########################
   def OnSchema( self, schema ):
      self._schema = Schema( schema )

   ########################
   # Get GLrecord from libMDDirect.rtEdgeData contents
   #
   # @param mddMsg - libMDDirect.rtEdgeData
   # @return GLrecord if found; None if not
   ########################
   def _GetRecord( self, mddMsg ):
      #
      # 1) Stats
      #
      self._nMsg += 1
      self._tMsg  = time.time()
      if mddMsg.IsActive():
         self._nUpd += 1
         self._tUpd  = self._tMsg
      #
      # 2) Search and find
      #
      rec = self.GetRecByOid( mddMsg.UserArg() )
      if not rec:
         rec = self.GetRecord( mddMsg.Service(), mddMsg.Ticker() )
      return rec

## \endcond
