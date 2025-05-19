import os, sys
from distutils.core import setup, Extension

## Makefile-ish

PY3    = ( sys.version_info.major >= 3 ) 
PY3_11 = PY3 and ( sys.version_info.minor >= 11 ) 
if PY3_11: PYLIB = 'MDDirect311'
elif PY3:  PYLIB = 'MDDirect36'
else:      PYLIB = 'MDDirect27'

WARNS    = [ '-Wno-misleading-indentation' ]
INC_DIR  = [ '../inc', './inc', './inc/stats' ]
LIB_DIR  = [ '/usr/lib64' ]
LIBS     = [ 'expat' ]
for pLib in [ 'mddWire', 'rtEdge' ]:
   MDD_ROOT = '../../'
   INC_DIR += [ '%s/lib%s/inc' % ( MDD_ROOT, pLib ) ]
   INC_DIR += [ '%s/lib%s/expat/xmlparse' % ( MDD_ROOT, pLib ) ]
   LIB_DIR += [ '%s/lib%s/lib' % ( MDD_ROOT, pLib ) ]
   LIBS    += [ '%s64' % pLib ]
SRCS        = [ './src/PyAPI.cpp',
                './src/EventPump.cpp',
                './src/LVC.cpp',
                './src/LVCAdmin.cpp',
                './src/PubChannel.cpp',
                './src/RecCache.cpp',
                './src/Stats.cpp',
                './src/SubChannel.cpp',
                './src/version.cpp' ]
 
MDDirect = Extension( PYLIB,
                       include_dirs       = INC_DIR,
                       library_dirs       = LIB_DIR,
                       libraries          = LIBS,
                       extra_compile_args = WARNS,
                       sources            = SRCS )

setup( name        = PYLIB, 
       version     = '1.0', 
       ext_modules = [ MDDirect ] )
