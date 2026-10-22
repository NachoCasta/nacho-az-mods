# SPDX-License-Identifier: GPL-3.0-only
# SDK container supplies aarch64 gcc and AZ_SDK_INCLUDE; flags retain0.1.4 behavior.
MOD_ID := usb-hub-support
.DEFAULT_GOAL := all
AZ_SDK_INCLUDE ?= ../../azmod
OUT ?= out
ifeq ($(origin CC),default)
CC := gcc
endif
.PHONY: all eject-ui-prototype safe-eject-emulator sharing-emulator broker-probe-emulator sharing-network-helper az-candidate test test-portable test-sdk-ui
.PHONY: sharing-broker-service-emulator
.PHONY: sharing-broker-allocate-emulator
.PHONY: sharing-broker-policy-emulator
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
sharing-emulator: $(OUT)/$(MOD_ID)-sharing-emulator.so
broker-probe-emulator: $(OUT)/$(MOD_ID)-broker-probe.so
$(OUT)/$(MOD_ID)-broker-probe.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -DLF_PUBLIC_SDK_PROBE -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
sharing-network-helper: $(OUT)/$(MOD_ID)-network-helper
sharing-broker-allocate-emulator: $(OUT)/$(MOD_ID)-network-helper-allocate-emulator
# Explicit local candidate build; never installs or activates a mod.
.PHONY: sharing-broker-candidate
sharing-broker-candidate: $(OUT)/$(MOD_ID)-network-helper-candidate
$(OUT)/$(MOD_ID)-network-helper-candidate: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_CANDIDATE -pthread -o $@ public-network-helper.c
# Emulator entry for the candidate lifecycle/peer/jitter implementation.
.PHONY: sharing-broker-candidate-emulator
sharing-broker-candidate-emulator: $(OUT)/$(MOD_ID)-network-helper-candidate-emulator
$(OUT)/$(MOD_ID)-network-helper-candidate-emulator: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_CANDIDATE_EMULATOR -pthread -o $@ public-network-helper.c
sharing-broker-policy-emulator: $(OUT)/$(MOD_ID)-network-helper-policy-emulator
$(OUT)/$(MOD_ID)-network-helper-policy-emulator: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_ALLOCATE_EMULATOR -DLF_PUBLIC_DYNAMIC_POLICY_EMULATOR -pthread -o $@ public-network-helper.c
$(OUT)/$(MOD_ID)-network-helper-allocate-emulator: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_ALLOCATE_EMULATOR -pthread -o $@ public-network-helper.c
sharing-broker-service-emulator: $(OUT)/$(MOD_ID)-network-helper-service-emulator
$(OUT)/$(MOD_ID)-network-helper-service-emulator: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_SERVICE_EMULATOR -pthread -o $@ public-network-helper.c
$(OUT)/$(MOD_ID)-network-helper: Makefile public-network-helper.c public-dhcp-child.h public-broker-probe-child.h public-broker-wire.h
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -o $@ public-network-helper.c
$(OUT)/$(MOD_ID)-sharing-emulator.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_PUBLIC_SHARING_EMULATOR -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
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

# Explicit integrated-controller candidates; builds never install or reload.
.PHONY: sharing-runtime-emulator sharing-runtime-candidate
sharing-runtime-emulator: $(OUT)/$(MOD_ID)-sharing-runtime-emulator.so
$(OUT)/$(MOD_ID)-sharing-runtime-emulator.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -DLF_PUBLIC_SHARING_RUNTIME -DLF_PUBLIC_RUNTIME_EMULATOR -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread
sharing-runtime-candidate: $(OUT)/$(MOD_ID)-sharing-runtime-candidate.so
$(OUT)/$(MOD_ID)-sharing-runtime-candidate.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -DLF_PUBLIC_SHARING_RUNTIME -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread

# Read-only native cache observer for separately invoked emulator research.
.PHONY: native-status-observer-emulator
native-status-observer-emulator: $(OUT)/$(MOD_ID)-native-status-observer.so
$(OUT)/$(MOD_ID)-native-status-observer.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -DLF_PUBLIC_NATIVE_STATUS_OBSERVER -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread

# Integrated runtime plus read-only emulator diagnostics; never a release build.
sharing-runtime-observer-emulator: $(OUT)/$(MOD_ID)-sharing-runtime-observer-emulator.so
$(OUT)/$(MOD_ID)-sharing-runtime-observer-emulator.so: Makefile community.c $(wildcard *.c *.h) $(AZ_SDK_INCLUDE)/azmod-mod.h
	mkdir -p $(OUT)
	$(CC) -O2 -fexceptions -Wall -Wno-unused-function -DLF_EJECT_UI -DLF_SAFE_EJECT -DLF_EJECT_AZ -DLF_PUBLIC_SHARING_RUNTIME -DLF_PUBLIC_RUNTIME_EMULATOR -DLF_PUBLIC_NATIVE_STATUS_OBSERVER -shared -fPIC -fvisibility=hidden -I$(AZ_SDK_INCLUDE) -o $@ community.c -ldl -lpthread

# Explicit temporary parent RX diagnostic; never enabled by default candidates.
.PHONY: sharing-parent-rx-probe sharing-parent-rx-probe-emulator
sharing-parent-rx-probe: $(OUT)/$(MOD_ID)-network-helper-rx-probe-candidate
$(OUT)/$(MOD_ID)-network-helper-rx-probe-candidate: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_CANDIDATE -DLF_PUBLIC_PARENT_RX_PROBE -pthread -o $@ public-network-helper.c
sharing-parent-rx-probe-emulator: $(OUT)/$(MOD_ID)-network-helper-rx-probe-candidate-emulator
$(OUT)/$(MOD_ID)-network-helper-rx-probe-candidate-emulator: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_CANDIDATE_EMULATOR -DLF_PUBLIC_PARENT_RX_PROBE -pthread -o $@ public-network-helper.c

# Emulator-only validation trace for a failed mode-recovery control.
.PHONY: sharing-validation-trace-emulator
sharing-validation-trace-emulator: $(OUT)/$(MOD_ID)-network-helper-validation-candidate-emulator
$(OUT)/$(MOD_ID)-network-helper-validation-candidate-emulator: Makefile public-network-helper.c $(wildcard *.h)
	mkdir -p $(OUT)
	$(CC) -O2 -Wall -Wextra -Wno-unused-function -DLF_PUBLIC_BROKER_CANDIDATE_EMULATOR -DLF_PUBLIC_LIBRARY_VALIDATION_TRACE -pthread -o $@ public-network-helper.c
