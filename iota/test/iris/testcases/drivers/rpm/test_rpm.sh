#!/bin/bash

# 
#  From a list of rpms, choose the likely correct one for the
#  system we're currently running on.  Requires a directory name
#  as an argument for finding the list of RPM files.
#

PROG=$0
tarfile=linux-rpms.tar.xz

# check if building rpm for RHEL or SLES
if [ -f /etc/redhat-release ] ; then
	temp=$(cat /etc/redhat-release | tr -dc '0-9')
	major=${temp:0:1}
	minor=${temp:1:4}
	DISTRO="rhel${major}u${minor}"
elif [ -f /etc/os-release ] ; then
	DISTRO=$(cat /etc/os-release | grep VERSION_ID | awk -F= '{print $2}' | tr -d '\"' | sed 's/\./sp/')
	DISTRO="sles${DISTRO}"
else
	echo "Error: $PROG: this appears to not be a Red Hat or SLES Linux machine"
	exit 1
fi

rpmfile=`ls *.${DISTRO}.* | grep -v src`
srcfile=`ls *.${DISTRO}.* | grep src`
if [ -z "$rpmfile" ] ; then
	echo "error: $PROG: no RPM file found in $tarfile for distro $DISTRO"
	exit 1
fi
if [ -z "$srcfile" ] ; then
	echo "error: $PROG: no RPM source file found in $tarfile for distro $DISTRO"
	exit 1
fi

echo "found rpm file $rpmfile"

# try to install the RPM
rpm -ihv $rpmfile
if [ $? -ne 0 ] ; then
	echo "error: $PROG: failed to install $rpmfile"
	exit 1
fi

modprobe ionic
if [ $? -ne 0 ] ; then
	echo "error: $PROG: failed to modprobe ionic"
	exit 1
fi
sleep 3
lsmod | grep ionic


# try to remove the RPM
rpmname=`rpm -qp $rpmfile`
rpm -ehv $rpmname
if [ $? -ne 0 ] ; then
	echo "Error: $PROG: failed to uninstall rpm $rpmname"
	exit 1
fi


# try to unload the ionic driver
rmmod ionic
if [ $? -ne 0 ] ; then
	echo "Error: $PROG: failed to rmmod ionic"
	exit 1
fi

# try to build the src rpm
rpmbuild --rebuild $srcfile
if [ $? -ne 0 ] ; then
	echo "Error: $PROG: failed to rebuild the RPM src $srcfile"
	exit 1
fi


echo "$PROG: pass"

exit 0
