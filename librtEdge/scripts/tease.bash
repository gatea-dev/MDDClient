
clear
SVC=mdm
SVC=mdp-swapcurve
PACK="-packed NO"
LOGU="-logUpd false"
TEASE="-tease true"
TEASE=
UPD=1.0
if [ -n "$1" ]; then
   UPD=$1
fi

ARGS="-h localhost:9997 -s ${SVC} -pub ${UPD} -x tease ${TEASE} "
ARGS="${ARGS} -circBuf true ${PACK} ${LOGU}"
echo "./bin64//Publish ${ARGS}"
./bin64//Publish ${ARGS}
