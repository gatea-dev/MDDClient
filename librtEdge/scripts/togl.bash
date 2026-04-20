
clear
SVC=mdm
SVC=mdp-swapcurve
PACK="-packed NO"
LOGU="-logUpd false"
UPD=1.0
if [ -n "$1" ]; then
   UPD=$1
fi

ARGS="-h localhost:9997 -s ${SVC} -pub ${UPD} -tx toggle"
ARGS="${ARGS} -circBuf true ${PACK} ${LOGU}"
echo "./bin64//Publish ${ARGS}"
./bin64//Publish ${ARGS}
