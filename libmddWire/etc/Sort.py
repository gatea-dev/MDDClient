#!/usr/local/bin/python
import csv, sys
import GLcommon

try:    rows = csv.reader( open( sys.argv[1], 'rb' ) )
except: sys.exit()
nr = 0
db = []
for row in rows:
   tkr1 = row[0]
   tkr2 = row[4]
   try:
      cor  = float( row[9] )
      db  += [ ( cor, tkr1, tkr2 ) ]
   except:
      pass
nr = len( db )
db.sort()
print 'Most  : %s' % db[:10]
print 'Least : %s' % db[-10:]
