# Orbit Build History

Every app build reserves the next product build number and advances the product
version before compilation. Failed builds consume their reserved number.
`Tools/build-orbit-dev.sh` also records successful development iterations in
`DEV_BUILD_COUNTER.json`; this counter is separate from product build numbering.
The build script updates CMake metadata, the app's displayed build label, version,
README, and this history row together.
Open `Builds/README.txt` for the short Finder guide. The current app is copied
to `Builds/Current/Orbit.app`; full and incremental source-only ZIPs live in
`Builds/Source Archives/`. Build 24 is the complete source baseline. Later
iteration ZIPs are incremental and name their baseline inside the archive.

| Build | Product version | Date | Contents | Result |
|---:|---|---|---|---|
| 1 | 0.1.0 | 2026-10-25 | Initial JUCE + Tracktion Engine application shell; first user-reported Xcode run. | Succeeded |
| 2 | 0.1.1 | 2026-10-25 | Orbit project-document schema foundation, format notes, and CMake source registration. | Built successfully with CMake/Unix Makefiles. |
| 3 | 0.1.2 | 2026-10-25 | First interactive project lifecycle implementation. | Built successfully; superseded before user testing to correct JUCE bundle build metadata. |
| 4 | 0.1.3 | 2026-10-25 | Same project lifecycle, with JUCE bundle build number and visible app build label aligned. | Built successfully with CMake/Unix Makefiles. |
| 5 | 0.1.4 | 2026-10-25 | First Tracktion-backed instrument workflow: embedded engine edit state, built-in 4OSC instrument, editable 16-step MIDI pattern, and playback controls. | Built successfully with CMake/Unix Makefiles. Manually verified add instrument, create/edit pattern, transport start/stop, save, reopen with edited MIDI and Tracktion session restored, then create a new project. A crash from a pre-rebuild Orbit process was observed during earlier manual work; the freshly launched Build 5 completed the same reopen/new-project path without a crash. Actual sound was not independently confirmed by listening. |
| 6 | 0.1.5 | 2026-10-25 | Export the current pattern as a standard MIDI file with tempo and time-signature metadata. | Built successfully with CMake/Unix Makefiles. Manually exported both the starter pattern and an edited five-note pattern; parsed the result as format-1 Standard MIDI with two tracks, 960 PPQ, 120 BPM, 4/4, and note events at the expected 240/960-tick positions. |
| 7 | 0.1.6 | 2026-10-25 | Audio-device settings and active-output status; transport reports the selected output. | Built successfully. Manually checked Audio Settings, the selected MacBook Air Speakers device at 44.1 kHz with two output channels, playback status routing through that device, and save/reopen of an Orbit project with its Tracktion session and four-step pattern restored. Routing is verified by the app; audible sound was not independently confirmed by a human listener. |
| 8 | 0.1.7 | 2026-10-25 | Multi-pitch 16-step editor with selectable pitches C3–C6; MIDI export includes those pitches. | Built successfully with CMake/Unix Makefiles. Manually added D4 notes alongside the default C4 notes, switched between pitch rows to confirm both stayed intact, saved and reopened the project, and confirmed D4 steps persisted. Exported MIDI and parsed its events: C4 note-ons at ticks 0, 960, 1920, 2880 and D4 note-ons at 240 and 1200, in format 1 with 2 tracks and 960 PPQ. Playback UI reported routing to MacBook Air Speakers. Human listening check remains open. |
| 9 | 0.1.8 | 2026-10-25 | Eight-bar arrangement lane that places/removes the current pattern by bar and loops the arrangement. | Built successfully with CMake/Unix Makefiles. Manually placed the pattern in bars 1, 2, and 4, saved and reopened the project, and confirmed those placements returned. Started/stopped the arranged transport; the app reported routing through MacBook Air Speakers. This is an early fixed eight-bar lane, not a full drag-and-drop playlist. |
| 10 | 0.1.9 | 2026-10-25 | First Tracktion-backed mixer controls: instrument level (-60 to +6 dB) and pan (-1 to +1). | Built successfully with CMake/Unix Makefiles. In a fresh Build 10 session set Instrument 1 to -6.0 dB and pan 0.35, saved and reopened the project, and confirmed both values were restored from Tracktion state. |
| 11 | 0.2.0 | 2026-10-25 | Track output activity meter reads the Tracktion instrument track's audio level while playing. | Built successfully with CMake/Unix Makefiles. Opened the saved Build 10 mixer project in an isolated fresh app copy; confirmed its C4 notes, -6.0 dB level, and 0.35 pan were restored. Started playback and observed `Track output: audio signal detected · peak -12.6 dBFS`; stopped and confirmed the indicator returned to idle. This verifies generated signal in Tracktion's instrument track, but a person still needs to confirm the speakers are audibly producing it. |
| 12 | 0.2.1 | 2026-10-25 | Add/select multiple instrument tracks; the pattern editor and mixer controls follow the selected track. | Built successfully with CMake/Unix Makefiles. Created two instrument tracks, gave each its own pattern, toggled step 2 only on Instrument 2, saved and reopened the project, and switched between tracks to confirm Instrument 1 had step 2 Off while Instrument 2 had it On. This is multi-track groundwork; the Orion-style A–H × 1–8 pattern bank (64 patterns per generator) is the next pattern-system milestone and is not implemented yet. |
| 13 | 0.2.2 | 2026-10-25 | Add an Orbit-owned, schema-v2 A1–H8 pattern bank per instrument, stable slot IDs, independent MIDI notes, bank/slot selectors, and migration of older projects' MIDI into A1. The eight-bar lane previews the selected pattern. | Built successfully with CMake/Unix Makefiles and launched in an isolated app copy; the window displayed Build 13 / v0.2.2. Full manual pattern independence and save/reopen verification is still needed. This build does not yet give Playlist clips independent pattern-slot references. |
| 14 | 0.2.3 | 2026-10-25 | Deliver the A1–H8 pattern-bank milestone with schema-v2 stable slot IDs, independent per-slot MIDI notes, legacy migration into A1, and selectors enabled as soon as an instrument is added to a new project. The eight-bar lane previews the selected pattern. | Built successfully with CMake/Unix Makefiles and launched in an isolated app copy; the window displayed Build 14 / v0.2.3. Manual pattern independence and save/reopen verification remains to be performed. Playlist clips do not yet retain independent pattern-slot references. |
| 15 | 0.2.4 | 2026-10-25 | Reserved for replacing the prototype arrangement lane with a pattern-aware Playlist. | Failed during compilation because Tracktion time-position types were referenced through the engine namespace alias. Number consumed; corrected as Build 16. |
| 16 | 0.2.5 | 2026-10-25 | Replace dropdowns with direct A–H and 1–8 pattern buttons; add a separate resizable Playlist window with generator rows and timeline cells; persist pattern-aware clips by stable pattern ID and regenerate Tracktion MIDI clips for playback. | Built with CMake/Unix Makefiles and launched from an isolated app copy. In the app, created A1 and A2, placed them in separate Playlist bars, played the arrangement and observed Tracktion output signal at -23.6 dBFS, saved the project, reopened it, and confirmed both labeled clips and the A1 MIDI editor state returned. A human listening check remains open. |
| 17 | 0.2.6 | 2026-10-25 | Align the A–H / 1–8 selector as paired stacked columns and add a separate editable piano-roll window launched by a compact button. Remove microphone input selection and microphone usage metadata until audio recording is implemented. | CMake build succeeded and an isolated app launch showed Build 17 / v0.2.6. Full UI verification remains open. |
| 18 | 0.2.7 | 2026-10-25 | Make the pattern bank explicitly belong to the selected generator: hide the editor when no generator is selected, group the paired A–H / 1–8 controls and step editor in a labeled panel, and show the generator and selected pattern in its heading. Keep audio input unavailable while audio recording is not implemented. | Built and opened. Manual UI check found that the Add Instrument path did not refresh the generator editor; corrected in Build 19. |
| 19 | 0.2.8 | 2026-10-25 | Refresh the generator-scoped pattern editor immediately after adding an instrument, so the selected generator owns and displays its A–H / 1–8 controls. | Built and manually checked. The selected Instrument 1 showed A–H with 1–8 directly below; visual review then caught panel framing and a missing piano-roll glyph, corrected in Build 20. |
| 20 | 0.2.9 | 2026-10-25 | Enclose the full generator editor including its step row, use an ASCII piano-roll launcher label for reliable font rendering, and use plain text separators in the generator heading. | Built and manually opened. Added a test project and instrument, confirmed the editor heading names Instrument 1 and that A–H sits directly above 1–8. |
| 21 | 0.3.0 | 2026-10-25 | Begin adding a persistent transport toolbar and Mixer / Master window. | Failed during compilation due to a mixed-type initializer list in mixer controls. Number consumed; corrected in Build 22. |
| 22 | 0.3.1 | 2026-10-25 | Correct the mixer control component list and complete the transport toolbar / Playlist / Mixer / Master wiring. | Built successfully with CMake. Opened Build 22, created a project, and confirmed the Mixer window opens with generator selection, level, pan, and master output controls. The new toolbar controls are exposed in the main window accessibility tree. |
| 23 | 0.3.2 | 2026-10-25 | Replace the selected-generator utility mixer with blue channel strips for each generator and a master strip; connect each strip's gain and pan directly to Tracktion's volume/pan plugin. Keep Playlist and Mixer as separate windows opened from the transport toolbar. | Built successfully; manual UI inspection found a black backdrop behind the strip panels and an irrelevant pan knob on Master, corrected in Build 24. |
| 24 | 0.3.3 | 2026-10-25 | Fix Mixer backdrop transparency and remove the inactive Master pan control while preserving generator gain/pan and master gain. | Built successfully with CMake. Opened an isolated Build 24 copy, created a project and instrument, verified the Playlist and Mixer buttons open their separate windows, and inspected the Mixer: blue strips appear without a black backing, Master has gain only, and generator gain/pan are exposed. Changed the generator controls to -6.0 dB and +0.35 in the disposable test project and observed those slider values update. |
| 25 | 0.3.4 | 2026-10-26 | Generator drag and embedded piano roll resize corrections; Orion menu scaffold and persistent Show Hints preference. | Built successfully with CMake. Direct runtime movement confirmation remains outstanding. |
| 26 | 0.3.5 | 2026-10-26 | Correct product build reservation and history completion in the Orbit iteration build script. | Built successfully. |
| 27 | 0.3.6 | 2026-10-26 | Add Folder and Remove list dialogs for Sample Paths and VST Paths; update Orion video notes and menu status; retain VST hosting as next-pass scope. | Built successfully. |
| 28 | 0.3.7 | 2026-10-26 | Fix generator resize hit-test bounds and document verified move/resize behavior. | Build failed; reserved number consumed. |
| 29 | 0.3.8 | 2026-10-26 | Fix generator resize hit-test bounds and document verified move/resize behavior. | Built successfully. |
| 30 | 0.3.9 | 2026-10-26 | Add compact movable generator collapse and restore while preserving pattern controls and editor state. | Built successfully. |
| 31 | 0.3.10 | 2026-10-26 | Keep Playlist Draw/Move and timeline zoom controls accessible above the scrolling grid. | Built successfully. |
| 32 | 0.3.11 | 2026-10-26 | Integrate generator collapse and restore with the Window menu and Minimize All. | Built successfully. |
| 33 | 0.3.12 | 2026-10-26 | Enable Edit Select All Notes with visible piano-roll selection feedback. | Built successfully. |
| 34 | 0.3.13 | 2026-10-26 | Add first VST3 instrument insert path; review Orion window behavior and preserve multi-folder Sample/VST path settings | Built successfully. |
| 35 | 0.3.14 | 2026-10-26 | Keep Orion-inspired linked movable generator and embedded piano roll; retain multi-folder search-path dialogs; keep VST hosting out of scope | Built successfully. |
| 36 | 0.3.15 | 2026-10-26 | Give the generator a dedicated visible resize handle; preserve its draggable header and linked piano roll | Build failed; reserved number consumed. |
| 37 | 0.3.16 | 2026-10-26 | Diagnostic rebuild for the dedicated generator resize handle | Build failed; reserved number consumed. |
| 38 | 0.3.17 | 2026-10-26 | Fix the dedicated generator resize grip compiler error and keep the linked generator workspace resizable | Built successfully. |
| 39 | 0.3.18 | 2026-10-26 | Add VST3 folder scanning, instrument discovery, and generator insertion | Build failed; reserved number consumed. |
| 40 | 0.3.19 | 2026-10-26 | Add VST3 folder scanning, instrument discovery, and generator insertion | Build failed; reserved number consumed. |
| 41 | 0.3.20 | 2026-10-26 | Add VST3 folder scanning, instrument discovery, and generator insertion | Built successfully. |
| 42 | 0.3.21 | 2026-10-26 | Preserve mixer and meter plugins on VST3 generator tracks | Built successfully. Visual runtime verification of plugin scanning/insertion is pending because macOS Launch Services refused to open the isolated preview bundle. |
| 43 | 0.3.22 | 2026-10-26 | Add automatic standard VST3 discovery, optional startup scan, and Orion-style VST3 Insert menu | Build failed; reserved number consumed. |
| 44 | 0.3.23 | 2026-10-26 | Fix VST3 menu group paths and build the automatic standard VST3 discovery workflow | Built successfully. |
| 45 | 0.3.24 | 2026-10-26 | Move VST3 startup scanning to VST Paths and add scan progress, desktop scan logs, and fatal crash reports | Built successfully. |
| 46 | 0.3.25 | 2026-10-26 | Include previously crash-blacklisted VST3 bundles in scan diagnostics | Built successfully. |
| 47 | 0.3.26 | 2026-10-26 | Make installed VST3 bundles selectable directly and explain when a project is required | Built successfully. |
| 48 | 0.3.27 | 2026-10-26 | Match Orion Insert menu hierarchy; separate built-in Synths from VST Instruments and remove red plugin labels | Built successfully. |
| 49 | 0.3.28 | 2026-10-26 | Start directly in a fresh project; open each inserted VST3 editor and expose the active generator selector | Built successfully. |
| 50 | 0.3.29 | 2026-10-26 | Dock the mixer inside Orbit and remove its header and surrounding padding to match Orion | Build failed; reserved number consumed. |
| 51 | 0.3.30 | 2026-10-26 | Complete the docked mixer view without pop-up chrome, heading, or surrounding padding | Built successfully. |
| 52 | 0.3.31 | 2026-10-26 | Embed the VST3 editor within its generator and allow discarding unsaved projects when starting a new project | Built successfully. |
| 53 | 0.3.32 | 2026-10-26 | Make the in-app mixer compact, movable, resizable, and bounded, with saved layout and scrollable channel strips | Build failed; reserved number consumed. |
| 54 | 0.3.33 | 2026-10-26 | Make the in-app mixer compact, movable, resizable, bounded, and persistent | Built successfully. |
| 55 | 0.3.34 | 2026-10-26 | Make Playlist pattern drawing and selection direct, add eight assignable mixer buses, and add compact, expanded, and minimized in-app Mixer modes | Built successfully. |
| 56 | 0.3.35 | 2026-10-26 | Piano Roll/instrument view switching, generator minimize dock, mixer focus and output bus routing | Build failed; reserved number consumed. |
| 57 | 0.3.36 | 2026-10-26 | Fix compile issues; piano roll view switch, generator minimize dock, mixer focus and output bus routing | Built successfully. |
| 58 | 0.3.37 | 2026-10-26 | Show 48 piano-roll time columns and shorten black keys; align mixer buses, generator focus and collapse, and piano-roll switching | Built successfully. |
| 59 | 0.3.38 | 2026-10-26 | Clear stale project dock and mixer state; make generator minimize cleanly and focus the piano roll over plugin GUIs | Built successfully. |
| 60 | 0.3.39 | 2026-10-26 | Add fitted draggable mixer header and persistent piano-roll note lengths/settings | Build failed at compile: unsupported JUCE AlertWindow toggle API; reserved number consumed. |
| 61 | 0.3.40 | 2026-10-26 | Fix piano-roll settings menu; add fitted mixer header and persistent note-length painting | Compilation reached link; disk ran out of space; reserved number consumed. |
| 62 | 0.3.41 | 2026-10-26 | Complete mixer workspace controls and piano-roll length painting with controller options | Compile/link succeeded; app staging ran out of disk space; reserved number consumed. |
| 63 | 0.3.42 | 2026-10-26 | Finalize mixer auto-sizing/header and piano-roll persistent note-length controls | Built successfully. |
| 64 | 0.3.43 | 2026-10-26 | Remove mixer scrollbar gutter while preserving fitted strips and overflow scrolling | Built successfully. |
| 65 | 0.3.44 | 2026-10-26 | Keep mixer hidden on insert; restore clicked-surface focus, piano-roll settings, and Playlist clip actions | Build failed; reserved number consumed. |
| 66 | 0.3.45 | 2026-10-26 | Keep mixer hidden on insert; restore clicked-surface focus, piano-roll settings, and Playlist clip actions | Built successfully. |
| 67 | 0.3.46 | 2026-10-26 | Fix Piano Roll note hit-testing; support resize and delete across full note body | Built successfully. |
| 68 | 0.3.47 | 2026-10-26 | Put Piano Roll velocity, keyboard colour and note-name controls in a visible icon strip | Built successfully. |
| 69 | 0.3.48 | 2026-10-26 | Add visible lower-right Playlist editing icons and right-click drag erase behavior | Built successfully. |
| 70 | 0.3.49 | 2026-10-26 | Audition selected patterns without Playlist clips; show transport playhead and clean up menu punctuation | Built successfully. |
| 71 | 0.3.50 | 2026-10-26 | Explicit Pattern and Song playback modes with per-generator remembered patterns | Build failed; reserved number consumed. |
| 72 | 0.3.51 | 2026-10-26 | Explicit Pattern and Song modes with persistent per-generator pattern positions | Build failed; reserved number consumed. |
| 73 | 0.3.52 | 2026-10-26 | Explicit Pattern and Song modes with persistent per-generator pattern positions | Build failed; reserved number consumed. |
| 74 | 0.3.53 | 2026-10-26 | Explicit Pattern and Song modes with persistent per-generator pattern positions | Built successfully. |
| 75 | 0.3.54 | 2026-10-26 | Fix stacked Pattern and Song toolbar selector layout | Built successfully. |
| 76 | 0.3.55 | 2026-10-26 | Live piano-key audition through the selected instrument; verify note-on/note-off routing | Built successfully. |
| 77 | 0.3.56 | 2026-10-26 | Mixer strips: remove repeated track headers, enlarge send knobs, and expose all bus routes | Built successfully. |
| 78 | 0.3.57 | 2026-10-26 | Fit full mixer strip and route controls while retaining larger sends | Built successfully. |
| 79 | 0.3.58 | 2026-10-26 | Keep mixer output routing visible by compacting channel EQ rows | Built successfully. |
| 80 | 0.3.59 | 2026-10-26 | Fit full mixer bus routing bank and controls in the visible strip | Built successfully. |
| 81 | 0.3.60 | 2026-10-26 | Show Master and eight bus route controls on every mixer channel | Built successfully. |
| 82 | 0.3.61 | 2026-10-26 | Expose Master and eight bus outputs for all mixer strips | Built successfully. |
| 83 | 0.3.62 | 2026-10-26 | Size mixer strips to the viewport so all bus routing buttons stay visible | Built successfully. |
| 84 | 0.3.63 | 2026-10-26 | Add independently movable and resizable piano-roll window behavior | Built successfully. |
| 85 | 0.3.64 | 2026-10-26 | Remove redundant mixer channel-name headers, enlarge send knobs, and keep routing buttons above the generator dock | Built successfully. |
| 86 | 0.3.65 | 2026-10-26 | Keep maximized piano-roll restore and resize gestures attached to the pointer | Built successfully. |
| 87 | 0.3.66 | 2026-10-26 | Fix first VST track mapping and add Audio MIDI diagnostics | Built successfully. |
| 88 | 0.3.67 | 2026-10-26 | Keep piano-roll note create and resize drags responsive; rebuild MIDI clip after gesture release | Built successfully. |
| 89 | 0.3.68 | 2026-10-26 | Pattern and Playlist automation, separated Master bus section, Orion FX compatibility inventory | Build failed; reserved number consumed. |
| 90 | 0.3.69 | 2026-10-26 | Pattern and Playlist automation, separated Master bus section, Orion FX compatibility inventory | Built successfully. |
| 91 | 0.3.70 | 2026-10-26 | Align Playlist and song automation timing to the active tempo | Built successfully. |
| 92 | 0.3.71 | 2026-10-26 | Preserve host automation curves and package Orion effect compatibility inventory | Built successfully. |
| 93 | 0.3.72 | 2026-10-26 | Add Playlist generator clone and persistent track colors; document loop and context-menu roadmap | Build failed; reserved number consumed. |
| 94 | 0.3.73 | 2026-10-26 | Fix track color persistence and build Playlist clone/color workflow | Built successfully. |
| 95 | 0.3.74 | 2026-10-26 | Orion Playlist loop settings and visible loop range; generator clone and color workflow | Built successfully. |
| 96 | 0.3.75 | 2026-10-26 | Embed Playlist under fixed transport; extend timeline and loop drag range; add instrument audition keyboard and remove duplicate resize grip | Build failed; reserved number consumed. |
| 97 | 0.3.76 | 2026-10-26 | Repair embedded Playlist and loop controls; add audition keyboard after failed Build 96 attempt | Build failed; reserved number consumed. |
| 98 | 0.3.77 | 2026-10-26 | Correct Playlist ruler event handlers; rebuild embedded Playlist, loop-range zoom and instrument audition keyboard | Built successfully. |
| 99 | 0.3.78 | 2026-10-26 | Finish 300-bar Playlist zoom, embedded timeline and drag-to-loop workflow; add audition keyboard and hide duplicate editor resize grip | Built successfully. |
| 100 | 0.3.79 | 2026-10-26 | Fix live instrument keyboard audition, virtualize Playlist rendering, replace garbled controls | Built successfully. |
| 101 | 0.3.80 | 2026-10-27 | Playlist foreground, readable track labels, and complete timeline tool strip | Built successfully. |
| 102 | 0.3.81 | 2026-10-27 | Movable resizable Playlist panel and Mixer maximize controls | Built successfully. |
| 103 | 0.3.82 | 2026-10-27 | Fix mixer strip sizing and controls; keep VST instruments out of inserts; improve Playlist resizing and generator activation. | Build failed; reserved number consumed. |
| 104 | 0.3.83 | 2026-10-27 | Fix mixer strip sizing and controls; keep VST instruments out of inserts; improve Playlist resizing and generator activation. | Built successfully. |
| 105 | 0.3.84 | 2026-10-27 | Playlist Orion-style controls, four-corner resizing, uniform background, and visible MIDI clip previews | Build failed; reserved number consumed. |
| 106 | 0.3.85 | 2026-10-27 | Playlist Orion-style controls, four-corner resizing, uniform background, and visible MIDI clip previews | Built successfully. |
| 107 | 0.3.86 | 2026-10-27 | Unify Playlist canvas background with its dark timeline theme | Built successfully. |
| 108 | 0.3.87 | 2026-10-27 | Mixer width follows visible strips; enlarge EQ controls and compact fader section | Build failed; reserved number consumed. |
| 109 | 0.3.88 | 2026-10-27 | Mixer width follows visible strips; enlarge EQ controls and compact fader section | Built successfully. |
| 110 | 0.3.89 | 2026-10-27 | Playlist lower-right zoom controls; title-bar minimize/maximize; direct settings toggles | Built successfully. |
| 111 | 0.3.90 | 2026-10-27 | Compact mixer EQ labels, band panels and rotary controls | Build failed; reserved number consumed. |
| 112 | 0.3.91 | 2026-10-27 | Compact mixer EQ labels, band panels and rotary controls | Built successfully. |
| 113 | 0.3.92 | 2026-10-27 | Keep Playlist controls accessible; draw explicit window icons; optimize clip editing and velocity lane reveal | Built successfully. |
| 114 | 0.3.93 | 2026-10-27 | Mixer layout refinements, panel controls and resize behavior, paste shortcut, and workspace state persistence | Built successfully. |
| 115 | 0.3.94 | 2026-10-27 | Polished mixer fader artwork on top of Build 114 panel and workspace fixes | Build failed; reserved number consumed. |
| 116 | 0.3.95 | 2026-10-27 | Finish mixer fader artwork and resolve slider pointer typing after Build 115 compile failure | Built successfully. |
| 117 | 0.3.96 | 2026-10-27 | Start with blank workspace; allow Open without a false save requirement | Built successfully. |
| 118 | 0.3.97 | 2026-10-27 | Hide Playlist on startup; smooth mixer dragging and reclaim send-heading space for EQ graph | Built successfully. |
| 119 | 0.3.98 | 2026-10-27 | Unify generator panel colors; piano roll resizing, note length, and velocity lane fixes | Built successfully. |
| 120 | 0.3.99 | 2026-10-27 | Compact centered pattern selectors and prevent generator drag repaint artifacts | Built successfully. |
| 121 | 0.3.100 | 2026-10-27 | Piano roll lane visibility, velocity edit mode, and automation target controls | Built successfully. |
| 122 | 0.3.101 | 2026-10-27 | Piano roll lane toggles, empty Playlist grid, contrast, and light theme | Built successfully. |
| 123 | 0.3.102 | 2026-10-27 | Match Orion transport controls: BPM stepper, bar display, loop range steppers, and master volume knob | Built successfully. |
| 124 | 0.3.103 | 2026-10-27 | Add piano-roll zoom and settings, resizable automation lane, gradient automation drawing tools, and remove confusing note-step selector | Built successfully. |
| 125 | 0.3.104 | 2026-10-27 | Polish piano-roll settings gear icon | Built successfully. |
| 126 | 0.3.105 | 2026-10-27 | Add audio tracks and relocate master volume control | Built successfully. |
| 127 | 0.3.106 | 2026-10-27 | Reposition piano roll zoom controls | Built successfully. |
| 128 | 0.3.107 | 2026-10-27 | Repaint full generator bounds when switching instrument editor and piano roll | Built successfully. |
| 129 | 0.3.108 | 2026-10-27 | Add draggable velocity and automation lane height divider | Built successfully. |
| 130 | 0.3.109 | 2026-10-27 | Audio Playlist click-to-import, continuous clip spans, context actions, and snapped clip moves | Built successfully. |
| 131 | 0.3.110 | 2026-10-27 | Audio Playlist click-to-import, continuous clip spans, context actions, and snapped clip moves | Built successfully. |
