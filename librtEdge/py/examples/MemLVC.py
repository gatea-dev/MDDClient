#!/usr/bin/python
#################################################################
#
#  MemLVC.py
#     Snap LVC Values : <LVC_filename> <Service> <Tickers> <tSnap>
#
#  REVISION HISTORY:
#      5 SEP 2023 jcs  Created
#     20 SEP 2023 jcs  GetTickers
#
#  (c) 1994-2023, Gatea Ltd.
#################################################################
import sys, time
import libMDDirect

def Log( msg ):
   if msg:
      libMDDirect.Log( msg )
   return

Log( libMDDirect.Version() )

############################################
#
# main()
#
############################################
if __name__ == "__main__":
   #
   # Args : <LVC_filename>
   #
   argc = len( sys.argv )
   mem0 = libMDDirect.MemSizeKb()
   py0  = libMDDirect.NumPyObjects()
   m0   = mem0
   p0   = py0
   if argc < 3:
      print( sys.argv[0] + ' <LVC_filename> <Num> <GetTicker>' )
      sys.exit()
   try:    file = sys.argv[1]
   except: file = './cache.lvc'
   try:    num  = int( sys.argv[2] )
   except: num  = 10
   try:    bTkr = ( sys.argv[3] != 'as;ldfkjasl;f' )
   except: bTkr = False
   #
   # Open LVC
   #
   fmt = '[%02d] MEM   : %6d vs %6d Kb  : Leak %6d Kb'
   for i in range( num ):
      lvc  = libMDDirect.LVC()
      lvc.Open( file )
      tdb = None
      if bTkr: tdb = lvc.GetTickers()
      else:    lvc.SnapAll()
      lvc.Close()
      mem1 = libMDDirect.MemSizeKb()
      py1  = libMDDirect.NumPyObjects()
      msg  = fmt % ( i, mem0, mem1, mem1-mem0 )
      if tdb:
         msg += '; %d tkrs' % len( tdb )
      Log( msg )
      mem0 = mem1
      py1  = py0
   Log( 'TOTL MEM   : %6d vs %6d Kb  : Leak %6d Kb' % ( m0, mem1, mem1-m0 ) )
   Log( 'TOTL PyObj : %6d vs %6d obj : Leak %6d obj' % ( p0, py1, py1-p0 ) )
   nf   = libMDDirect.MemFree()
   mem1 = libMDDirect.MemSizeKb()
   fmt = 'TOTL MEM   : %6d vs %6d Kb  : Leak %6d Kb; nFree=%d'
   Log( fmt % ( m0, mem1, mem1-m0, nf ) )
   Log( 'Done!!' )
