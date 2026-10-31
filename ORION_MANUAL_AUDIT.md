# Orion Help reference review — 2026-10-29

Use the installed Orion Help as a behavior reference for future Orbit work, in
addition to the user's recordings. The running Orion 64 app is accessible through
the connected native-app tools. Its installed main manual is compiled HTML at
`~/Library/Application Support/CrossOver/Bottles/orion/drive_c/Program Files (x86)/Orion/Help/Orion Help.chm`.
This review accessed Help directly. No credentials or additional permissions were
needed; the manual and Orion's reference assets are not part of source archives.

## User correction — Event Editor / View menu (2026-10-29)

Event Editor is Orion's automation editor; Piano Roll is the note editor. They
must not be combined or substituted for one another. Orbit currently labels
View item 304 "Event Editor / Piano Roll" and calls showPianoRoll(), which is
incorrect. The user explicitly requested logging this for the View/Automation
manual pass, so no replacement UI is claimed here. The reference View menu has
Playlist, Event Editor, Master Section, Mixing Console, Tempo and Time, with no
Piano Roll entry. Review MenuView.htm and EventEditor.htm before changing it.
The existing Piano Roll automation lane and Playlist song lane are Orbit's
current interfaces, not evidence that Orion's Event Editor has been reproduced.

## Section-by-section review

**Current implementation checkpoint — 2026-10-29:** the user subsequently
authorized approximately two hours of autonomous implementation, with no blocking
questions while asleep. Builds 255–259 completed Automation, Mixer, Master,
Sidechains and the Effects guide increments documented in sections 6–10 below.
Build 260 addresses independent built-in generator controls. The older workflow
and coverage tables below retain their original review context; later numbered
implementation records supersede them where stated. Detailed proprietary DSP,
tempo automation and hardware-dependent checks remain open. The Dynamics
increment includes the Brickwall Limiter in Build 261. Build 263 adds an original Peak Compressor. The later user-directed pass prioritizes Tube/Two-Band Distortion after window and automation usability.
Do not claim that the whole manual or every row in a reviewed chapter is done.

