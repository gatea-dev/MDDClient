#!/bin/sh

EXE=bin64/Subscribe

VOPT="--time-stamp=yes --track-origins=yes --leak-check=full"
VAL="nohup valgrind --log-file=./val.out ${VOPT}"
${VAL} ./${EXE} -s mdm -t val -r 3600 -vector YES
