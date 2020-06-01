#!/bin/bash

# check for release number
if [ -z $1 ] ; then
	RELEASE="1"
else
	RELEASE=$1
fi

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

# determine ionic driver version and build number
VERSION=$(cat drivers/eth/ionic/ionic.h | grep VERSION | awk '{print $3}' | awk -F- '{print $1 "." $2}' | sed 's/"//g')

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

echo "Building IONIC rpm for version $VERSION"
rpmbuild -ba --define "_topdir $RPMDIR" ionic.spec
