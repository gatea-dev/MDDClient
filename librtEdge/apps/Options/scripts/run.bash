#!/bin/bash

clear
CFG=faang
if [ -n "$1" ]; then
   CFG=$1
fi

./scripts/stop.bash
./scripts/clean.bash
echo "./bin64//OptionsCurve ./scripts/cfg/${CFG}.xml"
nohup ./bin64//OptionsCurve ./scripts/cfg/${CFG}.xml &
