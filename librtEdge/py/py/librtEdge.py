#!/usr/bin/python
#################################################################
#
#  librtEdge.py
#     MDDirect add-in driver
#
#  REVISION HISTORY:
#      3 APR 2019 jcs  Created.
#     18 NOV 2020 jcs  rtEdgeSchema
#
#  (c) 1994-2020, Gatea Ltd.
#################################################################
try:    ## Python 3.x
    import MDDirect64 as MDDirect
    raw_input = input
except: ## Python 2.x
    import MDDirect
import math, time, threading

_UNDEF = 'Undefined'


#################################
# Globals
#################################
def Version():
   """Version() is self-explanatory
"""
   return MDDirect.Version()


######################################
#                                    #
#   r t E d g e S u b s c r i b e r  #
#                                    #
######################################
class rtEdgeSubscriber( threading.Thread ):
   """Subscription channel from an MD-Direct data source - rtEdgeCache3 or 
Tape File.

The 1st argument to Start() defines your data source as follows:

   1st Arg    | Data Source     Data Type
   ---------- | ------------- | ----------------------
   host:port  | rtEdgeCache3  | Streaming real-time data
   filename   | Tape File     | Recorded market data from tape

This class ensures that data from both sources - rtEdgeCache3 and Tape File - 
is streamed into your application in the exact same manner using the following 
API calls:

    + Subscribe()
    + OnData()
    + OnDead()
    + Unsubscribe()

The Tape File data source is specifically driven from this class as follows:
   API              | Action
   ----------------   -------------------------
   StartTape()      | Pump data for Subscribe()'ed tkrs until end of file
   StartTapeSlice() | Pump data for Subscribe()'ed tkrs in a time interval
   StopTape()       | Stop Tape Pump
"""
   #################################
   # Constructor
   #################################
   def __init__( self ):
      threading.Thread.__init__( self )
      self._run    = True
      self._cxt    = None
      self._schema = None
      self._msg    = rtEdgeData()
      self._ready  = threading.Event()


   #################################
   # Operations
   #################################
   def Version( self ):
      """rtEdgeSubscriber.Version() is self-explanatory
"""
      return MDDirect.Version()

   def Start( self, svr, usr, bBin ):
      """rtEdgeSubscriber.Start()

      \param svr - host:port of rtEdgeCache3 server to connect to
      \param usr - rtEdgeCache3 username 
      \param bBin - True for binary protocol 
"""
      if not self._cxt:
         self._cxt = MDDirect.Start( svr, usr, bBin )
      self.start()
      self._ready.wait()

   def Stop( self ):
      self._run = False
      self.join()
      if self._cxt:
         MDDirect.Stop( self._cxt )
      self._cxt = None

   def IsTape( self ):
      return MDDirect.IsTape( self._cxt )

   def SetTapeDirection( self, bTapeDir ):
      """rtEdgeSubscriber.SetTapeDirection() sets the tape direction to be 
forward (bTapeDir = False), or reverse (bTapeDir = True).  Must be sent into 
MDDirect.pyd as integer
"""
      if bTapeDir: iDir = 1
      else:        iDir = 0
      return MDDirect.SetTapeDir( self._cxt, iDir )

   def SnapTape( self, svc, tkr, flds, maxRow, tmout=2.5, t0=None, t1=None ):
      """rtEdgeSubscribe.SnapTape() snaps full tape or slice of tape for a 
single ( Service, Ticker ) stream.  If slice, the Tape Slice Start / End times 
is formatted as [YYYY-MM-DD] HH:MM[:SS.mmm]

      \param svc = Service Name
      \param tkr = Ticker Name
      \param flds = CSV list of field ID's or names
      \param maxRow = Max number rows to return
      \param tmout = Timeout in secs
      \param t0 - Tape Slice Start; None for all ticks
      \param t1 - Tape Slice End; None for all ticks
      \return [ [ ColHdr1, ColHdr2, ... ], [ row1 ], [ row2 ], ... ]
"""
      cxt = self._cxt
      if t0 and t1:
         return MDDirect.SnapTape( cxt, svc, tkr, flds, maxRow, tmout, t0, t1 )
      return MDDirect.SnapTape( cxt, svc, tkr, flds, maxRow, tmout )

   def QueryTape( self ):
      return MDDirect.QueryTape( self._cxt )

   def PumpTape( self, t0=None, t1=None ):
      """rtEdgeSubscriber.PumpTape() pumps full tape or slice of tape where 
the Tape Slice Start / End times is formatted as [YYYY-MM-DD] HH:MM[:SS.mmm]

      \param t0 - Tape Slice Start; None for all ticks
      \param t1 - Tape Slice End; None for all ticks
"""
      return MDDirect.PumpTape( self._cxt, t0, t1 )

   def Subscribe( self, svc, tkr, uid ):
      """rtEdgeSubscriber.Subscribe() opens a subscription stream for the 
( svc, tkr ) data stream.  Market data updates are returned in the OnData() 
asynchronous call.

Returns unique non-zero stream ID on success
"""
      return MDDirect.Open( self._cxt, svc, tkr, uid )

   def Unsubscribe( self, svc, tkr ):
      """rtEdgeSubscriber.Unsubscribe() closes a subscription stream for the 
( svc, tkr ) data stream.  Market data updates are stopped.
"""
      return MDDirect.Close( self._cxt, svc, tkr )


   #################################
   # rtEdgeSubscriber Notifications
   #################################
   def OnConnect( self, msg, bUP ):
      """rtEdgeSubscriber.OnConnect() is called asynchronously when we 
connect or disconnect from rtEdgeCache3.

Override this method to take action when you connect or disconnect from 
rtEdgeCache3.
"""
      pass

   def OnService( self, svc, bUP ):
      """rtEdgeSubscriber.OnService() is called asynchronously when a 
real-time publisher changes state (goes UP or DOWN) in the rtEdgeCache3.

Override this method in your application to take action when a new publisher 
goes online or offline. The library transparently re-subscribes any and all streams you have Subscribe()'ed to when the service comes back UP.
"""
      pass

   def OnData( self, mddMsg ):
      """rtEdgeSubscriber.OnData() is called asynchronously when real-time 
market data arrives on this subscription channel from rtEdgeCache3.

Override this method in your application to consume market data.
"""
      pass

   def OnDead( self, mddMsg, msg ):
      """rtEdgeSubscriber.OnDead() is called asynchronously when real-time 
market data stream opened via Subscribe() becomes DEAD.

Override this method in your application to consume market data stream dead.
"""
      pass

   def OnRecovering( self, mddMsg, msg ):
      """rtEdgeSubscriber.OnRecovering() is called asynchronously when real-time 
market data stream is recovering

Override this method in your application to process recovering notification.
"""
      pass

   def OnStreamDone( self, msg ):
      """rtEdgeSubscriber.OnStreamDone() is called asynchronously when real-time 
market data stream from tape is complete.

Override this method in your application to process end of tape notification.
"""
      pass

   def OnSchema( self, schema ):
      """rtEdgeSubscriber.OnSchema() is called asynchronously when data 
dictionary arrives on this publication channel from rtEdgeCache3.

Override this method in your application to process the schema.
"""
      pass


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
            print 'OnByteStream( %s,%s )' % ( svc, tkr )
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



