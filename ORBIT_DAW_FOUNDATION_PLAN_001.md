# Orbit DAW — Foundation Plan 001

**Prepared:** 2026-10-25
**Status:** Foundation started; open-source direction and initial engine stack selected.
**Working names:** Orbit is the folder name and working product name; Orion is Synapse Audio's reference DAW. The public product name still needs a deliberate decision.

**Current build:** Build 24, product version 0.3.3. Builds 15 and 21 were reserved and failed at compilation. See `BUILD_HISTORY.md`.

## 1. Product intent

Build a new, native desktop DAW inspired by the musical workflow of Synapse Audio Orion 8.5: sound generators create patterns; patterns are arranged into a song; tracks feed a practical mixer; effects, automation, and audio recording/playback complete the production workflow. The goal is to preserve the fast, pattern-first way of making music that the user values, while writing original code and making a modern, maintainable product.

The Orion 8.5 press kit describes a pattern-based sequencer, high-precision piano roll, song playlist, multitrack audio playback/recording, and automatically managed but flexible mixer routing. These are useful behavior-level references for a feature map, not a mandate to reproduce every feature or visual detail. [Synapse Audio: Orion 8.5](https://www.synapse-audio.com/news-orion-studio-85-isoutnow.html), [Orion 8 press kit](https://www.synapse-audio.com/downloads/presskit/Orion8.pdf)

## 2. Key decision: standalone DAW, not a plug-in

This is a desktop application that owns its audio devices, transport, project, timeline, plugin hosting, and mixer. It is not something that loads inside Studio One. It can still host instruments and effects as plug-ins, and Orbit's own instruments/effects can later be packaged separately as plug-ins if that becomes useful.

JUCE supports native applications as well as audio plug-ins. For a standalone application the app must initialize and manage audio/MIDI devices; JUCE's `AudioDeviceManager` and `AudioProcessorPlayer` are examples of the responsibilities involved. [JUCE AudioAppComponent](https://docs.juce.com/master/classjuce_1_1AudioAppComponent.html), [JUCE standalone AudioProcessorGraph tutorial](https://juce.com/tutorials/tutorial_audio_processor_graph/)

## 3. Technical direction selected for the first implementation

### Selected starting point — Tracktion Engine + JUCE shell

Best fit for a small team that wants to build a complete DAW rather than spend years first implementing a sequencer engine. Tracktion Engine is a C++20 JUCE module explicitly intended for sequence-based audio applications, including full DAWs. The repository includes CMake examples and supports macOS, Windows, and Linux. The engine is not the same product as Tracktion's Waveform DAW; it is a framework for our application. [Tracktion Engine repository](https://github.com/Tracktion/tracktion_engine)

**Decision (2026-10-25):** Orbit's application code will be open source under GNU AGPLv3 from the beginning. The project will use JUCE under AGPLv3 and Tracktion Engine under GPLv3, preserving each upstream repository's license and notices. This is an intentional copyleft product direction, not a plan to wait for revenue thresholds and change licenses later. Review the current upstream terms and dependency notices again before publishing releases; this note is a project decision, not legal advice. [Tracktion Engine license agreement](https://engine.tracktion.com/agreement), [JUCE licensing](https://juce.com/get-juce/), [GNU license compatibility FAQ](https://www.gnu.org/licenses/gpl-faq.html)

### Candidate B — JUCE application plus our own DAW engine

Gives maximum control over the project model and behavior, but means we own and must build/test transport, timing, MIDI/audio sequencing, routing, plugin hosting, latency compensation, rendering, recovery, and many years of edge cases. Choose this only if Tracktion Engine's licensing or architecture does not fit, or if a narrower custom engine is a deliberate product advantage.

### Candidate C — another permissively licensed engine

Could reduce commercial licensing constraints, but still needs a careful feature, platform, maintenance, plugin-format, and license audit. Don't choose based only on license headline; a DAW engine is a long-term dependency.

**Implementation path:** Tracktion Engine v3.2.0 with its pinned JUCE submodule is checked out in `Dependencies/tracktion_engine/`. The first CMake application target is in place. Keep Orbit's own project model and UI-facing interfaces separated from Tracktion APIs. The app currently creates multiple built-in 4OSC instrument tracks, edits 16-step MIDI patterns across pitches C3–C6, exposes direct A–H bank and 1–8 slot buttons, and stores 64 independent stable-ID patterns per generator. Build 16 replaces the prototype arrangement strip with a separate resizable Playlist window showing numbered generator rows and bar cells labeled by pattern name. Each clip stores a pattern ID and bar position in the Orbit project, while Tracktion MIDI clips are regenerated for playback. The first create → arrange → playback → save/reopen flow was manually verified with A1 and A2 on separate bars; Tracktion reported audio signal during playback. The Playlist editing model is currently click-to-place/remove on a fixed one-bar grid. A full mixer, third-party plug-in insertion, and other Orion workflow details remain ahead.

**Integration terminology:** Orbit will link to and call Tracktion Engine as a C++ framework/library. We are not copying or transplanting its source into Orbit, and it is not a ready-made DAW interface. Its engine provides substantial playback, MIDI, plug-in, automation, routing, rendering, and editing capabilities; Orbit still needs its own user interface, product workflow, project/document layer, and integration code. JUCE supplies the application framework and audio/plugin building blocks used alongside the engine.

## 4. Build system and project files

Use **CMake as the source of truth** and generate an Xcode project/workspace for macOS development. Xcode is the IDE/debugger and Apple signing/distribution tool; it does not need to be the only build-system definition. Tracktion Engine itself documents CMake examples and a CMake build path. A checked-in CMake configuration makes command-line builds and future CI/agent builds repeatable, while the generated Xcode project can be recreated at any time.

Use a pinned JUCE/Tracktion Engine revision (submodules or another explicit versioned dependency mechanism), record exact versions and licenses, and upgrade intentionally. Avoid copying vendor code into Orbit source without provenance. Don't start with Projucer unless a specific JUCE/Tracktion example requires it; it is not necessary to hand-create an Xcode project.

The initial CMake/JUCE app shell has been created in the Orbit DAW folder. CMake is the versioned build definition; it generates an Xcode project, and Xcode remains the day-to-day IDE/debugger. The vendor engine checkout and Orbit app sources have distinct directories.

## 5. Groundwork that prevents expensive rewrites

1. **Project format:** versioned, documented project schema; stable UUIDs for tracks, generators, patterns, clips, mixer channels, and automation; migrations between versions; readable diagnostic/export form. Never use array position as persistent identity.
2. **Time model:** store musical time in integer ticks with an explicit PPQ and tempo/time-signature map; keep audio time/sample positions distinct and provide conversion functions. Avoid float beat positions as the canonical saved time.
3. **Media references:** keep samples/audio external to the project by default with explicit relinking; offer collect/copy-into-project later. Store relative paths when safely possible; never silently rewrite originals.
4. **Realtime boundary:** audio callbacks must not allocate, lock, access disk, wait on UI, or call an agent/network. Route all edits through commands/messages into the audio engine; send meters/playhead data back through bounded lock-free or otherwise realtime-safe channels.
5. **Undo and transactions:** user actions become named commands with undo/redo, stable IDs, and atomic multi-object changes. This also becomes the safe substrate for automation and agents.
6. **Crash recovery:** autosave/recovery journal separate from the main project file, periodic recovery checkpoints, clear recovery prompts, and fault-isolated plug-in scanning/loading where feasible.
7. **Plug-in resilience:** scan asynchronously; persist plug-in identity and parameter state; handle missing/broken plug-ins gracefully; never block the audio/UI thread on scan or load. Treat third-party plug-in execution as an isolation boundary.
8. **Versioning and diagnostics:** semantic product version plus a unique build identifier, build configuration, dependency lock/revisions, crash logs with user consent, and exportable project diagnostics that omit secrets and personal file paths unless selected.
9. **Testable seams:** keep the project model, command system, engine adapter, UI, storage, and agent interface as separate modules. Add deterministic tests for serialization/migration, tempo conversion, undo/redo, and command validation as those pieces are implemented.
10. **Scope discipline:** first prove one dependable path: create/open project → add one generator → create/edit pattern → arrange/loop → hear it → save/reopen. Delay broad instrument inventory, collaboration, cloud accounts, and agent autonomy until this path is solid.

### Pattern-bank model to preserve Orion's workflow

The user-provided Orion 8 screenshots show a generator-focused pattern editor: select one generator, choose a bank letter A–H and a pattern number 1–8, edit that pattern in the piano roll/step editor, then place named pattern blocks such as A1 or F2 on that generator's Playlist lane. That is **64 pattern slots per generator**, not 64 mixer channels. The Orion 8 press kit confirms the larger model: pattern-based sequencing with step sequencing and a high-precision piano roll, then a Playlist that arranges patterns into songs. A historical Orion Pro review independently describes eight banks of eight patterns per generator and adjustable pattern steps. Orbit's Build 18 groups the selector with the active generator and hides it when no generator is selected; the next structural step is to make generator editors open from distinct generator rows/windows and let the Playlist select which generator's pattern is being arranged.

Orbit represents the pattern library as 64 stable-ID slots owned by each generator track. Each slot has a bank letter, slot number, independent MIDI notes, and a length field. The editor exposes direct bank and slot buttons. Playlist clips reference the generator and pattern IDs while storing arrangement position and duration; they do not duplicate MIDI data. Playlist blocks show compact pattern labels. The current first pass supports one-bar clips placed by clicking cells; the next milestone is more Orion-like Playlist editing, including drag placement, clip resizing/duplication, clearer transport position, and useful horizontal navigation.

Reference screenshots supplied by the user are saved in the project root: `synapse-audio-orion-8-116025 (1).jpeg` (Playlist, generator sequencer, and mixer) and `qjakH9o-nB-zn-IGrhd2gQggUDTRoF89uWNKXkj-2gUEvAkj2yS6HsrM02-cGJjJDkFrrylhkvxF3Rl7-teiR5gB78yD_OcYR2_BT5rsdy5rh4i9VsY2ActG1CZ2CwL_WS8eLkEk8aqBDNVin5j0KlxBJxIapX09BSH3f5_CyXZJTvBMN4psoPX66iyP1mSw (1).jpeg` (pattern editor bank/slot buttons over the Playlist).

## 6. Agent readiness: design the seam now, build connectivity later

Do **not** make the first milestone “connect an LLM.” Build the same structured command layer that makes a good agent API possible, then expose a small interface when the DAW's project model is stable.

Suggested future stack:

- **Core:** a versioned command/query API over documented project concepts (inspect project, list tracks/patterns, create pattern, add notes, arrange clip, set tempo, render preview). Each request has a schema, validation, stable object IDs, and a human-readable result.
- **Local first adapter:** a CLI for scripts and agents; then an optional local MCP server/adapter for Codex and other MCP clients. Consider a localhost HTTP/JSON-RPC adapter only when cross-process/client needs justify it. Keep the core independent of any one agent vendor or protocol.
- **Safety:** read-only by default; explicit opt-in for writes; preview/dry-run before mutation; per-operation permission scopes; undoable transactions; clear provenance (“created by agent X”); no direct SQL/project-file mutation; no network or prompt execution on the realtime thread.
- **User control:** agent proposals appear as reviewable changes. Require user approval before destructive actions, overwrites, publishing, purchases, or accessing external services. Keep credentials and private audio out of model context unless the user explicitly chooses to share them.
- **Extensibility:** publish schemas and example clients; protocol version negotiation; capability discovery; rate limits and cancellation; API compatibility policy. Agents should work through the same commands available to the UI, not hidden backdoors.

This groundwork creates option value without making an agent integration the first engineering risk.

## 7. First milestones

### M0 — Project and engine foundation (in progress)

- Done: CMake app shell builds against pinned dependencies and generates the Xcode project.
- Implemented in source: `Source/Project/ProjectDocument.*` defines the Orbit-owned project document schema. Version 2 records a project UUID, integer musical time at 960 ticks per quarter note, tempo and meter maps, stable track and mixer-channel IDs, 64 stable pattern IDs and independent MIDI note lists per instrument, plus distinct clip/plugin/automation/media containers. Version 1 documents receive empty pattern-bank slots when loaded.
- First engine slice implemented: create a Tracktion Edit, add a built-in 4OSC instrument, create/edit a 16-step MIDI pattern, run/stop transport, and save/reopen the Tracktion Edit state in `.orbit`. Fresh Build 5 was manually exercised through project replacement after reopening without a crash. Build 6 also exports the pattern as a Standard MIDI file with tempo and meter metadata.
- Completed in Build 7: expose device selection/settings and show the active output and sample rate. Transport reports the selected device. A person still needs to confirm that the notes are audible.
- Completed in Build 8: edit note pitches over a three-octave C3–C6 range by selecting a pitch row and toggling its 16 steps; save/reopen and Standard MIDI export preserve multiple pitches.
- Completed in Build 9: place the same pattern in/out of eight song bars; the Tracktion arrangement survives project reopen and transport loops across the lane.
- Completed in Build 10: adjust an instrument channel's level and pan through Tracktion's Volume and Pan plugin; save/reopen restores both values.
- Completed in Build 11: show the Tracktion instrument-track output level during playback; the reopened Build 10 session reported -12.6 dBFS while playing. This confirms signal at the Tracktion track meter, while physical audible output still needs a human check.
- Completed in Build 12: create and select multiple instrument tracks, each with independent pattern and mixer controls; verified distinct C4 step states survive save/reopen.
- Build 16 implementation: direct A–H × 1–8 selectors; separate resizable Playlist window; numbered generator rows; stable-ID pattern clips in a bar grid; Tracktion playback regenerated from Orbit's project model. Manual A1/A2 arrangement, playback signal, save and reopen have been verified. Next make the Playlist editing and navigation closer to Orion; then expand mixer/master metering and add plug-in scanning, insertion, and missing-plug-in recovery. Continue validating schema migration and crash recovery.
- Build 17–20: separate editable piano roll; pair A–H above 1–8; make the controls generator-scoped and hide them unless an instrument is selected. Build 21 adds tempo and transport controls plus working Playlist/Mixer/Master buttons. The mixer first pass exposes Tracktion-backed channel level/pan and master level; the next mixer work is channel strips, meters, inserts, and routing. Continue toward Orion's generator windows and Playlist track rows, then add File/Insert menus, plug-in scanning/insertion, and fuller arrangement editing.
- Record what the spike proves and what remains unknown before promoting the design into a broader product workflow.

### M1 — DAW application workflow

- Create/open/save projects through Orbit's document layer; add tracks and route commands through undoable model operations.
- Implement audio device settings, transport controls, a visible arrangement, and a clear mapping from Orbit concepts to Tracktion Engine objects.
- Use the engine's capabilities where they fit; keep product-facing project data and behavior behind Orbit-owned interfaces.

### M2 — First musical vertical slice (in progress)

- Completed in Build 5: create a one-bar C4 step pattern, run/stop transport, and save/reopen the Tracktion session.
- Completed in Build 8: edit multiple pitch rows from C3 to C6, export those notes as standard MIDI, and save/reopen the same pitch data.
- The basic multi-generator → per-generator pattern → eight-bar arrangement → channel level/pan path now works. Next, implement each generator's 64 named pattern slots, then a real Playlist that arranges those slots and a multi-channel mixer with a master bus, meters, mute/solo, and clear routing.

### M3 — Plug-ins and production workflow

- Host a third-party instrument and effect, persist plug-in identity/state, and handle missing or failing plug-ins.
- Add undo/redo, audio clips and recording, routing, automation, audio render/export, autosave, recovery, and diagnostics in tested increments.

### M4 — Reliability and extensibility

- Validate timing, save migration, device recovery, and responsiveness under load; widen platform support deliberately.
- Publish a stable command/query API and CLI; then add an optional agent adapter with read-only inspection and user-approved, undoable edits.

### UI references to retain

- The user supplied a compact transport-bar screenshot on 2026-10-25. Keep it as a reference for a future transport concept: transport and record controls, time displays, sync/metronome/timing, key/transposition, tempo, meters, and Edit/Mix/Browse navigation.
- The user supplied two Orion screenshots showing the generator pattern editor (A–H banks and 1–8 pattern slots), named pattern blocks in Playlist lanes, and the channel mixer. Use these as behavior and layout references while keeping Orbit's implementation and visual identity original.
- These images are references only; prioritize the data model and working pattern/playlist behavior before GUI polish.

## 8. Initial YouTube video script (draft, 3–5 minutes)

**Working title:** *Building a Modern DAW Around a Workflow Worth Preserving*

**[Opening — camera / screen]**
“I’m starting a new project: building a DAW called Orbit. I’ve spent a lot of time with Synapse Audio Orion, and what stayed with me wasn’t just a list of features. It was the way the workflow connected instruments, patterns, the playlist, and the mixer. It made a certain kind of music-making feel direct.”

**[Show a few Orion references you have permission to show, or use a hand-drawn diagram]**
“Orion 8.5 is my reference point for that pattern-first workflow. Synapse Audio’s developer has been okay with us learning from the ideas, while asking us to write our own code. That’s the line I want this project to follow: preserve the musical ideas and the feeling of the workflow, while building the software itself from scratch and giving it its own identity.”

**[Show the first Orbit app window / planning notes]**
“There’s a basic app shell now, but the DAW features haven’t been built yet. I’m starting with the groundwork because audio software gets complicated quickly. This needs to be a real desktop application that talks to audio devices, keeps projects safe, handles timing and plug-ins, and eventually works across more than one platform. It’s not a plug-in that I load into Studio One. It’s a DAW in its own right.”

**[Show a simple diagram: generator → pattern → playlist → mixer → output]**
“The first goal is deliberately small: create a project, make a pattern, place it in an arrangement, hear it, save it, and open it again. If that foundation is dependable, we can grow it into the richer workflow I’m aiming for.”

**[Show notes about CMake/JUCE/Tracktion Engine]**
“The first foundation is C++ with JUCE and Tracktion Engine. JUCE provides application and audio building blocks; Tracktion Engine provides DAW engine capabilities; and CMake generates the Xcode project I use to build and work on it. I’ve chosen an open-source AGPLv3 path for Orbit from the start, while keeping each dependency’s own license and notices. The revenue limits people sometimes mention are attached to separate commercial tiers, not a substitute for following the open-source licenses.”

**[Show a note: commands → review → undo; no agent code yet]**
“I’m also planning for a future where software agents can help with repetitive music-production tasks. I don’t want an AI bolted onto the audio thread or given unchecked control of a project. I want the DAW’s actions to be clear, reviewable commands that the user can undo. That can help a human workflow today and give agents a safe way to work with the project later.”

**[Closing — camera]**
“This is the beginning, not a release announcement. I’ll share the decisions, prototypes, mistakes, and progress as I go. The next goal is a project that can be created, saved, and reopened safely; after that, I want to build the first generator-to-pattern-to-arrangement-to-mixer musical slice. If you used Orion or have thoughts about pattern-based workflows, tell me what made it work for you—and what you’d want a modern version to do differently. Thanks for following along.”

### Recording notes

- Keep this as a planning/prototype video; don't imply a working build or release date.
- Replace “the developer has been okay…” with wording you can substantiate publicly; avoid implying a formal license, endorsement, or partnership unless there is one.
- Use screenshots, logos, or product footage only where you have permission or a clear basis to use them. A simple original workflow diagram is enough for the first video.
- Add a brief on-screen label “Early planning — no release date” if the video includes roadmap material.
- Ask one concrete question at the end and invite viewers to follow the development thread.

## 9. Decisions to record before the first public product build

- Product name: Orbit, Orbital, or another cleared name.
- First release platform: macOS-only first release vs multi-platform from day one.
- Engine path and license model: initial stack selected as Tracktion Engine + JUCE, under their applicable upstream GPLv3/AGPLv3 terms.
- Ownership and source policy: open-source from the beginning under AGPLv3 for Orbit code; preserve dependency licenses and notices.
- Plugin support for the first vertical slice: hosted third-party plug-in vs a built-in/test instrument.
- Minimum first audience promise and update cadence; keep the public thread accurate about planning vs playable software.
