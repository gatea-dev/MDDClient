
echo off

rem Usage : <host:port> <Svc> <tUpdMs> <ChainFile>

cls
PublishCLI.exe localhost:9995 %1% 250 ./chain.tkr

