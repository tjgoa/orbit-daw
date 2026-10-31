# Orbit Drum Rack scope and engine notes

## Current controls

- Sixteen sample pads are mapped to MIDI notes 48–63 in Tracktion Engine's built-in Sampler.
- Each pad has a sample slot, file picker, expand control, reverse, Gate/Shot/Loop mode, and compact Gain, Pan, Tune, and Length controls.
- The expanded sample editor shows the waveform across the panel, trim handles, Start/Length/Tune/Gain/Track/Cutoff/Pan/Decay controls, velocity response controls, and a velocity lane for the selected pad.
- The current sequencer still exposes variable step-count controls. The next Orion-style Drum Rack pass must remove that control: each bar is exactly 16 steps, and each pattern has four selectable bars (64 steps total). Show one 16-step bar at a time with a Bar 1–4 selector, matching Orion's compact sequencer; do not stretch this into a 64-column row. Keep the bar selector state and all four bars of hits/velocities in save, undo, and reopen paths.
- This fixed four-bar layout is specific to the Drum Rack. Keep the piano roll and imported MIDI patterns capable of representing longer patterns; do not apply the Rack's 64-step duration to the project-wide pattern model.
- Pattern notes, Rack hits, and hit velocities are stored sparsely. Rack bar selection should address the correct 16-step segment while preserving sparse storage and per-pad velocity values. The Playlist should show the full four-bar Rack pattern as a four-bar clip.
- Pattern MIDI import/export and MIDI song import/export are now available through the File menu. The future Orion SFS bridge should map Rack hits and velocity data when present, while warning about unsupported instrument and effect state.

## Engine boundary and portability limits

- The Rack is a front end for Tracktion Engine's Sampler, not a separate Orion-compatible sampler engine. Pad sound, pitch, excerpt, looping, gate/open-ended playback, gain, pan, release, filter cutoff, track level, and velocity response are sent to Sampler's per-sound parameters.
- Reverse renders a reversed WAV copy under the user's Orbit application-data folder and switches the Sampler to that file. Restore uses the original source path. These files are external to the project, so moving a project without its original samples and generated reversed WAVs can leave media references unresolved.
- Start/Length edits change the Sampler's excerpt and are non-destructive. The current minimum excerpt is 5 ms. There is no destructive waveform editor or per-pad MIDI channel/output routing in this Rack pass.
- The visible waveform is a thumbnail preview. It does not embed audio in the project or replace the Sampler's file reference.

## Next Orion-style Drum Rack requirement

- Replace the adjustable step-count dropdown with a fixed four-bar pattern: a Bar 1–4 selector and exactly 16 step buttons for the selected bar.
- Keep the Rack's pattern duration at 64 sixteenth-note steps. Preserve each pad's hits and velocity across all four bars when switching the selector, saving/reopening, undoing/redoing, and placing the pattern in the Playlist.
- Keep the four-bar Rack rule local to Drum Rack. General MIDI/piano-roll patterns remain uncapped by this UI choice.
- The Drum Rack goal remains paused until resumed.

## Remaining visual comparison

The Rack should continue to be compared with the supplied Orion screenshots for knob shape/response, the compact sample slot and mode placement, step-cell proportions, and the selected-pad editor layout. Runtime verification of trimming, reversal, project reopen, and undo remains necessary before calling the replication complete.
