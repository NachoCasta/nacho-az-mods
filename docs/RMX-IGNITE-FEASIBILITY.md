# RMX-IGNITE digital send/return feasibility

Status: unresolved; blocked pending transport evidence. Updated 2026-11-05.
This public summary contains published references, research questions and acceptance
criteria. It excludes private firmware analysis, binary metadata, memory layouts,
patch locations, local paths and device observations.

## Research objective

Determine whether an existing XDJ-AZ could support a usable RMX-IGNITE digital
send/return loop through a community mod, evaluating top USB-A and rear USB-C
separately. The proposed scope excludes hardware modifications and manual changes
to internal firmware, OS, services, core, settings or process memory.

A positive verdict requires evidence of a complete implementable architecture or
an authorized end-to-end demonstration. A negative verdict requires a demonstrated
necessary limitation across the permitted alternatives. Missing documentation,
a missing API or failure of one approach cannot establish impossibility.

No mod implementation, build, emulator execution, installation or hardware test
was performed for this investigation. No definitive YES or NO is established.

## What public documentation establishes

- AlphaTheta documents digital send/return through the RMX MULTI I/O connector
  with compatible mixers. A connector adapter alone does not establish XDJ-AZ
  compatibility. See the [official connection FAQ](https://support.alphatheta.com/en-US/articles/53214453590656?product=51352009960346).
- The RMX manual states that PC/Mac connections do not send or receive audio.
  This does not identify the protocol used with compatible mixers, or prove that
  another implementation is impossible. See [troubleshooting](https://downloads.support.alphatheta.com/manuals/dj-effectors/RMX-IGNITE/HTML/en/RMX-IGNITE/Additional_information/Additional_information.htm?rhtocid=_12).
- The RMX has a 96 kHz audio engine. Its published engine and sample-file rates
  do not establish its USB streaming formats. See [specifications](https://downloads.support.alphatheta.com/manuals/dj-effectors/RMX-IGNITE/HTML/en/RMX-IGNITE/Specifications/Specifications.htm?rhtocid=_11).
- MIXER DELAY SYNC compensates send/return latency for compatible mixers and
  returns to ON at power-up. The utility documentation does not describe its
  messages, numerical delay or clock mechanism. See [settings](https://downloads.support.alphatheta.com/manuals/dj-effectors/RMX-IGNITE/HTML/en/RMX-IGNITE/Settings/Settings.htm).
- The XDJ-AZ manual documents its rear USB connection for PC/Mac use. That does
  not establish support for hosting an external effector. See the [rear panel](https://downloads.support.alphatheta.com/manuals/all-in-one-dj-systems/XDJ-AZ/html/en/000COV_en/Part_names/Part_names.htm).

## Port-specific disposition

| Candidate | Public research disposition | Evidence still required |
|---|---|---|
| Top USB-A | Remains a candidate; RMX compatibility is unproved | RMX transport, host compatibility, integrated audio return and coexistence with storage |
| Rear USB-C | Remains unproved; PC/Mac connectivity is insufficient | Physical host-role capability, power/role requirements and the complete audio loop |

Neither candidate has a demonstrated complete architecture. This summary does not
claim that a driver, connector, recording feature or audio API alone proves support.

## Requirements for a decisive verdict

1. Identify the RMX normal-operation USB descriptors, configurations, alternate
   settings, endpoints, audio formats and compatible-mixer initialization.
2. Establish the exact AZ send selection and insertion point, return routing and
   suppression of the original audio, without creating a feedback loop.
3. Establish a supported framework integration path, ownership alongside other
   mods, activation behavior and safe bypass when integration is unavailable.
4. Resolve sample-rate conversion where necessary, clock drift, buffering,
   sustained full-duplex operation and acceptable measured round-trip latency.
5. Establish cue, master, booth and recording behavior for the proposed insert.
6. Verify reconnect, unplug, failure recovery and storage/hub coexistence.

USB enumeration, recording, PRO DJ LINK synchronization and an analog master-output
connection do not satisfy the requested digital send/return outcome.

## Evidence acquisition and emulation

No verified public descriptor dump, successful-session USB capture or emulator
reproducing this exact link was located during the research. This describes the
search outcome, not proof that such material cannot exist.

A useful external artifact would include complete RMX descriptors and the control
transfers that initialize and start streaming with a supported mixer. A trace must
also account for clock/feedback behavior and mixer-delay compensation. Capturing
a separate mixer-to-RMX link requires an analyzer or equivalent observation on
that link; a computer cannot passively observe another host's USB traffic merely
by running a USB capture utility.

Firmware-based partial emulation is a possible research avenue, not a demonstrated
solution. A mock built from assumed protocol behavior cannot independently validate
those assumptions. An emulator would need independently grounded USB behavior;
physical timing and electrical compatibility would still need separate evidence.

If suitable evidence becomes available, compare it with the permitted AZ integration
options before proposing implementation. Later implementation and hardware tests
require their own authorization. One failed host/configuration test is not a NO.

## Effort and decision status

A reliable implementation estimate is premature while the transport and integrated
return path remain unproved. Effort depends on whether the link uses supported
standard audio behavior, additional initialization or a different transport, and
on whether a permitted return architecture can satisfy timing and monitoring.

The research objective remains unfinished. This summary is not a recommendation
to implement, purchase equipment or change the AZ.
