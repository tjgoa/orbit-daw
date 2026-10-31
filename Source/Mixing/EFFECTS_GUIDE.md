# Effects guide — Build 259

Reviewed the installed Orion Effects guide, Effects index, and first dynamics
references. Existing native Compressor/Reverb/Delay/Chorus/Phaser/Low-pass effects
remain available. This checkpoint completes the general hosting/chain workflow
below; it does not reproduce Orion's complete proprietary effect inventory.

**Insert > VST Effects** lists scanned non-instrument VST3 plugins. Scan/Manage
opens an effect-filtered browser. Main-menu insertion targets the selected
generator/audio channel, or Master if none is selected. For another target,
click an empty insert on its Mixer/Master/return/bus strip: the chooser offers
built-in and VST3 effects for that channel. Audio I/O is validated on the live
instance, because fast VST3 metadata can omit channel counts. MIDI-only plugins
without audio I/O are refused without changing a slot.

A populated effect-name button opens its native GUI when available, otherwise
host parameters. Right-click for **Host Parameters / Events**, bypass, remove,
copy settings and move earlier/later. Serial processing follows the displayed
order. Generators retain three visible slots; Master Section channels (Master,
returns and buses) have four. Internal EQ, gain, saturation, routing and mute
nodes have a separate engine budget and do not consume these user slots.

Copy one effect or the complete insert chain. Paste can append or explicitly
replace the chain, as one Undo transaction. It creates distinct plugin IDs,
preserves settings/bypass and excludes copied automation curves/baselines.
Destination event lanes remain retained; lanes targeting replaced effect IDs do
not silently retarget new instances. Same-project sidechains are retained only
when safe; cross-project or feedback-producing sidechains are disconnected.
Missing effects retain their state bypassed. Rack-linked effects need a separate
rack-aware copy and are currently refused. Capacity refusal leaves the chain
unchanged. Channel-strip contexts also expose chain copy/replace.

Projects/templates now carry a VST3 description catalogue, independently of the
global scan cache. Loading registers installed local descriptors before creating
the engine graph. Older projects without a catalogue can resolve installed VST3
files by exact saved class IDs, or a unique name when no ID exists. Ambiguous or
unavailable matches remain unresolved rather than being guessed. Actual plugin
binaries are external; a catalogue does not make an uninstalled plugin available.

New saves use schema 12 and require Build 259 or newer. The higher internal graph
budget permits four Master effects alongside pre-gain/EQ/saturation/mute nodes;
older builds must not edit that graph under their former total-plugin limit.

Validation: `python3 Tools/test-options-host.py --sanitize --effects` covers menus,
browser filtering, chain state/IDs/Undo, event preservation, sidechain safety,
reorder/bypass, capacity refusal and four visible Master/return slots. Add
`--effect-plugin /path/to/an/installed/effect.vst3` for integration. QDelay was
checked locally: discovery, insertion, native DSP render with finite/non-silent
output, a vendor parameter round trip, cache-independent/fresh reopen and
cross-project clipboard passed. No plugin binary is included in source archives.
Its native GUI and human listening remain manual checks.

## Remaining guide/dynamics work

- MultiFX serial/parallel rack containers and rack-aware state transfer.
- CV-82/RNA-160 features and exact responses. Build 261 adds an original
  sample-peak look-ahead limiter; see LIMITER.md.
- The remaining effect-family chapters and their missing native processors.
- Particular external plugin bus layouts, versions and native GUI behavior.

## Dynamics acceptance criteria (Brickwall increment implemented in Build 261)

The Brickwall increment is described in LIMITER.md. Next: `FxCompressor.htm`.
The current native Compressor is a conventional compressor; renaming it would
not provide look-ahead limiting. Keep the following as acceptance criteria for
an original implementation, and the implemented/tested scope is recorded in LIMITER.md:

- Input gain and release controls; a linked stereo detector and a sample-peak
  ceiling at 0 dBFS. Under-threshold audio should pass unchanged apart from the
  declared delay. Do not claim inter-sample/true-peak limiting without oversampling
  and corresponding measurement.
- Allocate look-ahead buffers outside the render callback. Report latency to the
  native graph so parallel buses, dry/wet paths and offline exports stay aligned.
  Reset correctly on seek/rate changes; retain the final delayed samples at export.
- Expose input level and gain reduction without audio-thread locks or UI writes.
  Parameter changes, bypass, removal, Undo and project reopen need native checks.
- Render impulses and sustained signals across block boundaries and several sample
  rates, including asymmetric stereo, abrupt gain changes, silence and recovery.
  Measure peak ceiling, finite output, transparent below-threshold behavior, linked
  image stability, reported delay and graph compensation before user listening.

For the following compressor topic, compare detector weighting, input/output gain,
attack/release, parallel blend and sidechain behavior independently. Existing
sidechain routing is tested, but does not establish CV-82/RNA-160 DSP equivalence.
The source must remain independent of Orion's proprietary processor code.


## Following topic: Compressor CV-82 — implementation comparison

The pinned native Compressor supplies threshold, ratio, attack/release, output
gain and a sidechain detector gain, with the routed trigger tested in Build 258.
It declares no look-ahead delay. Its sidechain gain is not a general input gain;
its normal stereo detector combines channels before rectification. Therefore
anti-phase material, independent input gain and detector weighting need explicit
acceptance tests for any new original compressor rather than an assumed match.

Still to implement for this topic: an original look-ahead compression response,
independent input gain, detector midrange weighting, latency-aligned dry/wet
parallel compression and input/reduction metering. Preserve the existing native
Compressor type/state when adding a new processor; do not silently reinterpret
older projects' ratio or timing values. Check correlated/anti-phase stereo,
weighted versus unweighted detection, external sidechain isolation, transient
response, parallel cancellation/alignment, parameter events and native reopen.
The manual supplies behavior, not coefficients or authorization to copy DSP.
