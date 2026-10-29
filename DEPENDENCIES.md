# Dependency record

This file records the upstream source revisions used for the current Orbit foundation. Update it when deliberately upgrading a dependency; don't silently follow a moving branch.

| Dependency | Upstream | Revision in this checkout | License path used |
|---|---|---|---|
| Tracktion Engine | <https://github.com/Tracktion/tracktion_engine> | `v3.2.0` (`0a5f4e6a5f53d09c89b414a44386a12df7fa1ec6`) | GPLv3; retain upstream notices |
| JUCE | <https://github.com/juce-framework/JUCE> | `19edd538429c93d277bf95b55aaa7e3eb545f951` (Tracktion Engine submodule) | AGPLv3; retain upstream notices |

The full dependency sources currently live under `Dependencies/tracktion_engine/` as a pinned Git submodule. Its checked-out commit is recorded by the Orbit repository, and the Tracktion Engine revision pins its JUCE submodule. Tracktion Engine's `.gitmodules` file uses an SSH URL for JUCE; the HTTPS rewrite in the README supports public checkouts without SSH keys.

Orbit's Drum Rack extends Tracktion Engine's built-in sampler with loop playback, configurable release, and per-pad velocity response. The two project-owned patch files are `Dependencies/tracktion_engine/modules/tracktion_engine/plugins/effects/tracktion_SamplerPlugin.h` and `.cpp`. Incremental source archives include these changed files only; apply them over the pinned Tracktion Engine `v3.2.0` checkout before building. Unchanged dependency files are not copied into source archives.

The Drum Rack UI has 16 fixed-note pad rows, an 8/16/32/64-step pattern sequencer, compact per-pad controls, and a disclosure button on each loaded pad. Opening a pad's sample editor places a full-width waveform and draggable trim handles below the pad rows; the waveform stays hidden while the editor is collapsed. The editor also includes a velocity-expression lane for the selected pad's active steps. Step toggles, velocities, and sample controls use Orbit's project save/undo paths.

## Drum Rack engine boundaries

The rack is a front end for one Tracktion `SamplerPlugin` on an instrument track, not a separate drum-rack DSP engine. Each of its 16 pads currently maps to one fixed MIDI note and one sampler sound. Per-pad sample playback, excerpt trimming, reverse, gate/one-shot/loop modes, tuning, gain, pan, cutoff, release, and velocity response use that sampler's sound-zone controls. Step velocity is stored on the associated pattern notes. Pads do not yet have independent MIDI channels, output buses, or per-pad insert-effect chains; they share the instrument track's routing and effects. Reverse creates a rendered WAV copy and keeps the original path in sampler state so Orbit can switch back. These are current implementation limits, not guarantees about Tracktion Engine's full capabilities.

Orbit application source is licensed under AGPLv3 in `LICENSE`. This record is not a replacement for reading each upstream license and all notices for included third-party components.
