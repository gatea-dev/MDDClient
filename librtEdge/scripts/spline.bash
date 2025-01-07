#!/bin/bash

## Configurable

pkill -e -u ${USER} SplineMaker
if [ -n "$1" ]; then
   exit
fi

/bin/rm -f ./nohup.out
## nohup ./bin64/SplineMaker ./scripts/cfg/all.xml > /tmp/log_SplineMkr.log  &
nohup SplineMaker ./scripts/cfg/all.xml > /tmp/log_SplineMkr.log  &
