# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = ncsid
MODULE_PIPELINE := iris
MODULE_PREREQS  := nicmgr.proto delphi.proto

MODULE_SOLIBS   = ncsi logger nicmgrproto delphisdk halproto sdkfru sdkplatformutils device bm_allocator shmmgr catalog runenv penipc mctp lldp sensor event_thread thread
MODULE_LDLIBS   := dl pal evutils ${NIC_COMMON_LDLIBS} ${SDK_THIRDPARTY_CAPRI_LDLIBS} \
				${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                ${NIC_CAPSIM_LDLIBS}
MODULE_FLAGS    = -DCAPRI_SW

MODULE_INCS     := ${MODULE_SRC_DIR} ${BLD_PROTOGEN_DIR} ${TOPDIR}/platform/src/lib/mctp

include ${MKDEFS}/post.mk
