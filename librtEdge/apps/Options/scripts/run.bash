#!/bin/bash

clear
CFG=FAANG
if [ -n "$1" ]; then
   CFG=$1
fi

./scripts/stop.bash
./scripts/clean.bash
/bin/rm -fv ./core.* ./nohup.out
echo "./bin64//OptionsCurve ./scripts/cfg/${CFG}.xml"
nohup ./bin64//OptionsCurve ./scripts/cfg/${CFG}.xml &
