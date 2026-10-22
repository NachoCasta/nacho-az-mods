# Physical hub-port numbering work in progress

The active objective replaces insertion-order display labels with USB1-8 derived
from the physical USB topology. Sources keep their transport identities, native
server objects and independent caches. Published0.4.4 remains unchanged.

## Mapping

Read each parent USB device maxchild attribute in sysfs. Traverse ports in numeric
order, recursively replacing a connected hub port with its leaf ports. Empty
ports consume a label. A four-port secondary hub in primary port2 yields primary
port1=USB1, secondary ports1-4=USB2-5, primary ports3-4=USB6-7. The Linux logical
port order may differ from a casing's printed numbers; internally cascaded chips
appear as nested hubs. Physical correspondence requires hardware observation.

Flattened ports beyond8 cannot receive a label; reserving empty ports means the
number of addressable leaf positions, rather than occupancy, can hit this limit.
Missing required hub metadata or malformed attributes must not fall back to
insertion-order numbering. This numbering policy targets one primary external hub on the AZ hub port.
Two simultaneous primary hubs on different native roots can produce conflicting
ranks; a conflicting second source is not admitted. USB2/USB3 companion branches
and printed casing numbers are not merged heuristically. They require separate
hardware mapping research.

## Source lifecycle

The scanner emits a physical-port annotation with each configuration row. The
loader reserves its visible number separately from its private player/slot.
Reservations survive source removal and are shared by Source, Browse, Waveform
and safe eject. Dormant transports may be reused for the same reserved number.
Conflicting topology changes defer admission instead of stealing an existing
number. Same-mount device moves are handled by canonical physical-route comparison.
Reservations remain fixed until the mod process restarts because a removed
source can still have loaded/cached track references. After restructuring hubs,
finish using loaded tracks and reload normally to obtain a fresh flattened map.
This implementation is not accepted for deployment yet.

## Numeral alignment

The historical Source USB3/USB8 emulator screenshot places the strong digit ink
one pixel above the native USB lettering. At RGB minimum>240, native lettering
occupies rows54-67 relative to the source row, while digit3 occupies53-66.
The Source-only digit destination now moves down one pixel. Native icon, caption,
USB1/2 and Browse/Waveform placement are unchanged. Composition unit controls pass;
fresh emulator rendering ofUSB3 now measures matching solid rows54-67 for both
the digit and native USB lettering. User screenshot review remains pending.

## Current checks

Pure route flattening, nested sysfs fixtures, empty-port reservation, malformed
metadata rejection, display reservations and native composition pass. The target
AArch64 integrated runtime compiles. The first plain host run completed all C
components but could not finish the C++ unwind control because this WSL lacks
g++; this is not a full-suite pass. Sanitizers and integrated emulator topology,
loaded continuity, source reconnection and screenshots are still required.
No physical AZ action or release publication is authorized by this document.

Scanner-only emulator controls now read the real eight-port hub sysfs topology:
five sources retain1-5 after reversing the mount-table fixture, and filtering
ports1/2/4 leaves labels3/5 rather than compacting them. These controls do not
claim runtime source reconciliation, actual hotplug or loaded-track continuity.
Same-path port changes now retire the prior display identity before admission;
this lifecycle change still requires integrated tests. Component sanitizers are
running in the local offline compiler container. Its image lacks Python, so
hook registration is checked separately in WSL rather than skipped as passing.

## Route-aware reconciliation control

Configuration annotations now carry both flattened number and canonical physical
route. An existing source on the same route keeps its reserved display number
even if adding/removing another hub changes its calculated rank. A moved source
is withdrawn first; after native unmount completes, a separate dormant/new
transport for the destination number can be admitted. Returning to an earlier
route waits for the outgoing incarnation to drain before reviving it. The
presence pass withdraws all outgoing routes before considering any revival.

The actual loader and presence function bodies execute in a host control with
native-service stand-ins. It verifies reversed port admission, retained neighbor
socket/server/cache identities, unchanged labels through structural rank change,
move-after-drain and return-after-drain. This control caught and fixed an
iteration-order revival hazard. It does not replace actual emulator hotplug,
loaded-track, public-helper or eject-selector integration checks.

## Actual emulator USB observations

A real QMP detach/move/return trial now passes with unchanged EP147 PID: the
port1 stick appears asUSB2 after moving to port2 and asUSB1 on return. Neighbors
retainUSB3/4/5. Logs show native alias release before dormant transport reuse.
Both original mod bytes and physical emulated ports were restored afterward.
A separate fresh-start trial adds a real QEMU secondary eight-port hub at primary
port2 and attaches the stick to its port3. Source rendersUSB4, reserving primary
port1 and secondary ports1/2 despite their being empty. Later primary leaf
positions exceed8 and are intentionally not admitted on that fresh topology.
These are local runtime/UI controls, not public sharing or loaded playback proof.
Screenshots remain in ignored local diagnostics; see the progress acceptance
record for their directories and remaining gates.

## Sharing regression fixture correction

The first integrated sharing/eject attempt reached complete two-source menus and
exact file transfers but ejected source1 instead of source0. Linux sdX identities
changed after real nested-hub hotplug, while the historical harness assumed its
library root was always sdc1. The lifecycle client correctly rejected acceptance
because source0 remained exported. An upfront sysfs physical-route guard now
requires the supplied root to match protected port3-2.1; integrated eject permits
the observed device name rather than weakening that route guard. Repeat pending.

## Final component checks and candidate

Final checks pass98 components per plain/ASan+UBSan/TSan mode, SDK UI controls,
hook-registration extraction and the actual loader/presence control. Source,
Browse and Waveform callback tests now verify that reserving display8 for the
first internal source selects8 in all three families. The scanner retains all32
bounded observed routes for presence reconciliation while the loader enforces
eight transports; a9-route fixture proves lexicographic port10 cannot truncate
valid port8. Unknown/out-of-capacity routes remain unavailable for new admission.

The ignored0.4.5 candidate is packaged and validated with all17 expected members,
unchanged helper/kernel assets, executable asset modes and exact hash records.
It has not been installed on the physical AZ or published. A local source commit
exists; automatic approval review rejected pushing the unaccepted feature to the
public main branch. No push workaround was attempted. Public release0.4.4 remains
unchanged. Final own-source local playback testing and user gates remain open.

## Final own-source loaded regression

The final exact emulator runtime passes real native hub-track loading with the
physical bind verified as the selected source0 disk. Loaded/playing engine
samples remain valid before independent clients, after complete clients, after
own-source native eject and after reconnect. Complete two-source Beat Link menus,
exact Crate Digger files, original neighbor sessions and stale old handles pass.
The native four-beat loop is intentional; no rendered-audio or full-song playback
claim is made. Original mod/approval bytes, emulator USB topology and owned network
resources are restored. See the final own-loaded acceptance record.

The earlier neighbor-loaded run proved a different, useful case: the historical
row offset selected source1 and its native bind was verified. The corrected
harness accepts an explicitly reviewed source-row offset and still asserts the
actual requested backing; source0 own-loaded acceptance uses offset-1. A prior
selection-file timeout remains a failed pre-load attempt, not a mod failure.
