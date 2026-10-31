# Pro-9 / TR-909 implementation research — 2026-10-30

Initial research checkpoint after Build 270; implementation updates in Builds 271–272.
Geonkick DSP is now vendored for a limited audition instrument. The final
eleven-voice Pro-9 engine has not been selected. See DRUM_AUDITION_GUIDE.md.

## Build 272 adoption update

9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9

The pinned v2.7.2 engine is now available as **9W9 909 Audition**, a modified
Orbit port, not the original and not made or endorsed by A Thousand Details.
See NINEKIT_GUIDE.md and ThirdParty/9W9/UPSTREAM.md for adopted code/resources,
sample hashes and the retained GPL/section 7 notices. The adapter corrects sample
rate, adds native parameters/automation, GM/pad maps, step Accent and a protected
demo. Audio and host integration checks pass. This is an audition candidate;
full Pro-9 controls, per-drum pan and 6/11 stereo routing remain unimplemented.
The pending-work language below records the earlier research stage.

## Recommended direction — updated by user clarification

The user explicitly wants GPLv3 candidates included: access to reusable copyleft
DSP is one reason for making Orbit open source. MIT is not a requirement for this
edition. Orbit's existing LICENSE and README identify the application as AGPLv3;
Tracktion and JUCE licensing is separately recorded in DEPENDENCIES.md.

Evaluate existing GPLv3/AGPLv3 and permissive DSP on technical fit, rather than
preferring a new implementation solely because a candidate is not MIT. Compare
several established engines before selecting a base; the user subsequently asked
for a broader popularity/technical comparison and a Geonkick audition path.
9W9 remains one candidate. See DRUM_ENGINE_COMPARISON.md. Preserve upstream licenses, attribution and
modification notices. Build 271 adopts the credited Geonkick DSP subset for
audition; no external drum sample bank or complete 909 engine is adopted.

The earlier MIT-only recommendation below is superseded by this wider scope.
No claim of hardware/Orion sonic equivalence follows from a repository's name.

## GPLv3 candidates — 2026-10-30 follow-up

### 9W9 — direct 909 candidate, pending broader comparison

https://github.com/athousanddetails/schwung-9W9
https://github.com/athousanddetails/schwung-9W9/blob/main/LICENSE
https://github.com/athousanddetails/schwung-9W9/blob/main/NOTICE

Verified the GPLv3 license and read the full additional NOTICE. It requires
attribution to 9W9 by A Thousand Details with the repository link in source,
documentation and applicable credits/About/legal notices, plus marking modified
versions. These must be implemented if code is adopted, not dropped during a port.

Inspected src/dsp/9w9_engine.h and 9w9_circuit.h and the source directory. Plain C
core with eleven trigger IDs: kick, snare, three toms, rim, clap, two hats, ride and
crash. The model files contain per-voice circuit/envelope/filter processing;
hats/cymbals use sampler buffers. Its Ableton Move wrapper and GUI need not become
Orbit dependencies. Verify sample provenance against ER-99 and pin full revisions
before bundling. Its global accent/velocity policy differs from Pro-9's separate
accent lane; Pro-9's control ranges, note scheduling and 6/11-output routing still
need an adapter. Audit sample-rate assumptions, denormals, realtime allocation,
parameter smoothing, retrigger/choke and output access before selection.

### ER-99 — original GPLv3 909 reference / possible voice source

https://github.com/matthewcieplak/er-99
https://github.com/matthewcieplak/er-99/blob/main/LICENSE

Verified GPLv3 LICENSE and developer README: WebAudio/TypeScript synthesized drum
voices with sampled hats/crash/ride. Useful original voice graphs and historical
reference for the 9W9 sample resources. A native port still needs technical work.

### Geonkick — general percussion engine

https://github.com/quamplex/geonkick
https://github.com/quamplex/geonkick/blob/main/LICENSE

Verified GPLv3 LICENSE and developer README. C/C++ percussion synthesis with
layered oscillators, amplitude/pitch/filter envelopes, distortion, 16-instrument
kits, choke behavior and output mapping. This is a broader engine candidate;
it is not presented as a dedicated TR-909 or Pro-9 emulation. Reviewed source
layout separates src/dsp from the application UI.

### Current André Michelle TR-909 — AGPLv3 candidate also allowed in scope

https://github.com/andremichelle/tr-909/blob/main/LICENSE

Current AGPLv3 status is no longer a reason to exclude it from the open Orbit
edition. Its technical suitability and sample dependencies still need evaluation.
The historical MIT-revision research below remains a provenance record, not a
constraint to use only that revision.

## Orion behavior reference

Reread the complete installed `GenPro9.htm`, extracted locally to
`/private/tmp/orbit-orion-manual-review/GenPro9.htm` from the installed Orion Help.
The reference manual and artwork are not redistributed with Orbit source.

