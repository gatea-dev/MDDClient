
clear
PACK="-packed YES"
UPD=1.0
if [ -n "$1" ]; then
   UPD=$1
fi

ARGS="-h localhost:9997 -s mdm -pub ${UPD} -run 36000 "
ARGS="${ARGS} -circBuf true -full true ${PACK}"
echo "./bin64//Publish ${ARGS}"
./bin64//Publish ${ARGS}
