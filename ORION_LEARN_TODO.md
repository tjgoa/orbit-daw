# Orbit implementation TODO from Orion Learn

**Priority: HIGH**
**Source:** observations from `ORION LEARN.mov`, combined with the user's Orbit requirements already recorded in `ORION_VISUAL_TARGETS.md` and the supplied screenshots.
**Purpose:** a working product backlog, not a claim that every item is already present or should be delivered in one build.

## Implementation status — product Build 66 / version 0.3.45; development iteration 128

The current pass has implemented the following pieces while the larger Orion backlog remains active:

- Generator panel movement starts on the full-width labeled header, which now reserves space so its six-dot grip remains visible; current-runtime interaction confirmed that the panel moves and the piano-roll/pattern controls stay linked. The resize grip also resizes the panel. Build 29 fixes a hit-test bug that mutated the panel bounds during resize detection, causing repeated corner interactions to ratchet its size; the corner now shows a resize cursor. The piano roll remains an embedded, resizable surface and retains the A–H / 1–8 controls. The separate Orion-like restore/minimize window model remains outstanding.
- Build 30 adds a compact generator collapse/restore control. Collapsing hides the large instrument/editor area but leaves the generator identity and pattern bank/slot controls accessible; the smaller panel can still move. Restoring returns the previous panel size and piano-roll visibility, following the panel's new position if it was moved while collapsed.
- Build 31 moves Playlist Draw/Move and horizontal/vertical zoom into a fixed toolbar above the scrolling timeline. Draw and Move remain distinct modes, and the zoom controls stay accessible when the grid is scrolled.
- Build 32 adds Minimize Generator / Restore Generator to the Window menu and makes Minimize All collapse the generator to its pattern controls while minimizing the separate child windows.
- Build 33 enables Edit > Select All Notes for the active pattern and highlights its notes in the piano roll. Pattern changes and note edits clear the selection.
- Build 38 gives the generator resize corner its own pointer target and visible grip, parallel to the dedicated draggable title strip. This removes resize hit-testing from the parent canvas so controls layered inside the generator cannot steal the gesture. Builds 36–37 failed because JUCE's Component does not provide the tooltip API initially used on the grip; Build 38 removes that call and compiles. The open Orbit process still reports Build 27, so the new resize hit target needs hands-on confirmation in Build 38.
- Playlist Draw places the selected pattern on its owning generator lane. First placement can create that selected pattern record so it does not show the empty-slot warning. Move mode remains a separate clip gesture and does not create a clip from a blank cell.
- Edit menu actions include pattern-note cut/copy/paste, randomize/options, humanize, quantize, transpose, velocity scaling, and length scaling; these edits create undo snapshots.
- Window menu includes Hide All, Minimize All, and Cascade. Reopening Playlist/Mixer restores minimized windows. View includes full-screen control; Audio Input opens the device selector.
- Preferences persist update rate, meter decay, auto-save interval, and a default project path; Open and Save use that path. Project Settings stores description and creator URL.
- The entire generator title strip, including the six-dot grip, label, and blank header space, is now one drag target. The View/Options menus can edit the initial tempo and time signature. The time signature is applied to the Tracktion edit and saved project metadata; the transport and project summary refresh with it.
- MIDI Input Settings now opens the combined JUCE audio/MIDI device selector, which exposes available MIDI input devices.
- View > Modules lists the active built-in generators and activates the selected generator in the existing project workspace.
- Playlist Draw now creates the active A1–H8 slot on any clicked generator lane, without requiring that generator to be selected first. Clicking an occupied clip activates its pattern; the mouse wheel cycles the created patterns on that lane, and double-click opens a menu containing only created patterns.
- Playlist clip moves validate occupied destinations and, when moved across generators, copy notes, note lengths, and velocities into a free destination pattern slot instead of overwriting a same-numbered pattern. A single paint drag records one undo action.
- Options > Sample Paths now opens a persistent folder-list preference and validates each entered folder before saving. Sample browsing is not yet implemented, so this stores the locations for the sampler/import workflow.

