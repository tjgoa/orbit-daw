# Edit menu

## Mac clipboard and stable Piano Roll view — Build 252

Command-C/X/V/A/Z and Command-Shift-Z use the letter key code. JUCE's macOS peer
sets the text character to zero for Command events; Ctrl events can likewise
carry control characters. Text editors keep their own clipboard/Undo handling;
Option-modified commands and macOS Quit/Hide/Control-Command system chords are
not mapped to pattern edits. Playlist Select All uses the same normalization.

Paste/cut and other pattern edits preserve the first visible pitch row and the
musical horizontal scroll position after layout, including a late new-pattern
centering request. Undo/Redo within the same generator/pattern keep the current
view and return keyboard focus to the roll when it owned focus before Undo.
Switching to a different pattern can still perform its ordinary initial centering.
Clipboard note pitches and exact tick offsets remain unchanged; this fixes view
movement rather than transposing pasted notes.

Tools/test-options-host.py --sanitize --patterns covers native-shaped zero-text
Command events through Grid, Piano Roll and root dispatch, control-character
Ctrl input, repeated paste/cut/Undo/Redo, fractional pitch scroll, whole-pattern
length/switch metadata, coincident note velocities and native reopen. The Playlist
profile checks zero-text Command-A. Both include earlier editing regressions.

## Full Piano Roll workspace — Build 243

Opening the maximized Piano Roll fills the complete workspace below transport
and above the status bar, including the side/bottom edges. It follows application
resizes and overrides older saved inset bounds. The generator dock remains behind
the full editor; closing returns to the saved floating instrument bounds. A
deliberate generator drag/resize exits maximized mode. The Patterns host check
verifies geometry at two sizes, both blue skins, edge pixels against a bright
wallpaper, bottom control hit-testing and instrument-bound restoration.


## Patterns section — Build 242

The general Patterns chapter is implemented in this checkpoint. The user
approved leaving the Sampler groove-slicer RNG/slice-name ruler for the Sampler
section. See PATTERNS_IMPLEMENTATION.md for the scope and verification record.

- **Draw / Select** at the top left switches note drawing and rectangle selection; T toggles it. Shift-selection also works while drawing.
  Ctrl/Cmd adds a rectangle or note to the selection. Drag a selected note body
  to move the selected group; Ctrl/Cmd-drag duplicates it. One gesture has one
  Undo, with atomic pitch/time boundary checks. Right-edge note resizing stays
  separate. Exact-tick timing, velocities and coincident records are preserved.
- **Alt-drag** brushes separate notes using grid resolution and remembered note
  length, with one Undo for the stroke. Existing notes are not erased by brushing.
- **VEL** (beside the velocity-lane button, shortcut V) enables
  note-grid velocity editing: drag a note vertically to change that chord member,
  with velocity shading. The lower velocity lane has an individual bar/handle
  for each note; chords are separated horizontally instead of averaged. Its
  active handle and matching note are highlighted, with a pitch/value readout. Coincident notes at exactly the same pitch/onset form one editable
  stack, as in the existing selection model.
- Focused shortcuts include Up/Down semitone transpose, PgUp/PgDn octave
  transpose, Left/Right step rotation, 1–8 slot selection, 9/0 grid subdivision,
  C settings, E events, G snap, M ghosts, N rename, S Scissor, T tool, X/Y flips,
  V velocity, +/- time zoom, Tab/Ctrl-W close. Numeric-keypad 8/2 scroll pitch;
  4/6 move the pattern cursor. Text inputs retain native typing/clipboard keys.
- **Piano Roll Settings** adds per-generator grid/emphasis, snap, theme, colored
  keys, note labels, ghost notes, follow cursor, cursor display, edit audition,
  gradient and root/scale preferences. These are optional PIANOROLL metadata on
  the engine track and survive native project save/reopen. Application settings
  provide defaults for projects without this metadata. Ghost notes use other
  generators' remembered patterns and track colors, with no effect on playback.
- Note labels appear on note bodies. Follow scrolls the time viewport when the
  playing cursor leaves it. Edit audition pairs note on/off without feeding the
  performance recorder or adding duplicate recorded notes.
- Right-click the Pattern automation lane for an **LFO tool** with sine,
  triangle, rising/falling saw and square waves, cycles, phase, depth and centre.
  Preview changes only the displayed curve; Cancel preserves the document.
  Apply replaces the selected lane in one Undo. A bounded 4096-point curve spans
  the complete pattern, including beyond the former 4095-step storage limit;
  long curves use interpolation. Replacement is staged as one native lane update.
