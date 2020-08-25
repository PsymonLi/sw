#!/bin/bash
#
#
# Usage
# ------
# How to build a new VPP and upload it to minio:
#
# 0a. Have a workspace
# 0b. Know the GIT HASH of the commit in pensando/vpp tree you want to use
# 1. cd to `/sw/nic`
# 2. run `./tools/build_vpp.sh -v <GIT_REPO_PATH:GIT_HASH_OF_VPP>
#    Note that arguments are optional, default is pensando/vpp:pds-master.
#
# Common problems:
# Build failed because libnuma wasn't found?
# - Make sure the asset pull has completed; consider using -w option
#

#Uncomment for debugging script
#set -e

usage() {
    echo "Usage: $0 [OPTION]"
    echo "Options available:"
    echo "    -v <GIT_REPO_PATH:GIT_HASH_OF_VPP>   Default is git@github.com:pensando/vpp, pds-master commit hash"
    echo "    -a <third_party_libs minio version>  minio VERSION to use for asset-upload"
    echo "    -s                                   Skip VPP repo download"
    echo "    -w                                   Wait 120s for asset pull to complete"
    echo "    -k                                   Don't cleanup docker (debug)"
    echo "    -c                                   Cleanup vpp repo"
    exit 1
}

start_shell() {
    cd $NIC_DIR
    make docker/background-shell | tail -n 1
}

cleanup() {
    # stop shell
    [ ! -z "$DOCKER_ID" ] && echo "Stopping docker container" && docker stop $DOCKER_ID
}

get_repo() {
    [ ! -z $SKIP_GET_REPO ] && echo "Skipping VPP download!" && return

    echo "Download VPP Repo"

    cd $TP_DIR
    sudo rm -rf vpp
    git clone $VPP_REPO
    if [[ $? -ne 0 ]]; then
        echo "VPP repo download failed"
        exit 1
    fi
    if [ ! -z "$VPP_COMMIT" ]; then
        cd vpp && git checkout $VPP_COMMIT
        if [[ $? -ne 0 ]]; then
            echo "git checkout failed for VPP"
            exit 1
        fi
    fi

    cd $NIC_DIR
}

pwd | grep -q "\/sw\/nic$" || { echo Please run from "sw/nic" directory; exit -1; }

VPP_REPO="git@github.com:pensando/vpp"

while getopts 'v:d:a:swkch' opt
do
    case $opt in
        v)
            VPP_REPO=${OPTARG%:*}
            VPP_COMMIT=${OPTARG##*:}
            ;;
        a)
            VPP_ASSET_VERSION=${OPTARG}
            ;;
        s)
            SKIP_GET_REPO=1
            ;;
        w)
            WAIT_PULL_ASSETS=1
            ;;
        k)
            SKIP_CLEANUP=1
            ;;
        c)
            CLEANUP_VPP_REPO=1
            ;;
        h | *)
            usage
            ;;
    esac
done

TOP_DIR=$(dirname `pwd`)
NIC_DIR=${TOP_DIR}/nic
SDK_DIR=${NIC_DIR}/sdk
TP_DIR=${SDK_DIR}/third-party
VPP_DIR=${TP_DIR}/vpp
DOCKER_TOP_DIR=/usr/src/github.com/pensando/sw
DOCKER_NIC_DIR=${DOCKER_TOP_DIR}/nic
DOCKER_SDK_DIR=${DOCKER_NIC_DIR}/sdk
DOCKER_VPP_DIR=${DOCKER_SDK_DIR}/third-party/vpp
DOCKER_VPP_PKG_DIR=${DOCKER_SDK_DIR}/third-party/vpp-pkg
USERNAME=$(id -nu)

echo "Input variables:"
printf "%-25s = %-40s\n" "VPP_REPO" "$VPP_REPO"
if [ -z "$VPP_COMMIT" ]; then
    printf "%-25s = %-40s\n" "VPP_COMMIT" "Not specified, using pds-master"
else
    printf "%-25s = %-40s\n" "VPP_COMMIT" "$VPP_COMMIT"
fi
if [ -z "$VPP_ASSET_VERSION" ]; then
    printf "%-25s = %-40s\n\n" "VPP_ASSET_VERSION" "Not specified, skip upload"
else
    printf "%-25s = %-40s\n\n" "VPP_ASSET_VERSION" "$VPP_ASSET_VERSION"
fi

get_repo

echo "Starting docker instance"
DOCKER_ID=$(start_shell)

docker_exec() {
    echo "exec [$1]"
    docker exec -u $USERNAME $DOCKER_ID /bin/bash -c "$1"
}

docker_root() {
    echo "root [$1]"
    docker exec $DOCKER_ID /bin/bash -c "$1"
}

clean_vpp_build() {
    if [[ $CLEANUP_VPP_REPO -eq 1 ]]; then
        echo "Delete VPP repo"
        docker_exec "rm -rf $DOCKER_VPP_DIR"
    fi

    if [[ $SKIP_CLEANUP -eq 1 ]]; then
        echo "Skipped cleanup"
    else
        cleanup
    fi
}

trap clean_vpp_build EXIT

