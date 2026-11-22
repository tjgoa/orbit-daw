# Orbit Build 749 / v0.3.728 — executable-only tester update

Updates the previous public Build 612. Download Windows x64, universal macOS
(Apple Silicon + Intel), or Linux x64 (Ubuntu 24.04) packages under Assets.
The sampler and Drum Rack features remain available for your own content, but
**no sampler libraries, SoundFonts, Drum Rack factory kits or samples are bundled**.
Those content packs are still being finalized.

**Fixed 30-day preview:** issued 22 November 2026 at 02:15:18 UTC; expires
**22 December 2026 at 02:15:18 UTC** (21 December, 9:15:18 PM US Eastern).
Reinstalling does not extend that date. No purchase or online activation required.

## Update log since Build 612

### Editing, recording and arrangement
- MIDI patterns can be dragged from plug-in MIDI export controls (such as
  Microtonic's MIDI Drag icon) or .mid/.midi files into the piano roll or an
  instrument row in the Playlist. Drops use a free pattern slot, preserve song
  tempo and existing work, snap to the timeline, and support Undo.
- Preserve MIDI velocities, simultaneous notes, fractional timing and trailing
  rests. Improved piano-roll keyboard, ruler seek/loop editing, zoom and erase.
- Recording, selected-range looping and overdub fixes; step recording and active
  take erase; note-group resizing and shorter-pattern playback corrections.
- Playlist selection deletion, clip/range editing and audio slicing refinements;
  groove replacement and musical slice timing improvements.

### Instruments, hosting and workspace
- Consolidated instrument menu, dedicated original Drum Synth controls, playable
  PianoForte/modelled-piano panels, Resonance and experimental Vesp instruments.
- Expanded sampler program/keymap/SFZ-region editing and audio/instrument import,
  multi-selection, favorites, source navigation and background load diagnostics.
  Import coverage varies by format; complex legacy processing may not translate.
- Compact MIDI Out routing/controller editor; CLAP hosting and plugin/workspace
  improvements; resizable columns and remembered browser locations.
- Plugin editor sizing/fullscreen responsiveness work; reduced redundant layout
  callbacks and deferred geometry preference writes. Intermittent hangs remain
  under investigation rather than being declared universally resolved.

### Mixer and playback
- Mixer meter/skin refinements, clearer FX-send scaling/readouts and EQ controls.
- Expanded mixer minimum height and Modules-dialog dismissal improvements.
- Sampler/Drum Rack playback continuity, release-tail and rapid tempo-change work;
  preserve unchanged sample/runtime data during appropriate edits.

The separate TTU Drum Rack plugin's experimental A/B crossfade tools are not a
bundled plugin download in these Orbit executable packages. No external TTU
plugin binaries or its unfinished kit/sample collection are added here.

## Validation and known limits

Windows x64, Linux x64, and universal macOS native Release builds succeeded
in the same Build 749 invocation. Downloaded artifact digests and inner package
checksums matched; archive paths/content, native executable formats, build/version,
and the shared 30-day expiry passed verification. The Mac contains arm64 and
x86_64 code and its ad-hoc signature passed strict verification. The macOS
development baseline and targeted MIDI-drop/fullscreen/mixer regressions also
passed locally. Native CI is not desktop/audio-driver testing.
The Mac tester is ad-hoc signed and not notarized; Windows is unsigned; Linux
uses Ubuntu 24.04 system libraries. See INSTALL.md / READ_ME_FIRST.md.

These proprietary binary tester packages do not publish the current source.
Historical public source/releases retain their existing licenses. Existing
retired/unlicensed audition exclusions remain in force; private LoS909 is off.
