#! /bin/bash

export PIPELINE=apulu
CUR_DIR=$( readlink -f $( dirname $0 ))

source  ${CUR_DIR}/../../../tools/setup_env_mock.sh $PIPELINE

if [ ! -e $CONFIG_PATH/operd-regions.json ];then
    ln -s $CONFIG_PATH/$PIPELINE/operd-regions.json $CONFIG_PATH/operd-regions.json
    export OPERD_REGIONS=$CONFIG_PATH/operd-regions.json
fi

function finish {
    echo "pdsupgmgr exit"
    if [ -e $CONFIG_PATH/operd-regions.json ];then
        operdctl dump upgradelog > upgrademgr.log
    fi
    if [ ! -z "$OPERD_REGIONS" ];then
        rm -f $CONFIG_PATH/operd-regions.json
    fi
}
trap finish EXIT

echo "Starting pdsupgmgr: `date +%x_%H:%M:%S:%N`"
CMD="$BUILD_DIR/bin/pdsupgmgr $* 2>&1 &"
exec $CMD
