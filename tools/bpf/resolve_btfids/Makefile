# SPDX-License-Identifier: GPL-2.0-only

srctree := $(abspath $(CURDIR)/../../../)
OUTPUT ?= $(if $(O),$(abspath $(O)),$(CURDIR))

HOSTCC ?= gcc

RAW_CFLAGS := $(filter-out -maarch64linux -mabi=% -m32 -m64,$(CFLAGS) $(HOSTCFLAGS) $(EXTRA_CFLAGS))
RAW_LDFLAGS := $(filter-out -maarch64linux -mabi=% -m32 -m64,$(LDFLAGS) $(HOSTLDFLAGS))

CLEAN_CFLAGS := $(RAW_CFLAGS) -O2 -g -Wall -Wextra -std=gnu11 -idirafter $(srctree)/include/uapi

TARGET := $(OUTPUT)/resolve_btfids

.PHONY: all clean

all: $(TARGET)

$(OUTPUT):
	mkdir -p $@

$(TARGET): $(srctree)/tools/bpf/resolve_btfids/main.c | $(OUTPUT)
	$(HOSTCC) $(CLEAN_CFLAGS) $(RAW_LDFLAGS) -o $@ $< -lelf

clean:
	rm -f $(TARGET)
