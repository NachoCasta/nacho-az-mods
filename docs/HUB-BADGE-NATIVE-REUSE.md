# Native source badge reuse

This revision responds to physical review of the first local0.3.1 candidate:
the custom icon and monoline lettering were rejected. The eject selector was
approved and is outside this revision. These findings apply only to firmware
`ep147-9e9454e357389ece`; no compatibility expansion is intended.

## Native bodies

The native image selector at `0x1c872e0` interprets description kind6 as USB,
byte4 as the local USB ordinal and byte5 as the LINK flag. Local ordinals1/2
select existing image IDs `0x14`/`0x15`; generic USB is `0x13`. The provider
registers no numbered USB3..8 bodies, so passing larger ordinals is unsafe.
Presentation changes must never alter the actual transport MediaID.

The original bodies are runtime resources beneath
`/home/root/pdj/DynamicLoad/DeviceIcon/`:

| Family | Runtime file pattern | Confirmed dimensions |
|---|---|---|
| Source | `img_Source_USB1.png`, `img_Source_USB2.png` | 90x90 |
| Browse | `img_StatusArea_USB1.png`, `img_StatusArea_USB2.png` | 95x70 |
| Waveform | `img_Waveform_USB1.png`, `img_Waveform_USB2.png` | 170x41 |

Load these through the native file-image cache; do not package or export their
bytes. Retain originals as read-only handles. Copy construction followed by
`duplicateIfShared` makes a private image, and a distinct pixel-data handle must
be verified before drawing. A private composite preserves native USB icon,
lettering and spacing while adding only HUB/H identification.

## Exact native APIs

| Operation | Address and ABI |
|---|---|
| File image load | `0x255b740`: File pointer x0, Image return through x8 |
| Image copy construction | `0x251e8b0`: destination x0, source x1 |
| Detach shared Image pixels | `0x251eae0`: Image pointer x0 |
| Image destruction | `0x2511a90`: Image pointer x0 |
| Graphics from Image | `0x251c070`: Graphics pointer x0, Image pointer x1 |
| Graphics destruction | `0x251c0c0`: Graphics pointer x0 |
| Draw Image at | `0x251da80`: Graphics, Image, x, y, fill-alpha flag |
| Fill rectangle | `0x251cb60`: Graphics x0; x/y/width/height in s0..s3 |
| Save / restore Graphics | `0x251c390` / `0x251c3e0` |
| Set color | `0x251c5e0`: Graphics x0, ARGB-word pointer x1 |
| Native UI Font factory | `0x20b6928`: height s0, Font return through x8 |
| Explicit family Font | `0x2550d50`: Font x0, String family x1, style w2, height s0 |
| Font destruction | `0x2546a90`: Font pointer x0 |
| Set Graphics Font | `0x251ca30`: Graphics x0, Font pointer x1 |
| Fitted text | `0x25586a0`: Graphics x0, String x1, integer rectangle x2, justification pointer x3, max-lines w4, minimum horizontal scale s0 |
| UTF-8 String construction | `0x24a1180`: destination x0, UTF-8 text x1 |
| String destruction | `0x2469aa0`: constructed String pointer x0; not null-safe |

Image, Font and File/String handles occupy eight bytes. Graphics occupies24
bytes with eight-byte alignment. Font and String are nontrivial native objects;
construction, reference ownership and destruction must follow their exact ABI.
An absolute path represented by a native String has the File layout expected
by the read-only loader. The native UI Font factory selects Tsuku and its own
tracking; that fact alone does not prove equivalence to a pre-rendered caption.

Verified SoftwarePixelData vtable is `0x34f6228`. Format is at `+0x0c`, dimensions
at `+0x10/+0x14`, pixel pointer at `+0x48` and pixel/line stride at `+0x50/+0x54`.
Reject unexpected classes, formats, strides or dimensions before pixel access.
ARGB uses premultiplied BGRA bytes. Transparent source-over fills do not erase
existing pixels; an explicit private-image clear is needed for replacements.

## Emulator experiments and remaining work

The v4 diagnostic forces native local USB1/2 image descriptions for owned
sources. Its Source/Browse/Waveform bodies confirm the original typography and
icon can be reused. The v5 diagnostic loads the six runtime files before hooks,
clones them and adds HUB/H through native Graphics and Font APIs. Native APIs
and all six expected dimensions work in the emulator. Neither experiment is
a final all-eight-number candidate; neither has been delivered to the real AZ.

