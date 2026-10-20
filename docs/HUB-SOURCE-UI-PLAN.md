# Hub source identity and eject UI plan

## Objective

### Physical review revision

Implementation and emulator evidence for the revised native-body badges are
complete in [the native candidate record](acceptance/hub-source-ui-native-candidate.json).
Representative USB3/8 screenshots are saved and presented for user review.
This completes the implementation/screenshot gate; new-numeral visual approval,
physical delivery and publication remain separate pending decisions.

Subsequent user decision: retain the exact native icon, USB word and spacing.
Keep native USB1/2 captions intact; use the closest native font only for digits
3..8. Browse/Waveform appearance in the v13 screenshots is approved; preserve
those headers/markers. Save representative additional-digit screenshots before
another physical delivery. The earlier requirement for exact digit-mask equality
is superseded only for the newly rendered digits3..8, by this explicit decision.
The Source HUB banner keeps the native22px LINK overlay geometry. Its expected
overlap with the icon is accepted; preserve the original icon resource beneath
that overlay rather than shrinking the header or redrawing the icon.

The first0.3.1 candidate's eject selector was visually approved by the user.
Its custom source badges were rejected: their icon, letter weight, size and
spacing differ from native USB. Preserve the eject selector. The next badge
iteration must reuse native resources and rendering at runtime, with only the
HUB/H identification and consistent number changed. Do not redistribute native
artwork. Save emulator Source/Browse/Waveform comparison screenshots for user
review before any further physical delivery; emulator acceptance alone does
not establish that the revised visual design is approved.

Give local hub sources a consistent visual identity in the native Source, Browse
and Waveform views, and align the existing mod-owned eject selector with the AZ.
Deliver an emulator-validated candidate and saved native comparison screenshots
for user review before any further physical delivery. Published0.3.0 remains
immutable. Publication and fixing the brief source interruption during Link mode
changes are outside this goal. Runtime reuse of native badge artwork and rendering
is required by the physical review revision above; replacing the approved eject
selector with a native widget remains outside scope.

## Required presentation

| View | Hub-only presentation |
|---|---|
| Source | HUB header, USB icon and USB1..8; preserve the volume name |
| Browse | Same HUB header, USB icon and USB1..8 |
| Waveform | H in the member-number badge, followed by USB1..8 |
| Eject selector | Eject USB title, matching HUB / USBn badge and volume name |

Numbers remain stable for connected devices when another device is removed or
ejected. Never show internal member7/8 or the upstream physical hub port as the
user-facing identity. Match the selected source and loaded-track identity rather
than replacing every LINK/USB string globally. Native USB and actual external
player rendering must remain unchanged, including Link Off/On transitions.

The selector keeps its mod-owned surface: dark background, thin borders, subdued
selection, Cancel / BACK and Eject actions. Preserve short/long USB STOP gestures,
background teardown and accurate progress, success and failure notifications.
Closing the selection dialog must not imply that an unmount has completed.

## Implementation and evidence

1. Research exact-firmware renderers, arguments and ownership boundaries. Record
   reconstructed facts and scripts; keep raw firmware/decompilation/captures local.
2. Implement common stable display numbering and source-specific render adapters.
   Keep transport identities, routing and eight independent server/cache slots.
3. Restyle the selector and verify hit regions, encoder/BACK behavior, all eight
   rows, empty states and bounded text on the SDK surface.
4. Build a local candidate with the pinned SDK. Verify native/source controls,
   multiple hub sources, each native view, Link Off/On, individual/whole-hub eject
   and retention of remaining numbers in the emulator. Identify exact artifacts.
5. Write an evidence ledger and concise final AZ test steps. No implicit hardware
   deployment follows from build/testing. Follow the current testing policy.

Completion requires implemented presentation and sufficient current-artifact
emulator evidence for every required view/control. If a renderer cannot support
the planned design, document the limitation and agree on an alternative with the
user before treating the goal as complete.
