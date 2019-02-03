# {C} Copyright 2018 Pensando Systems Inc. All rights reserved

include ${MKDEFS}/pre.mk
MODULE_TARGET   := nicmgrd.bin
MODULE_PIPELINE := iris
MODULE_SOLIBS   := nicmgr nicmgr_sysmgr nicmgr_upgrade hal_api mnet \
                    pciemgr_if pciemgr pciemgrutils pciehdevices pcietlp cfgspace \
                    intrutils pal misc evutils \
                    ${NIC_SDK_SOLIBS} trace mtrack bm_allocator \
                    commonproto nicmgrproto halproto \
                    delphisdk upgrade upgrade_app upgradeutils sysmgr \
                    sdkcapri_csrint sdkcapri_asicrw_if \
                    sdkasicpd pdcapri pdcommon hal_mock \
                    sdkp4 sdkp4utils p4pd_common_p4plus_rxdma sdkxcvrdriver \
                    sdkfru p4pd_common_p4plus_txdma p4pd_${PIPELINE} \
                    asicpd ${NIC_HAL_PD_SOLIBS_${ARCH}}
MODULE_LDLIBS   := crypto ${NIC_COMMON_LDLIBS} \
                    ${NIC_THIRDPARTY_GOOGLE_LDLIBS} \
                    ${NIC_CAPSIM_LDLIBS}
MODULE_INCS     := ${BLD_PROTOGEN_DIR}/
include ${MKDEFS}/post.mk
