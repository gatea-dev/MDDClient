
echo off

rem <File> <Interval> <PubHost:port> <PubSvc> [ -service ]

cls
rem set STAT=C:\TEMP\MDDirectMon.stats
rem set STAT=C:\Gatea\bbPortal3\MDDirectMon.stats
set STAT=C:\Gatea\Tools\playback\MDDirectMon.stats-bbg
FeedMon.exe %STAT% 1.0 localhost:9995 tunahead