- Undo restores notes/session state while retaining the open Piano Roll and
  rebinding its instrument UI. The editor stays attached to the generator body.

Run the full section check with
`python3 Tools/test-options-host.py --sanitize --patterns`, plus the focused
Piano Roll, Playlist, generator and recording profiles as applicable. The check
uses the current native model source, not a stale normal-build model object.
Pure transformation coverage is in Tools/test-pattern-editor-tools.cpp.
These checks disable physical audio/MIDI and native desktop presentation;
third-party plugin compatibility and real device timing still need hands-on
validation. The existing headless JUCE assertions are not claimed resolved.

## Piano Roll scale snapping — Build 240

Open the Piano Roll gear > **Snap to scale**, choose a root key and scale, then
Apply. The saved application preference is off by default. All 12 roots and 17
scales are available, including major, natural/harmonic/melodic minor, double
harmonic minor (Hungarian minor), double harmonic major, modes, pentatonics,
blues, whole tone and diminished scales. Melodic minor uses its ascending form.

Newly drawn notes, vertical note moves and the clickable Piano Roll keyboard
use the nearest scale pitch; exact ties choose the lower note, with MIDI 0–127
endpoints respected. Off-scale grid rows are shaded but remain visible.
Dragging the keyboard across two keys mapping to the same scale pitch sustains
that note instead of retriggering it. Audition and recorded notes share the
mapped pitch, and changing the setting releases any held Piano Roll key.

As requested, this is Piano Roll only: hardware MIDI, the separate virtual/host
instrument keyboards, MIDI import and existing notes keep their pitch. Existing
accidentals remain selectable, resizable and erasable. Enabling the setting does
not rewrite a phrase; normal editing Undo applies to newly drawn/moved notes.
Build 240 used shared application defaults. Build 242 stores these choices per
generator in native project metadata, with the application choices as defaults. The source algorithms are original; no third-party DAW code is used.

The focused Piano Roll sanitizer check covers every scale/root/pitch and actual
draw/move/Undo, accidental erase, clickable-keyboard recording/glissando, external
MIDI independence, disabled snapping, both themes and fresh-process preferences.

## Existing Edit commands

Implemented from the user-supplied `Movies/EDIT MENU.mov` reference, with original
musical algorithms and Orbit's native note/engine model. No Orion implementation
code, parameter tables, groove files or pluck-template data are included.

All 15 commands have handlers and context-sensitive availability:

| Command | Behavior |
| --- | --- |
| Undo / Redo | Existing project/session snapshot history, including instrument controls and note selection. Focused text fields use their own text history. |
| Cut / Copy | Explicitly selected notes; without a note selection, the current pattern. Coincident records, exact tick timing and all MIDI pitches are retained. |
| Paste | Adds clipboard notes at the piano-roll edit cursor. Partial selections use relative timing; whole-pattern transfers retain the complete cycle length. Existing destination notes remain. |
| Select All | Selects every note in the current pattern, including notes outside the displayed pitch range. |
| Randomize | Current pattern and/or current instrument controls. Trance Flavor 1, Trance Flavor 2, Pure Random, and Chord/Scale generation; root, pitch range, density, and ask-every-time options. |
| Randomize Options | Saves generation targets/settings without changing notes. Native Drum Racks can randomize the selected pad only or the complete rack, using their actual pad-note mapping. |
| Humanize | Bounded native-tick timing and velocity variation, plus normalized instrument-control variation. Each amount is independent. |
| Quantize | Independent start-time and duration snapping, including triplet resolutions. MIDI input quantization policy is saved in engine state and restored by undo/load. |
| Transpose | Current pattern/selection, all patterns on the current generator, or all generators except native drums. Out-of-range MIDI pitches refuse the complete operation. |
| Scale Velocities | Chronological begin/end velocity ramp; notes at the same onset receive the same value. MIDI velocities remain in the playable 1–127 range. |
| Length | Independent start-time/duration scaling with exact tick passthrough for unchecked fields. |
| Apply Groove Template | None, two velocity-only accent patterns, Jazz Swing, Nice Bassline, Salsa and Shuffle. Resolution and position/length/velocity strengths are independent. |
| Apply Pluck Template | Linear/curved upward or downward strums, alternating strings and random strings. Only simultaneous chord groups are spread; isolated notes remain unchanged. |

Groove and pluck models are original musical interpretations of those controls;
they do not claim identical output to Orion's proprietary template data.
Generative randomization refuses patterns over 65,536 steps to bound allocation.
Control randomization/humanization protects gain, volume, level, pitch, tuning and
bypass parameters. Native pad control variation changes pan, cutoff, release and
velocity sensitivity while retaining sample identity, excerpt, gain and tuning.
Unavailable control targets are disabled until an instrument/sample is present.

