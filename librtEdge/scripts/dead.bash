
clear

SVC=mdm
SVC="bidfx|bunny"
ERR="bad bunny"
if [ -n "$1" ]; then
   ERR=$1
fi

EDG3=localhost:9012
echo "./bin64//Publish -h ${EDG3} -s ${SVC} -pub 1.0 -run 3600 -x ${ERR}"
./bin64//Publish -h ${EDG3} -s ${SVC} -pub 1.0 -run 3600 -x "${ERR}"
