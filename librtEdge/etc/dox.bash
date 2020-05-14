#!/bin/bash

doxygen etc/librtEdge.dox
cd doc
tar cvf /tmp/r.tar html
sudo /disk2/html/JibJab/.etc/librtEdge.bash
