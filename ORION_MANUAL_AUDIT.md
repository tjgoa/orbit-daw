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
| MIDI recording in Pattern and Song mode | Missing: the Record button remains disabled. Live MIDI playback and MIDI-file import do not implement recording. This is the largest basic music-making gap in this overview. |
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

1. **MIDI recording:** begin with armed Pattern-mode recording, including correct
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

**Next topic in contents order:** Welcome to Orion > System Requirements
(`SystemRequ.htm`), followed by Sequencing and Recording > Patterns. Neither has
yet been compared as a new section in this ordered pass; the earlier Patterns
review remains recorded below.

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
wider bus/Master automation) remain open. Tester distribution is being prepared
separately; see DISTRIBUTION.md. This does not resume SFS research.