copy_asset() {
    DOCKER_VPP_BUILD="$DOCKER_VPP_DIR/build-root/install-$2-$1/vpp"

    docker_exec "rm -rf $DOCKER_VPP_PKG_DIR/$1"
    docker_exec "mkdir -p $DOCKER_VPP_PKG_DIR/$1/lib $DOCKER_VPP_PKG_DIR/$1/bin"

    if [[ $1 == "aarch64" ]]; then
        # Pull include/ and share/ from aarch64
        docker_exec "command cp -f -L -r $DOCKER_VPP_BUILD/include/* $DOCKER_VPP_PKG_DIR/include/"
        if [[ $? -ne 0 ]]; then
            echo "VPP headers copy failed"
            exit 1
        fi

        docker_exec "command cp -f -L -r $DOCKER_VPP_BUILD/share/* $DOCKER_VPP_PKG_DIR/share/"
        if [[ $? -ne 0 ]]; then
            echo "VPP share copy failed"
            exit 1
        fi
    fi

    docker_exec "command cp -f -L $DOCKER_VPP_BUILD/lib/lib*.so.* $DOCKER_VPP_PKG_DIR/$1/lib/"
    if [[ $? -ne 0 ]]; then
        echo "VPP $1 libs copy failed"
        exit 1
    fi

    docker_exec "command cp -f -L -r $DOCKER_VPP_BUILD/lib/vpp_plugins $DOCKER_VPP_PKG_DIR/$1/lib/"
    if [[ $? -ne 0 ]]; then
        echo "VPP $1 plugins copy failed"
        exit 1
    fi

    docker_exec "command cp -f -L $DOCKER_VPP_BUILD/bin/* $DOCKER_VPP_PKG_DIR/$1/bin/"
    if [[ $? -ne 0 ]]; then
        echo "VPP $1 bin copy failed"
        exit 1
    fi
}

copy_assets() {
    echo "Copying Assets"

    docker_exec "rm -rf $DOCKER_VPP_PKG_DIR/include $DOCKER_VPP_PKG_DIR/share"
    docker_exec "mkdir -p $DOCKER_VPP_PKG_DIR/include $DOCKER_VPP_PKG_DIR/share"

    copy_asset "aarch64" "naples"
    copy_asset "x86_64" "vpp_debug"

    echo "Successfully updated assets in $DOCKER_VPP_PKG_DIR"
}

build_vpp() {
    BUILDTYPE="vpp_build_$1"
    echo "Building VPP - $1, check $BUILDTYPE.log"

    CONFIG="ARCH=$1 SDKDIR=$DOCKER_SDK_DIR DPDK_PATH=$DOCKER_SDK_DIR/build/$1/capri/out/dpdk_submake"
    if [[ $1 == "x86_64" ]]; then
        CONFIG+=" V=1"
    else
        CONFIG+=" PLATFORM=hw RELEASE=1"
    fi

    # Install dependencies
    docker_exec "cd $DOCKER_VPP_DIR && make -f Makefile SDKDIR=$DOCKER_SDK_DIR install_deps &> $DOCKER_NIC_DIR/$BUILDTYPE.log"
    if [[ $? -ne 0 ]]; then
        echo "$BUILDTYPE failed"
        exit 1
    fi

    # Clean
    docker_exec "cd $DOCKER_VPP_DIR && make -f Makefile $CONFIG wipe_vpp &>> $DOCKER_NIC_DIR/$BUILDTYPE.log"
    if [[ $? -ne 0 ]]; then
        echo "$BUILDTYPE failed"
        exit 1
    fi

    # Build; requires libdpdk.so and RTE headers
    docker_exec "cd $DOCKER_VPP_DIR && make -f Makefile $CONFIG all &>> $DOCKER_NIC_DIR/$BUILDTYPE.log"
    if [[ $? -ne 0 ]]; then
        echo "$BUILDTYPE failed"
        exit 1
    fi

    echo "$BUILDTYPE success"
}

build_sdk() {
    BUILDTYPE="sdk_build_$1"
    echo "Building SDK - $1, check $BUILDTYPE.log"

    # Clean
    docker_root "rm -rf $DOCKER_SDK_DIR/build"
    docker_root "umount /sdk 2> /dev/null"

    # Build
    docker_root "mkdir -p /sdk && mount --bind $DOCKER_SDK_DIR /sdk"
    docker_exec "cd /sdk && make ARCH=$1 NICDIR=$DOCKER_NIC_DIR &> $DOCKER_NIC_DIR/$BUILDTYPE.log"
    if [[ $? -ne 0 ]]; then
        echo "$BUILDTYPE failed"
        exit 1
    fi

    echo "$BUILDTYPE success"
}

build_all() {
    build_sdk  "aarch64"
    build_vpp  "aarch64"

    build_sdk  "x86_64"
    build_vpp  "x86_64"
}

upload_assets() {
    sed -i "s/third_party_libs.*/third_party_libs ${VPP_ASSET_VERSION}/" $TOP_DIR/minio/VERSIONS
    if [[ $? -ne 0 ]]; then
        echo "Asset version change failed"
        exit 1
    fi
    echo "Uploading assets with version ${VPP_ASSET_VERSION}"
    docker_exec "cd $DOCKER_TOP_DIR && UPLOAD=1 make create-assets &> $DOCKER_TOP_DIR/asset_upload.txt"
    if [[ $? -ne 0 ]]; then
        echo "Upload-assets failed for version ${VPP_ASSET_VERSION}, check $TOP_DIR/asset_upload.txt"
        exit 1
    fi
    echo "Uploaded assets with version ${VPP_ASSET_VERSION}, verify $TOP_DIR/asset_upload.txt"
}

docker_root "echo \"ip_resolve=IPv4\" >> /etc/yum.conf"
docker_root "rm -f ${DOCKER_VPP_DIR}/build-root/.deps_installed"

if [[ $WAIT_PULL_ASSETS -eq 1 ]]; then
    echo "Sleeping 120s to wait for asset pull..."
    sleep 120
fi

build_all

copy_assets
if [ -z "$VPP_ASSET_VERSION" ]; then
    echo 'Updated assets but skipping upload'
else
#    upload_assets
#    echo 'Done. Please check your diffs, push and create a PR'
    echo 'Uploading assets disabled with this script, please upload manually!'
fi
