# USB Hub Support 0.1.8

Adds a prominent PRO DJ LINK warning to the package and catalog description.

**While this mod is enabled, communication with external PRO DJ LINK players is blocked.**
Disable USB Hub Support and restart the AZ before using PRO DJ LINK with other players.
Switching PRO DJ LINK on does not disable the mod. USB sticks on the hub are available
only within the AZ and are not advertised to external CDJs.

For standalone hub use, leave the PRO DJ LINK user setting off. Turning it on may leave
hub browsing functional, as reported by the user, but does not establish external-player
compatibility. The firmware's normal Link-mode deck restrictions still apply.

This release changes only manifest version and description. Every other package member,
including the executable and kernel asset, is byte-identical to 0.1.7. Automatic disabling
has not been added: detecting the user's setting requires separate implementation and
validation, because the mod also enables an internal Link cache independently.

Package validation passed with SDK v1.2.3; the existing optional legacy settings-path
warning is unchanged. Hardware acceptance remains the user's 0.1.7 test with two hub
sticks plus one native USB1 stick. No new AZ test, restart or deployment was performed.
Other compatibility and known limits are unchanged; see [0.1.7](usb-hub-support-0.1.7.md).

Package SHA256: `320949f7178084d98423470bc9888452cdf415c07257d2ec6b7b00d4a623d44f` (308627 bytes).
