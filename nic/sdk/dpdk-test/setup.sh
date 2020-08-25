#!/bin/bash

DUT=$1
TESTER=$2

#
# SSH configuration
#
CLEANUP="rm ~/.ssh/known_hosts ~/.ssh/id_rsa ~/.ssh/id_rsa.pub"
GEN="cat /dev/zero | ssh-keygen -t rsa -N ''"
COPY="sshpass -p docker ssh-copy-id -o StrictHostKeyChecking=accept-new"

echo
echo "[>] Configuring SSH..."
ssh-copy-id -i root@$DUT > /dev/null 2>&1
ssh-copy-id -i root@$TESTER > /dev/null 2>&1

ssh root@$DUT "$CLEANUP"
ssh root@$TESTER "$CLEANUP"

ssh root@$DUT "$GEN"
ssh root@$TESTER "$GEN"

ssh root@$DUT "$COPY -i root@$TESTER"
ssh root@$TESTER "$COPY -i root@$DUT"

#
# Check penctl prereq
#
echo
echo "[>] Checking prerequisites..."
PENCTL=`ssh root@$DUT "test -f penctl.linux" ; echo $?`
PENTOK=`ssh root@$DUT "test -f penctl.token" ; echo $?`
if [[ "$PENCTL" != "0" ]]; then
   echo "Error: penctl.linux not found on $DUT, bailing out ($PENCTL)"
   exit 1
fi
if [[ "$PENTOK" != "0" ]]; then
    echo "Error: penctl.token not found on $DUT, bailing out ($PENTOK)"
    exit 1
fi

#
# Tarball
#
DIR=$(dirname "$0")
DIR=$(readlink -f "$DIR")
TBDIR=$(basename $DIR)

echo
echo "[>] Copying $TBDIR archive to $TESTER..."
cd $DIR/..
tar -czf /tmp/dpdk-test-dist.tgz $TBDIR dpdk
scp /tmp/dpdk-test-dist.tgz root@$TESTER:
ssh root@$TESTER "rm -rf $TBDIR ; tar -xzhf dpdk-test-dist.tgz ; mv dpdk dpdk-test/"

echo
echo "[>] Installing drivers-linux-eth on $DUT..."
cd ~/sw
./platform/tools/drivers-linux-eth.sh
scp platform/gen/drivers-linux-eth.tar.xz root@$DUT:
ssh root@$DUT "rm -rf drivers-linux-eth && tar -xvf drivers-linux-eth.tar.xz && cd drivers-linux-eth && ./build.sh"
cd -

#
# Complete setup on Tester
#
echo
echo "[>] Launching setup script on tester host $TESTER..."
ssh root@$TESTER "cd $TBDIR && ./scripts/setup_tester.sh $DUT $TESTER"
