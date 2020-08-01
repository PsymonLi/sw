#
# Dependency
# * libxml2
# * libjsoncpp-dev/jsoncpp
TOP = ../../

ifeq ($(BUILD_DIR),)
	BUILD_DIR = ./build
endif
ifeq ($(LIB_DIR),)
	LIB_DIR = $(BUILD_DIR)
endif

DSC_SPP_LIB=$(LIB_DIR)/libdscspp.so

CFILES = ionic_spp.c ionic_spp_util.c ionic_discovery.c
#CPPFILES = ionic_fw_meta.cc

GIT_TAG := $(shell git describe --abbrev=0 --tags)
ifeq ($(GIT_TAG),)
	GIT_TAG := ""
endif

CFLAGS	+= -DDSC_SPP_VERSION=\"${GIT_TAG}\"
CFLAGS	+= -fPIC
CFLAGS	+= -Werror -Wall -Wextra $(BUILD_FLAGS)
CFLAGS  += -I/usr/local/include
CPPFLAGS  = $(CFLAGS)
CFLAGS	+= `xml2-config --cflags`

LDLIBS = `xml2-config --libs`
#LDLIBS += -ljsoncpp
#LDLIBS += -ltar
LDLIBS += -lpthread
LDFLAGS	+= $(BUILD_FLAGS)
LDFLAGS	+= -L/usr/local/lib

# Gtest specific
GTEST = $(TOP)/nic/sdk/third-party/googletest-release-1.8.0/googletest
GTEST_LDLIBS = $(GTEST)/make/gtest_main.a $(GTEST)/make/gtest.a
GTEST_LDLIBS += $(LDFLAGS) $(LDLIBS)

ifeq ($(OS),Windows_NT)
    uname_S := Windows
else
    uname_S := $(shell uname -s)
endif

ifeq ($(uname_S), Linux)
CFLAGS	+= -Wno-unused-but-set-variable
CFILES += ionic_linux.c

all: lib penutil test

else ifeq ($(uname_S), FreeBSD)
CFILES  += ionic_pmd.c ionic_bsd.c
# PMD need ionic_if.h
CFLAGS	+= -I../drivers/common
CFLAGS  += -DPCIUTILS
LDLIBS  += -lpci

all: lib penutil

else # Build on ESX
all: penutil

endif

CGOFILES=penutil.go

OBJFILES := $(basename $(CFILES) $(CPPFILES))
OBJFILES := $(OBJFILES:%=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	@echo Compiling $@
	$(CC) $(CFLAGS) $< -c -o $(BUILD_DIR)/$(shell basename $@)

$(DSC_SPP_LIB): $(OBJFILES)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $@ $^ $(LDFLAGS) $(LDLIBS)

lib: $(DSC_SPP_LIB)

# Utility based on SPP interfaces.
penutil: $(DSC_SPP_LIB) $(BUILD_DIR)/penutil.o
	$(CXX) -o $(BUILD_DIR)/$@ $^ $(DSC_SPP_LIB)

# Gtest for SPP interfaces.
ionic_spp_test: $(DSC_SPP_LIB) ionic_spp_test.cc ionic_spp_util_test.cc
	$(CXX) -o $(BUILD_DIR)/$@ $^ $(DSC_SPP_LIB) $(GTEST_LDLIBS)  -I.  -I$(GTEST)/include -no-pie

test: ionic_spp_test
	LD_LIBRARY_PATH=./ $(BUILD_DIR)/ionic_spp_test

clean:
	@rm -rf $(LIB_DIR) $(BUILD_DIR)

.PHONY: all clean
