#!/bin/bash

# determine driver version
if [ -n "$1" ] ; then
	VERSION=$1
else
	VERSION=$(cat drivers/eth/ionic/ionic.h | grep VERSION | awk '{print $3}' | sed 's/"//g')
fi
BASEVER=$(echo $VERSION | cut -d- -f1)
RELEASE=$(echo $VERSION | cut -d- -f2- | tr - .)

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

KVER=$(uname -r)
KBASEVER=$(echo $KVER | cut -d- -f1)
KREL=$(echo $KVER | cut -d- -f2- | rev | cut -d. -f2- | rev | tr - .)

# cleanup if needed
rm -rf rpmbuild

# package tarball and build rpm
mkdir -p $RPMDIR/SOURCES
tar czf $RPMDIR/SOURCES/ionic-$VERSION.tar.gz --exclude=build-rpm.sh --exclude=ionic.spec --transform "s,^,ionic-$BASEVER/," *
cp ionic.files $RPMDIR/SOURCES/
cp kmod-ionic.conf $RPMDIR/SOURCES/

echo "===> DISTRO $DISTRO"
echo "===> KERNEL version $KVER base $KBASEVER release $KREL"
echo "===> IONIC version $VERSION base $BASEVER release $RELEASE"

rpmbuild -vv -ba -D "_topdir $RPMDIR" -D "_requires $REQUIRES" \
		-D "distro $DISTRO" \
		-D "kmod_version $VERSION" -D "kmod_basever $BASEVER" -D "kmod_release $RELEASE" \
		-D "kernel_version $KVER" -D "kernel_basever $KBASEVER" -D "kernel_release $KREL" \
		ionic.spec
