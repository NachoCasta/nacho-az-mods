# azhub

A kernel module that lets the XDJ-AZ enumerate a USB hub plugged into USB1 or USB2.

## Why hubs fail on a stock deck

The deck's kernel (4.4.194, built with `CONFIG_ATC`) defines `MAX_TOPO_LEVEL` as 1 in
`include/linux/usb/hcd.h` of the vendor tree, so `hub_probe()` refuses every external hub:

```
hub 2-1:1.0: Unsupported bus topology: hub nested too deep
hub: probe of 2-1:1.0 failed with error -7
```

and raises `HUBERR`, which `90-usb-caution.rules` turns into the "Hub not supported" caution.

## What the module does

It rewrites the one comparison in `hub_probe` (`cmp w0, #1` at `hub_probe+0x84`) to `cmp w0, #6`,
the stock Linux limit, with the kernel's own `aarch64_insn_patch_text`. Before writing, it checks
the four instructions around it word for word; any other kernel build is refused and nothing is
written. Removing the module (`rmmod azhub`) restores the original instruction, and a reboot
always does.

| Firmware | Kernel | Checked words at `hub_probe+0x7c` |
|---|---|---|
| 2.00 | 4.4.194 | `3953ef00 9100c354 7100041f 540002c1` |

## What it does not do

Sticks behind the hub are mounted by the stock `device-mount.sh` (`/media/usb/sdX1`), but it only
announces media to EP147 for bus ids `1-1` and `2-1`, so a stick behind a hub is not listed in
SOURCE on its own. Announcing it (`mount <path> <fs> protect:<ro>` into `/proc/udev_usb1` or
`/proc/udev_usb2`) is a separate step.

## Building

Against the deck's kernel tree from the vendor's GPL source package (firmware 2.00), configured
with the deck's own `/proc/config.gz`, and the Linaro 6.3.1 aarch64 toolchain it ships:

```sh
make -C "$KDIR" ARCH=arm64 CROSS_COMPILE="$TC/bin/aarch64-linux-gnu-" \
    CC="$TC/bin/aarch64-linux-gnu-gcc" HOSTCFLAGS="-std=gnu11 -fcommon -w" modules_prepare
make -C "$KDIR" ARCH=arm64 CROSS_COMPILE="$TC/bin/aarch64-linux-gnu-" \
    CC="$TC/bin/aarch64-linux-gnu-gcc" M="$PWD" KBUILD_MODPOST_WARN=1 modules
```

`CC` is set explicitly to skip the tree's Python 2 `scripts/gcc-wrapper.py`. The result must report
`vermagic: 4.4.194 SMP preempt mod_unload aarch64` (`modinfo azhub.ko`).

## Trying it

```sh
ssh root@DECK 'cat > /tmp/azhub.ko' < azhub.ko       # the deck has no scp/sftp server
ssh root@DECK 'insmod /tmp/azhub.ko && echo 2-1:1.0 > /sys/bus/usb/drivers/hub/bind'
```

(`2-1:1.0` for a hub in USB2, `1-1:1.0` in USB1; replugging the hub works too.)
