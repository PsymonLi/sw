# {C} Copyright 2020 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET     = liboperd_event.submake
MODULE_PIPELINE   = iris apollo apulu artemis gft athena
MODULE_PREREQS    =
MODULE_POSTGEN_MK = module_event_defs.mk
include ${MKDEFS}/post.mk
