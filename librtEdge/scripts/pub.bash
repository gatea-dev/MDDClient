
clear
if [ -n "$1" ]; then
   DEAD="-x $1"
else
   DEAD=""
fi

echo "./bin64//Publish -h localhost:9997 -s mdm -pub 1.0 -run 3600 ${DEAD}"
./bin64//Publish -h localhost:9997 -s mdm -pub 1.0 -run 3600 ${DEAD}
