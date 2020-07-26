#!/bin/bash
# check for routing log rotation and zip/stash if found

date > /tmp/cron_test_routing

LOG_STASH_DIR=/data/logstash
RLOG_STASH_DIR=$LOG_STASH_DIR/routing

mkdir -p $RLOG_STASH_DIR

function make_space () {
    oldest=$(ls -lt $RLOG_STASH_DIR/*.gz | tail -n1 | awk '{print $9}')
    rm $oldest
}

function have_space () {
    sz=$(du -s $RLOG_STASH_DIR | cut -f1)
    # cap stash at 30M
    if (( $sz >= 30000 )); then
        return 0
    else
        return 1
    fi
}

function bkup_zip ()  {
    if [ -f "$1" ]; then
       current=$(date +%s)
       last_modified=$(stat -c "%Y" $1)

       # ensure that its not getting rotated simultaneously
       if [ $(($current-$last_modified)) -lt 1 ]; then
           sleep 1
       fi
       mv $1 $LOG_STASH_DIR/
       have_space
       if (( $? == 0 )); then
           make_space $1
       fi
       gzip -c $LOG_STASH_DIR/$2.bak > "${RLOG_STASH_DIR}/$2_$(date +%H_%M_%S).bak.gz"
    fi
}

bkup_zip /var/log/pensando/ipstrc.bak ipstrc
bkup_zip /var/log/pensando/pdtrc.bak pdtrc
