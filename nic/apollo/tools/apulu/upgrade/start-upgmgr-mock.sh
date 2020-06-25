#! /bin/bash

export PIPELINE=apulu
CUR_DIR=$( readlink -f $( dirname $0 ))

source  ${CUR_DIR}/../../../tools/setup_env_mock.sh $PIPELINE

export OPERD_REGIONS=$CONFIG_PATH/operd-regions.json

echo "Starting pdsupgmgr: `date +%x_%H:%M:%S:%N`"

rm -f $CONFIG_PATH/operd-regions.json
ln -s $CONFIG_PATH/$PIPELINE/operd-regions.json $CONFIG_PATH/operd-regions.json

function finish {
    echo "pdsupgmgr exit"
    operdctl dump upgradelog > upgrade.log
    rm -f $CONFIG_PATH/operd-regions.json
}
trap finish EXIT

CMD="$BUILD_DIR/bin/pdsupgmgr $* 2>&1 &"
$CMD
