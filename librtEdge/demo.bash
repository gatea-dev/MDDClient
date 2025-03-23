#!/usr/bin/bash

FX=dji
SVC=factset
if [ -n "$1" ]; then
  FX=$1
  if [ -n "$2" ]; then
    SVC=$2
  fi
fi

FLDS="6:10:2,30:8,22:10:2,25:10:2,31:8,36:8:2,"
mddSubscribe -u gatea@gatea.com -table ${FLDS} -s ${SVC} -t /tmp/${FX}.tkr