######################################
#                                    #
#             L V C                  #
#                                    #
######################################
class LVC:
   """Read-only view on Last Value Cache (LVC) file

MEMBER VARIABLES:
   _cxt    : LVC Context from MDDirect library
   _schema : rtEdgeSchema
"""
   #################################
   # Constructor
   #################################
   def __init__( self ):
      self._cxt    = None
      self._schema = None


   #################################
   # Access
   #################################
   def GetFieldID( self, name ):
      """LVC.GetFieldID() returns FID from name in Schema
to be a stringified number
"""
      ddb = self._schema
      if ddb:
         return ddb.GetFieldID( name )
      return 0

   def GetFieldName( self, fid ):
      """LVC.GetFieldName() returns field name from FID in Schema
"""
      ddb = self._schema
      if ddb:
         return ddb.GetFieldName( fid )
      return 0


   #################################
   # Operations
   #################################
   def Open( self, file ):
      """LVC.Open() opens the read-only LVC file
"""
      ddb = self._schema
      if not self._cxt:
         self._cxt    = MDDirect.LVCOpen( file )
         self._schema = rtEdgeSchema( MDDirect.LVCSchema( self._cxt ) )
      return

   def GetTickers( self ):
      """LVC.GetTickers() returns a list of [ Service, Ticker ] tuples of 
all ticker in the LVC.
"""
      return MDDirect.LVCGetTickers( self._cxt )

   def Snap( self, svc, tkr ):
      """LVC.Snap() returns an rtEdgeData struct filled w/ LVC data for 
the ( svc, tkr ) ticker
"""
      blob = MDDirect.LVCSnap( self._cxt, svc, tkr )
      rtn  = None
      if blob:
         rtn = rtEdgeData( self._schema )
         rtn._SetData( blob[0], blob[1], blob[2:] )
      return rtn

   def Close( self ):
      """LVC.Close() closes the file opened via LVC.Open()
"""
      if self._cxt:
         MDDirect.LVCClose( self._cxt )
      self._cxt = None


