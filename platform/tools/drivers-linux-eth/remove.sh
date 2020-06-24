#!/bin/bash
#
#  remove.sh
#
#  Remove Pensando DSC RPM and penctl utility from host system.
#
#

PROG=$0
BASE_DIR=`pwd`


#
# remove penctl utility
#
PENCTL_FILES="penctl penctl.linux"
PENCTL_DIR=/usr/local/bin/

cd ${PENCTL_DIR}
rm -r $PENCTL_FILES


#
# remove RPM
#

if [ -f /etc/redhat-release ] ; then
	rpm="kmod-ionic"
elif [ -f /etc/os-release ] ; then
	rpm="ionic-kmp-default"
else
	echo "$PROG: no RPM available for this host, perhaps use drivers-linux-eth.tar.xz?"
	exit 1
fi

rpm -e $rpm
rmmod ionic

