
clear
PACK="-packed false"
TXQ=512
if [ -n "$1" ]; then
   TXQ=$1
fi

ARGS="-h localhost:9997 -s mdm -pub 1.0 -run 36000"
ARGS="${ARGS} -circBuf true -full true ${PACK} ${XON}"
ARGS="${ARGS} -txQ ${TXQ}"
echo "./bin64//Publish ${ARGS}"
./bin64//Publish ${ARGS}
