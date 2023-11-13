
clear
PACK=""
PACK="-packed NO"
if [ -n "$1" ]; then
   XON="-tx $1"
else
   XON=""
fi

EDG3=localhost:9997
echo "./bin64//Publish -h ${EDG3} -s mdm -pub 1.0 -run 3600 ${PACK} ${XON}"
./bin64//Publish -h ${EDG3} -s mdm -pub 1.0 -run 3600 ${PACK} $XON}
