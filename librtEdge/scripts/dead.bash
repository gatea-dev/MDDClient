
clear

ERR="bad bunny"
if [ -n "$1" ]; then
   ERR=$1
fi

EDG3=localhost:9997
echo "./bin64//Publish -h ${EDG3} -s mdm -pub 1.0 -run 3600 -x ${ERR}"
./bin64//Publish -h ${EDG3} -s mdm -pub 1.0 -run 3600 -x "${ERR}"
