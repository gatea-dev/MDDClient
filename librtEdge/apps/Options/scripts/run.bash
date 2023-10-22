#!/bin/bash

clear

./scripts/stop.bash

/bin/rm -fv ./core.* ./nohup.out
nohup ./bin64//OptionsCurve \
   -fg       false \
   -db       /RAMDISK//LVC//RUT3K.lvc \
   -svr      localhost:9015 \
   -svc      options.curve \
   -rate     30.0 \
   -xInc     1.0 \
   -yInc     1.0 \
   -maxX     1000 \
   -square   false \
   -dump     false \
   -surfCalc true \
   -log      /tmp/OptionsCurve.log &
