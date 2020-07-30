#!/bin/bash
#
#  install.sh
#
#  Install Pensando DSC RPM and penctl utility from
#  distribution tar file into host system.
#
#

PROG=$0
BASE_DIR=`pwd`


#
# check for expected directories
#
if [ ! -d bin -o ! -d drivers ] ; then
	echo "$PROG: Error: file directories missing"
	exit 1
fi

#
# install penctl utility
#
PENCTL_FILES="penctl penctl.linux"
PENCTL_DIR=/usr/local/bin/

cd ${BASE_DIR}/bin
for f in $PENCTL_FILES ; do
	if [ ! -e $f ] ; then
		echo "$PROG: Error: missing file $f"
		exit 1
	fi
done

install -D -p -t $PENCTL_DIR $PENCTL_FILES
if ! echo $PATH | grep $PENCTL_DIR ; then
	echo
	echo "$PROG: The penctl tool was installed in $PENCTL_DIR"
	echo "       Please be sure to add this to your PATH variable."
	echo
fi


#
# install RPM
#

if [ -f /etc/redhat-release ] ; then
	ver=$(cat /etc/redhat-release | tr -dc '0-9')
	major=${ver:0:1}
	minor=${ver:1:4}

	distro="rhel${major}u${minor}"
	rpm="kmod-ionic"
elif [ -f /etc/os-release ] ; then
	ver=$(cat /etc/os-release | grep VERSION_ID | awk -F= '{print $2}' | tr -d '\"' | sed 's/\./sp/')

	distro="sles${ver}"
	rpm="ionic-kmp-default"
else
	echo "$PROG: no RPM available for this host, perhaps use drivers-linux-eth.tar.xz?"
	exit 1
fi

cd ${BASE_DIR}/drivers/linux
RPMFILE=`ls ${rpm}-*.${distro}.*.x86_64.rpm`

rpm -ihv $RPMFILE
modprobe ionic