Build 52 embeds each VST3 editor inside the selected generator, below its persistent pattern controls; the piano roll overlays that same editor area. File > New Project offers Discard or Cancel when the current project has unsaved changes. Build 51 removed the Mixer heading and excess space above and below the strips. Builds 54–55 made the in-app Mixer movable, resizable, compact, expanded, and minimized without popping out of Orbit, and added Playlist pattern cycling/selection. Build 57 makes the piano roll and plugin GUI mutually exclusive in the same generator area, adds a generator minimize dock, raises the selected generator over the Mixer, and adds eight selectable output buses plus Master. Each strip exposes four send knobs in a 2×2 layout. Build 58 shows 48 piano-roll time columns and draws black keyboard keys at a shorter width than white keys. Build 59 clears stale dock/mixer state on new projects, focuses the piano roll above plugin editors, and leaves only dock tabs when generators are minimized. Build 63 adds a narrow draggable Mixing Desk header with minimize/close controls and fits mixer width to its channel strips. Build 64 removes the vertical scrollbar gutter while keeping wheel scrolling and only shows horizontal scrolling when strips exceed the available width. Build 66 keeps the Mixer hidden on instrument insert, makes clicked mixer/generator surfaces raise to the foreground, adds a keyboard icon and expanded Piano Roll settings, and adds Playlist right-click clip deletion, a cell-anchored pattern menu, MIDI note previews, quieter grid lines, and a settings entry. The Piano Roll retains its lower-edge note-length selector and persistent drag length. Builds 60–62 and 65 were reserved but did not complete; see BUILD_HISTORY.md. Manual runtime interaction and audio-routing verification remain outstanding. The Mixer stays inside Orbit; selecting it or a generator brings that surface forward. The source archives are incremental; see the latest archive's change summary for its baseline.

Build 27 / version 0.3.6 added Add Folder / Remove list windows for Sample Paths and VST Paths, storing multiple paths in user preferences. Build 41 / version 0.3.20 added a VST3 instrument browser; Build 42 / version 0.3.21 retains Tracktion's standard mixer and meter plugins on inserted VST3 generator tracks. Build 44 / version 0.3.23 added automatic standard VST3 search locations. Build 45 / version 0.3.24 moved the scan-at-startup option into VST Paths and added per-plugin scan progress, Desktop scan reports, and fatal-signal crash reports identifying the plugin being scanned. Build 47 / version 0.3.26 displays standard VST3 bundles as direct menu items and reports clearly when a project must be created or opened before insertion. Build 48 / version 0.3.27 reshapes Insert to match Orion's top-level grouping: built-in instruments under Synths, third-party instruments under VST Instruments, plus separate Templates, VST Effects, DirectX, FX categories, and track sections. Plugin entries use standard menu text. First use opens the VST3 browser and scans; selecting an instrument inserts it as a new generator. Custom-folder plugins are grouped by folder. AU and VST2 hosting remain out of scope. Runtime verification of scan, insert, save/reopen, and plugin editor behavior is still needed; the isolated Build 41 preview could not be launched through macOS Launch Services in this session. Sample browsing is separate.

### Menu implementation snapshot

| Menu area | Working today | Still remaining |
| --- | --- | --- |
| File | New/Open/Save, MIDI export, Project Settings | Project format recovery and media relinking |
| Edit | Undo/Redo, note clipboard, Select All, randomize, humanize, quantize, transpose, velocity/length dialogs | Groove/pluck templates |
| View | Playlist, docked in-app Mixer/Master, Piano Roll, audio output, tempo, time signature, Modules, Full Screen, Status Bar, Show Hints | Time display and persistent multi-surface/window focus behavior |
| Insert | Orion-style top-level grouping for internal Synths and VST Instruments; automatic standard-location discovery, extra folders, optional startup scanning from VST Paths, per-plugin progress and Desktop logs, VST3 instrument insertion with an embedded generator GUI, supported internal effect choices | Plugin state save/reopen and missing-plugin recovery; usable sampler/DrumRack and templates |
| Options | Loop/metronome, audio/MIDI device selection, paths, time signature, preferences, project/Playlist settings | MIDI controller mapping, timebase, appearance skins/wallpapers |
| Window | Open surfaces, Hide/Minimize All, Cascade | Stable Orion-like movable/restorable generator and piano-roll focus model |
| Help | About Orbit | Context help and user documentation |

VST3 instrument scanning and insertion are implemented. Orbit starts with a fresh project. Open **Insert > VST Instruments > Scan / Manage VST3 Instruments…** to perform the first scan; standard VST3 locations are included automatically. Add extra folders and set the **Scan VST3 plugins at startup** checkbox on the **Options > VST Paths…** page. Scans show the plugin currently being checked and save a detailed Desktop log. Insertion embeds each plugin GUI below the persistent generator controls; opening the piano roll overlays the same area. The generator header selector switches the active instrument and pattern data. Fatal macOS/Linux signals save a Desktop crash report that records the current plugin path; the host process can still terminate if a native plugin crashes during scanning. AU/VST2 hosting, VST effects, missing-plugin recovery, and project save/reopen with an external instrument still need separate implementation or runtime verification.

