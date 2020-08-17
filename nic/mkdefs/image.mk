#
# To create a new buildroot minio image do inside the nic container:
# sudo cp /opt/rh/devtoolset-7/root/bin/make /bin/make
# sudo cp /opt/rh/devtoolset-7/root/bin/make /bin/gmake
# cd /sw/nic/buildroot
# make capri_defconfig
# make
#
# Once done, bump up the version in minio/VERSIONS and do:
# UPLOAD=1 make create-assets
#

export CONFIG_DIR=${NICDIR}/buildroot/${OUT_DIR}
export TOOLS_DIR=${NICDIR}/tools/
export BR2_CONFIG=$(CONFIG_DIR)/.config
export BINARIES_DIR=${NICDIR}/buildroot/${OUT_DIR}/images
export TARGET_DIR=${NICDIR}/buildroot/${OUT_DIR}/target

HOSTPATH=PATH="${NICDIR}/buildroot/host/bin:${NICDIR}/buildroot/host/sbin:/opt/coreutils/bin:/opt/rh/devtoolset-7/root/usr/bin:$(PATH)"
LIBRARYPATH=LD_LIBRARY_PATH=${NICDIR}/buildroot/host/lib:$(LD_LIBRARY_PATH)
FAKEROOTOPTS=-l ${NICDIR}/buildroot/host/lib/libfakeroot.so -f ${NICDIR}/buildroot/host/bin/faked

.PHONY: fetch-buildroot-binaries
fetch-buildroot-binaries: package
ifndef CUSTOM_BUILDROOT
	TOPDIR=${TOPDIR} ${TOPDIR}/nic/tools/fetch-buildroot.sh
else
	echo "CUSTOM_BUILDROOT set, skipping buildroot refresh"
endif

