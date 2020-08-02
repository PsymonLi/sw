# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libpdsapi_athena.lib
MODULE_PIPELINE = athena
MODULE_SRCS     = $(wildcard ${MODULE_SRC_DIR}/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/core/*.cc) \
                  $(wildcard ${MODULE_SRC_DIR}/internal/*.cc)
MODULE_SOLIBS   = asicerror marvell operd operd_alerts operd_alert_defs shmstore upgshmstore

include ${MKDEFS}/post.mk
