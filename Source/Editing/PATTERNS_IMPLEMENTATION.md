# Current manual section: Patterns

Finish and test this section before advancing to Playlist/Recording, Automation
or the unread Mixer chapter. The user confirmed this workflow on 2026-10-29.
User-reported interaction bugs take priority within the work.

- [x] Full MIDI range and short black keys with continuous white tails (238).
- [x] Optional Piano Roll-only scale snapping, including mouse recording (240).
- [x] Generator-column visible-grip fix: sanitizer and fresh-process tests pass.
  Build 241 failed universal packaging because the disk filled; number consumed.
  The fix ships in the successfully installed Build 242 / v0.3.221.
- [x] One attached generator body, tight VST frame, mutually exclusive GUI/Piano
  Roll, native instrument editor and shared Drum Rack settings/pattern controls.
- [x] Held-click vertical Playlist pattern choice, alongside wheel and horizontal
  paint, with gesture Undo and the same blank/existing-slot policy.
- [x] Independent per-note/chord-member velocity with note-grid editing/shading.
- [x] Selected-group drag and Ctrl-drag duplication, arrow/octave editing.
- [x] Alt-brush and explicit Pencil/Rectangle tools; scissor and X/Y flips.
- [x] Remaining focused Piano Roll shortcuts, including rotation and navigation.
- [x] Generator-owned grid/display/follow/audition preferences, ghost notes,
  per-note labels and cursor visibility.
- [x] Pattern automation LFO drawing with cancellable preview and one Undo.
- [x] Focused regression/sanitizer checks, UI renders and numbered local build.

**Approved deferral:** on 2026-10-29 the user answered “Hold off until we work on
the sampler” for the Sampler groove-slicer RNG/slice-name ruler. Review/implement
that with the Sampler chapter, not as a prerequisite for this general editor.

No proprietary Orion implementation code, parameter tables or manual assets are
included. Preserve existing project notes, automation and the paused SFS scope.

Build 242 / v0.3.221 compiled successfully as a universal macOS Release, was
ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 240 is
preserved in Builds/Previous/Orbit_Build_240_before_242.app; the running app was
not closed. Patterns, Playlist, Piano Roll, generator and recording ASan/UBSan
profiles passed, plus pure editor transformations. Coverage includes real note
gestures, native/fresh-process preferences, long LFO storage, MIDI event playback,
blank-pattern curve reset, view containment and mock editor UI renders.
Build 241 was not installed. Physical plugin views and hardware timing remain
desktop checks; existing headless JUCE assertions remain documented.

Build 243 follow-up: maximized Piano Roll covers the uninset transport-to-status
workspace, follows app resize, overrides old inset bounds and keeps the dock
behind its controls. Both-theme renders, edge pixels, hit-testing, floating
instrument restoration and the full Patterns/fresh-process regression passed.

Build 244 follow-up: practical actions/results are in TESTING_GUIDE.md and Help >
Quick Test Guide. VEL note labels show numbers. Undo preserves floating instrument
restore bounds after a full Piano Roll, verified with the compact Acid Bass
editor. Full Patterns sanitizer/fresh-process regression passed again. Accent
and Slide rows are generator-specific additions; the approved Sampler deferral
and next-chapter workflow remain unchanged.

Build 245 addresses the four user gesture/plugin recordings: tools moved to the
top, floating-coordinate cell entry and pitch edges, full velocity endpoints,
View > Hotkeys, and a separate Piano Roll maximize/restore control. Generator
native views anchor at zero and refit reported-size changes through a guarded
parent scaling surface. Expanded Patterns sanitizer and fresh-process checks
passed; actual third-party native views require desktop re-testing.

Build 246 corrects the 07-59-07 velocity feedback: simultaneous notes now have
individual exact-onset lane handles rather than an averaged step bar. Active
note/bar highlighting and a numeric readout make 1–127 travel visible. Chord
heights/targets, neighbouring values, fractional onsets and stroke Undo passed
the full Patterns sanitizer profile; the Piano Roll profile passed too.
