import os
from distutils.core import setup, Extension

## Makefile-ish

INC_DIR  = [ './inc' ]
LIB_DIR  = [ '/usr/lib64' ]
LIBS     = [ 'expat' ]
for pLib in [ 'mddWire', 'rtEdge' ]:
   MDD_ROOT = '../../'
   INC_DIR += [ '%s/lib%s/inc' % ( MDD_ROOT, pLib ) ]
   LIB_DIR += [ '%s/lib%s/lib' % ( MDD_ROOT, pLib ) ]
   LIBS    += [ '%s64' % pLib ]
SRCS        = [ './src/Book.cpp',
                './src/EventPump.cpp',
                './src/LVC.cpp',
                './src/RecCache.cpp',
                './src/SubChannel.cpp',
                './src/PyAPI.cpp',
                './src/version.cpp' ]
 
MDDirect = Extension( 'MDDirect',
                       include_dirs       = INC_DIR,
                       library_dirs       = LIB_DIR,
                       libraries          = LIBS,
                       sources            = SRCS )

setup( name        = 'MDDirect', 
       version     = '1.0', 
       ext_modules = [ MDDirect ] )
