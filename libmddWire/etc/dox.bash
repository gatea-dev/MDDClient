#!/bin/bash

doxygen etc/libmddWire.dox
cd doc
tar cvf /tmp/w.tar html
sudo /disk2/html/JibJab/.etc/libmddWire.bash
