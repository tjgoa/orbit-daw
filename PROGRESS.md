# Orbit development progress

## 2026-10-26 — Build 100 / v0.3.79

The local app at `Builds/Current/Orbit.app` is now Build 100. This pass restarts Tracktion's playback graph immediately after inserting built-in and VST3 instruments, limits Playlist clip controls and drawing to the visible timeline, and replaces the broken bottom-right Playlist glyphs with explicit ASCII labels. The build completed successfully and source archive iteration 155 was created. Runtime keyboard audition and Playlist latency still need a fresh Build 100 launch for confirmation; the open Orbit window remains Build 99.

## 2026-10-26 — Build 94 / v0.3.73

Orbit has moved from a basic DAW shell toward the generator-first workflow that guides the project. This build compiles locally and adds several of the editing behaviors shown in the Orion references.

### Newly implemented

- Add an explicit Pattern/Song transport selector beside BPM. Pattern mode plays all generators’ remembered patterns in sync; Song mode plays the Playlist. Each generator retains its own selected pattern in project state, Playlist playback updates only tracks with clips at the current position, and switching modes preserves the last reached selection.

- Let Play audition the selected, note-filled pattern when the Playlist has no clips; keep arrangement playback when Playlist clips exist. Pattern audition loops until Stop.
- Draw the current transport position in the Piano Roll and Playlist while playing.
- Replace Unicode ellipses in menu strings with ASCII dots to avoid the garbled menu labels shown in the latest screenshot.

- Put Playlist Draw/Move, horizontal and vertical zoom, and grid settings in a persistent lower-right icon strip, matching the familiar Piano Roll control placement.
- Make right-click drag an erase gesture over occupied Playlist cells; empty cells stay empty and do not paint.

- Keep instrument creation focused: inserting a generator no longer opens the Mixer. The Mixer stays embedded in the main workspace, and clicking a generator or Mixer surface brings it forward.
- Make the Piano Roll settings easier to reach with a small keyboard icon and options for grid spacing, emphasis, velocity lane visibility, colored keys, and pitch labels.
- Improve Playlist editing with right-click clip deletion, a pattern menu anchored beside the clicked clip, subtle MIDI note previews, and lighter grid lines.
- Fix Piano Roll note hit-testing so clicking anywhere on a drawn note targets that note for resize or delete instead of creating an overlapping note in an occupied cell. New notes continue using the chosen paint length.
- Keep the velocity lane, keyboard-colouring, and note-name controls visible as direct icon buttons in the Piano Roll footer. Grid subdivision and emphasis remain in the gear menu.
- Continue the VST3 workflow with standard-folder scanning, configurable paths, scan progress, and local diagnostic reports.

### Current focus

Orbit remains in active development. Build 100 compiles successfully and stages the app at `Builds/Current/Orbit.app`. Next priority is confirming immediate keyboard audition and Playlist responsiveness in a fresh session, then checking loop playback and persistence, both automation layers, mixer bus/master routing, Playlist clone/color behavior, and the remaining mixer settings/effects clone options. Plugin-state recovery and broader project compatibility remain later work.

Recent passes added separate pattern/song automation lanes, a dedicated Master Section with four insert slots and no sends, automatic appearance of routed buses, preservation of host automation curves, and the paste-ready Orion native-effects inventory. Build 94 added Playlist generator context actions, pattern/arrangement cloning, and persistent track colors. Build 95 / v0.3.74 persists loop start/length/enabled state and highlights the loop region in the Playlist ruler; loop timing uses the project time signature. Runtime interaction for clone, color, loop wrap/save-reopen, automation and bus routing still needs hands-on verification. Mixer Settings/Effects cloning remains on the roadmap, along with remaining piano-roll, mixer, and Playlist parity work.

See [`BUILD_HISTORY.md`](BUILD_HISTORY.md) for the full build timeline and [`ORION_LEARN_TODO.md`](ORION_LEARN_TODO.md) for the working backlog.

## 2026-10-26 — Playlist workspace and audition follow-up

Build 98 / v0.3.77 embeds the Playlist below the persistent transport toolbar, expands its range to 512 bars, and lets a left-drag on the ruler set the loop start and length. Zoom can show roughly 300 bars in a wide workspace. Selecting Playlist toggles that embedded panel without being tied to Loop. Loop length up to 512 bars is saved with the project.

The generator's duplicate visible resize grip is hidden while keeping its corner resize hit area. A compact two-octave host keyboard appears beneath an open instrument editor and sends note-on/note-off directly to the selected generator, independent of transport state. Build 98 compiles and is installed at `Builds/Current/Orbit.app`. Runtime checks of ruler dragging, zoom range, plugin audition and editor resizing remain pending.
