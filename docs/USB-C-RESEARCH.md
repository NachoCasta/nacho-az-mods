# Rear USB-C host feasibility

Investigation date: 2026-10-17. This is research, not implemented support.
The user authorized read-only SSH inspection with an unpowered USB-C hub and
pendrive already connected to the rear port. No AZ file writes, mounts, role
changes, regulator changes, driver reloads, installations or restarts were performed.

## Conclusion

The rear hub does not work in the current configuration. The kernel fixes the
likely rear-port controller in peripheral mode. Its normal role-change interface
explicitly rejects changing that initial role. The GPL board description does
not expose a working rear-port VBUS switch or a Type-C configuration-channel
controller. Enabling a named regulator is therefore not an established way to
power this connector.

There is some software potential: the kernel includes dual-role DWC3 support,
and the SoC description contains both host and device paths. This establishes
driver capability, not a dual-role connector implementation. Hardware support
remains unresolved; source inspection cannot establish the CC resistors,
connector wiring, power switches or protection components on the AZ board.

Do not claim this connector as USB3 storage or propose a simple sysfs toggle.
The 5 Gb/s xHCI root hub belongs to a different controller from the configured
XDJ-AZ computer-facing gadget.

## Sources and physical-port mapping

The [official rear-panel manual](https://downloads.support.alphatheta.com/manuals/all-in-one-dj-systems/XDJ-AZ/html/en/000COV_en/Part_names/Part_names.htm)
documents the rear USB connector for a PC/Mac. It does not document USB storage
hosting there.

The live configfs gadget provides additional mapping evidence:

| Read-only path | Observed value |
|---|---|
| `/sys/kernel/config/usb_gadget/g1/UDC` | `fe800000.dwc3` |
| `g1/strings/0x409/product` | `XDJ-AZ` |
| `g1/strings/0x409/manufacturer` | `AlphaTheta Corporation` |
| `g1/configs/c.1/` | Links to `hid.usb0` and `midi.usb0` functions |
| `/sys/class/udc/fe800000.dwc3/state` | `not attached` |

The configured HID/MIDI gadget strongly associates `fe800000` with the
documented computer connection. A controlled future PC connection could verify
that mapping through a change in this exact UDC's state. This investigation did
not request another connection or run that test. No schematic proves the mapping
independently yet, and the HID/MIDI configfs function list should not be interpreted
as a complete account of all computer audio/Link hardware paths.

## Live controller and power evidence

| Item | Observation |
|---|---|
| Kernel | `4.4.194` |
| Board model | `Rockchip RK3399 ATC Board (Linux Opensource)` |
| `/proc/device-tree/usb0/dwc3@fe800000/dr_mode` | `peripheral` |
| Same node, `maximum-speed` | `high-speed` (480 Mb/s ceiling under this policy) |
| `/sys/bus/platform/devices/usb0/dwc3_mode` | `peripheral` |
| `/sys/kernel/debug/fe800000.dwc3/mode` | `device` |
| `/proc/device-tree/usb1/dwc3@fe900000/dr_mode` | `host` |
| `/sys/bus/platform/devices/usb1/dwc3_mode` | `host` |
| Host `fe900000` | Empty USB2/USB3 root buses 5/6 in the preceding authorized inspection |
| `/proc/config.gz` | `CONFIG_USB_DWC3_DUAL_ROLE=y`, `CONFIG_USB_OTG=y`, `CONFIG_PHY_ROCKCHIP_TYPEC=y` |
| OTG state machine | `CONFIG_USB_OTG_FSM` not set |
| Extcon 0, USB2 PHY `e450` | `USB=0`, `USB_HOST=0`, `USB_VBUS_EN=0` |
| Extcon 1, USB2 PHY `e460` | `USB=0`, `USB_HOST=1`, `USB_VBUS_EN=1` |
| Type-C / dual-role classes | `/sys/class/typec` and `/sys/class/dual_role_usb` absent |

The user's hub LED did not light. That is user-reported behavior, not a voltage
measurement. No new rear hub or stick enumerated. Existing devices remained on
EHCI buses 1/2; this does not map those ports to the empty xHCI buses.

Both `vcc_otg_vbus_typec0` and `vcc_otg_vbus_typec1` appear in
`/sys/kernel/debug/regulator/regulator_summary` with use count zero. Typec0 lists
`phy-ff770000.syscon:usb2-phy@e450.1` as a consumer. These entries report a nominal
5000 mV configuration, not a measured connector voltage.

Crucially, both live regulator nodes lack a `gpio` property. The live `/usb-5v`
node lacks `host1-gpio` and `typec1-gpio`. Boot logs explicitly report failure to
parse those four properties. The driver nevertheless logs probe success. Thus
neither the regulator names nor the success message establishes an electrical
power-control mechanism. The boot message `vcc_otg_vbus_typec0: disabling` cannot
by itself explain or fix the lack of hub power.

Visible I2C clients were RK808 (`1-001b`), SYR827 (`1-0040`), SYR828 (`1-0041`),
the touch panel (`3-0014`) and EEPROM addresses `4-0050` through `4-0053`.
No Type-C controller client was observed. This does not exclude passive CC
resistors, an unmanaged chip or control by another processor. No active I2C
address scan was performed.

## GPL source findings

Paths below are relative to the locally preserved GPL tree's `kernel/` directory
(`/home/icast/xdjaz-gpl/XDJ-AZ/kernel`), not files copied into this repository.

| Source reference | Finding |
|---|---|
| `arch/arm64/boot/dts/rockchip/rk3399.dtsi:426` | DWC3 instance 0 at `0xfe800000`; USB2 PHY `u2phy0_otg`, USB3 PHY `tcphy0_usb3`; default OTG declaration, capped at high-speed |
| Same file, line 461 | DWC3 instance 1 at `0xfe900000`; separate USB2/USB3 PHYs; host role |
| `arch/arm64/boot/dts/rockchip/rk3399-atc-TVI2326-V02.dtsi:162` | Typec0 fixed regulator has no GPIO; typec1 at line 170 also has no GPIO |
| Same file, line 340 | Typec0 regulator attached as USB2 PHY0 OTG VBUS supply |
| Same file, lines 381-398 | usb0 extcon from USB2 PHY0; explicit peripheral override; usb1 explicit host override |
| Same file, lines 51-55 and 416-420 | `usb-5v` selects `usba_xrst_18` pinctrl, GPIO0 B1/B2 output high; no host1/typec1 enable properties |
| `arch/arm64/boot/dts/rockchip/rk3399-atc-board-V03.dts:45` | Includes ATC core-board definitions and declares the observed model/compatibility; AlphaTheta D810 sound nodes provide additional board-family evidence |
| `drivers/misc/5v_en.c:34` | Probe reads optional `host1-gpio` and `typec1-gpio`; skips invalid GPIOs yet still reports success |
| `drivers/regulator/fixed.c:163` | A fixed regulator gets an enable GPIO only when its configured GPIO is valid |
| `drivers/usb/dwc3/dwc3-rockchip.c:92` | `dwc3_mode_store()` records original mode, then returns `-EINVAL` unless that original mode is OTG |
| Same file, lines 124-149 | Accepted switches coordinate extcon worker and PHY mode, rather than just a register write |
| `drivers/usb/dwc3/debugfs.c:317` | Debugfs `mode` write merely invokes `dwc3_set_mode()` under lock |
| `drivers/usb/dwc3/core.c:52` | `dwc3_set_mode()` only updates GCTL port-capability bits |
| Same file, lines 838-875 | Initial peripheral setup initializes gadget only; OTG setup initializes host and gadget separately |

The base SoC's OTG default is overridden by the board; it is not the running
role. The presence of the mode sysfs file or debugfs file is not proof of a
supported transition. In particular, writing debugfs `mode` would skip host
creation, gadget teardown, PHY coordination and electrical role negotiation.

Register-dump output returned the same `0x30c12004` value for GCTL, GHWPARAMS0
and GHWPARAMS3. Those readings are not treated as reliable silicon capability
identification; runtime power state/register-access behavior was not changed to
investigate the anomaly. No `/dev/mem` access was performed.

## Remaining decision gates

1. Confirm controller-to-connector mapping with a separately authorized normal
   PC connection and UDC/extcon observations, or obtain an authoritative board
   schematic. The current mapping is strong inference, not a continuity test.
2. Establish the actual rear connector's CC implementation and VBUS circuit from
   board documentation/component identification. Determine whether it can present
   a source role and provide protected, current-limited 5 V. A device-only resistor
   configuration or absent source circuit is a hardware obstacle, not a mod bug.
3. Search local firmware initialization/control paths for any documented rear USB
   role or power command, including control delegated to the ErP/other processors.
   The reviewed Linux DT and driver interfaces do not expose one. This would be
   further static research; it is not permission to send undocumented commands.
4. Only if electrical support is established, design a complete kernel lifecycle
   transition and rollback locally. That work must account for the bound HID/MIDI
   gadget and the board's fixed peripheral policy. A later hardware experiment
   would require separate user authorization and a concrete recovery procedure.

A powered hub alone does not create host data-role or CC negotiation support.
Do not inject 5 V, bridge power pins, change arbitrary GPIOs, or bypass the fixed-role
guard as a diagnostic shortcut. Wrong power assumptions could cause backfeeding;
an incomplete software switch could break the computer connection or wedge the
controller. None of those experiments was performed or authorized here.

Current development decision: keep rear USB-C hosting unsupported and exploratory.
The meaningful missing evidence is board-level CC/VBUS implementation, rather
than USB storage-library routing. No executable, package or release was changed.
