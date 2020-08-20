# {C} Copyright 2019 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
include $(TOPDIR)/nic/metaswitch/pre.mk
MODULE_TARGET   = libpdsmsmgmt.lib
MODULE_PIPELINE = apollo artemis apulu
MODULE_PREREQS  = metaswitch.submake pdsgen.proto
MODULE_INCS 	= ${BLD_PROTOGEN_DIR} $(addprefix $(MS_ROOT)/,$(MS_INCLPATH))
MODULE_LDFLAGS  = -L$(MS_LIB_DIR)
MODULE_LDLIBS   = $(MS_LD_LIBS) \
                  ${NIC_THIRDPARTY_GOOGLE_LDLIBS}
MODULE_FLAGS	= $(addprefix -D,$(MS_COMPILATION_SWITCH))
MODULE_SOLIBS   = logger pdsproto thread operd operd_event operd_event_defs
MODULE_SRCS     = $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/gen/mgmt/*.cc)
MODULE_CLEAN_DIRS  = ${TOPDIR}/nic/metaswitch/stubs/mgmt/gen
include ${MKDEFS}/post.mk
