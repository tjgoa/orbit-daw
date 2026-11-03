# Orbit development update: Builds 277–324

**November 3, 2026 — current source: Build 324 / v0.3.303**

This changelog covers development since the last published tester,
[Build 276 / v0.3.255](https://github.com/tjgoa/orbit-daw/releases/tag/tester-build-276).
GitHub's main source now contains the work through Build 324. Build 324 has been
compiled and tested locally as a universal macOS app. The currently published
Mac, Windows and Linux tester packages remain Build 276; this source update does
not relabel those older downloads or claim new cross-platform runtime testing.

Build numbers include reserved attempts that were withheld or superseded. The
features below describe the resulting implementation, not 48 separately shipped
releases. [BUILD_HISTORY.md](../BUILD_HISTORY.md) retains the individual records.

## Audio recording and editing

- Native mono/stereo audio input recording, including simultaneous armed tracks,
  input selection, monitoring, count-in and durable WAV takes.
- Audio-only projects can run the Song transport and record without inserting a
  synth. The AUDIO indicator and per-track arm state persist through Record/Play.
- Growing waveforms appear during recording, followed by normal editable Wave
  clips when the take is committed.
- Non-destructive left/right edge trimming, including restoring a trimmed release
  tail; source-file and occupied-destination bounds protect extensions.
- Select a time range within audio and drag, cut, copy or delete that excerpt.
  Unselected audio on either side remains in place. Undo/Redo restores the whole
  operation, and edited source offsets survive project reopen.
- Audio clip split, placement/trim/offset/gain properties, reverse, clone and
  independent placement actions.
- Normalize measures the played source excerpt and applies non-destructive clip
  gain to -0.1 dBFS. Quiet sources can receive more than +24 dB; source files remain
  intact. Audio menus now use clean punctuation.

See [audio recording](../Source/Recording/AUDIO_RECORDING.md) and
[audio/Playlist editing](../Source/Playlist/AUDIO_EDITING.md).

## Song Freeze and Unfreeze

- Freeze a generator's Song arrangement to a durable stereo WAV, retaining leading
  silence and an adjustable release/effect tail.
- Choose generator-only rendering or bake the track's mixer inserts/EQ, including
  an isolated Multi FX rack. Fader, pan, sends, buses and Master remain live.
- Reversible Freeze retains source patterns, automation, processor state and bypass
  flags. Unfreeze restores them; Freeze/Unfreeze support Undo/Redo.
- Optional live Pattern audition keeps the original source playable in Pattern
  mode while Song mode streams the frozen audio.
- Frozen lanes display editable waveforms, with an optional faint original-note
  overlay. Audio edits, splits and intentionally empty frozen lanes survive mode
  changes, refresh and save/reopen. Unfreeze restores the original arrangement;
  Undo Unfreeze restores the edited frozen audio.
- An explicitly destructive **Permanent Audio** choice bakes source processing,
  removes the original generator/printed effects and clears Undo. It is distinct
  from reversible Freeze.
- Bounded render batches provide cancellation and avoid the earlier cleanup hang.
  Incomplete renders do not commit; missing reversible caches recover the source.

This is Song Freeze. Per-pattern render banks, selection/region bounce and shared
or nested rack/sidechain baking are not implemented in this version. Details:
[Freeze options and signal paths](../Source/Freeze/README.md).

## Playlist, arrangement and performance capture

- CLIPS performance recording captures pattern selections into the Song, with
  queued bar launches, protected destination regions and one-take Undo.
- Arrangement block moves/copies carry enabled generator and track mixer Song
  automation. Instance-aware target mapping and recorded-value holds are retained.
- Fine/free placement, saved snap settings and Shift/Option overrides use actual
  clip extents and preserve audio excerpts and linked automation timing.
- Working arrangement Cut/Copy/Paste and Select All, with a context-sensitive Edit
  menu separate from Piano Roll note editing. Same-project paste validates the
  complete destination and commits as one Undo transaction.
- **F9: Add Patterns to Playlist** captures each unmuted generator's current
  selected nonempty/event-containing pattern. **Shift+F9** restricts the capture
  to soloed, unmuted generators and includes their blank selectors.
- Captures repeat complete patterns to a common cycle when it fits within 24 bars.
  Three-, four- and eight-bar patterns fill 24 bars. If the cycle exceeds the cap,
  use the longest pattern, capped at 24, and leave incomplete repetitions as gaps:
  seven and eight produce an eight-bar block with one empty bar after seven.
- Captures and Paste advance the ruler insertion point for consecutive presses.
- Ordinary Playlist placements follow their source pattern's shared length;
  deliberate exact trims retain their own boundaries. The pattern picker exposes
  all 64 slots in one panel.
- Adaptive overview numbering stays readable across hundreds of bars. Wide zoom
  uses quiet vertical guides; close zoom retains the boxed emphasis and previews.

## Acid Bass

- Advanced shared-oscillator filter bank with 2/4/8 filters, filter spread and
  stereo width, preserving the existing Accent/Slide voice behavior.
- Internal Tube and Two-Band distortion stages with their parameters stored in
  synth presets; Tube Shape is continuous rather than restricted to three values.
- Separate LFO section with two modulation routes and free-running or note-retrigger
  choices. Slow free-running modulation continues across notes, rests and loops.
- Independent three-band EQ section: low shelf, parametric mid, high shelf and
  output trim, with saved/automatable values.
- Dedicated preset library at `~/Music/Orbit/Presets/Acid Bass`, preset selector
  refresh, folder access and remembered save names.
- Per-instance Silver/White, Dark/Original and Dark/White appearances, selectable
  native knob styles, closer labels and refined shadow/contrast behavior.

See [filter spread](../Source/Synth/ACID_FILTER_SPREAD.md),
[internal effects](../Source/Synth/ACID_BASS_FX.md) and
[preset library/EQ](../Source/Synth/ACID_PRESETS_EQ.md).

## Multi FX Rack

- A serial insert rack hosts the ten existing built-in audio effects and already
  scanned VST3 effects in one insert slot.
- Per-slot bypass, reorder, remove, native controls and hosted plugin editors.
- Independent chain/settings copy, project merge with remapped rack/child IDs,
  child automation discovery and saved audio after project reopen.

This does not yet include the proposed wholesale extraction of Labyrinth's effect
collection. See [rack scope](../Source/Mixing/MULTI_FX_RACK.md).

## Mixer, knobs, skins and workspace

- Coordinated Compact Silver skins alongside preserved Navy Blue/Light Blue
  profiles; experimental flowing/metallic/compact headers remain selectable.
  Silver 2 has white note previews, and Playlist canvas overrides are independent.
- Live Mixer Layout controls for spacing, size and finish, including Warm White
  and silver-knob palettes, refined fader finishes and independently sized knob
  faces. Increasing knob size no longer scales the complete mixer strip.
- Optional KnobMan export tools and a searchable live native sprite browser for
  Mixer/Master, with background loading and persistent selection/fallback.
  External sprite libraries remain external, with their original authorship.
- A 33-style knob comparison uses actual JUCE renders for its HTML gallery;
  native code and previews share the implemented designs.
- Mixer/Master/Playlist maximize flush to the workspace; generator panels retain
  free edge placement. Instrument and Piano Roll share a coherent generator frame.
- Blank startup/New Project workspace, consistent name-only minimized tabs,
  one-click view restoration and owned foreground dialogs.
- Space toggles transport with text/modal protection. Generator insertion retains
  Pattern playback. Pattern double-clicks resolve the intended generator/slot.
- Double-clicks on Steps controls no longer maximize the generator; title-surface
  double-clicks still do. Project Save remembers the current/last filename.

## Validation and current limits

Local universal Mac builds and targeted native ASan/UBSan checks cover audio
capture, live thumbnails, sample comparisons for Freeze signal paths, audio
trim/slice gestures, source offsets, pattern capture rules, Undo/Redo and reopen.
These checks do not certify
all audio devices, VST3 plugins or Windows/Linux runtime behavior.

Audio input recording currently requires 4/4 Song mode with Loop off. Audio time
selection slices Wave clips; pattern clips remain whole selections. Cross-project
arrangement clipboard, advanced comping/punch, per-pattern Freeze banks and the
remaining manual-parity work are still open. Save As preserves media references;
carry the referenced audio folders when moving a project. Schema 25 protects the
newer saved state from older Orbit readers.

Orbit remains an early development DAW. Report the exact build/version, OS/CPU,
audio device/plugin versions, steps, expected/actual result and any relevant log
through [GitHub Issues](https://github.com/tjgoa/orbit-daw/issues).