## P0 — HIGH: make the core surfaces work together

### 1. Keep project surfaces open together

- [ ] Keep the main project workspace, Playlist, generator, piano roll, and Mixer alive together. Selecting one surface should bring it forward without closing or replacing the others.
- [ ] Keep the piano roll bound to the selected generator and pattern. Changing A–H or 1–8 while the editor is open should switch its note data immediately.
- [ ] Keep the generator's identity, bank/slot selectors, piano-roll action, settings, preset, and length controls accessible while its piano roll is active.
- [ ] Keep Mixer available over the project while the piano roll or generator is open. Maintain window bounds, focus, and stacking order when the user switches surfaces.
- [ ] Choose a consistent child-window model that works on macOS and fits the host window. The current detached/independent windows should not make the generator or editor disappear when Mixer is opened.

**Done when:** the user can open the generator, its piano roll, Playlist, and Mixer, switch among them, and edit the currently selected pattern without closing any of those surfaces.

### 2. Make pattern creation and Playlist painting reliable

- [ ] Fix the current empty-pattern-slot warning when the selected pattern exists in the generator bank. Resolve by stable generator ID + pattern ID, not only the selected row/cell label.
- [ ] Make the selected pattern explicit in the Playlist drawing tool. A click/drag in Draw mode should place that pattern on the selected generator lane at snapped bar positions.
- [ ] Preserve the separate Select/Move behavior for moving an existing clip. Do not make one gesture both paint and move unpredictably.
- [ ] Keep Mute, Solo, and Record Arm controls per track row and ensure the row maps to the same generator/channel everywhere.
- [x] Add compact horizontal timeline zoom and vertical track-height zoom in a fixed toolbar above the scrolling timeline.
- [ ] Keep long-arrangement scroll/navigation and the bar ruler synchronized with horizontal/vertical zoom.
- [ ] Keep track headers fixed while scrolling the timeline; keep bar ruler and clips aligned during zoom and scroll.

**Done when:** existing patterns paint into the Playlist without the “select instrument/create pattern” warning; clips can be placed repeatedly, selected, moved, and removed with unambiguous tools.

### 3. Restore direct piano-roll editing

- [ ] Preserve the preferred default 16-step view and its earlier useful landscape dimensions. Do not reopen with a 24-step/default oversized grid unless the user selected that size.
- [ ] Add clear increment/decrement controls beside the step/length value. Extending length should expose more active grid cells in consistent increments; keep the inactive future region visibly distinct and mark the active end.
- [ ] Support click-to-add, click-drag to extend/move a note without accidental deletion, and right-click to delete. Double-click/ordinary click on an existing note must not erase it.
- [ ] Keep keyboard, note grid, bar/step ruler, playhead, and velocity lane visible and usable at the preferred starting size. Fit the active grid to its panel and avoid a large dead area at the right.
- [ ] Keep note duration in the project model and preserve it through playback and project save/reopen.
- [ ] Make piano-roll and generator pattern state bidirectional: edits update the selected pattern, and switching pattern updates the visible editor.

**Done when:** a user can draw, lengthen, move, and delete notes intentionally; switch patterns without closing the roll; and see the chosen pattern's actual length and note data.

### 4. Provide a legible generator unit and lightweight instrument GUI area

- [ ] Tighten the generator header to one compact identity row plus pattern bank/slot controls; remove redundant title/status rows that consume vertical space.
- [ ] Enclose the generator in a defined panel with dividers. Avoid stretching the control group across the host or leaving an unbounded blank area.
- [ ] Present A–H above 1–8 in the established compact order; keep M/S, piano-roll, settings, preset load/save, and pattern length next to the bank/slot controls.
- [x] Put the plugin GUI directly below its generator controls, and overlay the piano roll in the same area when active.
- [ ] Let the piano roll occupy that instrument area when active, while preserving the generator and pattern selectors above it.
- [ ] Give each added generator a stable name/number and independent pattern bank, instrument state, preset state, and editor association.

**Done when:** the panel clearly reads as one generator with bank, selected pattern, instrument area, and editor state, without wasting most of the window width or height.

## P1 — HIGH: add the instrument/effect insertion path

### 5. Recreate Orion's menu and dialog workflows

