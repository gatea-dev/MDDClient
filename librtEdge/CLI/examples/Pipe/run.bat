
echo off

rem Usage: <SubHost:port> <User> <SubSvc> <PubHost:port> <PubSvc>

cls
set PERF=PARSE_IN_DOTNET REUSE_FIELDS NATIVE_FIELDS 
rem set PERF=PARSE_IN_DOTNET REUSE_FIELDS
rem set PERF=REUSE_FIELDS
set EDGE=localhost
set RDF=IDN_RDF
Pipe.exe %EDGE%:9998 pipe %RDF% %EDGE%:9995 PIPE %PERF%