**Workflow correction from the user (2026-10-29):** stop reading ahead to new
chapters. Return to Patterns and implement/test its identified requirements
before progressing to Playlist/Recording, then Automation. A deferral requires
the user's agreement. Sections 5–6 below are already-read reference material,
not permission to advance to Mixer. The next unreviewed chapter is Mixer, but
the active implementation section is Playlist/Recording following the Patterns
implementation and Builds 243–248 usability fixes. User-reported usability fixes
(including Build 241's Generator-column grip) take priority as they arrive.

Review one topic at a time in the manual's contents order. Record what the manual
describes, compare the current source, and discuss the useful gaps before choosing
implementation work. A recommendation below is not an implemented feature.

### 1. Welcome to Orion > Installation and First Steps

**Reviewed 2026-10-29 against Orbit Build 234 / v0.3.213.** Read the complete
`FirstUse.htm` topic from the installed manual, extracted into the temporary
directory `/tmp/orbit-orion-manual-review`. The contents hierarchy comes from
`Help.hhc`. Also inspected the topic's Audio Output Settings, MIDI In Settings and
Preferences screenshots, including options absent from the surrounding prose.
Extraction makes the reference readable without relying on CrossOver's menu
navigation. The manual is reference material only; it is not copied into
Orbit's source or deliverables.

This topic covers installation/content, audio hardware, MIDI hardware, plugin
paths and preferences. The comparison below is a source inspection, not a new
physical-device or native-dialog test.

| Manual behavior | Current Orbit coverage and gap |
| --- | --- |
| Install the application and optional sample libraries with consistent content paths | Sample Paths can be edited and used as the starting location for audio import and Drum Rack sample selection. A sample-content installer, library discovery and missing-content recovery are not established by this implementation. Orion's sample libraries are not Orbit assets. |
| Select audio driver/device, sample rate and latency | Audio Output Settings opens the real JUCE device/channel/rate/buffer selector. Use the platform's available drivers; the manual's old Windows driver advice does not define macOS requirements. Hardware performance still needs desktop validation. |
| Keep audio processing available while transport is stopped | Live MIDI allocates a playback context and injects notes into the engine. There is no explicit Always Running preference. Confirm stopped-transport audition in the desktop app before deciding whether another switch is useful. |
| Protect against CPU overload | A saved threshold calls the engine's CPU-before-muting API. This is overload muting, not a verified reproduction of Orion stopping its transport. |
| Enable multiple processor cores | A project multicore policy exists and updates the engine CPU policy/restarts playback when changed. No application restart is required by the current handler. |
| Open the audio driver's own control panel | Delegated to the device selector and platform/driver support; there is no separate Orion-style ASIO Panel command. |
| Route buses to separate hardware outputs | Orbit has internal output buses and selectable device channels. The current mixer routing command chooses Master or an internal bus, not an independent hardware output pair for each bus. Hardware multi-output bus assignment remains a gap. |
| Mix in 64-bit double precision | No selectable double-precision mixing implementation. Enabling more output channels or CPU cores does not add this. |
| Choose multiple MIDI input ports | Multiple input-device checkboxes and saved device IDs exist. |
| Easy mode follows the active instrument; Advanced mode uses configured routing | Both exist: Easy targets the selected generator; Advanced matches generator MIDI channels/Omni. This is the present routing scope, not an arbitrary per-port routing matrix. |
| Filter received MIDI messages | Pitch bend, pressure, program change and transport reception are configurable. Start/Continue/Stop/Song Position have handlers. Orbit ignores incoming clock; the manual's Start/Stop/SPP sync label alone does not establish a requirement for external tempo following. |
| Configure sustain-pedal polarity | Off, normal and inverted policies exist for the supported pedal CCs. Audible pedal behavior also depends on the instrument and still needs hardware validation. |
| Choose a separate mixer-controller input port | Controller bindings can filter by device/channel among enabled MIDI inputs. There is no independent mixer-control port selector that listens to an otherwise disabled input. |
| Enable external MIDI output | Device selection, enable/disable, clock/transport output and generator routing exist. There is no dedicated insertable MIDI Out generator yet; routing/thru and hardware timing need the validation already noted in Options documentation. |
| Configure recursive plugin search paths | Standard and additional VST3 locations feed a recursive scan. Manual scans can run without restarting Orbit; startup scanning is optional. Older VST/DX formats from the manual are not covered. |
| Set visual update frequency | The saved frame-rate preference controls ProjectView's timer. It is not a unified update policy for every editor, plugin and meter. |
| Set VU decay, including an unsmoothed display | The preference affects the selected-track diagnostic peak-text calculation and is clamped to 1–100 percent. It does not set a common decay time for all visible mixer/master meters or provide the manual's zero-decay behavior. |
| Select host preset files or the instrument's internal bank | No host preset-system preference. The generator preset selector is still Default-only and open/save are placeholders. Hosted plugins may provide their own preset controls. |
| Always store complete plugin banks in songs (Preferences screenshot) | No selectable bank-versus-program storage policy. Native projects save Tracktion plugin state; the exact state supplied by each plugin adapter needs a dedicated preset/state review before claiming full-bank compatibility. |
| Load a default song at startup | Missing. The field currently named Default project file is used for Open/Save suggestions, and recent-project updates replace it with a folder. Startup creates a fresh project. A separate startup-template preference is needed. |
| Automatically open the last song (Preferences screenshot) | Missing. Recent-project history exists, but startup does not automatically load its first entry. This should be an explicit startup-mode choice alongside a blank project and a chosen template. |
| Configure an external sound editor for audio clips | No external-editor preference or matching audio-clip launch workflow. |
| Keep numbered backups of overwritten projects | No versioned project-backup workflow. Existing source ZIPs and previous app builds are development backups, not user-project recovery. |
| Periodically write a separate recovery file | Partial and materially different: the timer calls normal save on the current project, requires a named dirty project, and clears its dirty state. It overwrites the main file rather than writing an independent recovery copy; untitled work is not covered. The normal temporary-file replacement protects a save operation, but is not version history or crash-recovery discovery. |
| Choose whether closing a generator hides or deletes it | View hiding and explicit deletion already exist; there is no preference for the manual's alternative destructive-close workflow. Review individual effect windows in their own section before changing this policy. |
| Choose zoom around the play cursor or viewport center | Playlist/Piano Roll zoom controls exist, but no shared anchor preference or matching recentering behavior is implemented. |

#### Recommended work from this section

1. **Project recovery first:** keep scheduled recovery separate from explicit Save;
   preserve a short version history on overwrite; cover untitled projects; offer
   recovery after a restart. Preserve file association, project name and dirty
   state when taking a recovery snapshot.
2. **A real startup template:** separate the startup song/template from the last
   Open/Save directory. Open the template as a fresh unsaved project so its source
   file is not accidentally overwritten. Offer explicit blank/template/last-project
   startup modes.
3. **Host preset workflow:** decide what an Orbit generator preset contains, then
   implement save/load and instrument program selection with appropriate undo and
   persistence. Do not treat Orion's proprietary preset format as already supported.
4. **Editing convenience:** shared play-cursor/view-center zoom preference and an
   optional external audio editor, with clear handling of externally changed files.
5. **Follow-up settings/audio work:** real meter decay/update policy, an independent
   mixer-controller port, hardware bus outputs and double-precision mixing. Each
   needs its own implementation and validation scope. External MIDI clock following
   is a separate possible enhancement, not a requirement inferred from this topic.

No application source or build number changed for this review. These priorities
are proposals for discussion; implementation has not been selected yet.

### 2. Welcome to Orion > Changes since Orion 7

**Reviewed 2026-10-29.** Read the complete comparison table in
`FeatureOverview.htm`, then checked Orbit's host/source paths for all 18 rows.
This topic is an overview, so a feature mentioned here still needs detailed
behavior review in its own chapter. Coverage below is source evidence; it does
not certify audible timing, hardware or every plugin's behavior.

| Orion 8 feature in this topic | Orbit coverage and useful follow-up |
| --- | --- |
| Delay compensation in Mixer and Master | Project Settings controls the engine's edit-wide latency compensation. Confirm actual alignment with latency-reporting inserts on tracks, buses and Master before claiming complete parity. |
| MIDI controller management | Multiple saved assignments, device/channel filters, learning and generator-specific bindings exist. |
| MIDI maps through templates | Controller bindings live in application preferences. Native project templates save project/engine state, but do not provide a portable controller-map import/export workflow. |
| MIDI recording in Pattern and Song mode | Missing at the original section-2 checkpoint; subsequently implemented in Builds 237–238 and retained in 239. Hardware, virtual and on-screen keyboard input share the take recorder, with sustain, quantization, atomic commit and Undo. Song takes currently require 4/4, looping off and an empty region. Build 251 adds count-in and audible Pattern loop overdub before commit; controller performance capture, audio recording and Song pattern-launch recording remain open. See section 5 and Source/Recording/README.md. |
| Eight parallel sub-buses | Eight internal output-bus choices exist. Detailed bus/return behavior belongs to the Mixing chapter and needs audible routing checks. |
| Three mixer inserts | Generator strips offer three insert slots; Master has four. This row is broadly represented. |
| Selectable mixer pan laws | Pan controls exist, but no host selector for balance/equal-power/linear pan-law choices. A pan knob alone does not implement these alternatives. |
| Optional double-precision mixing | Missing, as recorded in section 1. |
| Microtuning on supported generators | No host tuning-table workflow. Individual hosted plugins may support tuning themselves; that does not establish a shared tuning system for Orbit's own instruments. |
| Mouse-wheel support across editors, Mixer and instruments/effects | Piano Roll scrolling and Playlist pattern-wheel actions exist; many native controls use JUCE sliders. Consistency, focus behavior and third-party editor handling need a separate interaction pass. |
| Free assignment of buses to hardware output pairs | Missing host bus-to-hardware-output-pair assignment, as recorded in section 1. Internal buses are present. |
| Pattern automation for generators and Mixer | Pattern lanes can address automatable parameters in the selected generator track's plugin list, including its volume/pan, EQ and inserts. This is not a complete bus/Master automation interface. Playback/loop interactions still need dedicated validation. |
| Internal and VST sidechains | No host sidechain-source selection/routing workflow was found in Orbit source. The engine or a plugin having sidechain support does not make it available to the user. |
| Song automation | Separate song-time lanes exist for generator-track plugin parameters and MIDI targets. The host editor is not a general bus/Master automation interface; review routing, target identity and playback in the Automation chapter. |
| Smaller internal stream buffer | Orbit offers device-supported hardware buffer sizes. The manual's internal stream-buffer size is a separate engine detail; neither a matching internal processing quantum nor identical latency is established here. |
| Templates and song merge | Native `.orbittemplate` save/open and native project merge exist. Detailed merge/media/plugin restoration limits need the relevant File/Templates review. Legacy SFS research remains paused. |
| Minimize, hide, roll-up, window appearance, full screen and window snap | Several panels have minimize/hide/collapse/maximize controls and saved placement. Full Screen works through its View menu handler, but the advertised F11 shortcut has no matching key handler. There is no shared configurable window-position snap grid; note/Playlist snapping is separate. Double-click behavior differs by panel (Playlist maximizes). |
| View-centered or play-cursor-centered zoom | Missing shared anchor preference, as recorded in section 1. |

#### Recommended work from this section

1. **MIDI recording (first increment completed after this review):** begin with armed Pattern-mode recording, including correct
   timestamps, note lengths, sustain and loop/overdub behavior; then deliberate
   Song-mode destinations and undo. Review the Recording chapter before designing
   the exact behavior.
2. **Portable controller maps:** save/load named maps and decide explicitly how
   templates bind a map to generator parameters across machines/projects.
3. **Mixing additions:** selectable pan laws and usable sidechain source routing,
   followed by bus/Master automation and hardware-output assignments.
4. **Window/zoom consistency:** the real F11 shortcut, deliberate roll-up behavior,
   configurable window snapping and shared zoom anchoring.
5. **Instrument tuning:** shared tuning tables where Orbit controls the instrument;
   respect the actual tuning capabilities of each hosted plugin.

These are recorded ideas, not authorization to implement the entire table at once.
The immediate generator-menu correction is separate: the old gear was hidden with
the pattern toolbar, and Drum Rack had no matching menu. The correction places a
drawn downward triangle in each header and keeps the host dropdown available with
hidden pattern controls or a rolled-up generator. Both menus use the same existing
generator handlers with pinned generator/pattern identity.

### 3. Welcome to Orion > System Requirements

**Reviewed 2026-10-29 against local Build 239 / v0.3.218.** Read
`SystemRequ.htm`. Its Windows XP/Vista/7, SSE2, RAM, screen-resolution and
DirectX/ASIO recommendations describe the historical Orion product. They are
not measured Orbit minimum requirements and do not call for implementing old
Windows support.

Orbit currently has a universal Mac tester package, Windows x64 package and
Ubuntu 24.04 x64 package. The Mac deployment target is macOS 11; that target is
not evidence of testing every supported OS version. CPU/RAM minimums and audio
latency still need measured tester results. Include a 1280×800 workspace in a
future desktop layout pass, since it is the manual's reference minimum; this
review did not run that layout test. Current packaging instructions belong in
DISTRIBUTION.md rather than copying Orion's installation advice.

### 4. Sequencing and Recording > Patterns: the basic building blocks

**Reviewed 2026-10-29; implemented in Build 242 after the user's workflow
correction.** Read the complete `Sequencer.htm`, shortcut table and configuration
screenshot. The original Build 239 gap list drove this implementation. Do not
advance to Playlist/Recording implementation until this section is delivered and
its validation reviewed; the already-read chapters below remain references.

| Manual behavior | Build 242 coverage |
| --- | --- |
| Generator-owned A–H / 1–8 patterns, step length, Pattern/Song modes | Existing 64 slots, remembered selection, native persistence and transport are retained. Pattern automation-only slots are recognized for Pattern playback. Live recording limits remain documented in the Recording chapter. |
| Keyboard audition; add/delete, move and resize notes | Full MIDI range and scale-constrained clickable-key recording remain. Optional inserted/moved-note audition now pairs on/off without entering the performance recorder. |
| Pencil/Rectangle and additive selection | Explicit Pencil/Select button and T shortcut; Ctrl/Cmd adds rectangles/notes. No selection still means the complete pattern for musical edit tools. |
| Drag a selected group, arrow editing and Ctrl-drag duplication | Implemented with atomic pitch/time bounds, original timing/velocities, duplicate-note preservation and one Undo per gesture. Right-edge resizing stays separate. |
| Alt-draw brush | Implemented separate notes at the current resolution/remembered length, one Undo per stroke. |
| VEL edits one note in the grid, including one chord member | Visible VEL mode and V shortcut edit that pitch/onset stack with vertical drag and velocity shading. Other chord pitches are unchanged. The lower lane retains its separate step-wide behavior. |
| Pattern events and LFO tool | Existing lane tools are retained; right-click opens five-shape LFO drawing with cycles/phase/depth/centre, document-free preview, Cancel and one-Undo Apply. Long patterns use up to 4096 samples across their complete length; native storage no longer clamps them to step 4095. Performance automation capture and Song Event Editor belong to the later Automation implementation section. |
| Generator-specific configuration, follow, ghosts, names, cursor, audition | Optional PIANOROLL engine-track metadata stores each generator's choices and survives native reopen. Ghosts show other generators' remembered patterns/colors. Names appear on notes, follow scrolls the viewport, cursor display is optional, and edit audition is separate from recording. |
| Sampler RNG/slice-name ruler | Explicitly deferred by the user: “Hold off until we work on the sampler.” Implement with the Sampler/groove-slicer section. |
| Shortcut table | Semitone/octave arrows, step rotation, slot/subdivision keys, C/E/G/M/N/S/T/X/Y/V, +/- zoom, keypad pitch/cursor navigation, Tab/Ctrl-W and existing Ctrl/Cmd editing are implemented with text-entry protection. |

The user's additional GUI report is handled by a shared generator content parent,
exclusive GUI/Piano Roll/Rack visibility, shared Rack settings/pattern controls,
responsive tight plugin sizing and removal of the extra INSTRUMENT GUI frame.
Built-in 4OSC now creates a paged parameter editor; Undo keeps the Piano Roll
open and rebinds to restored instruments. Held vertical Playlist pattern travel
also shares the wheel policy and one-gesture Undo without changing horizontal
painting.

The section-specific sanitizer check covers real note gestures, shortcuts,
independent preferences, ghosts, audition/recording separation, LFO preview,
long-curve commit/reopen, native project persistence, Rack switching and an
exact-size mock native editor. Supporting Playlist, Piano Roll, generator and
recording profiles cover their existing behavior. Physical audio/MIDI, native
plugin views and device timing still require desktop validation; no bit-for-bit
Orion or universal third-party compatibility is claimed. See
Source/Editing/PATTERNS_IMPLEMENTATION.md and Source/Editing/README.md.
Build 252 corrects real macOS Command shortcut dispatch (zero text character)
and preserves the Piano Roll pitch/time view during paste and same-pattern
Undo/Redo. This is a usability follow-up while Playlist/Recording remains active.

### 5. Sequencing and Recording > Playlist: Arranging and Recording

**Reviewed 2026-10-29 against local Build 239.** Read the complete `Playlist.htm`
and configuration screenshot, including count-in and track Record behavior.
The contents place recording in this topic; there is no separate Recording
Patterns topic at this position in the manual.

| Manual behavior | Orbit coverage and remaining gap |
| --- | --- |
| Paint pattern-sized blocks; delete placements without deleting the source pattern | Represented. Build 239 stamps source-duration copies during Draw strokes, supports intentional right-edge placement extensions, rejects collisions and keeps multi-bar bodies opaque. The supplied October 29 screenshots/video informed that implementation. Old extended placements are preserved. |
| Choose patterns from a block, move blocks and remember generator selection | Wheel choice now includes blank A1–H8 slots by default; an existing-slots-only preference is available. Single-clip moves and the remembered paint slot exist. Build 242 adds held-click vertical travel with the same wheel policy and one gesture Undo. A label-click chooser still differs from Orbit's menu gesture. |
| Rectangle-select multiple clips, move the group and Ctrl-drag a copy | Build 249: Select-mode rectangle, Shift-toggle/add, group drag and Ctrl/Cmd-copy, with one Undo and complete collision/bounds/type validation. Native audio and pattern placements can be selected together; cross-generator transfers clone complete patterns into unused slots. |
| Shift temporarily toggles snapping; Alt snaps to nearest bar; configurable step snap or free placement | Missing corresponding modifiers and sub-bar/free placement. The current placement model/gestures use whole-bar positions. Snap interval appears in Settings but is not consumed by the gesture code; the visible setting is not a completed snapping implementation. |
| Copy/move generator and mixer Song events with selected blocks | Song event movement/copy policy remains missing. Build 249 cross-generator transfers preserve complete source pattern data, including Pattern automation, Accent/Slide and coincident notes. Song events remain at their original arrangement times. Module/Mixer event range and target policies must be implemented before this row is complete. |
| Mute/Solo/Record and generator Freeze | Mute/Solo and MIDI-note recording arms exist. Freeze rendering and the separate Orion arrangement-recording meaning of R are missing. |
| Record selected pattern changes into Song arrangement while playing | Missing. Orbit's Song recorder writes performed MIDI notes into fresh patterns at commit; it does not capture a performance of switching existing patterns or grow their Playlist placements live. |
| Record audio, choose per-track hardware input and monitor it | Audio configuration/monitoring controls do not implement recording to audio clips. Per-track input-source capture, simultaneous channels, media handling and commit/recovery need a separate audio-recording pass. |
| Expand one multitimbral VST into several Playlist tracks | Missing corresponding channel-track workflow. Cloning an entire generator creates another instance and is a different operation. |
| Follow Play Cursor, note/velocity previews and navigation bar settings | Build 249 connects Follow Play Cursor to viewport scrolling, authored note velocities to preview stems, and Show Navigation Bar to the horizontal scrollbar. Options persist across launches. Focused Hover-N renames the pattern/audio placement; Shift+S toggles Follow. Note and optional grid previews remain available. |
| Metronome with quarter-note count-in | Build 251 adds Options > Recording Settings count-in (0/1/2/4/8 quarters in 4/4), using native engine preroll/clicks. METRO independently controls continuing clicks. Precount notes are audition-only; silent cancellation restores the punch cursor without Undo. |
| MIDI Pattern/Song performance recording | Builds 237–238 implement the first note-take workflow: pinned/armed routing, timestamped capture, sustain, quantization, one atomic commit and Undo, and project reopen. Build 251 replays completed Pattern take notes on later passes while retaining the document unchanged until one final commit. Different Pattern MIDI lengths repeat over a common cycle. Song takes require 4/4, looping off and an empty region; they are not general punch/replace recording. Controller/automation capture is also missing. |

**Current implementation status:** Build 249 completes group editing and these
view controls; Build 251 adds count-in and audible Pattern overdub. The chapter
is still in progress. See Source/Playlist/IMPLEMENTATION.md for
the remaining requirements. The user-requested Song Event Editor usability fix is implemented in Build 254; the remaining chapter work is still queued.

**Recording status:** Build 251 implements count-in and audible Pattern loop
overdub. Next is a separate Song arrangement recorder for pattern changes. Keep those
distinct from audio recording and automation performance capture. Playlist
group selection and moving/copying its associated events need shared Undo and
collision rules rather than reusing a single-clip move blindly.

### 6. Sequencing and Recording > Automation: Pattern and Song events

**Reviewed 2026-10-29 during the Build 240 follow-up.** Read the complete
`EventEditor.htm`, both Song Event Editor screenshots, and `MenuView.htm` to
resolve the user's View-menu correction. This is the next chapter after
Playlist; none of the three Sequencing and Recording contents topics was
skipped. Reviewing this chapter does not implement its missing features.

| Manual behavior | Orbit coverage and remaining gap |
| --- | --- |
| Record knob/fader or MIDI-controller movements while playing; Pattern/Song mode determines destination | Build 255 captures native parameter movements with fractional timing, a pinned Pattern destination or exact-instance Song lane, and one-take Undo. Song editor Record or Options > Recording Settings > Automation starts immediately. Native loop write/punch-out policy applies; physical-device timing remains manual. |
| View > Event Editor opens the selected module's Song automation editor | Build 254: item 304 opens a dedicated Song Event Editor for a chosen track/plugin parameter. Piano Roll remains a separate generator view. |
| Right-click a parameter to edit its Pattern or Song events directly | Build 255 adds event menus to host volume/pan, Acid Bass and generic parameter controls. EQ/send direct menus remain pending; their targets are selectable in the Song editor. External plugin GUI menus remain plugin-owned. |
| Parameter selector and meaningful vertical value ruler | Build 254 enumerates instrument/audio-track plugins with slot labels and persisted plugin instance IDs; the ruler formats native values/units. Legacy name lanes are read only for unique targets. Build 255 exposes Master, buses and FX returns; tempo remains unexposed. |
| Pencil, line, erase and whole-graph horizontal move | Pattern lanes have pencil/line/erase/single-point move. The move tool moves an individual point rather than translating the entire graph. Build 254 Song editor implements all four tools, including whole-graph horizontal translation bounded without truncating points. |
| Copy/Paste/Delete all events, Smooth and Scale from graph context menu | Build 254 Song editor provides independent curve clipboard, Clear, neighbor Smooth and normalized Scale 50%/200%, bounded data and one project Undo per gesture/operation. Escape cancels preview. The Playlist settings entry now opens this editor. |
| Pattern events repeat with patterns; Song events remain arrangement-time data | Separate native lane types exist. Build 255 repeats plugin Pattern curves across arranged occurrences and extended placements, restoring baseline in gaps. Nonempty Song events override Pattern projection; Clear restores Pattern playback. MIDI CC/bend/pressure events have a separate clip path. Pattern/Song precedence, repetition and multiple same-plugin instances need dedicated regression/audio tests before claiming parity. |
| Song tempo automation via View > Tempo (related View reference) | Current action opens an initial-tempo dialog, not an automation editor. Variable-tempo editing, timing conversion and playback/render/export behavior need their own implementation scope. |

**Build 255 follow-up:** Song transport cursor, loop boundaries and follow, optional
gradient fill, Master/bus/return modules, host-control event entry, arranged Pattern
repetition and native automation take capture are implemented. Sanitizer/native
curve tests cover precision, stop handoff, one-take Undo, save/reopen and precedence.
Physical input/listening remains manual. Tempo automation, direct EQ/send menus,
duplicate-instance Pattern identity and Pattern whole-graph Move remain open.
The user's 2026-10-29 autonomous session instruction permits advancing with these
explicitly recorded gaps; no claim of complete Orion automation parity is made.

**Next topic in contents order:** Mixing and Mastering > Mixer (`Mixer.htm`).
Master Section and Sidechains follow it; these detailed chapters have not yet
been reviewed in the ordered pass.

### User-requested follow-up after sections 4–5 — Build 240

The supplied `2026-10-29 03-33-57.mov` and report prompted a latched header drag
with screen-coordinate tracking/release-only bounds saving, ruler click-to-seek
without loop activation, explicit drag-to-loop and Song playback from the cursor.
Piano Roll Settings gains root/scale snapping for drawing, moves and its own
clickable keyboard recording. The user explicitly chose Piano Roll only;
external/virtual MIDI is not remapped. These changes do not complete the wider
recording, group-editing or automation backlog identified above.

## Reviewed in this pass

- Sequencing and Recording > Patterns: generator toolbar, bank/slot layout,
  note/event separation, and the additional Piano Roll shortcut table.
- Additional Features > Arpeggiator and Shuffle: the complete chapter, including
  chord-reset behavior, order/range/distance, gate, retrigger, finite repeats,
  rate, rhythm rests, Render and independent Shuffle.
- The visible reference chapter tree, including the separate Effects, Generators,
  Menus, Groove Templates, Pluck Templates, Styles, T-Slide, Templates and Tuning
  Tables topics. Listing a topic is not a claim to have audited its entire chapter.
- The supplied generator-options recording: six menu entries and the arpeggiator
  panel. Native Help navigation worked; native main-window coordinate activation
  failed through the automation surface. Detailed instrument-specific settings
  still need direct comparison in a later pass.

## Current generator coverage

| Reference | Orbit coverage |
| --- | --- |
| Generator options menu | Rename generator, generator MIDI settings, rename current pattern, Arpeggiator/Shuffle, CC linking, receive generator MIDI. |
| Arpeggiator controls | Working pre-instrument audio-graph MIDI processor for live/transport/offline processing, named orders, range/distance, gate, tempo, repeats, retrigger, 16 rhythm steps and user MIDI order. |
| Shuffle | Independent non-destructive playback timing with its own rate; available with arp off. |
| Render | Bakes into the selected pattern and disables the two playback effects, with one undo restoring both notes/settings. |
| Native persistence | Processor/configuration, pattern names, generator names and native MIDI routes in the project/session. CC assignments remain user preferences. |
| Full keyboard | Build 233's MIDI 0-127 scrolling and editing retained. |

## Differences and follow-up work

- Compare all synth-specific Generator Settings controls directly. Orbit's current
  dialog supplies host MIDI input/output channel, transpose and velocity scaling;
  instrument-specific polyphony/glide/tuning stay in each instrument's editor.
- Confirm Orion's less-documented alternate note-order traversals and precise
  retrigger/rhythm interaction by listening to controlled examples. Orbit uses
  original explicit algorithms and does not claim bit-for-bit parity.
- Orion arpeggio presets use a separate format. Orbit loads a short standard MIDI
  user order; no undocumented proprietary preset parsing is claimed.
- Extend the native engine's one-destination track MIDI routing to deliberate
  fan-out if required, and include those inter-generator routes in offline audio
  rendering. Confirm MIDI passthrough/output from each hosted plugin.
- Generator preset open/save buttons still show the preexisting unimplemented
  status. Instrument/program/pattern preset format and persistence need a separate
  implementation pass; arpeggiator MIDI note-order loading does not replace them.
- The manual lists additional Piano Roll actions beyond current Ctrl/Cmd editing:
  semitone/octave transpose arrows, one-step rotation, number-key bank slot
  selection, subdivision shortcuts, event-editor toggle, multi-generator ghost
  notes, Scissor, horizontal/vertical flips, and a tool toggle. Compare focus and
  text-entry behavior before adding these shortcuts.
- Review Styles, T-Slide, Tuning Tables, project templates, Window-menu behaviors,
  live recording and song arrangement recording in detail. Their presence in the
  manual is a backlog reference, not evidence of an Orbit implementation.
- Verify physical MIDI keyboard/controllers, sustain, transport looping, audible
  output and native dialog interaction on the actual desktop. Headless processing
  and persistence checks do not establish these hardware outcomes.

The earlier File/Edit/Options implementation notes remain authoritative for their
tested scope and limits. The legacy SFS importer goal stays paused; this manual
review does not authorize resuming proprietary format/mapping research.


## Implementation started after section 2

The user chose implementation of missing features on 2026-10-29. The first
increment is MIDI take recording in Pattern and Song modes, with atomic native
commit, undo, source timestamps, sustain and input quantization. Exact scope and
remaining recording workflows are in Source/Recording/README.md. The remaining
section 2 ideas (portable maps, pan laws, sidechains, tuning, windows/zoom and
wider bus/Master automation) remain open. Ready-to-run Mac, Windows and Linux
Build 238 tester downloads are published at
https://github.com/tjgoa/orbit-daw/releases/tag/tester-build-238; compilation is
not required. Local Builds 239–242 contain the subsequent Playlist, scale-snapping, Patterns
and generator-containment work. They are not published as cross-platform tester
releases. Hardware/audio/plugin validation remains open. See DISTRIBUTION.md. This does not resume SFS research.


## Patterns follow-up — Build 247

The two supplied generator/overlap recordings prompted a shared-surface fix
before advancing the manual review. Header, toolbar and body now move and stack
as one panel; the Piano Roll’s title/grip manipulate that host instead of moving
a nested child. Native macOS VST views receive body clipping and foreground
workspace occlusion. Actual RipplerX/Labyrinth GUI captures and native hit
pass-through supplement the headless Patterns checks. Other vendors/platforms
and hardware/audio remain separate compatibility checks. The Sampler-only RNG
ruler remains deferred until the Sampler chapter, as requested. No new manual
chapter is marked reviewed by this bug-fix pass.


### 7. Mixing and Mastering > Mixer — Build 256

Read all of installed `Mixer.htm`, including routing, effect/EQ/mix sections,
additional functions and busses. Existing strips cover stereo meters, levels,
pan/mute/solo, four sends, inserts, EQ and a single bus destination. Build 256
adds channel/EQ settings clipboard with one-paste Undo, channel/return/bus rename,
pre/post-fader volume-linked send routing and parameter-bound EQ/send event entry.
The native volume/pan stage is combined: Orion's separate gain/pan send tap order
remains open. Multiple output destinations, soft saturation, exact EQ calibration,
multi-output generators and effect-state clipboard remain gaps. See
Source/Mixing/README.md for the precise implemented behavior and test scope.
Next detailed chapter: Master Section (`Master.htm`), then Sidechains.


### 8. Mixing and Mastering > Master Section — Build 257

Read all of `Master.htm` after Mixer. Four shared FX returns are now present in
new projects (and created if missing when opening Master Section). Multiple
Master/bus destinations are implemented; inactive bus state is retained while
its strip hides when no other channel routes to it. Native graph renders verify
sums, bus-only isolation and FX-send operation with all dry routes disconnected.
Direct/aux/sidechain feedback paths are checked before routing changes.

New projects put Master volume before EQ/inserts; old projects preserve saved
order. The Master strip context can switch order with one Undo. Plugin IDs,
parameter IDs, native curves and baseline keys migrate together, including
unambiguous legacy Master lanes. A compressor render verifies the changed
processing order. Save/reopen and fresh-process checks cover routes and automation.

Mixer follow-up in the same checkpoint: EQ Soft Saturation is an original optional
post-EQ stage, coupled to EQ bypass and settings clipboard. Render/finite-output
checks cover it; exact proprietary EQ response is not claimed. Separate gain/pan
send taps, multi-output generators and hardware outputs remain open. Next:
Sidechains (`Sidechains.htm`), then Reference Guide > Effects > Guide/Index.


### 9. Mixing and Mastering > Sidechains — Build 258

Read complete `Sidechains.htm` and the Compressor sidechain explanation.
Supported effect parameter windows now expose a source selector, None/disconnect,
stable saved IDs and one-choice Undo. Self/direct/aux/mutual-sidechain feedback is
refused. Native Compressor trigger state follows the selector. A native graph
render measures ducking/recovery with separate source/destination frequencies;
source level, disconnect, Undo/Redo and fresh reopen are checked. External VST
layouts remain plugin-specific; Orion's other listed sidechain effects require
separate DSP implementations and are not claimed here.

Automation display follow-up: generic host sliders/Mixer controls follow native
values without writing edits or fighting drags. Master mute is a final gate, so
level automation cannot unmute the mix; insert ordering, legacy mute migration,
Undo and silent fresh reopen have render tests. Schema 11 protects that behavior.
Next contents topic: Reference Guide > Effects > Guide (`Effects.htm`), then Index
and individual effect families. The guide and first dynamics references have been
read in preparation; their remaining features are not yet claimed implemented.


### 10. Reference Guide > Effects > Guide — Build 259

Read `Effects.htm`, `Effects-Index.htm`, `Brickwall.htm` and `FxCompressor.htm`.
General workflow now includes VST3 effect catalogue/browser/targeted insertion,
native GUI or host parameter entry, serial ordering, settings/chain clipboard,
one-operation Undo, bypass/remove and four Master/return/bus slots. A separate
internal plugin budget avoids routing/EQ/gain/mute consuming visible slots.
All eight buses plus Master are render-tested concurrently. Effect copy excludes
curves, preserves destination lanes, uses unique IDs and drops unsafe sidechains.
Rack-dependent copies are explicitly refused pending rack-aware support.

A saved project VST3 catalogue and exact legacy class-ID discovery support fresh
loading without the global scan cache. Native QDelay integration passed discovery,
insertion, finite/non-silent DSP rendering, vendor parameter state, same/fresh
reopen, cache-independent legacy loading and cross-project copy. Its binary is not
bundled. Particular vendor GUI/layout/version checks remain manual.

The dynamics references identified missing native look-ahead limiting, detector EQ,
parallel compression and CV-82/RNA-160-specific behavior. Builds 261–263 add
original limiter/peak-compressor implementations; exact Synapse behavior remains open;
the existing native Compressor is not claimed to be a CV-82 emulation. Remaining
effect-family chapters and MultiFX serial/parallel containers are not complete.
Build 260 follows up the user's simultaneous Acid Bass/Drum Rack request with
independent built-in controls (also 4OSC), owner-bound pattern selectors and safe
Undo/Edit lifetime. The main Piano Roll remains shared; simultaneous external
instrument workspace panels remain open. See Source/Generator/README.md.


### 11. Reference Guide > Effects > Brickwall Limiter — Build 261

Implemented an original linked-stereo sample-peak look-ahead limiter with Input
and Release parameters, approximately 5 ms declared latency, attack anticipation,
exponential recovery and input/reduction readouts. It is available in FX Dynamics
and native insert choosers; native automation, Undo, chain copy and persistence
apply. Schema 13 protects its native plugin registration. DSP is original, not
Orion source or an asserted emulation. No true-peak capability is claimed.

Core sanitizer checks cover 44.1/48/96/192 kHz, exact delay/under-threshold passage,
large peaks, linked stereo, nonfinite input and recovery. Native render checks
cover parallel compensation, export end samples, bypass and boosted peaks; direct
contexts cover loop versus seek. See Source/Mixing/LIMITER.md for checks and
remaining live listening/device tests. The following manual topic is Compressor
CV-82. Its detector weighting, look-ahead and parallel blend remain to implement;
the existing native Compressor remains separately available.


### 12. Reference Guide > Effects > Compressor CV-82 — Build 263

Implemented a separate original Peak Compressor with the manual's functional
control set: Input, Threshold, Ratio, Output, detector-only EQ, Attack, Release,
Dry/Wet, look-ahead, external sidechain and input/reduction/output readouts.
The peak detector links stereo by maximum magnitude; the original detector bell
uses 1 kHz/Q 0.707 rather than claiming unknown Synapse coefficients. The signal
path adds no saturation or clipping. Host controls respect parameter skew and
capture one Undo per gesture; native automation, copy and schema-14 state apply.
Existing Compressor instances are unchanged. See Source/Mixing/PEAK_COMPRESSOR.md.

Core and native validation cover transfer/timing, stereo/sidechain isolation,
multirate PDC and blend, automation, Undo and fresh-process reopen. A native export
frame-offset correction also fixes first-sample latency trimming. Exact CV-82
sonics, legacy parameter/state mapping and hardware listening remain open. The
next manual effect topic is Compressor RNA-160; its analog-style behavior has
not been implemented by relabelling this clean peak compressor.


### 13. User video follow-up — Build 264

Reviewed both 22:20/22:28 recordings and the four 22:24–22:36 screenshots. Normal
built-in access now opens independent embedded controls; Playlist name/Mixer footer
double-clicks use the same entry. Event Editor is an embedded document with its
own explicit window controls and compact drawing tools. Transport no longer hides
it behind the parent OS window. Settings hold fill palettes/follow, while ruler
seeking/loop edge/body edits share the native loop. A shared live-preview LFO tool
serves Song and Pattern automation, including fades/curvature/random hold and
Cancel/one-Undo Apply. Main MIDI/AUTO selection clarifies recording content;
armed automation can play an otherwise blank pattern/arrangement.

Generator, Event/LFO/loop, native automation/fresh-process and MIDI recording
checks pass. Source/listening equivalence to Orion is not asserted. Multiple
external-instrument workspace GUIs still need the separate native-view ownership
pass; the built-in Acid Bass/Drum Rack report is addressed. The user prioritizes
Tube/Two-Band Distortion next and Pro-9 as the next generator. RNA-160 remains
pending rather than being silently marked complete. File-menu re-audit is queued.


### 14. User-prioritized Tube / Two-Band Distortion — Build 267

Read the complete installed FxTubeDistortion.htm and Fx2BandDistortion.htm. Added
original native effects covering drive/filter/shape/level and split-band drive/
tone/blend roles, respectively. The reference does not supply transfer functions,
filter orders, oversampling or reliable shape symmetry (its sentence repeats a
word). Orbit choices are explicit in Source/Mixing/DISTORTION.md. Original cores
and existing JUCE 4x FIR processing avoid importing a candidate's old VST wrapper.
Native render/automation/PDC, state/copy/Undo and fresh reopen tests pass. Listening
and source-effect A/B remain open. This is not legacy SFS effect-state restoration.

Verified MIT candidates and ABL2 history are recorded in DISTORTION_RESEARCH.md.
AudioRealism confirms its old drive/distortion returned in ABL3.3.0, but the checked
primary sources do not identify its circuit/formula or an identical open DSP.
GenPro9.htm has been read for the next generator's requirements; no stereo-only
instrument is being called complete Pro-9 parity. The 11 parts, global accent and
6/11 output routing remain implementation work. RNA-160 is still pending.

### 15. File-menu recheck (reference versus source, not new implementation)

Re-read MenuFile.htm and checked current entry-point handlers: New/Open/Merge,
Save/Save As/Template, Stream to File, MIDI import and Pattern/Song export,
Preferences/Project Settings, recents and Exit are present. SFS import remains
experimental recovery. Audio export lists the build's writable codecs rather
than asserting MP3 on every OS. Existing export scopes are combined mixes;
per-channel stem files and a dedicated sampler production-quality override are
still differences. Their absence is not hidden by marking all File behavior done.


Build 267 follow-up: Event Editor Copy/Paste/Cut in the main Edit menu now uses
the focused event document's data/clipboard when the pattern has no MIDI notes.
The sanitizer profile substitutes headless focus only and verifies menu enablement,
production handlers and three-operation Undo restoration. The native automation,
loop/LFO and fresh-process suite passed after the change.

### 16. Mixer and Playlist visual workflow follow-up — Build 268

Reference: user screenshots at 11:12 PM October 29 and movie
`2026-10-29 23-16-52.mov` (reviewed frames at 7 and 18 seconds).
Mixer slots use the left indicator as the only bypass control; full-width name
opens the editor. Inline duplicate bypass and X are hidden. Direct context menu
contains native/VST effect categories; occupied-slot assignment replaces that
slot atomically with Undo, after constructing the replacement successfully.
Closing an editor only hides its GUI; Remove Effect is an intentional context
menu action. This distinction avoids changing audio when dismissing an editor.

Playlist wheel/held travel clamp rather than wrap, including existing-only mode
from an uncreated current slot. Bottom selector strip opens all A1–H8 slots;
right-edge resize stays separate. Peak Compressor receives a dedicated original
vector rack GUI; its DSP/automation schema remains unchanged. KVR sine-fold lead
is documented in `Source/Mixing/DISTORTION_RESEARCH.md`; an exact ABL match is not
claimed and no third-party DSP source was imported in this follow-up.


### 17. Independent FX and manual returns — Build 269

User screenshots from October 30, 1:33–1:34 AM expose implicit FX assignment,
missing auto-open and a knob pointer/arc mismatch. Replaced the singleton native
FX editor with instance-keyed embedded panels. Minus restores from the shared
bottom bar; X explicitly removes the effect with Undo, superseding Build 268's
hide-only X behavior. Kept the single insert bypass light and deliberate context
Remove action. New insert panels open immediately. FX state/layout is preserved
across unrelated undo reconstruction, while deleted/replaced owners are pruned.

External VST generator panels are independent too; selecting/inserting a generator
leaves the Piano Roll open. Two actual RipplerX and two actual QDelay GUIs passed
an isolated native-peer coexistence/geometry/restore test. Shared native hosting
code is extracted to Generator/PluginEditorContent.h with the same Cocoa clip
implementation. New FX returns start without processors; send context menus
assign the chosen effect manually. Existing saved FX are not removed. Knob arcs
and pointers use one clockwise-from-top convention. No DSP range/schema change.


### 18. Acid Bass internal FX and transport persistence — Build 270

User-requested extension beyond Monobass's original control set: collapsible
post-synth Effects / Drive with Soft/Sine Fold, intensity, blend, wet filtering
and output. Permanent Acid Bass value fields are replaced by drag popups.
Existing tune/filter/envelope/accent/slide/waveform IDs and Open303 voice remain
unchanged. ABL public manual/control descriptions and the unverified KVR foldover
lead informed an independently written processor, not an exact emulation claim.
This follow-up does not mark any remaining Orion chapter as completed.
