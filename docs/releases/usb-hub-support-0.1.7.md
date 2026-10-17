# USB Hub Support 0.1.7

Updates the package description to match the simplified catalog text: connect multiple
USB sticks through a hub and browse their playlists and tracks directly on the XDJ-AZ.
Removes implementation details and the emulator test count from the displayed description.

This is a metadata-only patch. The executable, kernel module, helper, icon and licenses are
byte-identical to 0.1.6. Only manifest version, summary and description changed.
Compatibility and known limits remain unchanged; see [0.1.6 notes](usb-hub-support-0.1.6.md).
No new runtime test was performed during packaging. Subsequently, the user confirmed
installing 0.1.7 from our repository and successfully browsing playlists, loading tracks and
playing music on an XDJ-AZ with two sticks on a hub plus one on native USB1 (three total).
This is user-reported manual acceptance; no additional agent-instrumented test is claimed.

Validated with SDK v1.2.3 and compared every package member with 0.1.6.
The existing optional legacy settings-path validator warning is unchanged.

Package SHA256: `2b553cbdd452edeee7be4de8a3a47f7b45d6826cbdc8857cace60ab3eed8371f` (308415 bytes).
Executable SHA256: `86f264c27644b399e07226827874b2e41c9a152925ba03c9479c4eb1aa7ee0ba`.

Refresh the repository in AZ-MODS and update USB Hub Support. The ID remains
`usb-hub-support`. Any legacy `hub-libraries` installation must still be uninstalled
before enabling USB Hub Support; this release does not migrate the old ID.
