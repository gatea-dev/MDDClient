#!/bin/bash

clear
CFG=anees
if [ -n "$1" ]; then
   CFG=curve
fi

./scripts/stop.bash
/bin/rm -fv ./core.* ./nohup.out
nohup ./bin64//OptionsCurve ./scripts/cfg/${CFG}.xml &
