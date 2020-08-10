# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET        = upgrade_fsm_test.bin
MODULE_PREREQS       = graceful_test.upgfsmgen hitless_test.upgfsmgen
MODULE_LDLIBS        = stdc++ m rt
MODULE_SOLIBS        = operd penipc thread event_thread utils upgrade_ev
include ${MKDEFS}/post.mk
