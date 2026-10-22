# USB Hub Support 0.4.4

Experimental Ethernet sharing of hub USB libraries over PRO DJ LINK, while retaining local hub browsing, native USB sources, HUB labels and safe eject.

## Behavior

- Each hub source has independent library/file services and caches. Public sources are advertised as rekordbox collection libraries; native USB library slots are not borrowed.
- Four local decks with Link Off, or the native two-local-deck mode with Link On.
- Public services withdraw with Link Off and return with Link On, while local hub mounts remain available.
- Short-press USB STOP to select a stick, or hold to eject the hub. Wait for safe-to-remove confirmation. External players handle source loss according to their own cache; model-specific behavior is not yet validated.
- Correct labelled Ethernet IPv4 enumeration and retain a descriptor-owned parent receive-mode reference for the helper session. The reference is released on helper exit; no persistent NIC configuration is written.

## Validation

On the physical XDJ-AZ, one hub stick passed independent software-client Ethernet discovery, complete browsing of3172 tracks/166 ordered nodes/11022 memberships, and exact complete PDB and MP3 downloads. Native USB1 file access passed alongside the hub. A user-operated native Link Off/On cycle preserved the EP147 process and native/private hub mounts; public withdrawal, rediscovery, complete menus and exact file reads passed again.

Portable components passed97 tests per plain, ASan/UBSan and TSan mode. Emulator coverage includes eight-source capacity, per-source isolation, loaded-source safe eject, stale handles, Link/deck topology and cleanup. An initial emulator mode-recovery run failed with EOF/STALE; two subsequent complete repeats passed, and its original cause remains unresolved. These results do not prove absence of intermittent failures.

## Compatibility and limits

Firmware2.00, exact tag `ep147-9e9454e357389ece`; kernel4.4.194; Community Mods core>=1.2.3. No broader firmware compatibility is claimed. Includes self-built GPL hub-depth and MACVLAN kernel modules with source/build materials.

Actual-CDJ interoperability, rendered audio/phase alignment, Wi-Fi sharing, eight-source hardware operation, long-session timing and cache capacity remain unvalidated. Local private track identities are not translated for external instant doubles/current-deck metadata. AZ-MODS stages files internally; do not describe persistence as RAM-only or USB-only.

The immutable package retains its tested candidate manifest wording. Current physical validation supersedes that manifest's pre-trial MACVLAN wording. See the acceptance records and physical Ethernet report for exact scope, failed-run history and rollback.

Package SHA256: `4bad6bad276e56b7eb33e508f8dc3328df5695271bca35637ca0298ace93a705`.
Runtime SHA256: `ccf155934164689901c50f80f05dd965ee23ac8afad4e0443f6f666cfd7a1963`.
Helper SHA256: `046cd1e6f3101f260fed565de3e759a63e37dfe3dae667b600e36ad89f3fb453`.
