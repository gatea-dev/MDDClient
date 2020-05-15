#!/bin/bash

HOSTS=localhost:14321,localhost:14322
EXE=./bin64/LogUsage
clear ; ${EXE}  ${HOSTS} 6120 "Mr Username" 250000 Column1 "Col1 Values"