.PHONY: copy-overlay
copy-overlay: fetch-buildroot-binaries
	rsync -a --ignore-times --keep-dirlinks \
		--chmod=u=rwX,go=rX --exclude .empty --exclude '*~' \
		${TOPDIR}/fake_root_target/aarch64/ $(TARGET_DIR)/
	PATH=${NICDIR}/buildroot/host/bin:${NICDIR}/buildroot/host/sbin:/opt/rh/devtoolset-7/root/usr/bin:/opt/coreutils-8.30/bin:$(PATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-build.sh
	PATH=${NICDIR}/buildroot/host/bin:${NICDIR}/buildroot/host/sbin:/opt/rh/devtoolset-7/root/usr/bin:/opt/coreutils-8.30/bin:$(PATH) ${NICDIR}/buildroot/board/pensando/scripts/post-fakeroot.sh

.PHONY: build-rootfs
build-rootfs: copy-overlay
	rm -rf ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs
	mkdir -p ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs
	rsync -auH ${NICDIR}/buildroot/${OUT_DIR}/target/ ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/target
	echo '#!/bin/sh' > ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	echo "set -e" >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	echo "chown -h -R 0:0 ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/target" >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	cat ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/users_table.txt >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/users_table.txt
	printf '        sshd -1 sshd -1 * - - - SSH drop priv user\n\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/users_table.txt
	$(HOSTPATH) ${NICDIR}/buildroot/support/scripts/mkusers ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/users_table.txt ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/target >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	cat ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/device_table.txt > ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/device_table.txt
	printf '        /bin/busybox                     f 4755 0  0 - - - - -\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/device_table.txt
	if [ -f ${NICDIR}/buildroot/${OUT_DIR}/target/bin/ping ]; then \
		printf '        /bin/ping        f 4755 0 0 - - - - -\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/device_table.txt; \
	fi
	if [ -f ${NICDIR}/buildroot/${OUT_DIR}/target/bin/traceroute6 ]; then \
		printf '        /bin/traceroute6 f 4755 0 0 - - - - -\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/device_table.txt; \
	fi
	echo "${NICDIR}/buildroot/host/bin/makedevs -d ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/device_table.txt ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/target" >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	printf '        tar cf ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/rootfs.common.tar --numeric-owner --exclude=THIS_IS_NOT_YOUR_ROOT_FILESYSTEM -C ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/target .\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	chmod a+x ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs
	$(LIBRARYPATH) $(HOSTPATH) ${NICDIR}/buildroot/host/bin/fakeroot $(FAKEROOTOPTS) -- ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/fakeroot.fs

.PHONY: build-squashfs
build-squashfs: build-rootfs
	rm -rf ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs
	mkdir -p ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs
	echo '#!/bin/sh' > ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot
	echo "set -e" >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot
	printf '        mkdir -p ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/target\n  tar xf ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/rootfs.common.tar -C ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/target\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot
	printf '   \n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot
	printf '        ${NICDIR}/buildroot/host/bin/mksquashfs ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/target ${NICDIR}/buildroot/${OUT_DIR}/images/rootfs.squashfs -noappend -processors 8 -comp gzip\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot
	chmod a+x ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot
	$(LIBRARYPATH) $(HOSTPATH) ${NICDIR}/buildroot/host/bin/fakeroot $(FAKEROOTOPTS) -- ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/squashfs/fakeroot

.PHONY: build-rootfs.cpio
build-rootfs.cpio: build-rootfs
	rm -rf ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio
	mkdir -p ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio
	echo '#!/bin/sh' > ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	echo "set -e" >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	printf '   	mkdir -p ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target\n	tar xf ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/rootfs.common.tar -C ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	printf '   	if [ ! -e ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target/init ]; then /bin/install -m 0755 ${NICDIR}/buildroot/fs/cpio/init ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target/init; fi\n	mkdir -p ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target/dev\n	mknod -m 0622 ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target/dev/console c 5 1\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	printf '   \n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	printf '   	cd ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/target && find . | cpio --quiet -o -H newc > ${NICDIR}/buildroot/${OUT_DIR}/images/rootfs.cpio\n' >> ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	chmod a+x ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot
	 $(LIBRARYPATH) $(HOSTPATH) ${NICDIR}/buildroot/host/bin/fakeroot -- ${NICDIR}/buildroot/${OUT_DIR}/build/buildroot-fs/cpio/fakeroot

.PHONY: build-image
build-image: build-squashfs
	${NICDIR}/upgrade_manager/meta/upgrade_metadata_restore.sh
	$(HOSTPATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-image.sh

.PHONY: build-image-issu
build-image-issu: build-squashfs
	mv ${NICDIR}/buildroot/${OUT_DIR}/images/Image ${NICDIR}/buildroot/${OUT_DIR}/images/Image.orig
	cp ${NICDIR}/buildroot/output/capri_ramfs/images/Image ${NICDIR}/buildroot/${OUT_DIR}/images/Image
	${NICDIR}/upgrade_manager/meta/upgrade_metadata_restore.sh
	$(HOSTPATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-image.sh
	rm ${NICDIR}/buildroot/${OUT_DIR}/images/Image
	mv ${NICDIR}/buildroot/${OUT_DIR}/images/Image.orig ${NICDIR}/buildroot/${OUT_DIR}/images/Image

.PHONY: build-gold-image
build-gold-image: gold_env build-rootfs.cpio
	$(eval KERNEL_DIR=linux-custom)
	$(HOSTPATH) BR_BINARIES_DIR=${NICDIR}/buildroot/${OUT_DIR}/images /bin/make -j HOSTCC="/bin/gcc -O2 -I${NICDIR}/buildroot/host/include -L${NICDIR}/buildroot/host/lib -Wl,-rpath,${NICDIR}/buildroot/host/lib" ARCH=arm64 INSTALL_MOD_PATH=${NICDIR}/buildroot/${OUT_DIR}/target CROSS_COMPILE="${NICDIR}/buildroot/host/bin/aarch64-linux-gnu-" DEPMOD=${NICDIR}/buildroot/host/sbin/depmod INSTALL_MOD_STRIP=1 -C ${NICDIR}/buildroot/${OUT_DIR}/build/${KERNEL_DIR} Image -j 24
	/usr/bin/install -m 0644 -D ${NICDIR}/buildroot/${OUT_DIR}/build/${KERNEL_DIR}/arch/arm64/boot/Image ${NICDIR}/buildroot/${OUT_DIR}/images/Image
	$(HOSTPATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-image.sh

.PHONY: build-minigold-image
build-minigold-image: minigold_env build-rootfs.cpio
	$(eval KERNEL_DIR=linux-custom)
	$(HOSTPATH) BR_BINARIES_DIR=${NICDIR}/buildroot/${OUT_DIR}/images /bin/make -j HOSTCC="/bin/gcc -O2 -I${NICDIR}/buildroot/host/include -L${NICDIR}/buildroot/host/lib -Wl,-rpath,${NICDIR}/buildroot/host/lib" ARCH=arm64 INSTALL_MOD_PATH=${NICDIR}/buildroot/${OUT_DIR}/target CROSS_COMPILE="${NICDIR}/buildroot/host/bin/aarch64-linux-gnu-" DEPMOD=${NICDIR}/buildroot/host/sbin/depmod INSTALL_MOD_STRIP=1 -C ${NICDIR}/buildroot/${OUT_DIR}/build/${KERNEL_DIR} Image -j 24
	/usr/bin/install -m 0644 -D ${NICDIR}/buildroot/${OUT_DIR}/build/${KERNEL_DIR}/arch/arm64/boot/Image ${NICDIR}/buildroot/${OUT_DIR}/images/Image
	$(HOSTPATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-image.sh

.PHONY: build-diag-image
build-diag-image: diag_env build-rootfs.cpio
	$(eval KERNEL_DIR=linux-custom)
	$(HOSTPATH) BR_BINARIES_DIR=${NICDIR}/buildroot/${OUT_DIR}/images /bin/make -j HOSTCC="/bin/gcc -O2 -I${NICDIR}/buildroot/host/include -L${NICDIR}/buildroot/host/lib -Wl,-rpath,${NICDIR}/buildroot/host/lib" ARCH=arm64 INSTALL_MOD_PATH=${NICDIR}/buildroot/${OUT_DIR}/target CROSS_COMPILE="${NICDIR}/buildroot/host/bin/aarch64-linux-gnu-" DEPMOD=${NICDIR}/buildroot/host/sbin/depmod INSTALL_MOD_STRIP=1 -C ${NICDIR}/buildroot/${OUT_DIR}/build/${KERNEL_DIR} Image -j 24
	/usr/bin/install -m 0644 -D ${NICDIR}/buildroot/${OUT_DIR}/build/${KERNEL_DIR}/arch/arm64/boot/Image ${NICDIR}/buildroot/${OUT_DIR}/images/Image
	$(HOSTPATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-image.sh

.PHONY: upg_meta_create
upg_meta_create:
	${NICDIR}/upgrade_manager/meta/upgrade_metadata_create.sh

.PHONY: build-upg-image
build-upg-image: upg_meta_create build-squashfs
	$(HOSTPATH) ${NICDIR}/buildroot/board/pensando/${FW_PACKAGE_DIR}/post-upg-image.sh
	${NICDIR}/upgrade_manager/meta/upgrade_metadata_restore.sh

.PHONY: diag_env
diag_env:
	$(eval OUT_DIR := output_diag)
	$(eval NAPLES_FW_NAME := naples_diagfw.tar)
	$(eval FW_PACKAGE_DIR := capri-diagimg)
	$(eval AARCH64_LINUX_HEADERS := ${NICDIR}/buildroot/output_diag/build/linux-custom)
	@echo OUT_DIR=${OUT_DIR} NAPLES_FW_NAME=${NAPLES_FW_NAME} FW_PACKAGE_DIR=${FW_PACKAGE_DIR} AARCH64_LINUX_HEADERS=${AARCH64_LINUX_HEADERS}

.PHONY: firmware-normal
firmware-normal: build-image
	if [ ${ASIC} == "capri" ]; then \
		cp ${NICDIR}/buildroot/${OUT_DIR}/images/${NAPLES_FW_NAME} ${NICDIR}/dsc_fw_${RELEASE}.tar; \
		ln -frs ${NICDIR}/dsc_fw_${RELEASE}.tar ${NICDIR}/${NAPLES_FW_NAME}; \
	else \
		cp ${NICDIR}/buildroot/${OUT_DIR}/images/${NAPLES_FW_NAME} ${NICDIR}/dsc_fw_${ASIC}_${RELEASE}.tar; \
		ln -frs ${NICDIR}/dsc_fw_${ASIC}_${RELEASE}.tar ${NICDIR}/naples_fw_${ASIC}.tar; \
	fi

.PHONY: firmware-issu
firmware-issu: build-image-issu
	if [ ${ASIC} == "capri" ]; then \
		cp ${NICDIR}/buildroot/${OUT_DIR}/images/${NAPLES_FW_NAME} ${NICDIR}/dsc_fw_issu_${RELEASE}.tar; \
		ln -frs ${NICDIR}/dsc_fw_issu_${RELEASE}.tar ${NICDIR}/${NAPLES_FW_NAME}; \
	else \
		cp ${NICDIR}/buildroot/${OUT_DIR}/images/${NAPLES_FW_NAME} ${NICDIR}/dsc_fw_issu_${ASIC}_${RELEASE}.tar; \
		ln -frs ${NICDIR}/dsc_fw_issu_${ASIC}_${RELEASE}.tar ${NICDIR}/naples_fw_${ASIC}.tar; \
	fi

.PHONY: firmware-upgrade
firmware-upgrade: build-upg-image
	if [ ${ASIC} == "capri" ]; then \
		cp ${NICDIR}/buildroot/${OUT_DIR}/images/naples_upg_fw.tar ${NICDIR}/dsc_upg_fw_${RELEASE}.tar; \
		ln -frs ${NICDIR}/dsc_upg_fw_${RELEASE}.tar ${NICDIR}/naples_upg_fw.tar; \
	else \
		cp ${NICDIR}/buildroot/${OUT_DIR}/images/naples_upg_fw.tar ${NICDIR}/dsc_upg_fw_${ASIC}_${RELEASE}.tar; \
		ln -frs ${NICDIR}/dsc_upg_fw_${ASIC}_${RELEASE}.tar ${NICDIR}/naples_upg_fw_${ASIC}.tar; \
	fi

.PHONY: penctl-version
penctl-version:
	${TOPDIR}/penctl/tools/penctl_version.sh

.PHONY: firmware
firmware:
ifeq ($(PIPELINE),apulu)
ifeq ($(CUSTOMDOCKERCONTEXT), 1)
	echo "Skip certain targets for custom dev_docker build"
endif
ifneq ($(CUSTOMDOCKERCONTEXT), 1)
	OUT_DIR=output/${ASIC} FLAVOR=-venice NAPLES_FW_NAME=naples_fw.tar FW_PACKAGE_DIR=${ASIC} make -C . firmware-normal
	mv dsc_fw_${RELEASE}.tar naples_fw_venice.tar
endif
endif
	OUT_DIR=output/${ASIC} NAPLES_FW_NAME=naples_fw.tar FW_PACKAGE_DIR=${ASIC} make -C . firmware-normal
ifeq ($(PIPELINE),iris)
	make penctl-version
	OUT_DIR=output/${ASIC} NAPLES_FW_NAME=naples_fw.tar FW_PACKAGE_DIR=${ASIC} make firmware-upgrade
endif
ifeq ($(PIPELINE),apulu)
	OUT_DIR=output/${ASIC} NAPLES_FW_NAME=naples_fw_issu.tar FW_PACKAGE_DIR=${ASIC} make -C . firmware-issu
endif
	${TOOLS_DIR}/relative_link.sh ${NICDIR}/build/${ARCH}/${PIPELINE}/${ASIC}

.PHONY: gold_env
gold_env:
	$(eval OUT_DIR = output_gold)
	$(eval NAPLES_FW_NAME := naples_goldfw.tar)
	$(eval FW_PACKAGE_DIR := capri-goldimg)
	$(eval AARCH64_LINUX_HEADERS := ${NICDIR}/buildroot/output_gold/build/linux-custom)
	@echo OUT_DIR=${OUT_DIR} NAPLES_FW_NAME=${NAPLES_FW_NAME} FW_PACKAGE_DIR=${FW_PACKAGE_DIR} AARCH64_LINUX_HEADERS=${AARCH64_LINUX_HEADERS}

.PHONY: gold-firmware
gold-firmware: build-gold-image

.PHONY: minigold_env
minigold_env:
	$(eval OUT_DIR = output_minigold)
	$(eval NAPLES_FW_NAME := naples_minigoldfw.tar)
	$(eval FW_PACKAGE_DIR := capri-mini-goldimg)
	$(eval AARCH64_LINUX_HEADERS := ${NICDIR}/buildroot/output_minigold/build/linux-custom)
	@echo OUT_DIR=${OUT_DIR} NAPLES_FW_NAME=${NAPLES_FW_NAME} FW_PACKAGE_DIR=${FW_PACKAGE_DIR} AARCH64_LINUX_HEADERS=${AARCH64_LINUX_HEADERS}

.PHONY: minigold-firmware
minigold-firmware: build-minigold-image

diag-firmware: build-diag-image
