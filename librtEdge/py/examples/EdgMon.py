#!/usr/bin/python
#################################################################
#
#  EdgMon.py
#     MDD.MONITOR.EDGE.exe
#
#  REVISION HISTORY:
#     21 JAN 2025 jcs  Created
#
#  (c) 1994-2025, Gatea Ltd.
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
   # Args : <stats_file> [<tInterval>]
   #
   try:    file = sys.argv[1]
   except: file = './STATS/MDDirectMon.stats'
   st = libMDDirect.EdgMon( file )
   st.SnapAndDump()
   st.Close()
   Log( 'Done!!' )
