# Hub waveform visibility regression

Status: complete for the scoped regression. The exact corrected hardware runtime
was installed/approved/reloaded through the framework and the user confirmed
waveform restoration. See [completion audit](acceptance/usb-hub-support-waveform-completion.json).
This does not publish0.4.5 or expand actual-CDJ/eight-source/long-session acceptance.

The user reports that physical candidate0.4.5 loads and plays a hub track, and
its device name/color work, but the waveform is absent. Recent emulator captures
also show blank waveforms. The earlier presentation controls checked source
labels, loaded state and moving playheads; they did not assert waveform pixels.
Those checks are not waveform rendering acceptance.

## Required result

- Reproduce with the exact candidate and a track with known analysis data.
  Compare native USB and hub paths for the same track. Distinguish the full-track
  overview from the scrolling deck waveform and establish a native emulator
  positive control before trusting visual emulator results.
- Identify the cause and restore both affected views. Preserve names/colors,
  numbering/order, native artwork, independent source servers/caches, playback,
  safe eject and existing Link behavior and exact compatibility.
- Add automated visual regression assertions that fail on blank/missing waveform
  content, including negative controls. Labels, time progression and successful
  analysis transfer alone must not pass that test.
- Validate locally and emulator-first, retain ignored local captures and scoped
  evidence, then validate the exact hardware candidate through the authorized
  USB/AZ-MODS workflow. Do not interrupt playback or manually edit controller
  internals beyond the documented approval exception. Obtain user confirmation
  that waveforms are visible on the physical AZ.
- Record English documentation and reviewed commits. Public push still requires
  resolving the earlier automatic-review rejection through explicit approval.
  Release/catalog publication requires a separate user decision.

## Regression commands

`test-host.sh` runs the actual announcement and mount-enqueue bodies with native
queue stand-ins. They require retirement before announcement, deadline clearing,
same-source metadata retention during remount and invalidation on replacement.
Native per-worker task ordering still requires the emulator controls.

For local1280x800 two-deck captures, use:

```powershell
python src/azhub/research/scripts/check-waveform-visible.py CAPTURE.png --deck 1
python src/azhub/research/scripts/test-waveform-capture-controls.py COMPARISON.png --native-deck 1
```

The first command exits1 unless both graph regions contain colored samples with
horizontal and vertical coverage. A flat baseline fails. The second checks a real
native-positive/blank-hub-negative comparison and verifies CLI exit status as well
as each independent graph assertion. Captures stay ignored; they are not CI assets.
Retain selected-track/source provenance separately: a screenshot of an unloaded
deck or failed file load cannot establish a waveform regression or restoration.
