
clear
if [ -n "$1" ]; then
   XON="-tx $1"
else
   XON=""
fi

echo "./bin64//Publish -h localhost:9997 -s mdm -pub 1.0 -run 3600 ${XON}"
./bin64//Publish -h localhost:9997 -s mdm -pub 1.0 -run 3600 ${XON}
