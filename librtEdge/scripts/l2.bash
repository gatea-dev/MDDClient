#!/bin/bash

LVL=25
if [ -z "${1}" ]; then
   echo "Usage : ${0} <ticker> [<NumLvl>]; Exitting ..."
   exit
fi
TKR=${1}
if [ -n "${2}" ]; then
   LVL=${2}
fi

TBL=2547:10:4,2497:10:2,2472:10:2,2522:10:4
TBL=-7026:13,2547:10:4,2497:10:2,2472:10:2,2522:10:4,-7001:13
mddSubscribe -u shaffer -s zero.hash -t ${TKR} -table ${TBL} -level2 ${LVL}
