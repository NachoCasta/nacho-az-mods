# SPDX-License-Identifier: GPL-3.0-only
# SDK container supplies aarch64 gcc and AZ_SDK_INCLUDE; flags retain0.1.4 behavior.
MOD_ID := usb-hub-support
AZ_SDK_INCLUDE ?= ../../azmod
OUT ?= out
ifeq ($(origin CC),default)
CC := gcc
endif
.PHONY: all
all: $(OUT)/$(MOD_ID).so
$(OUT)/$(MOD_ID).so: community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wno-unused-function -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
