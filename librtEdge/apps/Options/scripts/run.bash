#!/bin/bash

clear
CFG=curve
if [ -n "$1" ]; then
   CFG=anees
fi

./scripts/stop.bash
/bin/rm -fv ./core.* ./nohup.out
nohup ./bin64//OptionsCurve ./scripts/cfg/${CFG}.xml &
