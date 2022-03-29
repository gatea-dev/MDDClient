#!/usr/local/bin/python
import sys

def Sum( i0, i1 ):
   r = 0
   for i in range( i0,i1 ): r += i
   return r

try:    i0 = int( sys.argv[1] )
except: i0 = 0
try:    i1 = int( sys.argv[2] )
except: i1 = 1000

n0 = min( i0,i1 )
n1 = max( i0,i1 )
print 'Sum( %d,%d ) = %d' % ( n0, n1, Sum( n0, n1 ) )
