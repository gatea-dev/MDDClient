
echo off

rem Usage : <host:port> <Svc> <PubIntvl> <FieldSize> <NumFlds>

cls
FileSvr.exe localhost:9995 %1% 100 65536 10

