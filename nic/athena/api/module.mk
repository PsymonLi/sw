# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   = libpdsapi_athena.lib
MODULE_PIPELINE = athena
MODULE_SRCS     = $(wildcard ${TOPDIR}/nic/apollo/api/*.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/api/core/*.cc) \
                  $(wildcard ${TOPDIR}/nic/apollo/api/internal/*.cc)
MODULE_SOLIBS   = asicerror marvell operd operd_event operd_event_defs shmstore pdsupgapi

include ${MKDEFS}/post.mk
