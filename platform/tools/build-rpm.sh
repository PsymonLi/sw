#!/bin/bash

# check for release number
# if given by the jenkins build system, it should look like 1.11.0-38
#     1.11.0 = Pensando version -> rpm VERSION
#        106 = Pensando build # -> rpm RELEASE
# else find it from the driver version string which might
#   look something like 1.11.0-106-11-g03d67f7
#     1.11.0          = Pensando version         -> rpm VERSION
#     106-11-g03d67f7 = Pensando build/gitcommit -> rpm RELEASE
if [ -n "$1" ] ; then
	verinfo=$1
else
	verinfo=$(cat drivers/eth/ionic/ionic.h | grep VERSION | awk '{print $3}' | sed 's/"//g')
fi

VERSION=$(echo $verinfo | cut -d- -f1 )
RELEASE=$(echo $verinfo | cut -d- -f2- | tr - .)

# check if building rpm for RHEL or SLES
if [ -f /etc/redhat-release ]
then
	temp=$(cat /etc/redhat-release | tr -dc '0-9')
	major=${temp:0:1}
	minor=${temp:1:4}
	DISTRO="rhel${major}u${minor}"
	REQUIRES="kernel"
elif [ -f /etc/os-release ]
then
	DISTRO=$(cat /etc/os-release | grep VERSION_ID | awk -F= '{print $2}' | tr -d '\"' | sed 's/\./sp/')
	DISTRO="sles${DISTRO}"
	REQUIRES="kernel-default"
fi

RPMDIR=$(pwd)/rpmbuild

KERNEL=$(uname -r)
KDIST="$DISTRO.$(echo $KERNEL | cut -d- -f2- | rev | cut -d. -f2- | rev | tr - .)"

# cleanup if needed
rm -rf rpmbuild

# package tarball and build rpm
mkdir -p $RPMDIR/SOURCES
tar czf $RPMDIR/SOURCES/ionic-$VERSION.tar.gz --exclude=build-rpm.sh --exclude=ionic.spec --transform "s,^,ionic-$VERSION/," *
cp ionic.files $RPMDIR/SOURCES/
cp kmod-ionic.conf $RPMDIR/SOURCES/

echo "===> DISTRO $DISTRO"
echo "===> KERNEL version $KERNEL dist $KDIST"
echo "===> IONIC version $VERSION release $RELEASE"

rpmbuild -vv -ba -D "_topdir $RPMDIR" -D "_requires $REQUIRES" \
		-D "kmod_version $VERSION" -D "kmod_release $RELEASE" \
		-D "distro $DISTRO" \
		-D "kernel_version $KERNEL" -D "kernel_dist $KDIST" \
		ionic.spec
