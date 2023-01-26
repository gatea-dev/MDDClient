#!/bin/bash

## Configurable

PID=`pgrep SplineMaker`
if [ -n "$PID" ]; then
   pkill -e SplineMaker 
   sleep 1
fi

/bin/rm -f ./nohup.out
## SplineMaker /tmp/fumunder.xml > /tmp/log_splineMaker.log 
nohup SplineMaker ./scripts/cfg/spline.xml > /tmp/log_splineMaker.log  &
