#!/bin/bash

## Configurable

PID=`pgrep SplineMaker`
if [ -n "$PID" ]; then
   pkill -e SplineMaker 
   sleep 1
fi

## SplineMaker /tmp/fumunder.xml > /tmp/log_splineMaker.log 
nohup SplineMaker /tmp/spline.xml > /tmp/log_splineMaker.log  &
