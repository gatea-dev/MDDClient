#!/usr/bin/python
#################################################################
#
#  LVCAdmin.py
#     LVCAdmin tester
#
#  REVISION HISTORY:
#     20 JUL 2022 jcs  Created
#     14 AUG 2023 jcs  Python 2 / 3
#     24 AUG 2023 jcs  Named Schema; args like LVCAdmin.cpp
#
#  (c) 1994-2023, Gatea Ltd.
#################################################################
import sys, time
import libMDDirect

def Log( msg ):
   if msg:
      libMDDirect.Log( msg )
   return

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
             '       [ -c  <ADD | DEL> ] \\',
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
   lvc = libMDDirect.LVCAdmin( adm )
   Log( 'LVCAdmin to %s' % adm )
   if cmd == 'REFRESH':
      lvc.RefreshTickers( svc, tkrs )
   elif cmd == 'ADD':
      lvc.AddTickers( svc, tkrs, schema )
   else:
      Log( 'Unknown command %s' % cmd )
      cmd = None
   if cmd:
      Log( '%d tickers %s-ed' % ( len( tkrs ), cmd ) )
   Log( lvc.Close() )
   Log( 'Done!!' )
