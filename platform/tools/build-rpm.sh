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
	KERNEL=$(uname -r | sed 's/.x86_64//')
	REQUIRES="kernel"
	RPM="kmod-ionic"
elif [ -f /etc/os-release ]
then
	DISTRO=$(cat /etc/os-release | grep VERSION_ID | awk -F= '{print $2}' | tr -d '\"' | sed 's/\./sp/')
	DISTRO="sles${DISTRO}"
	KERNEL=$(uname -r | sed 's/-default/.1/')
	REQUIRES="kernel-default"
	RPM="ionic-kmp-default"
fi

RPMDIR=$(pwd)/rpmbuild

# update spec file
sed -i "s/ionic_version [0-9].[0-9].[0-9]/ionic_version $VERSION/" ionic.spec
sed -i "s/RELEASE/$RELEASE/" ionic.spec
sed -i "s/DISTRO/$DISTRO/" ionic.spec
sed -i "s/RPM/$RPM/" ionic.spec
sed -i "s/KERNEL/$KERNEL/" ionic.spec
sed -i "s/REQUIRES/$REQUIRES/" ionic.spec

# cleanup if needed
rm -rf rpmbuild

# package tarball and build rpm
mkdir -p $RPMDIR/SOURCES
tar czf $RPMDIR/SOURCES/$RPM-$VERSION.tar.gz --exclude=build-rpm.sh --exclude=ionic.spec --transform "s,^,$RPM-$VERSION/," *

echo "Building IONIC rpm for version $VERSION release $RELEASE"
rpmbuild -ba --define "_topdir $RPMDIR" ionic.spec
