#!/bin/bash

if [ -z $1 ]
then
 	BUILD_NUMBER=1
else
	BUILD_NUMBER=$1
fi

if [ ! -f /etc/redhat-release ]
then
	echo "Exiting: not a Redhat/CentOS system."
	exit
fi

TOPDIR=$(pwd)/rpmbuild

VERSION=$(cat drivers/eth/ionic/ionic.h | grep VERSION | awk '{print $3}' | sed 's/"//g')

# update spec with current version

sed -i "s/[0-9].[0-9].[0-9]/$VERSION/" ionic.spec
sed -i "s/1%/$BUILD_NUMBER%/" ionic.spec

if [ -d rpmbuild ]
then
	rm -rf rpmbuild
fi

mkdir -p $TOPDIR/SOURCES/ionic-$VERSION
cp drivers/eth/ionic/ionic.ko $TOPDIR/SOURCES/ionic-$VERSION
echo "ionic" > $TOPDIR/SOURCES/ionic-$VERSION/ionic.conf

tar czf ionic-$VERSION.tar.gz -C $TOPDIR/SOURCES .
mv ionic-$VERSION.tar.gz $TOPDIR/SOURCES

echo "Building IONIC RPM for version $VERSION"
rpmbuild -ba --define "_topdir $TOPDIR" ionic.spec 
