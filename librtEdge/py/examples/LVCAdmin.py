#!/usr/bin/python
#################################################################
#
#  LVCAdmin.py
#     LVCAdmin tester
#
#  REVISION HISTORY:
#     20 JUL 2022 jcs  Created
#     14 AUG 2023 jcs  Python 2 / 3
#      4 SEP 2023 jcs  Named Schema; args like LVCAdmin.cpp; DelTickers()
#      5 FEB 2025 jcs  Biuld 14: LVCAdmin.OnXxx()
#
#  (c) 1994-2025, Gatea Ltd.
#################################################################
import sys, time
import libMDDirect

_CDB = { True : 'UP',  False : 'DOWN' }
_ADB = { True : 'ADD', False : 'DEL' }

def Log( msg ):
   if msg:
      libMDDirect.Log( msg )
   return

class MyLVCAdmin( libMDDirect.LVCAdmin ):
   ################################
   # Constructor
   #
   # @param adm : host:port of LVC Admin Channel
   ################################
   def __init__( self, adm ):
      libMDDirect.LVCAdmin.__init__( self, adm )

   ########################
   # @override : Called asynchronously when we connect or disconnect
   #
   # @param msg - Textual description of connect event
   # @param bUP - True if UP; False if DOWN
   ########################
   def OnConnect( self, msg, bUP ):
      Log( 'CONN.{%s} : %s' % ( _CDB[bUP], msg ) )

   ########################
   # @override : Called asynchronously when an ACK message arrives
   #  
   # @param bAdd - true if ADD; false if DEL
   # @param svc - Service Name
   # @param tkr - Ticker Name 
   ########################
   def OnAdminACK( self, bAdd, svc, tkr ):
      Log( 'ACK.%s .{%s,%s}' % ( _ADB[bAdd], svc, tkr ) )

   ########################
   # @override : Called asynchronously when an NAK message arrives
   #  
   # @param bAdd - true if ADD; false if DEL
   # @param svc - Service Name
   # @param tkr - Ticker Name 
   ########################
   def OnAdminNAK( self, bAdd, svc, tkr ):
      Log( 'NAK.%s .{%s,%s}' % ( _ADB[bAdd], svc, tkr ) )


############################################
#
# main()
#
############################################
if __name__ == "__main__":
   #
   # -h <host:port> -c <cmd> -s <service> -t <ticker> -x <Schema>
   #
   Log( libMDDirect.Version() )
   argc   = len( sys.argv )
   cmd    = "ADD"
   adm    = "localhost:8775"
   svc    = "bloomberg"
   tkr    = ""
   schema = ""
   if argc < 2:
      s  = [ 'Usage: %s \\' % sys.argv[0],
             '       [ -c  <ADD | DEL | REFRESH> ] \\',
             '       [ -h  <LVC Admin host:port> ] \\',
             '       [ -s  <Service> ] \\',
             '       [ -t  <Ticker : CSV or Filename flat ASCII> ] \\',
             '       [ -x  <Schema Name> ]',
             '   Defaults:',
             '      -c       : %s' % cmd ,
             '      -h       : %s' % adm,
             '      -s       : %s' % svc,
             '      -t       : <empty>',
             '      -x       : <empty>',
           ]
      print( '\n'.join( s ) )
      sys.exit()
   for i in range( 1,argc,2 ):
      sel = sys.argv[i]
      try:    val = sys.argv[i+1]
      except: break ## for-i
      if sel == '-h':   adm    = val
      elif sel == '-s': svc    = val
      elif sel == '-t': tkr    = val
      elif sel == '-x': schema = val
      elif sel == '-c': cmd    = val
   if not adm.count( ':' ): adm += ':8775'
   try:
      fp   = open( tkr, 'rb' )
      tkrs = [ tt.strip('\n').strip('\r') for tt in fp.readlines() ]
      fp.close()
   except:
      tkrs = [ tt.strip('\n').strip('\r') for tt in tkr.split(',') ]
   #
   # Open LVC
   #
   #
   # Rock and Roll
   #
   lvc = MyLVCAdmin( adm )
   Log( 'LVCAdmin to %s' % adm )
   if cmd == 'REFRESH':
      lvc.RefreshTickers( svc, tkrs )
   elif cmd == 'DEL':
      lvc.DelTickers( svc, tkrs, schema )
   elif cmd == 'ADD':
      lvc.AddTickers( svc, tkrs, schema )
   else:
      Log( 'Unknown command %s' % cmd )
      cmd = None
   if cmd:
      Log( '%d tickers %s-ed' % ( len( tkrs ), cmd ) )
   py3 = libMDDirect.IsPY3()
   msg = 'Hit <ENTER> to terminate\n'
   if py3: notUsed = input( msg )
   else:   raw_input( msg )
   Log( lvc.Close() )
   Log( 'Done!!' )