The piano roll supports Shift/Ctrl/Cmd-click selection, Shift-drag lasso,
Ctrl/Cmd-drag additive lasso, Escape to clear selection, and Delete/Backspace for
selected notes. Click a position in the note grid to set the paste cursor. Normal
pencil/drag gestures clear the explicit note selection. Musical tools affect the
selection when present, otherwise the current pattern. Global transpose uses its
chosen scope. A whole-pattern copy preserves silent tail length as well as notes.

The Piano Roll supports the full MIDI pitch range, 0–127 (C-1 through G9 using
Orbit's existing C4 = MIDI 60 convention). Mouse-wheel vertical scrolling and the
right-side scrollbar move the keyboard and note grid together; Shift-wheel and
the horizontal scrollbar move along time. Vertical zoom changes row height, and
velocity/automation lanes remain docked at the bottom. Drawing, key audition,
moving, resizing, lasso selection and erasing all use the same scrolled pitch
coordinates. Changing generator or pattern focuses its note range, with C4 used
for an empty pattern. Native note data and project schema are unchanged. Playlist
miniature previews expand their pitch frame when notes exceed the former range.
Each completed drawing/move/resize gesture has its own undo entry; right-click
erasing also records an independent undo entry. Merely clicking an existing note
without changing it does not add gesture history.

Standard editing shortcuts accept Ctrl and Cmd: Z (undo), Shift-Z or Y (redo),
X/C/V (clipboard), A (select all), and R (randomize). Humanize uses Ctrl-H and
Quantize uses Ctrl-Q; macOS Cmd-H/Cmd-Q retain their system meanings. Focused text
fields keep native text shortcuts and the Edit clipboard commands route to them.

The auto-quantize option persists in native engine state and is used by the MIDI
take recorder for independent start/duration quantization. Record now captures
Pattern/Song MIDI takes; see `../Recording/README.md` for scope and limitations.
Audio recording and hardware latency validation remain unfinished.

## Verification

The focused Piano Roll check exercises production host/grid callbacks with
isolated settings and hardware/display initialization disabled:

```sh
python3 Tools/test-options-host.py --sanitize --piano-roll
```

It checks MIDI endpoints and intermediate pitches, mouse/scrollbar/zoom mapping,
key audition, scrolled move/selection/erase gestures, full-range velocity editing
and clipboard, fixed lanes, undo/redo, exact native save/reopen and pattern focus.
It also renders both blue skins at the lowest, middle and highest registers.
This headless check does not establish physical mouse/trackpad or audio-device
behavior on the desktop.

The product is compiled using the project's numbered build process. All new pure
musical transformations are exercised by a standalone sanitizer check:

```sh
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined Tools/test-pattern-tools.cpp \
  -o /tmp/orbit-pattern-tools-check
/tmp/orbit-pattern-tools-check
```

Checks include independent timing fields, rational/triplet snapping, transpose
rollback, exact 100-percent scaling, chronological/chord-consistent velocity,
humanize bounds, all groove/pluck algorithms and zero-strength identity, seeded
random generation, the chord/scale catalog, valid drum pitches, independent
coincident records, and maximum representable timing.

Additional temporary host checks invoke the production Edit handlers and real
panel controls with display/modal setup bypassed, hardware disabled and settings
in `/tmp`. They cover all menu IDs, clipboard/selection, single-action undo/redo,
input-policy persistence, no-op transforms, generation confirmation, instrument
control restoration, protected parameters, drum transpose exclusion, selected-pad
and whole-rack generation, native pad-control variation and UI refresh, atomic
transpose refusal, and native save/reopen. All eight panels are rendered and their
controls checked for clipping. Existing app text-encoding debug assertions remain;
this is not an assertion-clean whole-app run.

A separate native validation app launched successfully. The automation surface
could not activate its menus through accessibility or a coordinate fallback;
interactive desktop-menu confirmation remains a manual check. The isolated
validation bundle is not included in source archives. The SFS importer goal
remains paused and its format mappings were not changed during this work.

Build 245 adds View > Hotkeys, a searchable reference to the implemented commands
and their focus context. A separate generator-header Restore/Maximize button
keeps the Piano Roll open and remembers its floating size. Entry uses the clicked
subdivision, including fractional mouse/pitch-scroll coordinates; move/resize
still use nearest-grid snapping. Velocity bars and input share one 1–127 plot,
and the lane edits directly without activating individual-note VEL mode.
