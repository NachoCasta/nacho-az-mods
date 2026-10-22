# External hub library physical trial plan

This is a proposed acceptance plan, not a compatibility list. Independent
software-client protocol evidence does not establish actual-CDJ interoperability.
Run physical steps only after action-specific user authorization. Do not deploy
from a build or manually edit controller firmware, OS or settings files.

## Record before each trial

- AZ firmware exact tag `ep147-9e9454e357389ece`, kernel `4.4.194`, installed
  Community Mods core version (minimum1.2.3), candidate package/executable/helper/
  kernel-module hashes, and baseline package hashes.
- External player exact model, firmware version, player number and network path;
  hub make, power arrangement, stick filesystem, library export format and source
  count. Keep music, captures and screenshots local and ignored.
- Separate user-reported audible results from instrumented protocol, mount and
  engine observations. Record failures and untested cells explicitly.

Candidate hashes must come from the final packaged bytes. This plan does not
designate any intermediate diagnostic binary as an installable candidate.

## Per-model matrix

Create a separate result for each available model/firmware combination. Suggested
coverage is CDJ-2000nexus, CDJ-2000NXS2 and CDJ-3000; none is currently confirmed.
Do not substitute one generation's result for another or require buying hardware.

| Gate | Procedure | Required evidence |
|---|---|---|
| Native baseline | With public sharing disabled, browse/load native USB through Link | Model-specific baseline source labels, load and playback result |
| Single hub source | Enable candidate through AZ-MODS; discover, browse playlists/tracks, load and play | Actual player screen/source identity, successful real library requests/file transfer and user playback report |
| Identity range | Repeat with each advertised public identity, including41 and48 | Record accepted/missing/rejected identities; do not infer support from software discovery |
| Source capacity | Increase available hub sources progressively to eight where equipment permits | Per-model visible, browsable and loadable source count; lower tested count remains the accepted limit |
| Concurrent use | Load locally from one hub source and remotely from another, then from the same source | Independent libraries/caches, local playback observation, remote playback observation, no native USB regression |
| Loaded eject | Keep the external deck loaded/playing; eject that source through the native hub selector | Source withdrawal, closed services, backing unmount, no manual external stop/unload, neighbor remains browsable/loadable |
| Reconnect | Reconnect the ejected stick and load a fresh track externally | Rediscovery, fresh access, previous handles rejected as stale where instrumented, no cross-source content |
| Mode continuity | With a local hub track loaded, toggle native Link On/Off/On using normal UI | Local source continuity, four local decks Off/two On, public withdrawal Off and fresh external access On |
| Native preservation | Browse/load both native USB sources alongside the hub | Native source labels, libraries and fresh loads preserved |
| Network recovery | Disconnect/reconnect the authorized network path | Old endpoints withdraw, fresh sessions recover, native/local sources remain usable |
| Identity conflict | Introduce an authorized second participant with a conflicting identity | No takeover of an active peer, bounded retry or explicit unavailable state, unaffected sources retained |
| Input filter timing | After sustained local/remote use, inspect core filter-budget logs and exercise individual eject/encoder ownership | Record any quarantine/selector refusal; emulator observations do not prove physical timing, and SDK guards must remain enabled |
| Sustained use | Run an agreed-duration mix with repeated browsing/loading and eject/replug | Duration, resource observations, errors and recovery recorded; do not extrapolate beyond the actual duration |

For loaded controls, bind the reported source identity, slot and rekordbox track
ID to the exact file being read. Public computer libraries use COLLECTION slot4;
native USB controls use USB slot3. A matching filename alone does not prove the
loaded status refers to the same track. Record the independent PDB mapping.

Outgoing local-hub deck status still redacts private source identities. Public
library browsing/loading is the candidate feature; translation for instant
doubles or complete current-deck metadata is not implemented or promised.

For loaded eject, first reproduce the model's native shared-USB baseline under
the same loaded state. Compare withdrawal/unmount behavior, not uninterrupted
audio: each player handles source loss according to its cache/capabilities.
Report whether audio continues, stops or enters an emergency state without
promising full-track caching. Keep an unaffected source available as a control.

If the model rejects public identities or imposes a smaller source capacity,
retain the exact evidence and discuss the scope with the user before accepting
a reduced outcome. Do not silently relabel partial visibility as eight-source
support.

## Completion and restoration

Record the exact tested hashes, every matrix cell's pass/fail/untested status,
model-specific limits and reproduction steps. Restore the agreed normal package
through AZ-MODS and verify its loaded status. A successful trial of one player
does not establish support for other models, other firmware or multiple AZs.
Publication remains a separate user decision.
