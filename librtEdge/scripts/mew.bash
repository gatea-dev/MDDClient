
clear
if [ -n "$1" ]; then
   NPUB=$1
fi

EDG3=localhost:9100
echo "./bin64//Publish -h ${EDG3} -s mdm -pub 1.0 -run 30 -nPub ${NPUB}"
./bin64//Publish -h ${EDG3} -s mdm -pub 1.0 -run 30 -nPub ${NPUB}