- [ ] Keep the File, Edit, View, Insert, Options, Window, and Help menus compact and available from every workspace state.
- [ ] File: New/Open/Save/Export MIDI and Project Settings, including project description and creator URL.
- [x] Edit: working undo/redo, pattern-note cut/copy/paste, Select All, Randomize and Randomize Options, Humanize, Quantize, Transpose, Scale Velocities, and Length actions.
- [ ] Edit: add groove and pluck template actions.
- [ ] View: Playlist, Event Editor/Piano Roll, Mixing Console, Master, Tempo, Time, Modules, Full Screen, Status Bar, and Show Hints.
- [ ] Insert: organized generator, track, template, instrument, and effect categories; activate only actions Orbit can perform and keep third-party hosting excluded.
- [ ] Options: Preferences, audio output and input, MIDI input/controllers, sample locations, time signature, timebase, skins, and wallpapers; settings that have no Orbit behavior must remain visibly unavailable until implemented. (Audio, MIDI input, and time signature are implemented.)
- [ ] Window: list open child surfaces, show/focus them without replacing the project, and provide Hide/Minimize All and Cascade behavior.
- [ ] Persist preferences separately from project settings; persist project metadata and musical settings in the `.orbit` file.
- [ ] Bind shortcuts to the same actions as their menu items, and keep enabled/check states synchronized with the selected project and open views.

**Suggested menu passes:** (1) menu shell and window commands; (2) pattern editing actions; (3) project settings and preferences; (4) Playlist/generator settings; (5) audio/MIDI configuration and insert categories; (6) polish and end-to-end verification. VST hosting is excluded from this scope.

**Done when:** each enabled menu item performs its labeled action, project and user settings survive their appropriate reopen cycle, shortcuts match menu actions, and unavailable features are not presented as working controls.

### 6. Build a useful Insert menu and generator instantiation model

- [ ] Add an Insert menu organized around user actions: Drum/Step Instrument, Sampler, Synth/Instrument, Template, Audio Track, MIDI Output, and Effect submenus.
- [ ] Start with Orbit's built-in 4OSC instrument and a basic DrumRack/step-sequencer path. Keep an extensible registry for future built-in instruments and supported plugin formats.
- [ ] Create each inserted instrument as a new generator/track with a corresponding Playlist row, mixer channel, pattern bank, and plugin/instrument state.
- [x] Embed each inserted instrument GUI inside its generator area, with its pattern controls remaining above the GUI.
- [ ] Provide clear failure messaging for unavailable plugins; do not silently create an unbound pattern or empty track.

**Done when:** inserting an instrument creates one coherent generator that appears in the generator area, Playlist, and Mixer, and its pattern data is routed to that instrument.

### 7. Separate instrument hosting from effect insertion

- [ ] Add a plugin scan/catalog and stable plugin identity, including missing-plugin handling and re-link/recovery behavior.
- [ ] Support instrument plugin insertion as an instrument source, and effect plugin insertion as an ordered effect on a channel/return/master path.
- [ ] Allow opening/closing/reopening plugin GUIs while other Orbit surfaces remain available.
- [ ] Add basic preset load/save selection and persist selected preset/plugin state with the project.
- [ ] Define supported platforms/formats and plugin sandbox/crash boundaries before shipping third-party hosting broadly.

**Done when:** an instrument produces sound through its own track, an effect can be assigned to the intended channel, and plugin GUI state does not replace or disconnect the editor/mixer.

## P1 — HIGH: align Playlist and transport workflow

### 8. Make the top-level controls compact and complete

- [ ] Keep the menu row and transport row compact, with the dark background fully containing every control and no icon clipping.
- [ ] Use the demonstrated order: BPM; play, stop, previous/rewind, next/forward, record; loop; start; length; metronome and meter; Playlist, Mixer, Master; output meter at far right.
- [ ] Make transport state visible and functional: play/stop, timeline position/bar, loop range, start and length, metronome, and time signature.
- [ ] Keep Playlist/Mixer/Master navigation persistent when opening other surfaces.
- [ ] Add keyboard shortcuts and menu equivalents for common actions, including undo/redo, play/stop, surface focus, and pattern creation where appropriate.

**Done when:** the control sequence is stable and readable at normal host sizes and every displayed control has a defined, working action.

### 9. Improve Playlist editing affordances

- [ ] Keep the bar ruler, labeled tracks, M/S/Record controls, and clip labels readable at overview and detail zoom levels.
- [ ] Use snapping and show the snap/quantize setting near the Draw/Move controls.
- [ ] Support drawing repeated selected-pattern clips by drag, moving clips between bars/tracks, and resizing clip duration where the project model supports it.
- [ ] Provide horizontal and vertical zoom separately, plus a fit/overview action and navigation that does not move track labels out of alignment.
- [ ] Show the current transport/playhead and loop region in the Playlist; keep it synchronized with the top transport and piano roll.

