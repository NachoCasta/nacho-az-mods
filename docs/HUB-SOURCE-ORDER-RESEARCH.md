# Source row ordering

Current integrated evidence passes native USB1/2 first and numeric hub USB1-5,
including safe native USB1 removal/reconnect after all hub libraries are present.
The current runtime retains PID102793 and an advancing cached deck during that
control. See `acceptance/usb-hub-support-source-presentation-native-reconnect-emulator.json`.
The failed attempts below remain historical evidence, not the current hook set.

Exact EP147 static findings:

- Source controller add path `0x1c338d0` calls model add `0x1c373f0`, refreshes
  the list and finds the selected source by identity through `0x1c34750`.
- The model stores its row buffer at offset `0x08`, capacity at `0x10` and
  row count at `0x18`. Rows have a `0x28` byte stride.
- Constructor `0x1c2df78` stores the source kind at row `0x04` and identity
  at `0x08`. Retained native String handles occupy `0x10` and `0x18`.
- Model add performs native sorting before returning. Reordering must happen
  before the controller refresh/identity-selection step, not after painting.

The portable `source-order.h` helper reorders only hub row positions by their
display numbers. It preserves every byte of each row and leaves native/external
positions unchanged. It rejects invalid numbers, more than eight hub rows or
an excessive model size before mutation. This preserves existing native-first
placement while correcting the hub subsequence; emulator evidence must verify
native-first placement through reconnects as well.

The first emulator trial refused a hook at `0x1c373f0`: AZ-MODS already owns
model add there. Candidate initialization returned `-92`; no playback acceptance
was reached. The test restored the original bytes and cleaned its owned scope.
The failed evidence remains local in `source-order-own-loaded-v1`.

The second trial also found controller add `0x1c338d0` occupied by AZ-MODS.
Refresh `0x1c30340` was accepted, but initialization correctly failed overall.
The trial again restored the original package and cleaned its owned scope;
`source-order-own-loaded-v2` is failed evidence, not playback acceptance.

`source-order-native.h` now claims only Source list refresh `0x1c30340`.
The list's model pointer is at `0x108` (ListBox at `0x30`, model at `0xd8`).
Native update `0x2351d50` establishes that boundary. The outer native wrapper
uses method `0x234eaf0` and retains the inner model at `0x18`.

The first supported-UI activation loaded successfully but left the visible order
at 1,2,5,3,4. A bounded read-only emulator inspection identified the reason:
AZ-MODS replaces getNumRows in the Source model vtable. The prior method-address
check therefore rejected the valid model. The current guard recognizes the exact
Source model vtable `0x2e0b900`, preserving the core-owned method replacement.
Unknown table identities pass through unchanged. Count/capacity checks precede
row ordering. Host controls cover the direct/wrapped paths, a replaced count
method, unknown interfaces, full-row preservation and native call forwarding.
Eight-source shuffled-row, idempotence and fail-closed helper controls also pass.
The hook registration check and AArch64 emulator runtime build pass. The final supported-UI trial shows native USB rows first and five hub rows in
ascending order. Removing USB1 while USB4 is selected leaves 2,3,4,5; returning
it restores 1,2,3,4,5 with USB4 still selected and the same application PID.
The mapped runtime hash is verified and the original QMP port is restored. See
[scoped evidence](acceptance/usb-hub-support-source-order-emulator-progress.json).
Loaded-deck/network regressions, eight-source UI, user review and physical
acceptance remain pending.

The `source-order-own-loaded-v3` and `v4` attempts stopped during host preflight:
`wsl.exe -u root --exec ip netns list` exceeded the harness's twenty-second
timeout. Neither attempt installed the current candidate. They do not validate
or contradict its UI behavior. A separate namespace listing returned empty;
subsequent WSL commands also showed substantial latency. Resolve that local
execution issue before repeating the integrated network workload.
