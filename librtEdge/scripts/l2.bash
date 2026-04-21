#!/bin/bash

SVC=zero.hash
LVL=25
TBL=-7026:13,2547:10:5,2497:10:2,2472:10:2,2522:10:5,-7001:13
if [ -z "${1}" ]; then
   echo "Usage : ${0} <ticker> [<NumLvl> [<Service>]]; Exitting ..."
   exit
fi
TKR=${1}
if [ -n "${2}" ]; then
   LVL=${2}
   if [ -n "${3}" ]; then
      SVC=${3}
      TBL=2498:10:2,6715:10,730:10:5,436:10:2,441:10:2,735:10:5,6700:10,2473:10:2
   fi
fi

mddSubscribe -u shaffer -s ${SVC} -t ${TKR} -table ${TBL} -level2 ${LVL}
