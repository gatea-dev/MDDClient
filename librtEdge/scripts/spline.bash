#!/bin/bash

## Configurable

PID=`pgrep SplineMaker`
if [ -n "$PID" ]; then
   pkill -e SplineMaker 
   sleep 1
fi

/bin/rm -f ./nohup.out
## nohup ./bin64/SplineMaker ./scripts/cfg/spline.xml > /tmp/log_SplineMkr.log  &
nohup SplineMaker ./scripts/cfg/spline.xml > /tmp/log_SplineMkr.log  &
