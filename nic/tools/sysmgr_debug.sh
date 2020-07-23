#!/bin/sh

NOW=`date +"%Y%m%d%H%M%S"`
TOP=/data/sysmgr-dumps
LOCATION=${TOP}/debug-${NOW}-$1
MAX_DAYS="1"
MAX_RECORDS="11"

mkdir -p ${LOCATION}

# Check CPU load
top -b -n 1 -d 1 -w 256 > ${LOCATION}/top_start.txt

# Copy everything under `/log`
cp -a /var/log ${LOCATION}/log

# One final CPU load check
top -b -n 1 -d 1 -w 256 > ${LOCATION}/top_end.txt
cat /proc/vmstat > ${LOCATION}/vm_stat.txt
cat /proc/loadavg > ${LOCATION}/loadavg.txt
dmesg > ${LOCATION}/dmesg.txt

# Delete old logs
find ${TOP} -mtime +${MAX_DAYS} -type d -prune -exec rm -rf {} \;

# Also don't keep any more than MAX_RECORDS
for f in `ls -t ${TOP} | tail -n +${MAX_RECORDS}`; do rm -rf ${TOP}/$f; done

if [ -e /nic/tools/collect_techsupport.sh ]
then
    echo "Collect techsupport ..."
    /nic/tools/collect_techsupport.sh
    echo "Techsupport collected ..."
fi
