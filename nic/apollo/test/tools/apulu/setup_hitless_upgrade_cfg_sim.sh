#! /bin/bash

set -x

MY_DIR=$( readlink -f $( dirname $0 ))

export PIPELINE=apulu
source $MY_DIR/../../../tools/setup_env_sim.sh $PIPELINE
source $MY_DIR/../setup_upgrade_gtests.sh

mkdir -p /update

upg_setup $BUILD_DIR/gen/upgrade_hitless_sim.json upgrade_hitless.json
