# {C} Copyright 2020 Pensando Systems Inc. All rights reserved
include ${MKDEFS}/pre.mk
MODULE_ASIC     := elba
MODULE_TARGET   := elbmon.bin
ifeq ($(ARCH),x86_64)
MODULE_SOLIBS   := pal model_client sdkpal logger sdk_asicrw_if
else
MODULE_SOLIBS   := pal sdkpal logger sdk_asicrw_if
endif
MODULE_LDLIBS   := dl sknobs Judy ${SDK_THIRDPARTY_ELBA_LDLIBS}
MODULE_FLAGS    = -g -DELBA_SW
include ${MKDEFS}/post.mk
