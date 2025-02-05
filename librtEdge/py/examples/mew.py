#!/usr/bin/python
import rtEdgeCache2

edg = rtEdgeCache2.GLedgChan( 'localhost:9998', 'tunahead' )
edg.Start()
raw_input( 'Hit <ENTER> to stop: ' )
edg.Stop()
