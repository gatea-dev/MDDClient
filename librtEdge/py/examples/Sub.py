#!/usr/bin/python
#################################################################
#
#  sub.py
#     Subscribe to MDDirect
#
#  REVISION HISTORY:
#     20 JAN 2022 jcs  Created
#      3 FEB 2022 jcs  libMDDirect.NumPyObjects()
#     14 AUG 2023 jcs  Python 2 / 3
#     29 SEP 2023 jcs  Tape
#
#  (c) 1994-2023, Gatea Ltd.
#################################################################
import os, sys, time
import libMDDirect

def Log( msg ):
   if msg:
      libMDDirect.Log( msg )
   return

class Sub( libMDDirect.rtEdgeSubscriber ):
   #################################
   ## Constructor
   #################################
   def __init__( self, tdb ):
      libMDDirect.rtEdgeSubscriber.__init__( self )
      self._nSub  = len( tdb )
      self._nDone = 0

   #################################
   ## @overrides
   #################################
   def OnConnect( self, msg, bUP ):
      Log( 'CONN.{%s} : %s' % ( bUP, msg ) )

   def OnService( self, svc, bUP ):
      Log( 'SVC.{%s} : %s' % ( bUP, svc ) )

   def OnData( self, mddMsg ):
##      Log( 'PyObj=%d' % libMDDirect.NumPyObjects() )
      Log( mddMsg.Dump( bFldTy=True ) )

   def OnSymbol( self, tkr ):
      Log( 'SYM-ADD : %s' % tkr )

   def OnDead( self, mddMsg, err ):
      Log( 'DEAD : ' + err )

   def OnStreamDone( self, sts ):
      self._nDone += 1
      Log( 'STR-DONE [%d of %d]: %s' % ( self._nDone, self._nSub, sts ) )

   #################################
   ## Access
   #################################
   def IsDone( self ):
      return( self._nDone == self._nSub )

############################################
#
# main()
#
############################################
if __name__ == "__main__":
   #
   # -h <hostname> -s <service> -t <tickers CSV,file or *>
   #
   argc = len( sys.argv )
   svr  = 'localhost:9998'
   svc  = 'bloomberg'
   tkrs = 'AAPL US EQUITY,IBM US EQUITY'
   bds  = False
   if argc < 2:
      fmt  = 'Usage : %s -h <hostname> -s <service> -t <tickers CSV,file or *> '
      fmt += '-bds <true|false>'
      print( fmt % sys.argv[0] )
      sys.exit()
   for i in range( 1,argc,2 ):
      cmd = sys.argv[i]
      try:    val = sys.argv[i+1]
      except: break ## for-i
      low = val.lower()
      if cmd == '-h':     svr  = val
      elif cmd == '-s':   svc  = val
      elif cmd == '-t':   tkrs = val
      elif cmd == '-bds': bds  = ( low == 'true' ) or ( low == 'yes' )
   if not os.path.isfile( svr ) and svr.count( ':' ): svr += ':9998'
   try:
      fp  = open( tkrs, 'rb' )
      tdb = [ tkr.strip('\n').strip('\r') for tkr in fp.readlines() ]
      fp.close()
   except:
      tdb = [ tkr.strip('\n').strip('\r') for tkr in tkrs.split(',') ]
   try:    tSlp = float( sys.argv[3] ) 
   except: tSlp = 1.0
   #
   # Subscribe
   #
   Log( libMDDirect.Version() )
   sub = Sub( tdb )
   sub.Start( svr, sys.argv[0], True )
   if bds:
      [ sub.OpenBDS( svc, tkr, 0 ) for tkr in tdb ]
   else:
      [ sub.Subscribe( svc, tkr, 0 ) for tkr in tdb ]
   for tkr in tdb:
      Log( 'Subscribe( %s,%s )' % ( svc, tkr ) )
   #
   # Rock and Roll
   #
   if sub.IsTape():
      Log( 'Pumping tape %s ...' % svr )
      sub.PumpTape()
   if not sub.IsTape():
      Log( 'Hit <CTRL-C> to quit ...' )
   try:
      while not sub.IsDone():
         time.sleep( tSlp )
         sys.stdout.flush()
   except KeyboardInterrupt:
      Log( 'Shutting down ...' )
   sub.Stop()
   Log( 'Done!!' )
