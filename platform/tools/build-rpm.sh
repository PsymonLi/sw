#!/bin/bash

# determine driver version
VERSION=$(cat drivers/eth/ionic/ionic.h | grep VERSION | awk '{print $3}' | sed 's/"//g')
BASEVER=$(echo $VERSION | cut -d- -f1)
RELEASE=$(echo $VERSION | cut -d- -f2- | tr - .)

# check if building rpm for RHEL or SLES
if [ -f /etc/redhat-release ]
then
	temp=$(cat /etc/redhat-release | tr -dc '0-9')
	major=${temp:0:1}
	minor=${temp:1:4}
	DISTRO="rhel${major}u${minor}"
elif [ -f /etc/os-release ]
then
	DISTRO=$(cat /etc/os-release | grep VERSION_ID | awk -F= '{print $2}' | tr -d '\"' | sed 's/\./sp/')
	DISTRO="sles${DISTRO}"
fi

RPMDIR=$(pwd)/rpmbuild

# cleanup if needed
rm -rf rpmbuild

# package tarball and build rpm
mkdir -p $RPMDIR/SOURCES
tar czf $RPMDIR/SOURCES/ionic-$VERSION.tar.gz \
	--exclude=build-rpm.sh --exclude=ionic.spec \
	--transform "s,^,ionic-$BASEVER/," *
cp ionic.files $RPMDIR/SOURCES/
cp kmod-ionic.conf $RPMDIR/SOURCES/
cp ionic.conf $RPMDIR/SOURCES/

# define variables to pass to rpm spec
declare -A VARS
VARS["KMOD_VERSION"]="${VERSION}"
VARS["KMOD_BASEVER"]="${BASEVER}"
VARS["KMOD_RELEASE"]="${RELEASE}"
VARS["DISTRO"]="${DISTRO}"

# post process spec to fill-in defines
for i in "${!VARS[@]}"
do
	echo "===> ${i} : ${VARS[$i]}"
	sed -i "s/@${i}@/${VARS[$i]}/" ionic.spec
done

rpmbuild -vv -ba -D "_topdir $RPMDIR" ionic.spec
