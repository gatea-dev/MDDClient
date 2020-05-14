#!/bin/sh

EXE=bin64/Subscribe

export __JD_SLEEP=1

VOPT="--time-stamp=yes --track-origins=yes --leak-check=full"
VAL="nohup valgrind --log-file=./val.out ${VOPT}"
${VAL} ./${EXE} localhost:9998 tuna ULTRAFEED CSCO
