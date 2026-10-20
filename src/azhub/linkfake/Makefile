# SPDX-License-Identifier: GPL-3.0-only
# SDK container supplies aarch64 gcc and AZ_SDK_INCLUDE; flags retain0.1.4 behavior.
MOD_ID := usb-hub-support
.DEFAULT_GOAL := all
AZ_SDK_INCLUDE ?= ../../azmod
OUT ?= out
ifeq ($(origin CC),default)
CC := gcc
endif
.PHONY: all eject-ui-prototype safe-eject-emulator az-candidate test test-portable test-sdk-ui
SAN ?= plain asan tsan
TEST_CC ?= clang
AZ_SDK ?= $(abspath $(AZ_SDK_INCLUDE)/..)
test: test-portable test-sdk-ui
test-portable:
	AZ_SDK_INCLUDE="$(AZ_SDK_INCLUDE)" SAN="$(SAN)" TEST_CC="$(TEST_CC)" sh ./test-host.sh
test-sdk-ui:
	CC="$(TEST_CC)" sh "$(AZ_SDK)/tools/harness/sanitize.sh" --passes "$(SAN)" -I . usb-hub-support:4:test-sdk-ui.c
all: $(OUT)/$(MOD_ID).so
# Explicit historical UI-only emulator build.
eject-ui-prototype: $(OUT)/$(MOD_ID)-eject-ui.so
$(OUT)/$(MOD_ID)-eject-ui.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
safe-eject-emulator: $(OUT)/$(MOD_ID)-safe-eject.so
$(OUT)/$(MOD_ID)-safe-eject.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
# Explicit candidate output for acceptance before packaging.
az-candidate: $(OUT)/$(MOD_ID)-az-candidate.so
$(OUT)/$(MOD_ID)-az-candidate.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
$(OUT)/$(MOD_ID).so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
