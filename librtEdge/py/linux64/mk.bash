#!/bin/bash

clear
/bin/rm -rfv ./build
SETUP=setup.py
echo "python linux64//${SETUP} build"
python linux64//${SETUP} build
