# {C} Copyright 2019 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_TARGET = libftl.so
MODULE_SOLIBS = utils indexer
MODULE_FLAGS  = -O3
MODULE_PIPELINE = apollo iris artemis
include ${MKDEFS}/post.mk
