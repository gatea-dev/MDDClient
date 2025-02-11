#!/bin/bash

clear
/bin/rm -fv ./edg.pcap
echo "tcpdump -i lo port 9994 or port 9998 and host 127.0.0.1 -w ./edg.pcap"
tcpdump -i lo port 9994 or port 9998 and host 127.0.0.1 -w ./edg.pcap 
