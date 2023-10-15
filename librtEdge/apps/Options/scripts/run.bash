#!/bin/bash

clear
/bin/rm -fv ./core.*
./bin64//OptionsCurve \
   -db     /RAMDISK//LVC//RUT3K.lvc \
   -svr    localhost:9015 \
   -svc    options.curve \
   -rate   15.0 \
   -xInc   1.0 \
   -yInc   1.0 \
   -maxX   1000 \
   -square false
