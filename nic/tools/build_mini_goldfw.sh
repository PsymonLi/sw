#!/bin/sh
set -e

pwd | grep -q "\/sw\/nic$" || { echo Please run from "sw/nic" directory; exit -1; }

GIT_REMOTE_NAME="__upstream_buildroot"
TOPDIR=$(dirname `pwd`)
USERNAME=$(id -nu)
BUILDROOT_HASH=`grep buildroot $TOPDIR/minio/VERSIONS | cut -d' ' -f2`

#max mini_goldimg size is 10MiB
max_gold_img_sz=$((0xa00000))

echo "BUILDROOT_HASH is: $BUILDROOT_HASH"

#update_submodule() {
#    cd $TOPDIR/nic/buildroot
#    git remote add $GIT_REMOTE_NAME git@github.com:/pensando/buildroot
#    git fetch $GIT_REMOTE_NAME
#    git checkout $BUILDROOT_HASH
#    return 0
#}

start_shell() {
    cd $TOPDIR/nic
    make docker/background-shell | tail -n 1
}

cleanup() {
    # stop shell
    docker stop $DOCKER_ID
}

check_pen_lib_deps() {
    PATH=$PATH:/tool/toolchain/aarch64-1.1/bin/
    bin_list=`file $TOPDIR/nic/buildroot/output_minigold/target/nic/bin/* $TOPDIR/nic/buildroot/output_minigold/target/platform/bin/* | grep ELF | cut -d':' -f1`
    rm -f /tmp/libs_list.txt /tmp/pen_libs_deps.txt

    for bin in $bin_list 
    do
        aarch64-linux-gnu-readelf -d $bin | grep NEEDED | awk -F':' {'print $2'} >> /tmp/libs_list.txt
    done
    sort -u < /tmp/libs_list.txt > /tmp/pen_libs_deps.txt

    $TOPDIR/nic/tools/check_gold_libs_deps.py -f /tmp/pen_libs_deps.txt -s $TOPDIR/nic/buildroot/output_minigold/target/ > /tmp/missing_libs.txt
}

if [ "x${JOB_ID}" = "x" ] || [ "x${IGNORE_BUILD_PIPELINE}" != "x" ]; then
    trap cleanup EXIT

    echo "Starting docker instance"
    DOCKER_ID=$(start_shell)
    echo Docker_ID $DOCKER_ID

    #sleeping to let pull-asset finish
    sleep 90

    alias docker_exec="docker exec $DOCKER_ID sudo -i -u $USERNAME /bin/bash -c"
    alias docker_root="docker exec $DOCKER_ID /bin/bash -c"
else
    alias docker_exec="/bin/bash -c"
    alias docker_root="/bin/bash -c"
fi

echo 'Replacing make and gmake'
docker_root "cp /bin/make /bin/make_default"
docker_root "cp /bin/gmake /bin/gmake_default"
docker_root "cp /opt/rh/devtoolset-7/root/bin/make /bin/make"
docker_root "cp /opt/rh/devtoolset-7/root/bin/make /bin/gmake"

if [ "x${JOB_ID}" = "x" ] || [ "x${IGNORE_BUILD_PIPELINE}" != "x" ]; then
    echo 'Copying ssh keys into container'
    cp -a ~/.ssh $TOPDIR/
    docker_exec "cp -a /sw/.ssh ~/"
else
    export FORCE_UNSAFE_CONFIGURE=1
fi

echo 'Cleaning old files'
docker_exec "rm -rf /sw/nic/buildroot/output_minigold"

echo 'Generating minigold config'
docker_exec "cd /sw/nic/buildroot && make capri_mini_goldimg_defconfig O=output_minigold"

echo 'Copying u-boot files'
docker_exec "cd /sw/nic/buildroot && mkdir -p output_minigold/images && cp output/capri/images/u-boot* output_minigold/images/"

echo 'Copying fwupdate'
docker_exec "cd /sw/nic && cp tools/fwupdate buildroot/output_minigold/images/"

