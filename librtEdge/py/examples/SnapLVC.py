#!/usr/bin/python
#################################################################
#
#  SnapLVC.py
#     Snap LVC Values : <LVC_filename> <Service> <Tickers> <tSnap>
#
#  REVISION HISTORY:
#     20 JAN 2022 jcs  Created
#      3 FEB 2022 jcs  libMDDirect.NumPyObjects()
#     17 FEB 2022 jcs  fids
#     23 FEB 2022 jcs  rtData.Dump( bFldTy )
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

Log( libMDDirect.Version() )

## Hard-Coded Stuff

#######################
# Dump one rtEdgeData
#
# @param rtData : rtEdgeData instance
# @param fids : CSV list of fieds to dump; None for all
# @return string-ified dump of 1 ticker 
#######################
def Dump( rtData, fids ):
   dmp = 'None'
   if rtData:
      if fids:
         dmp = '%s,%s,' % ( rtData._svc, rtData._tkr )
         for fid in fids:
            try:    val = rtData.GetField( fid ).GetAsString()
            except: val = '-'
            dmp += val + ','
      else:
         dmp = rtData.Dump( bFldTy=True )
   return dmp

############################################
#
# main()
#
############################################
if __name__ == "__main__":
   #
   # Args : <LVC_filename> <Service> <Tickers : CSV,file or __ALL__> [<tSnap> <FID1,FID2>]]
   #
   argc = len( sys.argv )
   mem0 = libMDDirect.MemSizeKb()
   py0  = libMDDirect.NumPyObjects()
   if argc < 3:
      help  = sys.argv[0] + ' <LVC_filename> <Service> '
      help += '<Tickers : CSV,file or __ALL__> [<tSnap> <FID1,FID2>]]'
      print( help )
      sys.exit()
   try:    file = sys.argv[1]
   except: file = './cache.lvc'
   try:    svc  = sys.argv[2]
   except: svc  = 'bloomberg'
   try:    tkrs = sys.argv[3]
   except: tkrs = 'AAPL US EQUITY,IBM US EQUITY'
   try:    tSlp = float( sys.argv[4] ) 
   except: tSlp = 0.0
   try:    fids = [ int( f ) for f in sys.argv[5].split(',') ]
   except: fids = None
   try:
      fp  = open( tkrs, 'rb' )
      tdb = [ tkr.strip('\n').strip('\r') for tkr in fp.readlines() ]
      fp.close()
   except:
      tdb = [ tkr.strip('\n').strip('\r') for tkr in tkrs.split(',') ]
   #
   # Open LVC
   #
   bAll = ( tdb[0] == '__ALL__' )
   lvc  = libMDDirect.LVC()
   lvc.Open( file )
   #
   # Rock and Roll
   #
   Log( 'Snapping %d tkrs from %s; Hit <CTRL-C> to exit' % ( len( tdb ), file ) )
   if fids:
      hdr  = 'Service,Ticker,'
      for fid in fids:
         hdr += '%d,' % fid
##         hdr += lvc.GetFieldName( fid ) + ','
      Log( hdr )
   if bAll:
      rdb = lvc.SnapAll()
      print len( rdb )
      [ Log( Dump( rtData, fids ) ) for rtData in rdb ]
   else:
      try:
         run = True
         while run:
            for tkr in tdb:
               Log( Dump( lvc.Snap( svc, tkr ), fids ) )
            run = ( tSlp > 0.0 )
            if run:
               time.sleep( tSlp )
               Log( 'PyObj=%d' % libMDDirect.NumPyObjects() )
            sys.stdout.flush()
      except KeyboardInterrupt:
         Log( 'Shutting down ...' )
   Log( lvc.Close() )
   mem1 = libMDDirect.MemSizeKb()
   py1  = libMDDirect.NumPyObjects()
   Log( 'MEM   : %6d vs %6d Kb  : Leak %6d Kb' % ( mem0, mem1, mem1-mem0 ) )
   Log( 'PyObj : %6d vs %6d obj : Leak %6d obj' % ( py0, py1, py1-py0 ) )
   Log( 'Done!!' )
