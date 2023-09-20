#!/usr/bin/python
#################################################################
#
#  MemLVC.py
#     Snap LVC Values : <LVC_filename> <Service> <Tickers> <tSnap>
#
#  REVISION HISTORY:
#      5 SEP 2023 jcs  Created
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
      print( sys.argv[0] + ' <LVC_filename> <Num>' )
      sys.exit()
   try:    file = sys.argv[1]
   except: file = './cache.lvc'
   try:    num  = int( sys.argv[2] )
   except: num  = 10
   #
   # Open LVC
   #
   for i in range( num ):
      lvc  = libMDDirect.LVC()
      lvc.Open( file )
      lvc.Close()
      mem1 = libMDDirect.MemSizeKb()
      py1  = libMDDirect.NumPyObjects()
      Log( '[%02d] MEM   : %6d vs %6d Kb  : Leak %6d Kb' % ( i, mem0, mem1, mem1-mem0 ) )
##      Log( '[%02d] PyObj : %6d vs %6d obj : Leak %6d obj' % ( i, py0, py1, py1-py0 ) )
      mem0 = mem1
      py1  = py0
   Log( 'TOTL MEM   : %6d vs %6d Kb  : Leak %6d Kb' % ( m0, mem1, mem1-m0 ) )
   Log( 'TOTL PyObj : %6d vs %6d obj : Leak %6d obj' % ( p0, py1, py1-p0 ) )
   Log( 'Done!!' )
