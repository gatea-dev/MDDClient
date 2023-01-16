#!/bin/bash

# MDD Client

GIT_HOME=`pwd`

## MDD Tools

for EXE in `ls bin64/*Debug*`
do
   echo "Installing ${EXE}"
   sudo cp -p ${GIT_HOME}/${EXE}* /usr/local/bin/MDDTools/bin64
done
