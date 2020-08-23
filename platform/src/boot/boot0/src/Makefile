
CROSS_COMPILE=${NICDIR}/buildroot/host/opt/ext-toolchain/bin/aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

TEXT_ADDR = 0x400100

OBJDIR = ../obj/$(ASIC)

CFLAGS += -Os -Wall -Werror -Wuninitialized -ffreestanding
CFLAGS += -MMD -MP -I../include
CFLAGS += -DASIC=$(ASIC)
CFLAGS += -I../include/$(ASIC)

CFLAGS += -march=armv8-a+crc
LDFLAGS += -nostdlib -nodefaultlibs -Wl,--build-id=none
LDFLAGS += -Wl,--script=boot0.lds

VPATH = $(ASIC)

SOURCES = \
	bfl.c \
	board.c \
	cpld.c \
	fwsel.c \
	gpio.c \
	log.c \
	main.c \
	panic.c \
	printf.c \
	qspi.c \
	start.S \
	string.c \
	sys.c \
	uart.c \
	uboot.c \
	wdt.c \
	xcpt.c \
	$(NULL)

ifeq ($(ASIC),capri)
CFLAGS += -I$(NICDIR)/sdk/third-party/asic/capri/model/cap_top/csr_defines
SOURCES += regio.c
endif
ifeq ($(ASIC),elba)
CFLAGS += -I$(NICDIR)/sdk/third-party/asic/elba/model/elb_top
CFLAGS += -I$(NICDIR)/sdk/third-party/asic/elba/model/elb_top/csr_defines
endif

OBJS := $(basename $(SOURCES))
OBJS := $(OBJS:%=$(OBJDIR)/%.o)

MODULE_BIN_DIR := ${BLD_OUT_DIR}/boot0_submake

PROGRAM = $(OBJDIR)/boot0
PROGRAM_BIN = $(OBJDIR)/boot0.bin
PROGRAM_LST = $(OBJDIR)/boot0.lst

TARGETS = \
	$(PROGRAM) \
	$(PROGRAM_BIN) \
	$(PROGRAM_LST) \
	$(NULL)

check_all: all check_version export

all: $(OBJDIR)/.dir $(TARGETS)

$(PROGRAM_BIN): $(PROGRAM)
	@echo MAKEBIN $@
	@$(CROSS_COMPILE)objcopy -O binary $^ $@

$(PROGRAM_LST): $(PROGRAM)
	@echo MAKELIST $@
	@$(CROSS_COMPILE)objdump --disassemble $^ > $@

$(PROGRAM): $(OBJS)
	@echo LINK $@
	@$(CC) $(LDFLAGS) -Ttext=$(TEXT_ADDR) -o $@ $^

$(OBJDIR)/%.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S
	@echo CC $<
	@$(CC) $(CFLAGS) -D__ASSEMBLY__ -c -o $@ $<

$(ASIC)/version.txt: all
	@v=`perl -ane 'print int($$F[2]);' < ../include/$(ASIC)/version.h`; \
	echo "$$v `md5sum $(PROGRAM_BIN)`" > $@

.PHONY: check_version
check_version:
	@v=`perl -ane 'print int($$F[2]);' < ../include/$(ASIC)/version.h`; \
	echo "$$v `md5sum $(PROGRAM_BIN)`" > /tmp/v; \
	if ! cmp -s /tmp/v $(ASIC)/version.txt; then \
		echo "###" >&2; \
		echo "### ERROR: boot0.bin does not match version $$v."; \
		echo "### make new_version to increment the program version." >&2; \
		echo "### make version.txt to keep the current version." >&2; \
		echo "###" >&2; \
		rm -f /tmp/v; \
		exit 1; \
	fi; \
	rm -f /tmp/v

.PHONY: export
export: $(PROGRAM_BIN)
	@cp $(PROGRAM_BIN) $(ASIC)/boards.json $(MODULE_BIN_DIR)

.PHONY: new_version
new_version:
	v=`perl -ane 'print int($$F[2]) + 1;' < ../include/$(ASIC)/version.h`; \
	echo "#define BOOT0_VERSION $$v" > ../include/$(ASIC)/version.h; \
	make $(ASIC)/version.txt

clean:
	@rm -rf $(OBJDIR) $(TARGETS)

%/.dir:
	@mkdir -p $(OBJDIR) && touch $@

-include $(OBJDIR)/*.d
