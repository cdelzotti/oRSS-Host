# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = oRSS-Host

# all source are stored in SRCS-y
SRCS-y := main.c

PKGCONF ?= pkgconf

#Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)
DPDK_DIR = /home/clement/dpdk
LOCAL_DPDK_HEADERS = -I $(DPDK_DIR)/config -I $(DPDK_DIR)/build/ -I $(DPDK_DIR)/lib/eal/include/ -I $(DPDK_DIR)/lib/eal/linux/include/ -I $(DPDK_DIR)/lib/eal/x86/include/ -I $(DPDK_DIR)/lib/net/ -I $(DPDK_DIR)/lib/mbuf/ -I $(DPDK_DIR)/lib/mempool/ -I $(DPDK_DIR)/lib/ring/ -I $(DPDK_DIR)/lib/ethdev/ -I $(DPDK_DIR)/lib/meter/
LOCAL_DPDK_LIBS = -L$(DPDK_DIR)/build/lib/ -L$(DPDK_DIR)/build/drivers/

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(LOCAL_DPDK_HEADERS) $(CFLAGS) $(SRCS-y) $(LOCAL_DPDK_LIBS) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(LOCAL_DPDK_HEADERS) $(CFLAGS) $(SRCS-y) $(LOCAL_DPDK_LIBS) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true
