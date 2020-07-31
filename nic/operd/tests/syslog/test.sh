#!/bin/sh

WS_TOP="/sw"
TOPDIR="/sw/nic"
BUILD_DIR=${TOPDIR}/build/x86_64/apulu/capri/

export OPERD_REGIONS="./operd-regions.json"

pushd ${TOPDIR}

make PIPELINE=apulu operd-test-syslog-logger.bin operd.bin operdctl.bin

RET=$?
if [ $RET -ne 0 ]
then
    echo "Build failed"
    exit $RET
fi

popd

sudo yum install -y syslog-ng

rm -rf /dev/shm/syslog*
rm -rf ./syslog-ng.out

${BUILD_DIR}/bin/operd ./operd-daemon.json ./operd-decoders.json &
OPERD_PID=$!

syslog-ng -f /sw/nic/operd/tests/syslog/syslog-ng.conf -Fedv -R \
          /sw/nic/operd/tests/syslog/syslog-ng.persist &
SYSLOGNG_PID=$!

sleep 3

${BUILD_DIR}/bin/operdctl syslog syslog 127.0.0.1 5514 rfc

${BUILD_DIR}/bin/operd-test-syslog-logger test-rfc

sleep 5

${BUILD_DIR}/bin/operdctl syslog syslog 127.0.0.1 6514 bsd

${BUILD_DIR}/bin/operd-test-syslog-logger test-bsd

sleep 5

cat syslog-ng.out
grep -c "localhost operd: test-rfc" syslog-ng.out
RED=$?
if [ $RET -ne 0 ]
then
    echo "Didn't find expected string in logs"
    exit $RET
fi

grep -c "localhost operd: test-bsd" syslog-ng.out
RED=$?
if [ $RET -ne 0 ]
then
    echo "Didn't find expected string in logs"
    exit $RET
fi

kill -9 $OPERD_PID $SYSLOGNG_PID

