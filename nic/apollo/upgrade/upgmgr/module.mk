# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = pdsupgmgr.bin
MODULE_PIPELINE = apulu
MODULE_INCS     = ${MODULE_GEN_DIR}
MODULE_PREREQS  = pdsupggen.proto graceful.upgfsmgen hitless.upgfsmgen
MODULE_SOLIBS   = operd pdsupgsvc pdsupgproto penipc upgrade_core \
                  thread event_thread utils ipc_peer evutils pal logger \
                  pdsupgapi
MODULE_LDLIBS   = ${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                  ${NIC_COMMON_LDLIBS} edit ncurses
include ${MKDEFS}/post.mk