Original player-number PNGs exist in all three widget families (Source22x22,
Browse16x16 and Waveform22x22). They may provide a native numeral source for
USB3..8, but their cropped masks must first be compared against native USB1/2
caption digits without scaling. A nonzero comparison return only means the
measurement ran; only an exact dimension-and-coverage match proves equality.
If those glyphs differ, the next step is native font matching, not a guessed
replacement. Native assets and raw firmware research remain local.

Current experimental screenshots are saved under
`local/diagnostics/hub-native-ui/`. Final comparison screenshots and exact
candidate evidence must follow implementation of all required numbers.

## Native header and marker geometry

In-memory decoding confirms the LINK header background is opaque `#e1e1e1`.
The Source header is22px high; Browse is16px high. Source places its player
child at `(0,0,22,22)` and LINK child at `(22,0,69,22)` (68px resource width).
Browse places its16px player child at `(5,0)` and LINK child at `(21,0,79,16)`;
its95px body is painted with the native5px left inset.

Waveform's player resource is22x22 at `(10,9)`. All four outer edges are
opaque `#e1e1e1`, forming a1px border; the interior is black. Preserve those
native frame pixels when replacing its digit with H. The separate native
underlay is `(6,9,25,25)` in `#232323`. The earlier v5 hand-drawn frame and
`#eeeeee` header are provisional and do not establish final style compliance.

The native-image helper now compares every pixel outside explicit edit
rectangles, including alpha, without exporting resource pixels. Invalid or
whole-image exclusions fail. Synthetic positive and negative controls pass
with GCC and ASan/UBSan, including an unexpected alpha-only change outside the
allowed region. Runtime use on the final composite remains pending.

## Geometry diagnostic v8: emulator evidence

The isolated v8 diagnostic package SHA256 is
`34781c4bc283bb1db068a9ddaac23a320e507add42482b1b1cbbe32eb8648be9`;
ELF SHA256 is
`e8748594d035fea0e0c5095b72a4a1dbcf2262b46b75a61a0c14802b70894ddd`.
The normal emulator framework activated PID655280 with this staged ELF hash,
empty decks and completed framework status. No physical AZ delivery occurred.

Runtime comparisons found zero changed pixels outside declared edit regions
for all six native USB1/2 bodies:6120 checked pixels per Source image,5130 per
Browse image and5658 per Waveform image. Both native Waveform frames retain
all84 outer border pixels exactly. Source headers now use22px and Browse16px,
with native `#e1e1e1`. The HUB/H font choice remains provisional.

Source player glyph comparisons differ from caption coverage by at most one
8-bit level (14 and25 differing pixels). They are not exact matches. Browse
comparisons against the actual `#232323` matte still differ substantially
(maximum147 levels for USB1), so background rounding alone does not explain
the mismatch. Padding the diagnostic USB2 crop to8px also fails. Waveform
player glyph dimensions remain different. Search other native numeral resource
families before selecting a final rendering method.

Local receipts and metadata-only logs are in
`local/integration/hub-native-geometry-probe-v8/`; the inspected Source screenshot
is `local/diagnostics/hub-native-ui/source-native-geometry-v8.png`. This is a
two-number diagnostic, not final all-eight-number acceptance. The production
adapter still needs replacement; its existing custom3..8 assets are not accepted.

## Decoded numeral diagnostics v9/v10

V9 tested two Browse and five Waveform embedded numeral families with matching
bounding boxes. None matched the native captions; v10 direct-alpha and matte
checks also reject them. Equal dimensions alone are insufficient.

V10 found an exact Source glyph relationship: native orange numeral resources
`0x30be440` (length`0xbc`, digit1) and `0x30be050` (length`0x140`, digit2)
have identical alpha and identical decoded premultiplied red channels to every
pixel in the corresponding native white Source caption digit. Both error sums
are zero for both digits. Repeating that decoded red channel into B/G preserves
the exact native gray glyph; no alpha reconstruction or font approximation is
needed. Full-family digit-order verification and runtime composition remain open.

V10 package SHA256:
`47b6a0078fa79f1390fd560d846090dd06c41310bf9f638cd90fc56d5e193584`;
ELF SHA256:
`cd2589eab020abc330ca2207d2fb5c74a230637775589410acc6228a008b667b`.
Emulator PID669315 emitted the measurements. Local metadata-only receipts are
under `local/integration/hub-native-alpha-probe-v10/`. This does not establish
final eight-number rendering or visual approval; no physical AZ was changed.

The Source constructor at `0x1ef3bb8` loads12 resources (four digits, three
color states), selecting `digit_index * 3`. Thus the matching36x18 family
contains only1..4, with a second identical four-digit family elsewhere. It does
not supply5..8. Orange3 is `0x30bdc20`/`0x171`, crop `(13,1,10,16)`; orange4
is `0x30bd8d0`/`0x114`, crop `(13,1,10,15)`. Do not extrapolate sibling addresses
or claim an all-eight-native-number implementation from this four-digit proof.

