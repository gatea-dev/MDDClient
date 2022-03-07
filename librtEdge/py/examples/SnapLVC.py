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

## Hard-Coded Stuff

_ANSI_HOME  = '\033[1;1H\033[K'
_ANSI_CLEAR = '\033[H\033[m\033[J'
_CURSOR_ON  = '\033[?25h'
_CURSOR_OFF = '\033[?25l'

############################################
#
# main()
#
############################################
if __name__ == "__main__":
   #
   # Args : <LVC_filename> <Service> <Tickers : CSV,file or *> [<tSnap> <FID1,FID2>]]
   #
   argc = len( sys.argv )
   if argc < 3:
      help  = sys.argv[0] + ' <LVC_filename> <Service> '
      help += '<Tickers : CSV,file or *> [<tSnap> <FID1,FID2>]]'
      print help
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
   lvc = libMDDirect.LVC()
   lvc.Open( file )
   if tdb[0] == '*':
      all = lvc.GetTickers()  ## [ [ svc1, tkr1 ], [ svc2, tkr2 ], ... ]
      tdb = [ tkr for ( svc, tkr ) in all ]
   #
   # Rock and Roll
   #
   Log( 'Snapping %d tkrs from %s; Hit <CTRL-C> to exit' % ( len( tdb ), file ) )
   if fids:
      hdr  = 'Service,Ticker,'
      for fid in fids:
         hdr += lvc.GetFieldName( fid ) + ','
      Log( hdr )
   try:
      run = True
      while run:
         for tkr in tdb:
            rtData = lvc.Snap( svc, tkr )
            if rtData:
               if fids:
                  dmp = '%s,%s,' % ( svc, tkr )
                  for fid in fids:
                     try:    val = rtData.GetField( fid ).GetAsString()
                     except: val = '-'
                     dmp += val + ','
               else:
                  dmp = rtData.Dump()
               Log( dmp )
            else:
               Log( '[%s,%s] : No fields' % ( svc, tkr ) )
         run = ( tSlp > 0.0 )
         if run:
            time.sleep( tSlp )
            Log( 'PyObj=%d' % libMDDirect.NumPyObjects() )
         sys.stdout.flush()
   except KeyboardInterrupt:
      Log( 'Shutting down ...' )
   Log( lvc.Close() )
   Log( 'Done!!' )
