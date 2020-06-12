#!/bin/bash
#
#
# generate-rpms.sh  -  create the full set of RPMs for a Naples build.
#
#  Assumptions:
#	TOPDIR is defined for the sw path
#

PROG=$0

# check for release number on command line, else rely on env variable setting if used
if [ -n "$1" ] ; then
	RELEASE=$1
fi


# list of live VMs for building the RPMs
hosts="
rhel73-build.pensando.io
rhel75-build.pensando.io
rhel76-build.pensando.io
rhel77-build.pensando.io
rhel78-build.pensando.io
rhel80-build.pensando.io
rhel81-build.pensando.io
sles12sp4-build.pensando.io
sles12sp5-build.pensando.io
sles15-build.pensando.io
sles15sp1-build.pensando.io
"

if [ -z "$TOPDIR" ] ; then
	echo "$PROG: ERROR: no TOPDIR defined"
	echo current working directory is `pwd`
	exit 1
fi

echo TOPDIR=$TOPDIR

# test archive
TARBALL="$TOPDIR/platform/gen/drivers-linux-eth.tar.xz"
if [ ! -f $TARBALL ] ; then
	echo "$PROG: ERROR: did not find drivers-linux-eth.tar.xz in $TOPDIR/platform/gen/"
	exit 1
fi
xz -t $TARBALL
if [ $? -eq 1 ] ; then
	echo "$PROG: ERROR: tarball failed xz test"
	exit 1
fi

TEMP=$(mktemp -d rpms.XXXXXX)
tar xf $TARBALL -C $TEMP

count=0
retcode=0
for vm in ${hosts}
do
	# check for live build machines
	echo "Ping build machine $vm"
	ping -c 3 $vm > /dev/null
	if [ $? -ne 0 ] ; then
		echo "$PROG: FAIL: ping failed for $vm, quitting"
		retcode=1
		continue
	fi

	# copy bits to build machine
	sshpass -p docker scp -o StrictHostKeyChecking=no -r $TEMP root@${vm}: < /dev/null

	# launch build on build machine
	sshpass -p docker ssh -o StrictHostKeyChecking=no root@$vm "cd $TEMP/drivers-linux-eth ; ./build.sh" < /dev/null
	sshpass -p docker ssh -o StrictHostKeyChecking=no root@$vm "cd $TEMP/drivers-linux-eth ; ./build-rpm.sh $RELEASE" < /dev/null

	# copy rpms to platform/gen
	sshpass -p docker scp -o StrictHostKeyChecking=no root@$vm:$TEMP/drivers-linux-eth/rpmbuild/RPMS/x86_64/*.rpm $TOPDIR/platform/gen < /dev/null
	sshpass -p docker scp -o StrictHostKeyChecking=no root@$vm:$TEMP/drivers-linux-eth/rpmbuild/SRPMS/*.rpm $TOPDIR/platform/gen < /dev/null

	# cleanup
	sshpass -p docker ssh -o StrictHostKeyChecking=no root@$vm "rm -rf $TEMP" < /dev/null

	count=`expr $count + 1`
done


if [ $retcode -eq 0 ] ; then
	# create a tarball of the RPMs
	echo "$PROG: $count RPMs created in $TOPDIR/platform/gen"
	rm -f $TOPDIR/platform/gen/linux-rpms.tar $TOPDIR/platform/gen/linux-rpms.tar.xz
	tar cf $TOPDIR/platform/gen/linux-rpms.tar -C $TOPDIR/platform/gen/ *.rpm
	xz $TOPDIR/platform/gen/linux-rpms.tar
else
	echo "$PROG: errors seen, so not creating final tar of RPMs."
fi

# clean up our local mess
rm -rf $TEMP
