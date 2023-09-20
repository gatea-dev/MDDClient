#!/usr/bin/python
#################################################################
#
#  BBDailyStats.py
#     Dump BBDailyStats
#
#  REVISION HISTORY:
#     20 SEP 2022 jcs  Created
#
#  (c) 1994-2023, Gatea Ltd.
#################################################################
import sys
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
   # <statFile>
   #
   Log( libMDDirect.Version() )
   try:    statFile = sys.argv[1]
   except: statFile = 'feed.bloomberg.01.stats'
   Log( 'BBDailyStats from %s' % statFile )
   bb = libMDDirect.BBDailyStats( statFile )
   print( bb.Dump() )
   Log( 'Done!!' )