echo 'Generating sw_whitelist.txt from pack_minigold.txt'
docker_exec "echo nic/etc/VERSION.json > /sw/nic/buildroot/board/pensando/capri-mini-goldimg/sw_whitelist.txt"
docker_exec "cd /sw/nic && tools/pack_to_whitelist.py tools/package/pack_minigold.txt >> buildroot/board/pensando/capri-mini-goldimg/sw_whitelist.txt"

echo 'Building minigold buildroot'
PLATFORM_LINUX_DIR=/sw/nic/buildroot/output/capri/build/platform-linux/
docker_exec "rm -rf $PLATFORM_LINUX_DIR && mkdir -p $PLATFORM_LINUX_DIR && cd $PLATFORM_LINUX_DIR && tar -xf /sw/nic/buildroot/output/capri/build/platform-linux.tar.gz"

#Build the buildroot with LINUX_OVERRIDE
docker_exec "cd /sw/nic/buildroot && PATH=$PATH:/tool/toolchain/aarch64-1.1/bin/ make -j 24 LINUX_OVERRIDE_SRCDIR=$PLATFORM_LINUX_DIR O=output_minigold"

#Replace the make and gmake to default
docker_root "cp /bin/make_default /bin/make"
docker_root "cp /bin/gmake_default /bin/gmake"

echo 'Building Naples minigold firmware'
docker_exec "cd /usr/src/github.com/pensando/sw/nic && PATH=$PATH:/tool/toolchain/aarch64-1.1/bin/ make ARCH=aarch64 PLATFORM=hw clean"
docker_exec "cd /usr/src/github.com/pensando/sw/ && PATH=$PATH:/tool/toolchain/aarch64-1.1/bin/ make ws-tools"
docker_exec "cd /usr/src/github.com/pensando/sw/nic && PATH=$PATH:/tool/toolchain/aarch64-1.1/bin/ make ARCH=aarch64 PIPELINE=iris PLATFORM=hw FWTYPE=gold PKG_TARGET=minigold minigold-firmware"

rm -f /tmp/missing_libs.txt

check_pen_lib_deps
if [ -f "/tmp/missing_libs.txt" ]; then
    set +e
    missing_libs=`grep "lib" /tmp/missing_libs.txt`
    ret=$?
    set -e
else
    echo ""
    echo "Error in checking mini_goldfw libs dependency"
    echo ""
    echo "!!! Goldfw Build Failed !!!"
    exit 1
fi

if [ $ret -eq 0 ]; then
    echo "Some libs are missing in mini_goldfw filesystem"
    echo ""
    echo "Missing libs are: `cat /tmp/missing_libs.txt`"
    echo ""
    echo "!!! Mini Goldfw Build Failed !!!"

    exit 1
fi

echo ""
echo "lib dependency check for mini_goldfw is successful"
echo ""

image_sz=`stat -c %s $TOPDIR/nic/buildroot/output_minigold/images/kernel.img`

if [ $image_sz -gt $max_gold_img_sz ]; then
    echo "Error: MiniGoldFW size($image_sz bytes) is more than allowed size($max_gold_img_sz bytes) for Naples"
    exit 1;
else
    echo "Mini GoldFW is ready under sw/nic/buildroot/output_minigold/images/naples_minigoldfw.tar. Mini Goldfw Size: $image_sz bytes"
    echo 'Please check mini_goldfw sanity before publishing it'

    #Publish the artifacts if RELEASE is non-zero
    if [ -z $RELEASE ]
    then
        echo "RELEASE is not set, return"
        exit 0
    fi

    docker_exec "cd /usr/src/github.com/pensando/sw/asset-build/asset-push && go build"

    DEST="/release-arfifacts/gold"
    mkdir -p $DEST
    mv $TOPDIR/nic/buildroot/output_minigold/images/naples_minigoldfw.tar $DEST/
    asset-build/asset-push/asset-push builds hourly $RELEASE $DEST/naples_minigoldfw.tar

    exit 0
fi

