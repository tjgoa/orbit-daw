# Edit menu

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