The native-image helper supports bounded literal-ink transfer into a detached
body, optionally repeating its premultiplied red channel as gray while retaining
alpha. It rejects aliasing with the original or ink resource, invalid formats
and rectangles. Synthetic positive, unchanged-original and invalid-input
controls pass plain and ASan/UBSan. A full-image runtime equality check after
Source1/2 transfer is the next gate, separate from the earlier mask comparison.

## File-resource diagnostic v11

The full-image transfer gate passes: after replacing the native Source1/2 digit
with decoded orange-family red/alpha ink, all8100 pixels in each90x90 image
remain identical. This verifies actual transfer, not just matching crops. It
does not extend the four-digit family to5..8.

Fourteen native DynamicLoad digit-file families were tested against both caption
digits in all three views. None supplies a matching Browse or Waveform family.
The small time-family1 has6x16 ink, against Waveform's7x16; its2 has11x16 but
different alpha. The countdown family has12px ink height, against Browse's10px.
Do not rescale these rejected resources and claim exact native reuse.

V11 package SHA256:
`f19a266260204b477a8a75b771fdc30f2a5b676ac523dfc805ecb50c7d21a32a`;
ELF SHA256:
`5cdfdb5a2297b4468abbb1972b0e6e758129891b70056e2467b2a087002bf753`.
Normal emulator activation completed with PID678220, empty decks and the same
staged ELF hash. Local receipts are in `local/integration/hub-native-file-probe-v11/`.
The next research path is verification of actual native Font family/style
resolution before further font matching. No final eight-number implementation,
user visual approval or physical AZ delivery is claimed.

## Native Font resolution and parameter controls

The original height-only font probe did not test named `DB`/`B` styles or
horizontal scale. Its failure does not establish that no native face can match.
The production text helper now exposes explicit constructor styles0..3 and
finite horizontal scale0.5..2, retaining the existing plain/scale1 wrapper.
The factory path rejects nonzero constructor styles because its verified API
does not accept one. Plain and ASan/UBSan tests cover dispatch and invalid input.

Verified exact-firmware APIs for a bounded diagnostic:

| Operation | Address and contract |
|---|---|
| Horizontal scale | `0x2547b30(Font*, float)`; copy-on-write |
| Named style | `0x2547270(Font*, const String*)`; copy-on-write, clears cached face |
| Requested name/style | `0x25471c0` / `0x25471d0`; borrowed String pointers |
| Resolve Typeface | `0x2552c50(Font*)`; borrowed pointer, keep Font alive |
| String width | `0x2554820(Font*, const String*)`; float result |
| Ascent | `0x25542c0(Font*)`; float result |

Font data stores requested name/style at`+0x18/+0x20`, height at`+0x28`,
horizontal scale at`+0x2c` and tracking at`+0x30`. Requested face strings and
Typeface base strings do not prove actual font resolution. For resolved
FreeTypeTypeface vtable`0x34f6610` only, `Typeface+0x178` points to a wrapper,
whose`+0x10` is the public FT_Face. Its family/style C strings are at`+0x28/+0x30`
on the supported LP64 ABI. Reject other classes before reading these offsets.
Do not destroy the borrowed Typeface or FT_Face.

All five approved eject implementation files remain byte-identical to frozen
v3. The local hash comparison is saved as
`local/diagnostics/hub-native-ui/eject-approved-preservation.json`.

## Bounded font-resolution diagnostic v13 and subsequent user decision

V13 queried the actual resolved FreeType faces and tested3944 candidates per
view: all six installed families plus the native UI factory; plain/bold requests
and namedDB/B requests where applicable; heights10..32 in0.25 steps, followed
by per-family height/width refinement. Actual DB/B resolution was confirmed,
including cases where Regular/Bold requests select the available DB/B face.
No tested combination matches the caption digits exactly. Best normalized
coverage errors were0.13774101 Source,0.12954927 Browse and0.10603046 Waveform;
these scores are diagnostic discrepancies, not acceptance thresholds.

The exact native icon/USB lettering/layout can be retained, but complete matching
numerals1..8 have not been established. The native numbered body images only
supply1/2, and the proven matching Source numeral family supplies1..4. The user
was initially asked to choose between preserving that body design with native text for
additional numerals, or moving the1..8 identification into the HUB header while
retaining unnumbered native USB artwork. At that stage these were proposed
alternatives, not a claim that every possible native resource was exhausted.