######################################
#                                    #
#      r t E d g e D a t a           #
#                                    #
######################################
class rtEdgeData:
   """A single market data update from the rtEdgeSubscriber channel or LVC

This class is reused by rtEdgeSubscriber (and LVC). When you receive it in 
rtEdgeSubscriber.OnData(), it is volatile and only valid for the life of 
the callback.
"""
   #################################
   # Constructor
   #################################
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


   #################################
   # Operations
   #################################
   def MsgTime( self ):
      """rtEdgeData.MsgTime() returns stringified msg time
"""
      t   = self._tUpd
      lt  = time.localtime( t )
      tMs = int( math.fmod( t * 1000, 1000 ) )
      rc  = '%04d-%02d-%02d'  % ( lt.tm_year, lt.tm_mon, lt.tm_mday )
      rc += ' %02d:%02d:%02d' % ( lt.tm_hour, lt.tm_min, lt.tm_sec )
      rc += '.%03d' % tMs
      return rc

   def forth( self ):
      """rtEdgeData.forth() advances the iterator to the next field, returning 
field()
"""
      self._itr += 1
      return field()

   def field( self ):
      """rtEdgeData.field() returns the current rtEdgeField in the iteration 
or None if end of iteration 
"""
      fdb = self._flds
      nf  = len( fdb )
      rc  = None
      itr = self._itr
      if ( 0 <= _itr ) and ( _itr < nf ):
         ( fid, val, ty ) = fdb[itr]
         rc               = self._fld
         rc._Set( fid, val, ty, self._FieldName( fid ) )
      return rc

   def GetField( self, reqFid ):
      """rtEdgeData.GetField() returns specific field by ID
"""
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

   def GetError( self ):
      """rtEdgeData.GetError() is self-explanatory
"""
      return self._err

   def Dump( self ):
      """rtEdgeData.Dump() dumps message contents as a string
"""
      fdb  = self._flds
      fld  = self._fld
      s    = []
      for ( fid, val, ty ) in fdb:
         fld._Set( fid, val, ty, self._FieldName( fid ) )
         pn = fld.Name()
         pv = fld.GetAsString()
         s += [ '   [%04d] %-14s : %s' % ( fld.Fid(), pn, pv ) ]
      return '\n'.join( s )


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



######################################
#                                    #
#       r t E d g e F i e l d        #
#                                    #
######################################
class rtEdgeField:
   """A single Field from a rtEdgeData out of the rtEdgeSubscriber channel.

When pulling an rtEdgeField from an incoming rtEdgeData, this object is 
reused. The contents are volatile and only valid until the next call to 
rtEdgeData.forth().
"""
   #################################
   # Constructor
   #################################
   def __init__( self ):
      self._fid  = 0
      self._val  = ''
      self._type = MDDirectEnum._MDDPY_STR
      self._name = _UNDEF


   #################################
   # Access
   #################################
   def Fid( self ):
      """rtEdgeField.Fid() returns field ID
"""
      return self._fid

   def Name( self ):
      """rtEdgeField.Name() returns field name
"""
      return self._name

   def Type( self ):
      """rtEdgeField.Type() returns field type from the set of 
[ _MDDPY_INT, _MDDPY_DBL, _MDDPY_STR, ... ]
"""
      return self._type

   def GetAsInt( self ):
      """rtEdgeField.GetasInt() returns value as integer regardless 
of (native) data type
"""
      return int( self.GetAsDouble() )

   def GetAsDouble( self ):
      """rtEdgeField.GetasDouble() returns value as double regardless 
of (native) data type
"""
      try:    dv = float( self._val )
      except: dv = 0.0
      return dv

   def GetAsString( self, nDec=0 ):
      """rtEdgeField.GetasString() returns value as string regardless 
of (native) data type
"""
      rc = str( self._val )
      if nDec and rc.count( '.' ):
         ix = rc.index( '.' )
         rc = rc[:ix+nDec+1]
      return rc


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




######################################
#                                    #
#       r t E d g e S c h e m a      #
#                                    #
######################################
class rtEdgeSchema:
   """Schema : FID-to-Name and Name-to-FID

MEMBER VARIABLES:
   _byFid  : { fid1 : name1, fid2 : name2, ... }
   _byName : { name1 : fid1, name2 : fid2, ... }
"""
   #################################
   # Constructor
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
   # Access
   #################################
   def Size( self ):
      """rtEdgeSchema.Size() returns number of fields in Schema
"""
      return len( self._byFid )

   def GetFIDs( self, bSort=True ):
      """rtEdgeSchema.GetFIDs() returns a list of FIDs, optionally sorted.
"""
      rc = [] + self._byFid.keys()
      if bSort:
         rc.sort()
      return rc

   def GetFieldID( self, name ):
      """rtEdgeSchema.GetFieldID() returns FID from name, allowing for the 
name to be a string-ified number
"""
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

   def GetFieldName( self, fid ):
      """rtEdgeSchema.GetFieldeName() returns field name from FID
"""
      try:    i32 = int( fid )
      except: i32 = 0
      fdb = self._byFid
      try:    name = fdb[i32]
      except: name = fid
      return name



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
