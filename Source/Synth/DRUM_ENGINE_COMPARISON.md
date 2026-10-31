# Drum engine comparison and Geonkick port feasibility

Research date: 2026-10-30. The popularity/source comparison below preceded the
Build 271 audition implementation. Geonkick's selected DSP is now wrapped in a
native Orbit test instrument, and ChowKick is prepared as a separate Mac VST3.
See DRUM_AUDITION_GUIDE.md for scope, provenance and user listening tests. No
final Pro-9 engine selection or human A/B listening judgment is claimed.

The user wants a broader comparison of established open-source projects before
choosing a 909 engine, and would like Geonkick available for testing in Orbit.
Orbit's own instrument may combine credited upstream DSP with Orbit controls,
sequencing, routing, automation and state. GPLv3/AGPLv3 are explicitly in scope.

## Popularity snapshot and relevance

Counts below come from the GitHub repository API through the connected GitHub
tool, checked on this date. They are whole-project counts, not votes for an
individual oscillator, drum voice or algorithm. The raw snapshot is stored in
DRUM_CANDIDATES_2026-10-30.json. This shortlist does not establish a worldwide
popularity ranking, particularly for projects distributed through other hosts.

| Project / source | GitHub stars | License checked | Relevant role / limitation |
| --- | ---: | --- | --- |
| LMMS / Kicker | 10,561 | Kicker source GPL-2.0-or-later | Established sweeping kick/percussion oscillator with noise, pitch and distortion envelopes. Whole count belongs to the DAW. Not a complete 909 kit. |
| BespokeSynth / DrumSynth | 4,749 | DrumSynth source GPL-3.0-or-later | Eight-pad synthesis, oscillator/envelope/filter controls and individual outputs. Generic percussion rather than a 909 reconstruction; tied to Bespoke globals/module/UI infrastructure. |
| Hydrogen | 1,346 | README GPL-2.0-or-later | Cross-platform layered drum kits and mature note/sample/kit workflow. Predominantly a sampler/sequencer; not the first analog drum-voice source. |
| ChowKick | 205 | BSD-3-Clause LICENSE | Dedicated nonlinear resonator/pulse-shaper kick, inspired by TR-808 analysis. Mac VST3 already available; useful immediate listening reference, not a whole 909. |
| Geonkick | 194 | GPL-3.0-or-later DSP headers | Broad layered percussion engine: up to sixteen instruments, three layers and three oscillators/layer, pitch/amplitude/filter/distortion envelopes, FM, samples, choke and separate outputs. Mac full GUI unsupported upstream. |
| ER-99 | 80 | GPLv3 LICENSE | Focused 909-style synthesized analog voices plus sampled cymbals/hats. TypeScript/WebAudio requires native port. |
| drumkv1 | 44 | README GPL-2.0-or-later | Established drum sampler with stereo FX. Qt/JACK/LV2 integration; useful sample/kit behavior, lower priority for analog 909 synthesis. |
| 9W9 | 0 | GPLv3 + NOTICE | Focused plain-C 909-style engine; source/notice reviewed separately in PRO9_RESEARCH.md. Very new repository with little public adoption evidence. Keep as a candidate, not the automatic choice. |

Projects and source locations actually inspected:

- https://github.com/LMMS/lmms/tree/master/plugins/Kicker
- https://github.com/LMMS/lmms/blob/master/plugins/Kicker/KickerOsc.h
- https://github.com/LMMS/lmms/blob/master/plugins/Kicker/Kicker.cpp
- https://github.com/BespokeSynth/BespokeSynth/blob/main/Source/DrumSynth.cpp
- https://github.com/BespokeSynth/BespokeSynth/blob/main/Source/DrumSynth.h
- https://github.com/hydrogen-music/hydrogen/blob/main/README.md
- https://github.com/Chowdhury-DSP/ChowKick/blob/main/README.md
- https://github.com/Chowdhury-DSP/ChowKick/blob/main/CMakeLists.txt
- https://github.com/Chowdhury-DSP/ChowKick/blob/main/LICENSE
- https://github.com/quamplex/geonkick/blob/main/README.md
- https://github.com/rncbc/drumkv1/blob/main/README
- https://github.com/matthewcieplak/er-99
- https://github.com/athousanddetails/schwung-9W9

## Geonkick: what a Mac audition port involves

The complete upstream app/plugin is not a direct Mac build. Its Redkite GUI
CMake explicitly aborts on Darwin with `Redkite is not implemented yet for Mac
OS X`. Current Geonkick documentation lists GNU/Linux and Windows. The existing
Windows/Linux VST3 can be hosted on its supported OS; it cannot run as a Mac VST3.

Primary evidence:
https://github.com/quamplex/geonkick/blob/main/src/redkite/CMakeLists.txt
https://github.com/quamplex/geonkick/blob/main/doc/Geonkick_User_Guide.md

The DSP is a much more practical starting point than a full GUI port:

- It is a separate C static-library target, with a public C interface. JACK is
  conditional, so the Orbit wrapper can supply audio/MIDI without a JACK client.
- Sources separate envelopes, oscillators, filters, distortion, synth, audio
  outputs, mixer, worker and ring buffer.
- The current engine synthesizes each instrument into a buffer on its worker
  after sound changes; MIDI playback consumes those buffers. A control change
  should not be assumed to affect an already sounding hit sample by sample.
- The worker uses pthreads and shared/global worker state. Multi-instance
  creation/destruction and concurrent changes need testing rather than treating
  it as an isolated stateless sample function.
- Upstream build options/architecture flags distinguish GNU/Windows, C++23 and
  Redkite. A native Orbit audition wrapper should build the audited C core with
  its own clean platform configuration, preserving behavior and notices.

Inspected source:
https://github.com/quamplex/geonkick/blob/main/src/dsp/CMakeLists.txt
https://github.com/quamplex/geonkick/blob/main/src/dsp/src/geonkick.h
https://github.com/quamplex/geonkick/blob/main/src/dsp/src/geonkick_internal.h
https://github.com/quamplex/geonkick/blob/main/src/dsp/src/worker.c
https://github.com/quamplex/geonkick/blob/main/src/dsp/src/synthesizer.c
https://github.com/quamplex/geonkick/blob/main/src/dsp/src/gkick_audio.c

Assessment: an engine-only Mac audition wrapper is manageable but still real
integration work. A complete original Geonkick GUI port is substantially larger.
No compile or latency/CPU estimate has been established by this inspection.

## Useful listening / prototype order

1. Use existing Mac ChowKick VST3 for a quick kick reference; official product
   page supplies a universal Mac build. Orbit already hosts VST3 instruments.
   https://chowdsp.com/products.html#kick
2. Prototype a Geonkick engine audition instrument using Orbit/JUCE controls:
   choose an instrument/preset, expose a small useful control set and MIDI
   trigger, and retain kit/preset data. Clearly label this as an audition build,
   not the complete Geonkick interface or Pro-9 implementation.
3. Compare general percussion cores from Geonkick and Bespoke, kick behavior from
   ChowKick and Kicker, and 909-specific behavior from ER-99 / 9W9. Review source
   dependencies before choosing any adopted module.
4. Use identical notes, velocities, accents, rates and output levels; retain raw
   renders for comparison. Check tails, tone/tuning ranges, retrigger/choke,
   low-velocity behavior, sample accuracy, automation response, CPU, instance
   isolation, native project persistence and cross-platform builds.
5. Select DSP voice by voice based on those results. Implement the complete
   Orion Pro-9 behavior specification and 6/11-output routing in Orbit's own
   instrument, with proper upstream attribution for adopted code/resources.

The new audition is additive. Existing Orbit instruments remain available.