The user subsequently approved the v13 Browse/Waveform appearance and agreed
to use the closest native font for additional caption numerals3..8 only. Preserve
the exact native USB icon, USB word and spacing; retain original native numbered
bodies for1/2. This permits the agreed numeral approximation and ends font
research. It does not establish pixel equality for the new numerals or approve
screenshots that have not yet been produced. Final3..8 comparison screenshots
remain pending before another physical candidate delivery. The approved eject
selector remains outside this revision.

V13 package SHA256:
`50c8a34a841eaee88c5268b249cec984d76e8cfecd209e0a62464700b14fbf45`;
ELF SHA256:
`82e982533070ce293236351addeef68c98a2b298a8b37bbcf4b7579f75349200`.
Emulator PID690116 emitted the results. Exact receipts and the bounded search
definition are in `local/integration/hub-native-font-resolution-v13/`. V12 was
an intermediate local build only; it was not activated or delivered to the AZ.

Saved and inspected v13 screenshots under `local/diagnostics/hub-native-ui/`:
`source-native-v13-control.png` shows both native and hub USB1/2 rows;
`browse-native-v13-control.png` and `browse-native-usb1-v13-control.png` show
the same fixture track through hub and native USB1 respectively;
`waveform-native-v13-comparison.png` shows hub H/USB1 on deck1 and native USB1
on deck2. Both use the same fixture track. These demonstrate unchanged native
USB1 typography in the revised diagnostic. The later user approval covers the
shown Browse/Waveform appearance and the additional-numeral method, not final
numbers3..8 screenshots or physical audio acceptance. The emulator ends with both tracks
loaded; unload them before any subsequent managed candidate reload.

## Integrated native-body candidate v14

The user authorized the nearest native font only for the additional digits3..8
and accepted the shown Browse/Waveform layout. The Source banner retains the
native LINK banner's22px overlay, including its expected icon overlap; the
provisional20px adjustment was removed after the user's clarification.

`source-badge-native-compose.h` loads the original six USB1/2 body images at
runtime and prepares24 detached cache entries before installing hooks. USB1/2
captions remain literal. For3..8, a temporary private canvas renders one native
glyph, crops its alpha bounds and transfers it without image resampling into
the original numeral region. Icon, USB lettering and spacing outside the
expected header/marker and numeral regions must remain byte-identical. Every
entry checks this at preparation; a mismatch refuses preparation. Originals
remain read-only and partial construction releases temporary handles.

Nearest face/height/horizontal scale: Source Tsuku18.625/0.975, Browse
Tsuku13.125/0.975, Waveform PioneerDJ-FZLanTingHei-DB-BIG5 24.625/0.95. New glyphs
are baseline-aligned to the native caption. These are approved approximations
for the additional numerals, not exact matches to the original numeral artwork.
The rejected custom PNG header is no longer used or packaged.

Candidate package SHA256:
`a449ef3122c4da039c5bd0d3342b3a686e1bdb7862cf71831e2dd6f9db684dc8`;
ELF SHA256:
`60be7a089fe6cc71dcb404cfd80b96e2218da5f7a2ec63d36b8b70741a8bf462`.
Pinned SDK build/package checks pass and the ELF requires GLIBC2.17. Frozen
source checks pass29 portable components in each of plain, ASan/UBSan and TSan;
SDK UI checks pass10/8/8. TSan initially could not make its required personality
syscall under Docker's default seccomp profile; its scoped container-only rerun
passes with seccomp unconfined, no network and no host configuration changes.
Registration/order and zero/double negative controls pass separately using WSL
GCC because the host-test container has no Python. The source-settings test
uses the existing repository research fixture, omitted from the package export.

Emulator PID717906 activated exact v14 and measured all24 composites with zero
outside-region differences. New numeral bounds fit their declared regions.
The five approved eject implementation files remain unchanged. Transport
identities, ownership gates, native controls and independent libraries/caches
are unchanged by this presentation adapter. Independent source review found no
concrete lifetime or ownership issues.

The isolated v15 screenshot build changes one presentation-only line after
ownership validation: existing display1/2 become3/8. Its package SHA256 is
`3285e31b1f3b54dcd88e26bbde959e3b2a1f91b90a74e89cc2717845208708bb`;
ELF SHA256 is
`afa4419fd5e23f8e45e7dfb41b0ce26e1e1b0ea40a43ddd0efc99815d6d08bc7`.
This does not change MediaIDs, mount routes, server/cache identities or the
production display map. It demonstrates representative numeral rendering with
two attached hub libraries, not eight-source device acceptance. Restore v14
after screenshots. Final new-numeral visual approval and physical acceptance
remain separate, pending user decisions.
