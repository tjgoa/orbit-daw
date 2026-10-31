# Orbit implementation TODO from Orion Learn

**Priority: HIGH**
**Source:** observations from `ORION LEARN.mov`, combined with the user's Orbit requirements already recorded in `ORION_VISUAL_TARGETS.md` and the supplied screenshots.
**Purpose:** a working product backlog, not a claim that every item is already present or should be delivered in one build.

**Current agreed workflow (2026-10-29):** implement and test each manual section
before advancing. Return to Patterns first, then Playlist/Recording, then
Automation; do not proceed to Mixer merely because its chapter is next unread.
Get the user's agreement for any deferred requirement. The later chapter
reviews remain references, not completed implementations. Handle reported
interaction bugs immediately, including the Build 241 Generator-column grip.

**Patterns implementation checkpoint (Build 242):** general editor features are
implemented and covered by focused checks; see
[Patterns checklist](Source/Editing/PATTERNS_IMPLEMENTATION.md). The user approved
deferring only the Sampler RNG/slice-name ruler until the Sampler section.
Playlist/Recording is the next implementation pass after this checkpoint.

**Manual reference (2026-10-29):** the connected Orion Help and the supplied generator
options recording now inform generator-menu implementation. Working host MIDI
settings, Arpeggiator/Shuffle with Render/Undo, names, pinned CC learning and native
MIDI receiving are documented in [Source/Generator/README.md](Source/Generator/README.md).
Keep [ORION_MANUAL_AUDIT.md](ORION_MANUAL_AUDIT.md) as the explicit chapter-review and
remaining-differences checklist; its unchecked topics are not claimed as complete.

**Ordered manual pass through Patterns/Playlist (2026-10-29, local Build 239):**
System Requirements, Patterns and Playlist/Recording are now compared in sections
3–5 of the audit. Remaining priorities include per-note chord velocity editing,
selected-note group move/duplicate, Piano Roll shortcuts and per-generator
preferences; real Playlist follow/snap/preview controls, clip-group editing with
associated automation; and recording count-in, audible loop overdub and Song
pattern-change capture. MIDI note takes already exist; audio recording does not.
These are logged gaps, not a new implementation build. Next manual topic:
Automation: Pattern and Song events, including the distinct Event Editor.

**Automation chapter follow-up (2026-10-29):** section 6 of the audit now covers
the complete chapter and related View reference. Dedicated Song Event Editor,
parameter-specific context routing, stable plugin-instance targets, graph/lane
tools and live automation recording remain open. Current plugin Pattern lanes
also need arrangement repetition/precedence work. Next ordered topic is Mixer.
Build 240 addresses the user's subsequent Playlist header drag, ruler seek/loop
gestures and optional Piano Roll-only root/scale snapping, independently of that
automation implementation backlog.

