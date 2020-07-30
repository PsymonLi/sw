#/bin/sh

WS=$GOPATH/src/github.com/pensando/sw
BUILD_UTIL=$WS/iota/bin/build_util

#Parse Command line options first.
getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
    echo "I?m sorry, `getopt --test` failed in this environment."
    exit 1
fi

pipeline="iris"
targetdir=$WS
version=""
branch="master"
pull=0

set -ex

OPTIONS="v:b:d:p:l"
LONGOPTIONS="version:,branch:,target-dir:,pipeline:,pull,local"

PARSED=$(getopt --options=$OPTIONS --longoptions=$LONGOPTIONS --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
    # e.g. $? == 1
    #  then getopt has complained about wrong arguments to stdout
    exit 2
fi

eval set -- "$PARSED"


rm -rf $WS/upgrade-bundle
targetdir="$WS/upgrade-bundle/artifacts"
while true; do
    case "$1" in
        -v|--version)
            version=($2)
            shift 2
            ;;
         --pull)
	    pull=1
            shift
            ;;
         --local)
	    targetdir="$WS"
            shift
            ;;
        -b|--branch)
            branch=($2)
            shift 2
            ;;
        -d|--target-dir)
            targetdir=($2)
            shift 2
            ;;
        -p|--pipeline)
            pipeline=($2)
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Programming error"
            exit 3
            ;;
    esac
done

naples_image="dsc_fw_$version.tar"
naples_dst_image="naples_fw.tar"
dir="sw-iris-capri"
if [ $pipeline = "cloud" ];then
    export naples_image="naples_fw_venice.tar"
    export dir="sw-apulu"
fi

if [ "$branch" = "" ];then
   branch=""
else
    branch=" --target-branch "$branch
fi

bundle_version=$version
if [ "$version" = "" ];then
    version=""
else
    version=" --version "$version
    branch=""
fi



if [ $pull -eq 1 ];then
	mkdir -p $targetdir
	cd $targetdir

	mkdir -p $targetdir/nic
	$BUILD_UTIL $branch  $version --src-file $dir/sw/nic/$naples_image  --dst-file $targetdir/nic/$naples_dst_image
	$BUILD_UTIL $branch  $version --src-file $dir/sw/nic/host.tar  --dst-file $targetdir/nic/host.tar

	mkdir -p platform/gen
	$BUILD_UTIL  $branch  $version  --src-file $dir/sw//platform/gen/drivers-esx-eth.tar.xz  --dst-file  platform/gen/drivers-esx-eth.tar.xz
	$BUILD_UTIL  $branch  $version  --src-file $dir/sw//platform/gen/drivers-freebsd.tar.xz  --dst-file  platform/gen/drivers-freebsd.tar.xz
	$BUILD_UTIL  $branch  $version  --src-file $dir/sw//platform/gen/drivers-linux.tar.xz  --dst-file  platform/gen/drivers-linux.tar.xz
	$BUILD_UTIL  $branch  $version  --src-file $dir/sw//platform/gen/drivers-linux-eth.tar.xz  --dst-file  platform/gen/drivers-linux-eth.tar.xz




	venice_image="psm.tgz"
	if [ $pipeline = "cloud" ];then
	    export venice_image="psm.apulu.tgz"
	fi

	mkdir -p bin
	$BUILD_UTIL $branch $version --src-file $venice_image  --dst-file  bin/venice.tgz

    cd $WS
	$BUILD_UTIL $branch $version  --src-file build_iris_sim.tar.gz  --dst-file build_iris_sim.tar.gz
    tar -xvzf build_iris_sim.tar.gz  --strip-components=5 -C .
else
	GIT_COMMIT=`git rev-list -1 HEAD --abbrev-commit`
	GIT_VERSION=`git describe --tags --dirty --always`
	IMAGE_VERSION=${GIT_VERSION}
	bundle_version=${IMAGE_VERSION}
fi

BUILD_DATE=`date   +%Y-%m-%dT%H:%M:%S%z`
mkdir -p $WS/upgrade-bundle/bin
mkdir -p $WS/upgrade-bundle/nic
mkdir -p $WS/upgrade-bundle/bin/venice-install
ln -f $targetdir/bin/venice.tgz $WS/upgrade-bundle/bin/venice.tgz
ln -fL $targetdir/nic/$naples_image $WS/upgrade-bundle/nic/naples_fw.tar
touch $WS/upgrade-bundle/bin/venice-install/venice_appl_os.tgz
#bundle.py creates metadata.json for the bundle image
$WS/tools/scripts/bundle.py -v $bundle_version  -d ${BUILD_DATE} -p "$WS/upgrade-bundle/"
ln -f $WS/upgrade-bundle/bin/venice.tgz $WS/upgrade-bundle/venice.tgz
ln -fL $WS/upgrade-bundle/nic/naples_fw.tar $WS/upgrade-bundle/naples_fw.tar
ln -f $WS/upgrade-bundle/bin/venice-install/venice_appl_os.tgz $WS/upgrade-bundle/venice_appl_os.tgz
cd $WS/upgrade-bundle && tar -cf bundle.tar venice.tgz  naples_fw.tar venice_appl_os.tgz metadata.json
cd $WS/upgrade-bundle && cat metadata.json ; ls -al; tar -tvf bundle.tar

