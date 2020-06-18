#!/bin/bash

# BUILD_DIR is defined only on sim/mock mode
if [[ ! -z $BUILD_DIR ]];then
     $PDSPKG_TOPDIR/apollo/tools/$PIPELINE/upgrade/upgmgr_graceful_mock.sh $*
     [[ $? -ne 0 ]] && echo "Upgrade hooks, command execution failed for mock/sim!" && exit 1
     exit 0
fi

source $PDSPKG_TOPDIR/tools/upgmgr_base.sh
upgmgr_parse_inputs $*

SRC=$(cat <<-END
    /var/log/pensando/pds-agent.log
    /var/log/pensando/nicmgr.log
END
)
DESTLOGDIR="/data/pre-upgrade-logs"
mkdir -p ${DESTLOGDIR}
rm -rf ${DESTLOGDIR}/*
for file in $SRC; do
    echo "name : ${STAGE_NAME}_${file##*/}"
    logs=`ls -lt  "${file}"* | head -2 | awk -F " " '{print $9}'` 2>/dev/null
    if [ $? -ne 0 ]; then
        continue;
    fi
    for log in $logs; do
        cp $log "${DESTLOGDIR}/${STAGE_NAME}_${log##*/}"
    done
done
exit 0
