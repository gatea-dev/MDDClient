
clear
PACK="-packed NO"
LOGU="-logUpd false"
UPD=1.0
if [ -n "$1" ]; then
   UPD=$1
fi

ARGS="-h localhost:9997 -s mdm -pub ${UPD} -run 36000 "
## ARGS="${ARGS} -circBuf true -full true ${PACK}"
ARGS="${ARGS} -circBuf true ${PACK} ${LOGU}"
echo "./bin64//Publish ${ARGS}"
./bin64//Publish ${ARGS}
