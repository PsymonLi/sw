#!/bin/bash
#
#  install.sh
#
#  Install Pensando VENICE RPM from
#  distribution tar file into host system.
#
#

PROG=$0
#
# install RPM
#

cd /usr/local/bin
RPMFILE=kmod-ionic-driver.rpm

rpm -ihv $RPMFILE
MODULE_NAME=`find /lib/modules/ -name "ionic.ko"`
echo "insmod " ${MODULE_NAME}
insmod ${MODULE_NAME}
echo "done"
