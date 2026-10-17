#!/bin/bash
# Disassemble hub_probe from the locally built hub.o and show the topology check.
. "$(dirname "$0")/kenv.sh" true
OD=${CROSS_COMPILE}objdump
$OD -dr drivers/usb/core/hub.o | awk '/<hub_probe>:/,/^$/' > /tmp/hub_probe.dis
wc -l < /tmp/hub_probe.dis
grep -n "nested\|#0xfffffff9\|#-7\|kobject_uevent_env\|dev_err" /tmp/hub_probe.dis
