
echo off
cls

rem ##################################
rem SubTest.exe DUMP <edgHost>:<edgPort> <edgUser> <Service> <Ticker1>[,<Ticker2>,...]
rem ##################################

rem SubTest.exe DUMP localhost:9998 ATSETLIN-W7-55S$ BLOOMBERG "DAX INDEX"
rem SubTest.exe DUMP localhost:9998 EC-LIVEDATA$ BLOOMBERG "DAX INDEX"

rem SubTest.exe DUMP localhost:9998 ACDEBACA-W7-Z6S$ BLOOMBERG "DAX INDEX"
rem SubTest.exe DUMP localhost:9998 EC-LIVEDATA$ BLOOMBERG "EUR CURNCY"

SubTest.exe DUMP localhost:9998 SubTest32 PUB A,B,C,D,E,F,G

