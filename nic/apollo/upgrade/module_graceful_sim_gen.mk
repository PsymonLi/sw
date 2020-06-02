# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET    = graceful_sim.upgfsmgen
MODULE_PIPELINE  = apulu
MODULE_DEPS      = ${SDKDIR}/upgrade/core/fsm_gen.py \
                   ${NICDIR}/conf/${PIPELINE}/upgrade_graceful_sim.json
MODULE_ARGS      = -i ${NICDIR}/conf/${PIPELINE}/upgrade_graceful_sim.json \
                   -o ${BLD_GEN_DIR}/upgrade_graceful_sim.json
MODULE_BASECMD   = python2 ${SDKDIR}/upgrade/core/fsm_gen.py
include ${MKDEFS}/post.mk
