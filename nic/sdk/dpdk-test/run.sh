#!/bin/bash

DUT="XXX_DUT"

display_usage() {
	echo "Usage: run.sh [--skip-setup]"
}

if [[ ($1 == "--help") || $1 == "-h" ]]; then
	display_usage
	exit 0
fi

DATE=`date +%Y-%m-%d`
FW=`cat dpdk/drivers/net/ionic/ionic.h | grep IONIC_DRV_VERSION|cut -f3|cut -c2-|rev|cut -c2-|rev`

mkdir -p results
FILENAME="./results/${DATE}_fw${FW}.log"

echo ""
echo "[>] Cleanup.."
rm -f dts/dep/dpdk.tar.gz

echo ""
echo "[>] Packing DPDK.."
tar -zcf dts/dep/dpdk.tar.gz dpdk

echo ""
echo "[>] Re-running init_dut.sh on $DUT..."
ssh root@$DUT "./init_dut.sh"

echo ""
echo "[>] Starting test.."

# Copy the configuration files
cp -r pen-conf/* dts/

cd dts
if [[ ($1 == "--skip-setup") ]]; then
    echo "Skipping setup..."
    sudo ./dts $@
else
    sudo ./dts $@
fi
cd ..

echo "Writing results in ${FILENAME}"
echo "Test results on ${DATE}"  > ${FILENAME}
echo "Firmware version: ${FW}" >> ${FILENAME}
echo "DTS statistics:"         >> ${FILENAME}
cat dts/output/statistics.txt  >> ${FILENAME}

cat ${FILENAME}
