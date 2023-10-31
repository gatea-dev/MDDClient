#!/bin/bash

clear

./scripts/stop.bash
/bin/rm -fv ./core.* ./nohup.out
nohup ./bin64//OptionsCurve ./scripts/cfg/curve.xml &
