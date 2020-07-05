# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = athena_flow_logger.bin
MODULE_PIPELINE = athena 
MODULE_LDFLAGS  = -L$(MS_LIB_DIR)
MODULE_SOLIBS   = pdscore  pdsapi_athena_impl \
                  trace  logger pdsapi_athena \
                  pdsagent_athena\
                  event_thread penmetrics \
                  ${NIC_FTL_LIBS} \
                  ${NIC_${PIPELINE}_P4PD_SOLIBS} \
                  kvstore_lmdb \
                  sltcam  ${NIC_${PIPELINE}_NICMGR_LIBS}
MODULE_FLAGS    = -O3
ifeq ($(ARCH), x86_64)
MODULE_FLAGS  += -march=native
endif
MODULE_DEFS     = -DRTE_FORCE_INTRINSICS
MODULE_LDLIBS   =  ${NIC_COMMON_LDLIBS} \
                   AAPL ncurses dpdk lmdb zmq
include ${MKDEFS}/post.mk
