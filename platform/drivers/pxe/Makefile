#
# Copyright (c) 2017-2019, Pensando Systems Inc.
#
TOP = ../../..
IPXEASSETDIR = $(TOP)/platform/drivers/pxe/signed_oprom
IPXEGENDIR = $(TOP)/platform/gen/ipxe
EFI_CONFIG_IPXE = $(TOP)/platform/drivers/pxe/general.h
EFI_IPXE_INCLUDE = $(TOP)/platform/drivers/pxe/hii_strings/include/*
EFI_IPXE_INTERFACE = $(TOP)/platform/drivers/pxe/hii_strings/interface/*
IONIC_IPXE = $(wildcard $(TOP)/platform/drivers/pxe/ionic*.c) $(wildcard $(TOP)/platform/drivers/pxe/ionic.h) $(TOP)/nic/sdk/platform/drivers/ionic_base.h
IONIC_COMMON = $(wildcard $(TOP)/platform/drivers/common/*.h)
IONIC_SRC = $(IONIC_IPXE) $(IONIC_COMMON)
IPXE_TAR = ipxe-1cdf56f.tar.gz
IPXEORG = $(TOP)/platform/drivers/pxe/ipxe-org
IPXEMOD = $(TOP)/platform/drivers/pxe/ipxe-mod

all: ionic-oprom ionic-efiapp ionic-efidrv

ionic-efiapp: $(IPXEGENDIR)/ionic.efi

ionic-efidrv: $(IPXEGENDIR)/ionic.efidrv

ionic-oprom: $(BLD_BIN_DIR)/ionic.efirom

ionic-ipxepatch: $(IPXEGENDIR)/ionic-ipxe.patch

ifeq ($(DEBUG_OPROM),1)
OPROM_TARGET=$(IPXEASSETDIR)/ionic.efirom
else
OPROM_TARGET=$(IPXEMOD)/src/bin-x86_64-efi/ionic.efirom
endif

$(IPXEGENDIR):
	mkdir -p $(IPXEGENDIR)

$(BLD_BIN_DIR)/ionic.efirom: $(OPROM_TARGET)
	cp $< $@

$(IPXEGENDIR)/ionic.efi: $(IPXEMOD)/src/bin-x86_64-efi/ionic.efi $(IPXEGENDIR)
	cp $(IPXEMOD)/src/bin-x86_64-efi/ionic.efi $@

$(IPXEGENDIR)/ionic.efidrv: $(IPXEMOD)/src/bin-x86_64-efi/ionic.efidrv $(IPXEGENDIR)
	cp $(IPXEMOD)/src/bin-x86_64-efi/ionic.efidrv $@

$(IPXEMOD)/src/bin-x86_64-efi/ionic.efi: .ipxefiles
	make PLATFORM=efi ARCH=x86_64 SECUREBOOT=1 -C $(IPXEMOD)/src bin-x86_64-efi/ionic.efi
#	make PLATFORM=efi ARCH=x86_64 SECUREBOOT=1 -C $(IPXEMOD)/src bin-x86_64-efi/ionic.efi DEBUG=ionic:3,ionic_main:3

$(IPXEMOD)/src/bin-x86_64-efi/ionic.efirom: .ipxefiles
	make PLATFORM=efi ARCH=x86_64 SECUREBOOT=1 -C $(IPXEMOD)/src bin-x86_64-efi/ionic.efirom
#	make PLATFORM=efi ARCH=x86_64 SECUREBOOT=1 -C $(IPXEMOD)/src bin-x86_64-efi/ionic.efirom DEBUG=ionic:3,ionic_main:3

$(IPXEMOD)/src/bin-x86_64-efi/ionic.efidrv: .ipxefiles
	make PLATFORM=efi ARCH=x86_64 SECUREBOOT=1 -C $(IPXEMOD)/src bin-x86_64-efi/ionic.efidrv
#	make PLATFORM=efi ARCH=x86_64 SECUREBOOT=1 -C $(IPXEMOD)/src bin-x86_64-efi/ionic.efirom DEBUG=ionic:3,ionic_main:3

.ipxefiles: $(IPXEMOD) $(IONIC_SRC)
	cp $(IONIC_SRC) $(IPXEMOD)/src/drivers/net/
	cp $(EFI_CONFIG_IPXE) $(IPXEMOD)/src/config/local/general.h
	cp -r $(EFI_IPXE_INCLUDE) $(IPXEMOD)/src/include/.
	cp -r $(EFI_IPXE_INTERFACE) $(IPXEMOD)/src/interface/.
	touch $@

$(IPXEMOD):
	mkdir -p $(IPXEMOD); cd $(IPXEMOD); tar xf ../$(IPXE_TAR) --strip-components=1

$(IPXEGENDIR)/ionic-ipxe.patch: $(IPXEORG) .ipxefiles $(IPXEGENDIR)
	cd $(IPXEMOD)/src; make veryclean
	-diff -uprN $(IPXEORG) $(IPXEMOD) > $@

$(IPXEORG):
	mkdir -p $(IPXEORG); cd $(IPXEORG); tar xf ../$(IPXE_TAR) --strip-components=1

clean:
	rm -rf $(IPXEORG) $(IPXEMOD) $(IPXEGENDIR) $(BLD_BIN_DIR)/ionic.efirom .ipxefiles

.PHONY: all ionic-efiapp ionic-oprom ionic-ipxepatch clean
