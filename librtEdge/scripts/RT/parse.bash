#!/bin/bash

FILE=./edg.pcap
./tcp_pcap_time.py -f ${FILE} -d 9994 -l 1 -t true

if [ -z "$1" ]; then
   ./tcp_pcap_time.py -f ${FILE} -s 9998 -l 1 | awk '{ print $4; }' | sort | uniq -c
else
   echo "./tcp_pcap_time.py -f ${FILE} -s 9998 -d ${1} -l 1 -t true"
   ./tcp_pcap_time.py -f ${FILE} -s 9998 -d ${1} -l 1 -t true
fi