- Eleven drum instruments; each has Level, Pan, Tune and Decay.
- Kick adds Attack, Tone (filtered-noise/pulse blend), Tune Start and Tune Env.
- Snare adds Snappy (noise level), Tone (noise decay), Tune Env. Its ordinary
  Decay affects the body separately from the noise.
- Velocity and a separate sequenced global Accent row both affect hit emphasis;
  the global Accent knob sets the latter's strength.
- Instrument selection below the controls; 16-step paging, then the regular
  Piano Roll for precise timing/velocity and longer patterns.
- Stereo, six stereo groups (kick, snare, toms, clap/rim, hats, cymbals), or eleven
  individual stereo outputs. Multiple-output hosting is explicit work, not an
  automatic property of adding a stereo drum synth.

## Primary references and code actually checked

### André Michelle TR-909, historical MIT revision

https://github.com/andremichelle/tr-909/tree/11d4233
https://github.com/andremichelle/tr-909/blob/11d4233/LICENSE

The LICENSE at `11d4233` is MIT, copyright 2022 André Michelle. Current `main`
uses AGPL-3.0 instead:
https://github.com/andremichelle/tr-909/blob/main/LICENSE

Do not mix current AGPL files into a proposed MIT-derived component. Any reuse
requires pinning and recording the full selected historical commit and file
hashes with its notices; this research uses the short commit URL only.

Inspected old `typescript/audio/tr909/resources.ts`, `dsp/bassdrum.ts` and
`dsp/snaredrum.ts` plus the DSP directory. This is TypeScript, not a drop-in C++
engine. It loads `.raw` resources for every instrument. The kick combines a
sampled cycle oscillator and attack; the snare reads tone/noise resources.
This is a useful sound/behavior reference, but it does not provide an all-circuit
C++ model of eleven voices. Audit audio-resource provenance separately before
bundling any resources. No audio, branding, fonts or artwork were imported.

### C++ MIT TR-909 noise component

https://github.com/ryandorward/tr-909-noise-source
https://github.com/ryandorward/tr-909-noise-source/blob/master/LICENSE
https://github.com/ryandorward/tr-909-noise-source/blob/master/src/dsp/NoiseSource.cpp

Verified MIT LICENSE (copyright 2026 ryandorward) and inspected the C++ core.
It models the 31-stage LFSR and hardware-rate sampling; it supplies only noise,
not complete snare/clap voices. The plugin wrapper is unnecessary for Orbit.
Technical evaluation is still required: sample-rate/invalid-input guards,
startup policy for note-on at time zero, downsampling rejection, noise spectrum
and continuous shared-noise behavior. README claims of perfect antialiasing are
not established by source inspection; a short FIR must be measured.

Electric Druid's first-hand circuit analysis provides the comparison reference:
https://electricdruid.net/tr-909-noise-generator/
It identifies 31 stages and feedback taps 31/13, correcting the service manual's
32-stage description, and discusses hardware clock/startup behavior.

### Hardware architecture

Roland describes the hybrid design and the sampled hi-hats/cymbals here:
https://www.roland.com/uk/promos/roland_tr-909/?lang=en-GB

The precise TR-909 service circuit pages still need a dedicated voice-by-voice
read before claiming circuit-based modeling. Pro-9's extended synthesis controls
also require original decisions where neither manual supplies DSP equations.
Original/user-recorded or explicitly licensed hat/cymbal resources are an option
for a hardware-style hybrid; a code license alone does not settle asset origin.

### Other candidates, not selected

- CarlosFranzetti/LoS.9x9: README says MIT but the checked root has no LICENSE
  file (direct LICENSE request returned 404). `Source/Samples.cpp` contains
  prerendered synthetic voice generation and optional external reference WAV
  loading, rather than a ready-to-use eleven-voice realtime circuit model.
  Reuse is not approved by this research.
- ReinerBforartists/Drumgroover: current repository has MIT licensing, but its
  README calls the current engine TR-808 and the TR-909 engine in development;
  checked DSP directory contains the 808 models. Not selected as a complete 909.
- DaisySP: project documents MIT; its checked AnalogBassDrum header explicitly
  describes an 808 model. Useful generic drum building blocks, not proof of a
  TR-909 implementation.

## Next concrete work

Build the per-voice Pro-9 specification and follow the broader audition plan in
DRUM_ENGINE_COMPARISON.md. Compare existing GPL/permissive implementations before
writing replacements. Confirm
real-time parameter response, accents, shared noise, choke/retrigger behavior and
sample-rate consistency before GUI integration. Document every adopted file's
license, full revision, hash and notices. Extend native instrument/mixer routing
for all eleven voices and six/eleven-output variants, with automation, MIDI timing,
Undo, presets and save/reopen tests.
