# Drum engine auditions

Build 272 adds **9W9 909 Audition**, a complete eleven-sound listening candidate
with native step/Accent controls and a demo beat. See [909 guide](NINEKIT_GUIDE.md).
Geonkick and the separate ChowKick comparison below remain available.

## Geonkick inside Orbit

Build 271 adds **Insert > Drum Auditions > Geonkick Audition**. It runs credited
GPLv3-or-later Geonkick oscillator/envelope/filter/distortion code, with an Orbit
interface and six Orbit-authored sound recipes. This is a first listening tool;
it is not the complete Geonkick instrument or the finished Pro-9 kit.

1. Insert Geonkick Audition. Click **Hear hit**. Choose Kick, Snare, Hat, Tom,
   Clap or Metal, then preview again. Preview runs the chosen voice on its track
   without starting the transport or changing the notes in your pattern.
2. Click **Demo pattern**, select **Pattern** and **Play**. Eight hits alternate
   strong/soft velocities. Every MIDI note triggers the chosen sound; C2 (MIDI
   36) is its original pitch. Notes transpose the pitched layers; the clap is a noise-based recipe. Note-off does not cut
   off a percussion one-shot; Stop/Panic silences it.
3. Adjust Tune, Decay, Pitch Sweep, Noise, Tone, Drive and Output. Values appear
   while dragging. Sweep primarily shapes Kick/Tom; Noise mainly shapes the
   noise-bearing recipes. Tone and Drive change sounding voices; other sound
   settings take effect on the next hit. Double-click returns a knob to default.
4. Insert another instance and choose another recipe. Place the panels beside
   each other. They have separate sounds, pattern banks and window placement.
   Use the existing mute/solo buttons when comparing levels. Selection, inserting
   another instrument and transport actions should preserve the open panels.
5. Use the Piano Roll to draw a rhythm, change velocities and tune notes.
   Right-click a knob to edit/record its events. Song and Pattern automation use
   the same native parameter IDs. Save and reopen a `.orbit` project to check
   sound, notes and automation persistence.

Demo pattern uses an empty slot and never overwrites an occupied one. If every
slot is occupied it leaves the project unchanged and reports that condition.

The adapter has eight overlapping voices, three oscillator layers per voice,
one stereo output (duplicated mono), and no latency. It uses the upstream filter
and distortion equations, with bounded cutoff for stability. It currently does
not expose upstream FM/envelope editing, sample layers/import, Geonkick presets,
the sixteen-instrument kit, choke groups or separate outputs. The oscillator
and saturation paths are not oversampled. These remain audition limitations.

## ChowKick comparison on this Mac

ChowKick 1.2.0 is a separate BSD-licensed VST3, not compiled into Orbit. The
local copy is installed in `~/Library/Audio/Plug-Ins/VST3/ChowKick.vst3`. Use
**Insert > VST Instruments > Scan / Manage VST3 Instruments**, scan if needed,
then insert **ChowKick**. The local setup uses only its VST3; no AU/VST2/CLAP or
standalone installer is run. A local validation copy is also kept in
`Builds/Audition Plugins/ChowKick.vst3` and is excluded from source archives.

Official release:
https://github.com/Chowdhury-DSP/ChowKick/releases/tag/v1.2.0
Downloaded `ChowKick-Mac-1.2.0.dmg`, 18,907,522 bytes, SHA256:
`de1833ac2bfd7f9d74ba61cb64f3fb7c600de348cba6b46d9c05c8df14d158f7`.
Its package and plugin signatures did not verify locally. The extracted audition
copy is ad-hoc signed for local testing; this is not an upstream notarization
claim. BSD notice and a local-copy note accompany it. No system installer ran.

Use the same C2 notes and velocities in both tracks, compare at matched output
level, then try short/long decay, pitch sweeps, retriggers and very soft notes.
ChowKick focuses on a resonant kick; Geonkick covers general percussion. This
does not imply either one is a complete 909 reconstruction. User listening and
hardware CPU tests are still needed before choosing voices for Orbit's own kit.

## Continuing the comparison

See DRUM_ENGINE_COMPARISON.md for the researched shortlist and popularity
snapshot. The 9W9 candidate is now playable in Build 272. Next compare Bespoke DrumSynth/Kicker and the other dedicated 909 candidates
voice by voice, then implement the full Pro-9 specification and routing. We have
not selected one engine as the final eleven-voice implementation.
