# Dependency record

This file records the upstream source revisions used for the current Orbit foundation. Update it when deliberately upgrading a dependency; don't silently follow a moving branch.

| Dependency | Upstream | Revision in this checkout | License path used |
|---|---|---|---|
| Tracktion Engine | <https://github.com/Tracktion/tracktion_engine> | `v3.2.0` (`0a5f4e6a5f53d09c89b414a44386a12df7fa1ec6`) | GPLv3; retain upstream notices |
| JUCE | <https://github.com/juce-framework/JUCE> | `19edd538429c93d277bf95b55aaa7e3eb545f951` (Tracktion Engine submodule) | AGPLv3; retain upstream notices |

The full dependency sources currently live under `Dependencies/tracktion_engine/` as a pinned Git submodule. Its checked-out commit is recorded by the Orbit repository, and the Tracktion Engine revision pins its JUCE submodule. Tracktion Engine's `.gitmodules` file uses an SSH URL for JUCE; the HTTPS rewrite in the README supports public checkouts without SSH keys.

Orbit's Drum Rack extends Tracktion Engine's built-in sampler with loop playback, configurable release, and per-pad velocity response. The two project-owned patch files are `Dependencies/tracktion_engine/modules/tracktion_engine/plugins/effects/tracktion_SamplerPlugin.h` and `.cpp`. Incremental source archives include these changed files only; apply them over the pinned Tracktion Engine `v3.2.0` checkout before building. Unchanged dependency files are not copied into source archives.

Build 262 also patches `modules/tracktion_engine/playback/graph/tracktion_NodeRenderContext.cpp`
to respect frame-view offsets when writing latency-compensated exports. Run
`python3 Tools/prepare-dependencies.py` to verify both pinned revisions and apply
the sampler and `Tools/tracktion-export-offset.patch` overlays. Incremental
archives include this one changed export source file and its patch, in addition
to the sampler overlay; unchanged dependencies remain excluded.

The Drum Rack UI has 16 fixed-note pad rows, an 8/16/32/64-step pattern sequencer, compact per-pad controls, and a disclosure button on each loaded pad. Opening a pad's sample editor places a full-width waveform and draggable trim handles below the pad rows; the waveform stays hidden while the editor is collapsed. The editor also includes a velocity-expression lane for the selected pad's active steps. Step toggles, velocities, and sample controls use Orbit's project save/undo paths.

## Drum Rack engine boundaries

The rack is a front end for one Tracktion `SamplerPlugin` on an instrument track, not a separate drum-rack DSP engine. Each of its 16 pads currently maps to one fixed MIDI note and one sampler sound. Per-pad sample playback, excerpt trimming, reverse, gate/one-shot/loop modes, tuning, gain, pan, cutoff, release, and velocity response use that sampler's sound-zone controls. Step velocity is stored on the associated pattern notes. Pads do not yet have independent MIDI channels, output buses, or per-pad insert-effect chains; they share the instrument track's routing and effects. Reverse creates a rendered WAV copy and keeps the original path in sampler state so Orbit can switch back. These are current implementation limits, not guarantees about Tracktion Engine's full capabilities.

Orbit application source is licensed under AGPLv3 in `LICENSE`. This record is not a replacement for reading each upstream license and all notices for included third-party components.

## Geonkick audition DSP — Build 271

`Source/Synth/ThirdParty/Geonkick` is pinned to quamplex/geonkick commit
`a046d614198227cbc737bb1d717b6e155394f15a` (3.9.0), GPL-3.0-or-later. The
original file hashes, full license texts, authorship and conditional
single-owner portability changes are recorded there. Only five core C files
are built; Orbit's `GeonkickVoice.c` provides the private real-time voice/mixing
adapter and deterministic random helper. Each context is exclusively owned by
the render thread. Do not use its single-owner target with upstream shared
workers. App distributions carry the GPL/Quamplex notices. The complete pinned
source subset is included in the first source archive adopting it.

ChowKick 1.2.0 is installed separately for local comparison. No ChowKick binary
is bundled in Orbit or its source archives. See Source/Synth/DRUM_AUDITION_GUIDE.md
for provenance, signature handling, audition steps and current limitations.

## 9W9 909 audition DSP — Build 272

9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9

This is a modified Orbit port, not the original and not made or endorsed by
A Thousand Details. Source/Synth/ThirdParty/9W9 is pinned to
`a9e030d9bad46e3e53aafce92c7104b8e7bba47f` (v2.7.2), GPL-3.0 with the additional
section 7 terms in its unaltered NOTICE. Original LICENSE/NOTICE, source hashes,
modifications and original sample hashes are retained. The source archive
includes the adopted C subset, the three upstream WAV resources, the generated
PCM/metadata headers and Tools/generate-ninekit-data.py. No upstream Move shell
or external SDK is required. Reproduction requires only the pinned normal Orbit
dependencies and C11/C++20; regenerate tables with that Python script if editing
resources/metadata. App/tester distributions carry the 9W9 legal notices.

See Source/Synth/NINEKIT_GUIDE.md for control semantics and remaining Pro-9 gaps.
