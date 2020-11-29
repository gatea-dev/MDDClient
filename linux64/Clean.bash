#!/bin/bash

# 1) Us

clear
GHOME=`pwd`; export GHOME

# 2) Binaries

/bin/rm -f -v -r ${GHOME}/bin64

# 3) MDDClient

for LIB in libmddWire librtEdge
do
   echo ${LIB}
   cd ${GHOME}/${LIB}
   make -f Makefile64 clean
done
