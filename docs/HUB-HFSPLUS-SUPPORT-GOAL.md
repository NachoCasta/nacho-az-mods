# HFS+ USB hub compatibility

Status: active. The user requests support for a USB that works on a native AZ
port but not through the hub. A private, reduced HFS+ image is available locally.

## Required outcome

- Reproduce the failure with the preserved original layout, comparing native and
  hub connections. Distinguish mounting, library discovery and native browsing.
- Identify and fix the actual compatibility gap without changing the original
  USB or requiring users to reformat, rename directories or re-export libraries.
- Verify source discovery, playlists/tracks, loading and both waveform views for
  available analyzed sample audio. Account explicitly for omitted fixture audio.
- Preserve ordinary FAT/exFAT hub libraries, independent servers/caches, native
  USB behavior, source labels/colors, safe eject/reconnect and Link mode behavior.
- Add regression tests at the affected boundaries and record emulator evidence,
  candidate hashes, remaining physical checks and any environment limitations.
- Keep the stable mod ID and exact firmware/kernel/core compatibility. Do not
  publish or replace any release/catalog assets without a new publishing decision.

## Evidence boundaries

The local fixture retains original disk geometry, HFS+ metadata, database and
analysis files, and three audio samples. Omitted audio content reads as zeros;
this is not a complete backup. No proprietary files or raw research are committed.
Discovery, private mount layout and private analysis-root selection are separate
confirmed gaps; see [research](HUB-HFSPLUS-RESEARCH.md). Fragmented database extents
required correct fixture acquisition but are not established as a mod defect.
Physical acceptance must be distinguished from emulator work.

The owner subsequently confirmed that favlab was returned, accepts the emulator
HFS+ evidence for this stage and requests candidate installation/reload with an
ordinary USB regression check. Do not block this stage on access to the original
USB, and do not claim that a future HFS+ physical trial has already passed.
