# Emulator-first AZ testing policy

## Current authorization

The user clarified that this mod should use the normal AZ-MODS workflow and
explicitly removed the separate remove-USB/full-controller-restart recovery
verification prerequisite. Recovery testing is not a gate for installation or
activation. This supersedes earlier recovery-gate language in historical notes.
It does not establish that an unperformed recovery test passed.

For USB Hub Support / PRO DJ LINK development, validate locally and in the
emulator first, then copy candidates to the mods USB over SSH, use supported
Community Mods installation/approval/activation controls, perform bounded relevant
checks, and use standard application/controller restarts. Do not repeatedly ask
for permission within this scope. Publication, firmware/core replacement and
broader compatibility are not authorized by this policy.

## Device boundaries

Agent file writes on the device are limited to the mods USB. Never manually edit
internal firmware, OS files, services, core libraries, internal settings/approval
files, preload environments or process memory. Use supported framework UI/API/IPC
for managed installation, activation and settings. Do not bypass framework approval;
defer UI interaction to the user when needed.

Explicit user-authorized exception: SSH pre-approval of the exact USB Hub Support
candidate being tested may update its entry in
`/home/root/settings/azmod-community/approved`. Verify the reviewed SHA256 against
both USB and staged executables, back up the prior list on USB, hold the framework
approval lock, preserve all unrelated entries and replace the list atomically.
The related lock/temporary approval-list file is part of this narrow operation.
No other internal file/settings edits or process-memory writes are authorized.
Use normal framework apply/reload afterward, with empty decks.

Use read-only SSH preflight to identify the device, package, loader and deck state.
Do not restart during playback or in-flight writes. Retain a known rollback package
and configuration on USB. Identify candidates by version and hashes. Restore test
settings through supported controls. Physical plugging/unplugging and audio
confirmation require user presence. Builds never deploy as a side effect.

## Persistence and evidence

AZ-MODS caches executable files under `/home/root/settings/az-usbload` and stores
settings/approvals under `/home/root/settings/azmod-community`. Framework-managed
persistent internal writes are allowed; they are distinct from manual internal
editing. Do not describe them as RAM or USB-only persistence. Our mod must not add
its own persistent internal-storage writes; its `/run` and `/tmp` state is volatile.

Removing USB does not unload a running library. An application-only restart does
not reset the kernel patch. Do not promise a particular first-boot recovery result
without evidence. Existing emulator recovery and loader audits retain their
artifact-specific scope; physical recovery remains unverified, without blocking
normal AZ-MODS use under the user's revised instruction.

Preserve exact firmware `ep147-9e9454e357389ece`, kernel4.4.194 and core>=1.2.3.
Keep published artifacts immutable. Separate user-reported audio, instrumented
hardware checks, emulator tests and actual-CDJ interoperability. Historical
instructions requiring per-action permission or mandatory recovery verification
are superseded within this bounded workflow.
