#!/bin/bash

clear

TKRS=I:DJI
SVC=ULTRA3
if [ -n "$1" ]; then
   TKRS=$1
   if [ -n "$2" ]; then
      SVC=$2
   fi
fi

./bin64/Subscribe gatea.com:9999 tunahead ${SVC} ${TKRS} 
