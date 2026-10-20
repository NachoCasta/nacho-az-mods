# USB Hub Support 0.3.1

Hub libraries now have consistent labels in the native Source, Browse and
Waveform views. Source and Browse show HUB with USB1–8; Waveform shows H with
USB1–8. The mod reuses the AZ's own USB icons, lettering and layout at runtime.
USB1/2 captions remain original; the additional numerals3–8 use the closest
native font. No native artwork or fonts are included in this package.

The USB eject selector has a matching dark design, stable source numbers and
volume names. Its selection follows the same connected stick as the inventory
changes, instead of silently targeting another row. Short-press USB STOP to
select one stick, or hold it to eject the hub. Wait for safe-to-remove confirmation.
Existing loaded-track eject and native playback-cache behavior are unchanged.

## Validation

- The user approved the emulator screenshots, then approved the appearance on
  their physical AZ after installing and activating the exact release executable.
  SSH verified the supported firmware/kernel/core, matching package members,
  exact-candidate approval and normal framework activation with57 hooks.
  This is user-reported UI acceptance, not a new audio, eject or network trial.
- The emulator prepared all24 badges with zero pixel changes outside the
  intended HUB/H and numeral regions. Representative USB3/8 screenshots used a
  presentation-only mapping of two attached libraries; this is not eight-device
  acceptance. Native USB controls and empty Link-mode source recovery were checked.
- All29 portable components pass plain, ASan/UBSan and TSan checks. SDK UI
  checks pass10/8/8; parser/registration and failure controls pass separately.
  Source CI and anonymous release downloads are recorded after publication.

See the [candidate record](../acceptance/hub-source-ui-native-candidate.json),
[physical delivery record](../acceptance/hub-source-ui-native-az-delivery.json)
and [native rendering notes](../HUB-BADGE-NATIVE-REUSE.md).

## Compatibility and known limits

Firmware2.00, exact tag `ep147-9e9454e357389ece`, kernel4.4.194 and Community
Mods core>=1.2.3 only. Displayed2.01 on the tested unit identifies the
AZ-MODS-patched2.00 installation; compatibility has not been broadened.
The checked GPL kernel module remains included. Library servers, caches,
transport identities and native USB sources are unchanged.

Changing PRO DJ LINK On/Off still briefly disconnects and reannounces hub
sources, and loaded tracks can retain source-unavailable triangles and rely on
finite cached playback. Select Link mode before loading hub tracks. Four local
decks are available with Link Off; native two-local-deck mode applies with Link
On. Hub libraries remain local and are not shared with external CDJs.

Earlier0.3.0 network checks used a software peer over Wi-Fi. Actual-CDJ
interoperability, audible phase sync, eight-source hardware, broader formats,
long-session behavior and cache-capacity acceptance remain unvalidated.
AZ-MODS stages files in persistent internal storage; removing the USB does not
unload a running mod.

This release preserves the exact tested package, including its original
experimental candidate manifest wording. Catalog text and these notes describe
the subsequent acceptance. No package member was changed after the AZ test.

Package SHA256: `a449ef3122c4da039c5bd0d3342b3a686e1bdb7862cf71831e2dd6f9db684dc8`.
Executable SHA256: `60be7a089fe6cc71dcb404cfd80b96e2218da5f7a2ec63d36b8b70741a8bf462`.
