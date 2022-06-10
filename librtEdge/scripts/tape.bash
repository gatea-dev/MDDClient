#!/bin/bash

## Configurable

SVC=erisx
TKR=BCH/BTC.0.L2
T0=08:15
T1=08:20
if [ -n "$1" ]; then
   TKR=$1
   if [ -n "$2" ]; then
      T0=$2
      if [ -n "$3" ]; then
         T1=$3
      fi
   fi
fi

## Hard Coded

TAPE=/home/warp/MDD/gateaReplay/Tapes/erisx.mdd_20220609.1
TAPE=/home/warp/MDD/gateaReplay/Tapes/erisx.mdd

echo "./bin64/Subscribe -h ${TAPE} -s ${SVC} -t ${TKR} -t0 ${T0} -t1 ${T1}"
./bin64/Subscribe -h ${TAPE} -s ${SVC} -t ${TKR} -t0 ${T0} -t1 ${T1}
