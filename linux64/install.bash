#!/bin/bash

# MDD Client

GIT_HOME=`pwd`

## MDD Tools

for EXE in `ls bin64`
do
   echo "Installing ${EXE}"
   cp -p ${GIT_HOME}/bin64/${EXE}* ${HOME}/bin/MDDTools/bin64
done
