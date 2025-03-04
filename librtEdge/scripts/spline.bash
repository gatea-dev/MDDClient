#!/bin/bash

## Configurable

pkill -e -u ${USER} SplineMaker
CFG=all
CFG=spline
if [ -n "$1" ]; then
   exit
fi

/bin/rm -f ./nohup.out
## nohup ./bin64/SplineMaker ./scripts/cfg/${CFG}.xml > /tmp/log_SplineMkr.log  &
nohup SplineMaker ./scripts/cfg/${CFG}.xml > /tmp/log_SplineMkr.log  &
