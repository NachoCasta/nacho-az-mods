#!/usr/bin/env python3
"""Rebuild the emulator-only hub controller module from the local builder image."""
import subprocess
from pathlib import Path
script = r'''set -e
mkdir /tmp/hub-xhci
cp /kernel/drivers/usb/host/xhci*.[ch] /kernel/drivers/usb/host/pci-quirks.h /tmp/hub-xhci/
sed -i 's/IS_ENABLED(CONFIG_USB_XHCI_PCI_RENESAS)/0/g' /tmp/hub-xhci/xhci-pci.c /tmp/hub-xhci/xhci-pci.h
printf 'obj-m += xhci-pci.o\n' > /tmp/hub-xhci/Makefile
cp /kernel/vmlinux.symvers /kernel/Module.symvers
make -C /kernel M=/tmp/hub-xhci modules >&2
cat /tmp/hub-xhci/xhci-pci.ko
'''
r = subprocess.run(['docker','run','--rm','--platform','linux/arm64','--entrypoint','sh','azemu-builder','-c',script],capture_output=True)
print(r.stderr.decode())
if r.returncode: raise SystemExit(r.returncode)
assert r.stdout[:4] == b'\x7fELF'
out = Path(__file__).with_name('xhci-pci-emulator.ko')
out.write_bytes(r.stdout)
print(str(out), len(r.stdout))
