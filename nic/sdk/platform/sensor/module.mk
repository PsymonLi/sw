# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   := libsensor.lib
MODULE_SRCS     := ${MODULE_SRC_DIR}/sensor_api.cc
ifeq ($(ASIC),elba)
MODULE_SRCS     += ${MODULE_SRC_DIR}/sensor_elba.cc
MODULE_FLAGS    := -DELBA
else
MODULE_SRCS     += ${MODULE_SRC_DIR}/sensor.cc
endif

include ${MKDEFS}/post.mk
