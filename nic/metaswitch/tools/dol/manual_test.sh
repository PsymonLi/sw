#!/bin/bash
set -x
source /sw/nic/apollo/tools/setup_dol.sh --pipeline apulu --nostop
start_model
start_processes
start_upgrade_manager
check_health
