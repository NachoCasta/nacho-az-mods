# Eject callback lifetime and resource ownership

These facts apply only to `ep147-9e9454e357389ece`. They describe control flow
and layouts, not copied firmware implementation.

## Unmount completion adapters

The music unmount enqueue entry `0x1373928` creates a task with vtable
`0x2bb1bf8`. The information-server entry `0x1975468` creates a task with vtable
`0x2caed90`. Both tasks keep the server at `+0x18`, listener at `+0x20` and media
identity at `+0x28`. The listener's completion slot is `+0x18`.

Music task execution is at `0x1375220`. Its terminal callback is invoked at
`0x13753f0`; subsequent work destroys the task's local temporary, without using
the listener again. The task destructor at `0x1371750` does not dereference or
delete the raw listener. The information task executes at `0x1978ba8`; its
terminal path tail-calls the listener at `0x1979008` after restoring its frame.
Its destructor `0x1974718` also does not dereference the raw listener. These are
single-task completion notifications, not subscriptions retained by a server.

The revised adapter therefore uses sixteen static slots: two for each of eight
sources. There is one producer, the eject worker. A pending adapter cannot be
reacquired. A retry retains the existing source withdrawal instead of replacing
its listeners. Once both callbacks have completed and alias teardown finishes,
the injector clears the source pointers before publishing `eject_done`. Only a
later source lifecycle may acquire those slots again. No callback storage is
freed while async work might reference it, and request count does not increase
the storage bound.

The C callback publishes `done` as its final access to the adapter. Source/kind
and vtable remain immutable after the first acquisition. Pointer publication
between worker and injector is atomic. This relies on the exact one-shot native
contract above; it is not an argument for broadening firmware compatibility.

Host tests cover bounds, independent slots, refusal to reuse a pending adapter,
completion from another thread and 100,000 completed cycles over all sixteen
slots. They run under plain, ASan + UBSan and TSan. The final selected executable passed matching main eject and input regressions;
its extended media/resource checks remain in progress. Historical candidate
evidence remains separately identified.

## Other resources still under audit

Servers, mount caches and tagged caches are intended to be bounded by the eight
source slots and retained for the process lifetime. That intention does not
establish that native caches evict correctly or that all constructor temporaries
are released. Audit factory failures, registration nodes, native strings and
cache eviction before declaring resource stability. Whole-mod sanitizer and
sustained exact-binary integration coverage remain separate from adapter tests.

## Reviewed fixed source resources

Each source has one retained music server, information server, mount-cache block
(0x50 bytes), and tagged-cache receiver (0xc0 bytes). The latest local source
prepares all eight tagged receivers before claims. Their native cache entry limit
is initialized to 100; the receiver bound alone does not prove eviction behavior.
Factory retries reuse the mount-cache block, and information/tag allocations are
prepared together before publishing the information server. Registration prepares
all list nodes before publication and frees unpublished nodes on allocation
failure. Published list nodes belong to the native list, with at most sixteen
owner slots tracked by this adapter.

Factory names passed through `0x24876f0` are StringRef views of temporary labels,
not owned JUCE strings; they must not be released as owned strings. Existing
mount-path code constructs an owned string and releases its local reference after
queueing, whose native copy owns the asynchronous reference. See the ownership
correction in `src/azhub/research/GHIDRA-FINDINGS.md`.

These source observations do not replace sustained native cache/reconnect
measurements. They also do not establish cleanup of native constructor-internal
allocations or prove every allocation-failure path safe.

## Additional exact-firmware cache-limit evidence

Local read-only instruction inspection shows the service loads its `+0xb8`
limit at `0x1829e48` and passes it to `0x18246a0`. New per-media cache creation
also passes that value through `0x18247b0` at `0x1829ec4`. The service constructor
sets it to 100 at `0x1828e18`. Thus the private receiver preserves the native
limit input; this does not by itself establish every internal eviction path.
No disassembly or firmware bytes are included here. Reproduce with a local
matching ELF and bounded address inspection, retaining raw output outside Git.

## Owned-resource bounds

| Resource | Bound and lifetime |
|---|---|
| Source sockets | At most eight; bind failures and aborted init close them; absent source slots reuse their sockets. |
| Music/information servers | At most two per each of eight source slots; native async work requires process-lifetime retention. Media withdrawal closes their source state. |
| Mount-cache blocks | At most eight 0x50-byte blocks, reused on construction retries. |
| Private tagged receivers | Eight 0xc0-byte blocks prepared before claims; native limit input remains 100. |
| Completion adapters | Sixteen static slots; pending callbacks exclude reuse until their terminal completion. |
| Registration nodes | At most sixteen tracked owners, eight sources and two 24-byte list nodes each; unpublished batches free on allocation failure; native lists own published nodes. |
| Eject surface | One core-owned surface allocated at init; open/close reuse it. |
| Owned workers | Three SDK threads; normal idle sleeps/polls are bounded, aborted init causes exit. |
| Panel/transport FD metadata | Fixed arrays covering descriptors below 4096; higher/unknown descriptors do not index the arrays. |
| Load and eject ledgers | Sixteen fixed controller/physical-state slots; no per-request linked allocation. |
| Helper work | One fallback launch is awaited at a time; timeout/unknown status latches a fault and prevents further launches/eject claims. |
| Volatile files | One bounded journal, one empty fault marker, bounded discovery output; diagnostics are opt-in and SDK logs are capped. |

These bounds concern resources owned or published by the mod. Native container
internals, allocator retention and firmware-owned threads are separate dependency
coverage; host sanitizer runs do not instrument them. Exact-binary native churn
and resource observations complement source ownership review without proving
absence of every possible vendor-side leak.
