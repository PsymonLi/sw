# {C} Copyright 2018 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET   = diag_test
ifeq ($(ASIC),elba)
MODULE_SRCS     := ${MODULE_SRC_DIR}/cpld_test.cc \
                   ${MODULE_SRC_DIR}/diag_utils.cc	\
                   ${MODULE_SRC_DIR}/rtc_test.cc	\
                   ${MODULE_SRC_DIR}/serdes_test.cc \
                   ${MODULE_SRC_DIR}/temp_sensor_test_elba.cc
else
MODULE_SRCS     := ${MODULE_SRC_DIR}/cpld_test.cc \
                   ${MODULE_SRC_DIR}/diag_utils.cc	\
                   ${MODULE_SRC_DIR}/rtc_test.cc	\
                   ${MODULE_SRC_DIR}/serdes_test.cc \
                   ${MODULE_SRC_DIR}/temp_sensor_test.cc
endif
MODULE_SRCS     += ${MODULE_SRC_DIR}/main.cc
MODULE_SOLIBS   := sdkfru sdkpal catalog logger sensor
MODULE_LDLIBS   := dl sknobs Judy ${SDK_THIRDPARTY_CAPRI_LDLIBS}

include ${MKDEFS}/post.mk
