#!/usr/bin/python
#################################################################
#
#  sub.py
#     Subscribe to MDDirect
#
#  REVISION HISTORY:
#     20 JAN 2022 jcs  Created
#      3 FEB 2022 jcs  libMDDirect.NumPyObjects()
#
#  (c) 1994-2022, Gatea Ltd.
#################################################################
import sys, time
import libMDDirect

def Log( msg ):
   if msg:
      libMDDirect.Log( msg )
   return

Log( libMDDirect.Version() )

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
   # Args : <Tickers : CSV,file or *> <Service> <Edge3server>
   #
   try:    svr  = sys.argv[3]
   except: svr  = 'localhost:9998'
   try:    svc  = sys.argv[2]
   except: svc  = 'bloomberg'
   try:    tkrs = sys.argv[1]
   except: tkrs = 'AAPL US EQUITY,IBM US EQUITY'
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
   sub = Sub()
   sub.Start( svr, sys.argv[0], True )
   [ sub.Subscribe( svc, tkr, 0 ) for tkr in tdb ]
   for tkr in tdb:
      Log( 'Subscribe( %s,%s )' % ( svc, tkr ) )
   #
   # Rock and Roll
   #
   raw_input( 'Hit <ENTER> to quit ...' )
   run = False
   try:
      while run:
         time.sleep( tSlp )
         sys.stdout.flush()
   except KeyboardInterrupt:
      Log( 'Shutting down ...' )
   sub.Stop()
   Log( 'Done!!' )
