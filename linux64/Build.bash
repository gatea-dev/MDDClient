#!/bin/bash

# 1) Required Argument : Build Type

if [ -z "$1" ]; then
   echo "Usage : $0 <Debug|Release>; Exitting ..."
   exit
fi
BLD_TYPE=undefined
if [ "$1" == "Debug" ]; then
   BLD_TYPE=Debug
elif [ "$1" == "Release" ]; then
   BLD_TYPE=Release
else
   echo "Invalid build type ${1}; Must be Debug or Release; Exitting ..."
   exit
fi
echo "BUILD_TYPE = ${BLD_TYPE}"
export BLD_TYPE

# 2) Environment

BIN_DIR=../bin/bin64
GHOME=`pwd`; export GHOME
./linux64/Clean.bash
mkdir -p ./bin64

# 3) MDDClient

for LIB in libmddWire librtEdge
do
   echo ${LIB}
   cd ${GHOME}/${LIB}
   make -f Makefile64
   if [ -f ./cpp/LVCDump.cpp ]; then
      EDG_VER=55
      for TOOL in LVCAdmin LVCDump SplineMaker Subscribe
      do
         make -f Makefile64 ${TOOL}
         mv bin64/${TOOL} ${GHOME}/bin64/${TOOL}.Debug.${EDG_VER}
      done
   fi
   if [ -d ./py/linux64 ]; then
      cd ./py
      ./linux64/Build.bash
      cd ..
      mv py//build/lib.linux-x86_64*/MDDirect27.so ${GHOME}/bin64
   fi
done
