#!/bin/bash

DELAY=300  ## 5 mins / publisher
if [ -n "$1" ]; then
   DELAY=$1
fi

while [ -z "$2" ]; do
   DLY=${DELAY}
   for i in 1 2 3 4
#   for i in 1 2
   do
      CONN=localhost:999${i}
      SVC=PUB${i}
      DLY=$((${DLY}+${DLY}))
      date; echo "./bin64/Publish  ${CONN} ${SVC} 1.0 ${DLY} /tmp/chain.tkr"
#      for j in 1 2 3 4
      for j in 1 2
      do
         ./bin64/Publish  ${CONN} ${SVC} 1.0 ${DLY} /tmp/chain.tkr & 
      done
   done
   sleep ${DLY}
   date; echo "pkill Publish"
   pkill Publish
   sleep 1
   date; echo "pkill -9 Publish"
   sleep 1
done
