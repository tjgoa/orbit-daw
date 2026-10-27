# Orion Learn video: workflow and UI notes

**Reference:** `ORION LEARN.mov` in this project folder
**Runtime:** 4:38.7
**Priority:** HIGH — use this reference while bringing Orbit’s generator, Playlist, piano roll, mixer, and instrument workflow up to speed.

## How to read these notes

These are observations from the supplied screen recording of Orion 64. They describe what is visible or legible in the recording; they do not assume that every Orion feature shown is required for Orbit's first implementation. The video is largely a screen capture with no reliably transcribed narration. Other Orbit requirements the user gave in chat and in the separate screenshots are kept in the companion TODO/reference rather than presented as video evidence.

## Timestamped observations

| Approx. time | What is shown | Workflow/design observation |
|---|---|---|
| 00:00–00:20 | Orion launches; an audio recovery/configuration prompt appears. | Startup needs a useful recovery path when the previous audio configuration did not close cleanly. Audio device setup is an application-level task, not a music-editing window. |
| 00:20–00:40 | The Playlist opens as a large child window. A track lane and bar ruler are visible. | The arrangement is the main canvas: a labeled track header at left, bars along the top, clips on a grid to the right. The window has its own title/header and can be layered in the host workspace. |
| 00:40–01:05 | A DrumRack is inserted and shown as a compact floating child window; Playlist clips become visible. | A drum generator contains multiple sound rows, sample/source fields, per-row controls, and a step sequencer. The plugin/generator is its own movable window and does not replace the Playlist. |
| 01:05–01:30 | The Playlist shows a short run of pattern clips; the ruler displays bar numbers. The view changes scale/extent. | Clips are aligned to bars. A repeated pattern can be laid across multiple bars. Timeline zoom affects how many bars fit on screen; the user can see the clip run and empty arrangement area together. |
| 01:30–01:55 | Options menus open, including audio/MIDI-related settings. Audio Input Settings and MIDI In Settings dialogs are shown. | Setup functions are available through Options. Audio input exposes driver/device choices and input/monitoring/recording options; MIDI input has device selection, mode/filter options, and controller-related settings. Exact dialog labels are small in the recording. |
| 01:55–02:15 | The Playlist/host is briefly cleared, then generator windows return. | The work area supports several independent child windows and changing which one is in front. A blank or hidden surface should not imply that a project or its generators have been removed. |
| 02:15–02:40 | An Acoustic Grand instrument/plugin window is visible, alongside mixer strips. A piano-roll window opens. | Instrument GUI, mixer, and piano roll can coexist as separate windows. Opening the editor does not destroy the generator or hide the mixer/arrangement permanently. The active child window simply comes to the front. |
| 02:40–03:00 | Piano roll shows a note grid and keyboard; another compact Monobass GUI appears. | Notes are placed on a pitch/time grid; instrument identity remains visible in window titles. The recording demonstrates multiple instrument/plugin windows open in one project. |
| 03:00–03:20 | The piano roll is enlarged and shown with notes; generator GUIs and mixer remain behind it. | Editors should be sizable and usable while keeping other project surfaces available. The piano roll is a per-generator/per-pattern editor, not a replacement for the whole project workspace. |
| 03:20–03:40 | Insert menu is opened again; the host shows multiple generators and a small mixer. | Multiple instances of an instrument can exist; each needs distinct track/generator identity and its own editor/preset state. The mixer shows corresponding channel strips. |
| 03:40–04:00 | Insert menu categories are visible, then multiple plugin/effect windows are opened. | Orion groups insertion choices into generators/instruments, effects, audio tracks, and MIDI output. Visible category families include drum machines, sampler, synths, templates, VST/DirectX instruments, and FX groups such as dynamics, echo/delay, reverb, filter, and miscellaneous. This is a useful model for Orbit's Insert menu taxonomy. |
| 04:00–04:38 | Several instrument/effect interfaces are open over the workspace. | The host can keep multiple plugin GUIs available at once. These are distinct from mixer channel strips and the Playlist; window overlap and focus are part of the workflow. |

## UI and interaction patterns visible in the recording

### Main workspace and transport

- The host has a conventional application menu row (File, Edit, View, Insert, Options, Window, Help).
- A compact transport/control strip sits directly under the menus. It groups tempo, playback controls, bar/position, loop, start/length, metronome, and navigation to Playlist, Mixer, and Master.
- An output level display sits at the far right of the transport strip.
- The Playlist, generators, piano roll, mixer, and plugin GUIs appear as independent child surfaces layered within the same overall project workspace. The user can bring different surfaces to the front without discarding the others.

### Playlist and patterns

- Track names and track controls occupy a narrow fixed column at the left.
- The bar ruler runs across the top of a large grid. Vertical bar lines and horizontal track lanes remain visually distinct.
- Pattern clips are short labeled blocks, snapped to the bar grid. Repeated blocks can span many consecutive bars.
- The arrangement view can show a long timeline; zoom and horizontal navigation are needed to move between overview and detailed editing.
- The Playlist remains available behind floating generator and piano-roll windows.

### Generator and step sequencer

- DrumRack is presented as a compact generator window rather than a full-screen page. It has a generator title, pattern-bank/slot and preset/step controls in the header, then a divided instrument body.
- The DrumRack body is row-based: each row represents a drum/sample voice and has a source/name area, small controls, and a 16-step lane. Active steps are visible in blue.
- The row-based step sequencer is an instrument feature. It is distinct from the Playlist's song arrangement grid. Keep these two concepts visually and functionally separate in Orbit.
- Multiple generator instances have separate numbered titles (for example, DrumRack #1 and Acoustic Grand #1/#2 in the recording).

### Piano roll

- The piano roll is a separate, resizable child editor with keyboard pitches on the left, a time grid, note blocks, a ruler, and an editing/velocity area at the bottom.
- The editor can be layered over the generator/mixer surfaces. Opening it does not require closing the instrument or switching away from the project.
- Notes occupy time/pitch cells and have visible durations. A playhead is shown in the editor while working.
- The user’s prior request adds a specific Orbit behavior: retain the A–H and 1–8 selectors while the piano roll is active and update its contents immediately when a different pattern slot is chosen.

### Mixer and plugins

- Mixer strips can remain visible while plugin GUIs and editors are open. Channel strips are a different surface from instrument/plugin interfaces.
- Third-party instrument/plugin windows have their own titles and controls; several can be open at the same time.
- The Insert menu separates instrument creation from effect insertion. Effects can be instantiated and presented in their own GUI windows.
- The user’s supplied blue XR-909 mixer images remain the styling target for Orbit’s channel-strip appearance; this video supports the overlapping-window workflow, not the XR-909 artwork/layout.

## Limits of the evidence

- The recording is low-resolution at the original desktop scale for some menu/dialog text. The major menu families and interaction order are visible; fine dialog settings should be verified from a clearer reference before copying.
- It shows Orion 64 behavior and UI, not a specification that Orbit should copy Orion’s branding, exact graphics, or every legacy option.
- This recording is not evidence that Orbit already implements the demonstrated behavior. See `ORION_LEARN_TODO.md` for the proposed Orbit work and completion criteria.
