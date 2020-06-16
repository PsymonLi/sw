#!/bin/sh

CUR_DIR=$( readlink -f $( dirname $0 ) )
source $CUR_DIR/setup_env_hw.sh

export GOTRACEBACK='crash'
export GODEBUG=madvdontneed=1
export GOGC=50
export PERSISTENT_LOG_DIR='/obfl/'

ulimit -c unlimited

cd /

# set core file pattern
CORE_MIN_DISK=512
[[ "$IMAGE_TYPE" = "diag" ]] && CORE_MIN_DISK=1
mkdir -p /data/core
#echo "|$PDSPKG_TOPDIR/bin/coremgr -P /data/core -p %p -e %e -m $CORE_MIN_DISK" > /proc/sys/kernel/core_pattern
echo "/data/core/core_%p_%e" > /proc/sys/kernel/core_pattern

# if not already present, create a cache file recording the firmware inventory
# at boot-time.  will be preserved across a live-update, and so always
# provides a record of how things looked when we booted (seen with fwupdate -L)
if [[ ! -r /var/run/fwupdate.cache ]]; then
    $PDSPKG_TOPDIR/tools/fwupdate -C &
fi

# start sysmgr
PENLOG_LOCATION=$PERSISTENT_LOGDIR $PDSPKG_TOPDIR/bin/sysmgr &

echo "System initialization done ..."
