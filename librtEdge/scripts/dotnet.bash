#!/bin/bash

ODIR=./dotnet
/bin/rm -rfv ${ODIR} ./swig/librtEdgeCLI_wrap.c
if [ -z "$1" ]; then
   mkdir ${ODIR}
   swig -csharp -I./inc -outdir ${ODIR} ./swig/librtEdgeCLI.i 
fi