**Done when:** a short pattern can be arranged across a song, zoomed and navigated, and auditioned with a shared playhead/loop state.

## P2 — visual fidelity and production polish

### 10. Refine Mixer and Master against the blue references

- [ ] Keep only live instrument and active FX return channels plus Master and horizontally scroll when the channel count exceeds the viewport.
- [x] Remove the unused FX 1–4 header area; show four send controls in a 2×2 layout and eight output-bus assignments plus Master at each channel bottom. Build 57 implements direct Tracktion track-output routing; verify the signal path in the running app.
- [ ] Use the target strip proportions, recognizable panel dividers, larger/send knobs, and the reference's diagonal EQ control grouping.
- [ ] Use the intended EQ bands: Hi Shelf, Low Mid, and Low Shelf have Gain/Freq; High Mid has Gain/Freq/Q. Do not show Q on the shelves or Low Mid, per the user's mixer correction.
- [ ] Use three neutral insert slots with no “+ Insert” in the default strip; show the insert picker through an empty slot/action affordance.
- [ ] Keep the EQ graph clickable to reveal the detail/parameter view. Preserve the channel response display and make graph changes correspond to EQ state.
- [ ] Make Mute, Solo, pan, fader, stereo meter, and insert enable states legible and functional. Keep the Master strip's controls appropriate to a master channel.
- [ ] Use drawn JUCE controls for interactive geometry and resize behavior; use panel artwork/PNG only for decorative texture or bevel details that are too costly to reproduce cleanly.

**Done when:** all active channels fit without a large blank expanse, the strips scroll when needed, and the visual grouping/knobs/EQ/insert/meter behavior matches the supplied design reference closely.

### 11. Undo, project persistence, and recovery

- [ ] Make Undo/Redo cover user-visible structural edits first: insert/remove instrument, create pattern, place/move/delete clip, note edits, and mixer/EQ changes.
- [ ] Ensure each user action is one undo step and labels the action. Keep engine state and Orbit project data synchronized during undo/redo.
- [ ] Persist plugin identity/state, pattern bank data, clip arrangement, mixer state, and relevant editor preferences; recover missing media/plugins with actionable messages.
- [ ] Provide a clear audio-device recovery path at startup, including continuing with a safe/default configuration if the prior device is unavailable.

**Done when:** users can reverse recent operations predictably and reopen a project without losing its generators, clips, notes, mixer settings, or plugin references.

## P3 — later, after the core workflow is stable

- [ ] Expand DrumRack into a useful multisample/step sequencer with independent voice rows and per-step pattern data.
- [ ] Add MIDI device input settings, keyboard input, MIDI output, and controller mapping with visible device status.
- [ ] Add audio input/recording only when Orbit has a complete recording and media-management path.
- [ ] Expand effect categories and FX returns after channel insert ordering and routing are stable.
- [ ] Add project templates and factory/user preset browsers.
- [ ] Consider larger Orion-specific options (timebase, alternate MIDI modes, DirectX-era compatibility) only when they fit Orbit's platform and support plan.

## Suggested implementation sequence

1. Fix model/selection identity for patterns and get Playlist paint/move semantics correct.
2. Make all project surfaces coexist; keep the generator header and pattern selectors persistent while the piano roll is open.
3. Restore the preferred piano-roll starting dimensions, active-length controls, note drag/resize/delete behavior, and pattern switching.
4. Make the compact generator/instrument area a coherent unit.
5. Align transport and Playlist navigation; synchronize ruler, playhead, loop, zoom, and selection.
6. Improve high-priority mixer geometry and interactive controls against the references.
7. Verify VST3 scan/insert and save/reopen, then add built-in DrumRack and organize effect insertion.
8. Complete undo/redo and persistence across all above edits before broadening the feature set.

## Reference files to keep in view

- `ORION_VISUAL_TARGETS.md` — the existing ongoing visual requirements for Orbit.
- `MIXER.jpg`, `8512af06-78f6-426d-a711-0e62cd1bfa38.jpg` — blue mixer/channel-strip visual target.
- `videoframe_72931.png`, `Screenshot 2026-10-25 at 9.52.26 PM.png`, `Screenshot 2026-10-25 at 9.53.04 PM.png` — generator toolbar and piano-roll proportions/behavior.
- `gerner.jpg` — generator header and instrument-area grouping.
- `synapse-audio-orion-8-116025.jpeg` — Playlist track controls, zoom, editing tools, piano-roll context.
- `ORION LEARN.mov` — Orion menu hierarchy, child-window workflow, Playlist, DrumRack, plugin and piano-roll examples.
