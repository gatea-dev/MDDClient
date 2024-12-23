#!/bin/bash

while [ 1 ] ; do
   date; ./scripts/stop.bash
   for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
   do
      nohup mddSubscribe -u SOAK${i} -s idn_rdf -t /tmp/DJI.tkr -r 30 -csvF 5,22,25 > ./SOAK${i}.out &
   done
   sleep 15
   /bin/rm -v ./SOAK*.out
done