**Legacy import research:** Orionology's observed capabilities, AGPLv3 boundary, pending Synapse licensing proposal, Jouni's supplied reply (raw layouts derive from Orion headers; scaling tables require Richard's permission), and the recommended Transplant Map Compiler/IR stages are recorded in [ORIONOLOGY_ANALYSIS.md](ORIONOLOGY_ANALYSIS.md). Orion SFS import or merge remains a one-way, preview-first feature; supported independent recovery and unresolved coverage are detailed in [Source/Import/README.md](Source/Import/README.md).

## User-requested generator windows — 2026-10-29

- [x] Build 260: independent built-in Acid Bass/Drum Rack/4OSC control windows can stay open together, with their own pattern selectors, native instrument controls, Undo restoration and saved placement.
- [ ] Complete the larger multiple-generator workspace request: Each visible panel must keep its own generator identity, GUI, pattern/header controls and Piano Roll state; selecting/opening another generator must not replace the existing visible panel. Validate movement, focus, stacking, close/reopen and native plugin containment for simultaneous panels.

## Implementation status — product Build 206 / version 0.3.185; development iteration 244

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
- MIDI Input Settings now has device selection, Easy/Advanced channel routing, message filters and pedal policy; a separate MIDI Controllers panel adds CC assignments and learning.
- View > Modules lists the active built-in generators and activates the selected generator in the existing project workspace.
- Playlist Draw now creates the active A1–H8 slot on any clicked generator lane, without requiring that generator to be selected first. Clicking an occupied clip activates its pattern; the mouse wheel cycles the created patterns on that lane, and double-click opens a menu containing only created patterns.
- Playlist clip moves validate occupied destinations and, when moved across generators, copy notes, note lengths, and velocities into a free destination pattern slot instead of overwriting a same-numbered pattern. A single paint drag records one undo action.
- Options > Sample Paths now opens a persistent folder-list preference and validates each entered folder before saving. Audio import and Drum Rack sample loading now start in the first existing configured folder.
- Builds 202–203 removed the earlier short pattern-length bounds, stored pattern notes and Drum Rack hit/velocity data sparsely, and added overflow-safe tick/end calculations. Rack length is set before hit data loads, and cached hit/velocity state follows edits. The current Rack UI still exposes variable lengths; the next Rack pass replaces that with a fixed 16-step bar selector covering four bars (64 steps), while general MIDI/piano-roll patterns remain uncapped by the Rack UI. Build 206 adds MIDI track/song import and pattern/song MIDI export through File.

Build 52 embeds each VST3 editor inside the selected generator, below its persistent pattern controls; the piano roll overlays that same editor area. File > New Project offers Discard or Cancel when the current project has unsaved changes. Build 51 removed the Mixer heading and excess space above and below the strips. Builds 54–55 made the in-app Mixer movable, resizable, compact, expanded, and minimized without popping out of Orbit, and added Playlist pattern cycling/selection. Build 57 makes the piano roll and plugin GUI mutually exclusive in the same generator area, adds a generator minimize dock, raises the selected generator over the Mixer, and adds eight selectable output buses plus Master. Each strip exposes four send knobs in a 2×2 layout. Build 58 shows 48 piano-roll time columns and draws black keyboard keys at a shorter width than white keys. Build 59 clears stale dock/mixer state on new projects, focuses the piano roll above plugin editors, and leaves only dock tabs when generators are minimized. Build 63 adds a narrow draggable Mixing Desk header with minimize/close controls and fits mixer width to its channel strips. Build 64 removes the vertical scrollbar gutter while keeping wheel scrolling and only shows horizontal scrolling when strips exceed the available width. Build 66 keeps the Mixer hidden on instrument insert, makes clicked mixer/generator surfaces raise to the foreground, adds a keyboard icon and expanded Piano Roll settings, and adds Playlist right-click clip deletion, a cell-anchored pattern menu, MIDI note previews, quieter grid lines, and a settings entry. Build 67 makes Piano Roll note hit-testing cover the full drawn note, allowing right-click deletion and drag-length editing anywhere on a long note without mistaking it for an empty cell; new notes continue to use the selected paint length. Build 68 moves velocity-lane, coloured-key, and pitch-name toggles into always-visible icon buttons along the Piano Roll footer; the settings pop-up now contains only grid subdivision and emphasis choices. The Piano Roll retains its lower-edge note-length selector and persistent drag length. Builds 60–62 and 65 were reserved but did not complete; see BUILD_HISTORY.md. Manual runtime interaction and audio-routing verification remain outstanding. The Mixer stays inside Orbit; selecting it or a generator brings that surface forward. The source archives are incremental; see the latest archive's change summary for its baseline. Build 69 moves Playlist Draw/Move, zoom and settings into a persistent lower-right icon strip. Right-clicking an occupied clip deletes it, and dragging with the right button erases occupied cells along the gesture; empty cells remain empty and never invoke paint behavior. Build 70 lets Play audition the selected pattern when the Playlist has no clips, routes that pattern through the selected audio output, and draws the moving transport position in both the Piano Roll and Playlist. When the Playlist contains clips, Play runs the arrangement. Menu ellipses use plain ASCII to avoid the garbled â€¦ text seen in the current UI.

Build 27 / version 0.3.6 added Add Folder / Remove list windows for Sample Paths and VST Paths, storing multiple paths in user preferences. Build 41 / version 0.3.20 added a VST3 instrument browser; Build 42 / version 0.3.21 retains Tracktion's standard mixer and meter plugins on inserted VST3 generator tracks. Build 44 / version 0.3.23 added automatic standard VST3 search locations. Build 45 / version 0.3.24 moved the scan-at-startup option into VST Paths and added per-plugin scan progress, Desktop scan reports, and fatal-signal crash reports identifying the plugin being scanned. Build 47 / version 0.3.26 displays standard VST3 bundles as direct menu items and reports clearly when a project must be created or opened before insertion. Build 48 / version 0.3.27 reshapes Insert to match Orion's top-level grouping: built-in instruments under Synths, third-party instruments under VST Instruments, plus separate Templates, VST Effects, DirectX, FX categories, and track sections. Plugin entries use standard menu text. First use opens the VST3 browser and scans; selecting an instrument inserts it as a new generator. Custom-folder plugins are grouped by folder. AU and VST2 hosting remain out of scope. Runtime verification of scan, insert, save/reopen, and plugin editor behavior is still needed; the isolated Build 41 preview could not be launched through macOS Launch Services in this session. Sample browsing is separate.

### Menu implementation snapshot

| Menu area | Working today | Still remaining |
| --- | --- | --- |
| File | New/Open/Save, native project/template merge, Save As/Template, MIDI track/song import and pattern/song export, Stream To File, Preferences/Project Settings, recent projects | Orion SFS import/merge (one-way, preview-first; contingent on parser coverage and licensing), project recovery/media relinking |
| Edit | Undo/Redo, note clipboard, Select All, randomize, humanize, quantize, transpose, velocity/length dialogs | Groove/pluck templates |
| View | Playlist, dedicated Song Event Editor, docked in-app Mixer/Master, generator Piano Roll, audio output, tempo, time signature, Modules, Full Screen, Status Bar, Show Hints | Time display and persistent multi-surface/window focus behavior |
| Insert | Orion-style top-level grouping for internal Synths and VST Instruments; automatic standard-location discovery, extra folders, optional startup scanning from VST Paths, per-plugin progress and Desktop logs, VST3 instrument insertion with an embedded generator GUI, supported internal effect choices | Plugin state save/reopen and missing-plugin recovery; usable sampler/DrumRack and templates |
| Options | All reference entries: original skins/wallpapers, audio input/output, MIDI channel/filter policies, CC learning/assignments, MIDI output, virtual keyboard, paths, meter/triplet grid and export/display timebase; existing extra settings retained | Physical-device/native-desktop validation, end-to-end recording, external MIDI clock following, general mixed-meter arrangements |
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
- [x] Edit: all 15 commands wired with detailed dialogs, exact/selection-aware clipboard, shortcuts, Randomize targets/algorithms/chord scales, tick-based Humanize/control variation, independent Quantize fields, drum-safe Transpose scopes, Scale Velocities/Lengths, and original Groove/Pluck templates. Host/sanitizer checks pass; see Source/Editing/README.md for desktop and recording limits.
- [ ] Edit: add groove and pluck template actions.
- [ ] View: separate Event Editor automation from Piano Roll notes. Current item 304 is miswired to showPianoRoll(); review the manual before implementing the correction, as requested on 2026-10-29.
- [ ] Insert: organized generator, track, template, instrument, and effect categories; activate only actions Orbit can perform and keep third-party hosting excluded.
- [x] Options: all reference menu entries have working handlers and persisted settings, including original skins/wallpapers, separate audio input/output, MIDI routing/filters/controllers/learn, MIDI out, virtual keyboard, paths, time signature/triplets and non-destructive export/display timebase. Host and panel checks are documented in Source/Options/README.md; hardware/native presentation, recording and mixed-meter arrangement limits remain explicit.
- [ ] Window: list open child surfaces, show/focus them without replacing the project, and provide Hide/Minimize All and Cascade behavior.
- [ ] Persist preferences separately from project settings; persist project metadata and musical settings in the `.orbit` file.
- [ ] Bind shortcuts to the same actions as their menu items, and keep enabled/check states synchronized with the selected project and open views.

**Suggested menu passes:** (1) menu shell and window commands; (2) pattern editing actions; (3) project settings and preferences; (4) Playlist/generator settings; (5) audio/MIDI configuration and insert categories; (6) polish and end-to-end verification. VST hosting is excluded from this scope.

**Done when:** each enabled menu item performs its labeled action, project and user settings survive their appropriate reopen cycle, shortcuts match menu actions, and unavailable features are not presented as working controls.

### File workflows — Orion reference analyzed 2026-10-27

The supplied Orion recording shows these File menu entries: New, Open, Merge, Save, Save As, Save As Template, Stream To File, Import, Export, Preferences, Project Settings, Recent files, and Exit. Import and Export are submenus. These notes distinguish the Orion reference workflow from Orbit's supported formats.

- [x] Import MIDI Track… lets the user choose a MIDI track; Import MIDI Song… creates one Orbit instrument/pattern lane for each note-bearing MIDI track. Beat timing and durations become pattern ticks, and song clips span each imported track's full note range rather than a short fixed pattern.
- [x] Export Pattern as MIDI File… and Export Song as MIDI File…; song export follows placed MIDI pattern clips and includes the project's initial tempo and meter.
- [x] Merge… has a multi-select track dialog with Select All, Select None, Merge, Cancel, and Remove unused FX sends. Native `.orbit` projects and `.orbittemplate` files preserve track/pattern/arrangement/plugin state. Referenced media remains at its source path.
- [ ] Merge Orion `.sfs` projects. The dialog recognizes `.sfs` but rejects it clearly because Orbit has no SFS parser; no source data is changed.
- [x] Save As… and Save As Template… use `.orbit` and `.orbittemplate` formats. Opening a template creates an unsaved project copy so Save cannot overwrite the template.
- [x] Stream To File… renders asynchronously using writable audio formats registered in the current build, with supported 16/24/32-bit depths, 44100/48000/96000/192000 Hz, mono, and Entire Song / Instrument Tracks / All Track Channels combined-mix scopes. This build does not create individual stems; formats or bit depths unavailable from the active encoder are omitted.
- [x] Add a persistent recent-project list and retain New/Open/Save/Preferences/Project Settings/Exit actions.

**Scope boundary:** Orion's `.sfs` format is not Orbit's native `.orbit` format. Orbit does not claim to open `.sfs` projects losslessly. Native project merge keeps referenced media at the existing file paths rather than copying it into the destination project.

### File-menu implementation record

- The File menu now follows the Orion hierarchy and every enabled item has a working action: native project merge, MIDI track/song import, pattern/song MIDI export, Save As, Save As Template, recent projects, and asynchronous audio rendering.
- MIDI import preserves PPQN note timing/lengths and derives pattern duration from note endpoints, bounded only by the signed-tick project representation. Import applies the first tempo and meter events that occur at time zero; later changes and SMPTE-timed MIDI are not represented.
- Song MIDI export writes placed MIDI pattern clips only; audio clips are not converted into MIDI. It includes the project's initial tempo and meter.
- Merge is available for Orbit-native projects and templates. Orion `.sfs` parsing remains outstanding, and media/plugin dependencies are not copied or guaranteed to exist on the destination system.
- Audio render choices are driven by the current engine's writable formats and bit depths. Track scopes are combined mixes, not per-track stems; formats absent from the registered encoders are omitted.

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


Build 75 adds a visible Pattern/Song selector beside BPM. Pattern mode plays every generator’s remembered pattern simultaneously; each generator stores its A1–H8 selection in project state. Song mode plays Playlist clips and updates only generators with a clip at the current song position. Stopping or switching modes carries the last reached pattern into Pattern mode, while generators absent from the Playlist keep their prior selection. Older projects default to A1 per generator.

## Build 90 — automation and separate Master Section

- Pattern automation is available from the Piano Roll lower-lane selector. Choose an automatable instrument parameter, then draw or drag normalized values over the pattern steps. Points are stored by generator, pattern, plugin, and parameter.
- Song automation has a separate ∿ mode in the Playlist footer. Choose the target parameter from the Piano Roll selector, open the Playlist, enable automation mode, and draw across song time. Points are stored per generator and parameter independently of the clip arrangement.
- Pattern mode loads the selected pattern's automation curves. Song mode loads that generator's song-time curves. Switching modes does not replace the saved lanes for the other mode.
- Mixer now shows generator strips; Master shows FX returns, routed bus strips, and the Master strip. Bus strips appear when a generator is routed to them. The Master strip has four insert slots and no sends.
- `ORION_NATIVE_EFFECTS_COMPATIBILITY.md` is the paste-ready observed native-FX inventory and the tracking list for Orion project effect migration. The Dynamics names are marked for sharper-source verification.
- Build 90 / v0.3.69 compiled. Build 89 was reserved and failed during compilation; see `BUILD_HISTORY.md`.
- Follow-up: verify drawing/playback in the running app with more than one generator, non-120 BPM, and bus routes; confirm the parameter curve is audible with a plugin that exposes automatable parameters; refine effect identifiers from Orion project files before writing compatibility shims.

## Build 92 — preserve host automation and include compatibility notes

- Orbit now leaves Tracktion automation curves alone unless the same plugin parameter has an Orbit-owned pattern or song lane. When Orbit owns a lane, it loads only the lane for the active Pattern/Song mode and clears that Orbit-owned parameter curve if the active mode has no corresponding lane.
- Source-only iteration archives now include `ORION_NATIVE_EFFECTS_COMPATIBILITY.md`, so the paste-ready effect inventory ships with incremental source updates.
- Runtime verification of pattern/song automation gestures and playback, mixer bus population, and Master insert behavior is still pending.

## Orion Playlist video roadmap — 2026-10-26 21-37-03.mov

### Behaviors observed

- Song Playlist shows a loop range on the ruler. In the recording Loop is enabled at bar 21 for 12 bars, with the range highlighted while the arrangement spans well beyond it.
- Right-clicking a generator row opens actions including Show Generator, Rename Generator, Delete Generator, Clone Track To, Edit Song Events, and Track Color.
- Clone Track To opens a destination-generator submenu and then Clone Options. The options shown are Patterns and Arrangement and Mixer Settings and Effects.
- Track Color opens a color picker and applies color to that generator's Playlist row and clips.

### Implementation status and priority

- [x] Playlist row context menu: clone a generator's patterns and arrangement to another instrument generator, with a replacement confirmation. This copies the pattern bank, notes, lengths, velocities, pattern automation, song automation, and Playlist clips while remapping pattern IDs for the destination.
- [x] Persistent per-track color selection from a color palette; tint the Playlist row and pattern clips.
- [ ] Match Clone Options with separate Patterns and Arrangement / Mixer Settings and Effects toggles. Mixer settings/effect state copying is not included in the first pass.
- [x] Add generator actions: Show Generator, Rename Generator, Delete Generator, and Edit Song Events; runtime-check their behavior and undo support.
- [x] Persist loop start, length, and enabled state and show the active range on the Playlist ruler. [ ] Verify playback wrap, tempo/meter changes, and save/reopen in the running app.
- [ ] Runtime-check clone overwrites, source/destination identity, arrangement playback after cloning, track color after save/reopen, and undo behavior.

**Reference:** `2026-10-26 21-37-03.mov` in the user's Movies folder. This roadmap records visible Orion behavior and Orbit implementation scope; labels/text shown in the video are not treated as separate instructions.

### Build 95 implementation record

- Build 95 / v0.3.74 compiles successfully. Build 93 was reserved and failed; Build 94 fixed its compiler error and compiled. See `BUILD_HISTORY.md`.
- Playlist Loop, Start, and Length now persist in the Orbit project, and an enabled range is highlighted on the Playlist ruler. Loop timing follows the project's BPM and time signature.
- The Build 94 implementation includes generator-row actions for Show, Rename, Delete, Clone Track To, Edit Song Events, and Track Color. Pattern/arrangement clone and per-track color are implemented; Mixer Settings/Effects cloning remains outstanding.
- Runtime behavior still needs verification: loop playback wrap and save/reopen; all context actions; clone overwrite/undo; track color persistence; automation playback; and bus routing.

### Build 98 implementation record

- Build 98 / v0.3.77 embeds the Playlist in the main workspace below the always-visible transport, makes the Playlist button a show/hide toggle, and keeps Loop toggling independent of Playlist visibility.
- Playlist timeline capacity is 512 bars; minimum zoom is tuned so a wide window can show about 300 bars. Left-dragging the ruler sets and enables the loop span, updates the toolbar range, and preserves the blue ruler highlight.
- The visible duplicate generator resize grip is removed. A compact host keyboard appears below an open instrument editor and auditions the selected track while the transport is stopped.
- These changes compile, but still require manual runtime checks: drag a range such as bars 5–200, confirm the visible range and playback loop, save/reopen, click/hold/drag/release the host keyboard with a VST3 synth, and resize/move the editor without a second-grip artifact.

### Build 100 implementation record

- Build 100 / v0.3.79 restarts Tracktion's playback graph after adding a built-in or VST3 instrument so direct keyboard audition is initialized immediately.
- Playlist cell controls are now limited to the visible viewport. Grid clips and song automation render only visible bars to reduce work while painting and zooming.
- Replaced the lower-right Playlist Unicode glyphs with clear ASCII-labeled Draw, Move, zoom, Settings, and Automation controls.
- The app compiles and is staged at `Builds/Current/Orbit.app`; direct keyboard audition, scrolling at maximum zoom-out, and clip painting still need confirmation in a newly launched Build 100 session.

### Build 94 implementation record (prior)

- Build 94 / v0.3.73 compiles successfully. Build 93 was reserved and failed on a JUCE numeric conversion; see `BUILD_HISTORY.md`.
- Source implementation is in `ProjectDocument` and the Playlist generator-row context menu. Clone copies pattern names/created state, note pitch/step/velocity/length, pattern automation, song automation and playlist arrangement to another instrument generator, remapping each destination pattern ID. It asks before replacing destination patterns and clips.
- Track colors are saved as a track property and used to tint the Playlist row and clips.
- The clone/color UI and copied arrangement have not yet been exercised in the running app; treat those boxes above as implemented in source, pending runtime validation.


## 2026-10-29 — Build 235: visible generator menu and manual section 2

- Replaced the toolbar-only generator gear with a drawn downward triangle at the
  left of the generator header. It remains visible with hidden pattern controls
  and rolled-up generators.
- Added the same menu to the separate Drum Rack header, pinned to that rack and
  its remembered pattern even when another generator is selected.
- Built/installed Build 235 / v0.3.214. Preserved Build 234 in Builds/Previous.
  ASan/UBSan regression checks passed; the native header appearance was inspected
  in an isolated GUI harness. Native popup interaction and audible/hardware tests
  remain unverified. The user's running app/project was not closed.
- Reviewed all 18 rows in Changes since Orion 7. Recording in both modes,
  portable controller maps, pan-law choices, sidechains, tuning and window/zoom
  consistency are recorded ideas in ORION_MANUAL_AUDIT.md, not newly implemented
  capabilities. Next ordered topic: System Requirements. SFS work remains paused.


## 2026-10-29 — implementation and tester distribution

- User authorized starting section 2's missing features. MIDI take recording is
  the first increment; see Source/Recording/README.md for precise scope and limits.
- Event Editor is an automation editor, distinct from Piano Roll. The current
  combined View menu entry is wrong; correction is logged for the View chapter.
- Explicitly requested binary tester builds: native Mac universal package and
  Windows/Linux native-runner workflow. See DISTRIBUTION.md. No paid signing
  identity is available locally; no notarization is claimed. Source-only remains
  the ordinary backup default. SFS work remains paused.
