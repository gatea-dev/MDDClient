#!/bin/bash

PYTHON=python
if [ -n "${1}" ]; then
   PYTHON=python3
fi

clear
/bin/rm -rfv ./build
SETUP=setup.py
echo "${PYTHON} linux64//${SETUP} build"
${PYTHON} linux64//${SETUP} build
mkdir build/bin64
mv -v build/lib*/*.so build/bin64
