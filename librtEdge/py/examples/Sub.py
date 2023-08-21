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
#
#  (c) 1994-2023, Gatea Ltd.
#################################################################
import sys, time
import libMDDirect

def Log( msg ):
   if msg:
      libMDDirect.Log( msg )
   return

class Sub( libMDDirect.rtEdgeSubscriber ):
   def __init__( self ):
      libMDDirect.rtEdgeSubscriber.__init__( self )

   def OnConnect( self, msg, bUP ):
      Log( 'CONN.{%s} : %s' % ( bUP, msg ) )

   def OnService( self, svc, bUP ):
      Log( 'SVC.{%s} : %s' % ( bUP, svc ) )

   def OnData( self, mddMsg ):
      Log( 'PyObj=%d' % libMDDirect.NumPyObjects() )
      Log( mddMsg.Dump( bFldTy=True ) )

   def OnDead( self, mddMsg, err ):
      Log( 'DEAD : ' + err )

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
   if argc < 2:
      fmt = 'Usage : %s -h <hostname> -s <service> -t <tickers CSV,file or *> '
      print( fmt % sys.argv[0] )
      sys.exit()
   for i in range( 1,argc,2 ):
      cmd = sys.argv[i]
      try:    val = sys.argv[i+1]
      except: break ## for-i
      if cmd == '-h':   svr  = val
      elif cmd == '-s': svc  = val
      elif cmd == '-t': tkrs = val
   if not svr.count( ':' ): svr += ':9998'
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
   sub = Sub()
   sub.Start( svr, sys.argv[0], True )
   [ sub.Subscribe( svc, tkr, 0 ) for tkr in tdb ]
   for tkr in tdb:
      Log( 'Subscribe( %s,%s )' % ( svc, tkr ) )
   #
   # Rock and Roll
   #
   bPy3 = ( sys.version_info.major >= 3 )
   if bPy3: input( 'Hit <ENTER> to quit ...' )
   else:    raw_input( 'Hit <ENTER> to quit ...' )
   run = False
   try:
      while run:
         time.sleep( tSlp )
         sys.stdout.flush()
   except KeyboardInterrupt:
      Log( 'Shutting down ...' )
   sub.Stop()
   Log( 'Done!!' )
