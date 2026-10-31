# Orbit DAW

Orbit is a standalone DAW inspired by Orion's generator → pattern → playlist → mixer workflow. The application is being developed as original software under AGPLv3.

## Tester downloads

[Download Build 276 / v0.3.255 for Mac, Windows or Linux](https://github.com/tjgoa/orbit-daw/releases/tag/tester-build-276).
These are ready-to-run portable tester packages; no compiler is needed. See
[installation and Mac first-launch instructions](DISTRIBUTION.md) and the
[major changelog since Build 238](Releases/Build-276.md).

This branch contains the updated application source. The exact release source
is pinned by the [tester-build-276 tag](https://github.com/tjgoa/orbit-daw/tree/tester-build-276)
and [tester branch](https://github.com/tjgoa/orbit-daw/tree/codex/tester-build-276).
Existing project screenshots below are preserved.

## Current state

<img width="1680" height="963" alt="2" src="https://github.com/user-attachments/assets/11df3ec9-8955-4cdb-98cd-75fc7600b400" />

<img width="1680" height="967" alt="1" src="https://github.com/user-attachments/assets/4776994d-c6af-47f6-abd9-23b55acf3dc6" />

Orbit is an actively changing DAW foundation rather than a finished Orion-style workflow. The current source checkpoint is Build 276 / v0.3.255, with local development iteration **303**. It includes a Tracktion Engine-backed built-in 4OSC instrument, VST3 instruments, editable Piano Roll, generator-owned A1–H8 pattern banks, a 512-bar Playlist, MIDI export, and `.orbit` project save/reopen. Pattern mode plays each generator's remembered pattern together; Song mode follows Playlist clips and preserves the other generators' remembered positions. Automation has separate pattern-time lanes in the Piano Roll and song-time lanes in a dedicated Song Event Editor. Playlist generator context actions include Show, Rename, Delete, Clone Track To, Edit Song Events, and Track Color; patterns/arrangement clone and persistent track colors are implemented. Loop start/length/enabled state persists and is drawn on the Playlist ruler. The Mixer contains generator strips with sends and output-bus assignment; the separate Master Section contains bus/return strips and a Master strip with four inserts and no sends. Routed buses appear there automatically. The Mixer stays in the Orbit workspace and can be moved, minimized, or closed. Generator headers, pattern controls and editor bodies now move and stack as one panel; macOS native VST views are clipped below the toolbar and behind foreground workspace panels. VST3 scan and crash reports are written to the Desktop. Current follow-up work includes immediate instrument-key audition, Playlist responsiveness and controls, Mixer Settings/Effects cloning, and runtime verification of loop playback, automation, bus routing, and context actions. See `ORION_LEARN_TODO.md` for remaining work, `ORION_NATIVE_EFFECTS_COMPATIBILITY.md` for the observed Orion effect inventory and migration plan, `BUILD_HISTORY.md` for product checkpoints, and `Source/Project/PROJECT_FORMAT.md` for the project format. Orbit is not yet verified as a finished production DAW.

See [`PROGRESS.md`](PROGRESS.md) for a dated development update and current focus.

Experimental Orion SFS recovery is available through **File > Merge Project...**.
It recovers selected generators, tentative whole-bar placements and accepted dry
audio in one undoable import. Whole generators are selected by default. Missing
audio can remain silent placeholders, retaining original references for
**File > Find Missing Media…** and recursive folder relinking after save/reopen. The dialog defaults to editable 96 PPQ /
4/4 and allows direct Merge with visible validation errors; Preview is optional. Instruments/effects, routing and
Drum Rack pad/sample restoration remain unsupported. See
[`Source/Import/README.md`](Source/Import/README.md) for exact coverage and limits.

### Acid Bass and practical tests

**Effects / Drive** now offers **Soft**, **Sine Fold** and **Triangle Foldback**.
Raise Distort above zero to hear the selected shape. The new triangle mode keeps
old Soft/Sine Fold state and automation compatible. See the
[Acid Bass FX guide](Source/Synth/ACID_BASS_FX.md).

The generator dropdown's **Generator Settings…** includes **Retrigger oscillator
phase** for Acid Bass. It restarts phase on fresh attacks and preserves slides
and live legato. Default Off retains the original behavior; each instance saves
its own choice. See [Acid Bass guide](Source/Synth/README.md).

**Insert > Synths > Acid Bass** adds a native mono acid synth with eight controls,
saw/square and Piano Roll Accent/Slide rows. Click **Demo pattern**, choose Pattern
mode and Play. Sound settings and step switches survive native save/reopen.
See [Acid Bass scope and DSP credits](Source/Synth/README.md).

Open **Help > Quick Test Guide**, or use the
[practical testing guide](Source/Editing/TESTING_GUIDE.md) for specific actions
and expected results for velocity, group move/copy, brush and selection tools.
**File > Bypass Missing Effects** disables unavailable native external effect
slots while keeping their state; it does not reconstruct Orion effects.

### Drum engine auditions

**Insert > Drum Auditions > 9W9 909 Audition** adds an eleven-sound native kit,
paged step sequencer, per-step velocity and Accent, a protected four-bar demo,
and upstream Drive/Distortion/Reverb/Delay controls. See the
[909 audition guide](Source/Synth/NINEKIT_GUIDE.md) for practical tests, credits
and limits. This is a modified GPLv3 9W9 port with one stereo output.

**Insert > Drum Auditions > Geonkick Audition** adds a native percussion test
instrument using GPLv3-or-later Geonkick DSP. Choose a sound recipe, Hear hit,
or Demo pattern and Pattern Play. Multiple instances remain independent.
ChowKick is available separately as a VST3 comparison on this Mac. See the
[drum audition guide](Source/Synth/DRUM_AUDITION_GUIDE.md) for steps, credits,
local plugin setup and limitations; the full Pro-9 kit is still to come.

### Song automation

**View > Event Editor** opens a separate Song editor. Choose a track and plugin
parameter, draw its curve, then play in **Song** mode. Playlist track contexts
also offer **Edit Song Events**. Draw/Line/Erase/Move Graph, curve clipboard,
Clear/Smooth/Scale and one-gesture Undo are available. Repeated plugin instances
have separate identities; parameter labels use native units. See the
[Song Event Editor guide](Source/Automation/README.md) and practical tests below.
Build 255 adds transport/loop follow, gradient fill, native knob/MIDI automation
takes with one Undo, Master/bus/return targets and repeated arranged Pattern
curves with Song precedence. Tempo automation remains pending. New saves use
schema 20 and need Build 274 or newer; older supported projects still load.

### Mixer settings

Right-click a strip's name or empty area to copy/paste channel or EQ settings and
rename channels, returns or buses. Each paste is one Undo edit. Right-click a
send for volume linking and event entry; EQ knobs also open their bound events.
Build 257 adds simultaneous Master/bus destinations, feedback protection, four
factory returns, pre/post-insert Master volume and optional EQ soft saturation.
Build 258 adds sidechain source selection, live host control displays and Master
mute that stays silent during automation. See [Mixer guide and remaining gaps](Source/Mixing/README.md).

### VST3 effects and insert chains

**Insert > VST Effects** and empty Mixer insert slots now offer scanned VST3 audio
effects. Master/return/bus channels have four visible inserts. Right-click a slot
for host parameters/events, settings copy/paste, append/replace and reordering.
Projects retain VST3 descriptions for loading without the global scan cache.
See [Effects guide and remaining work](Source/Mixing/EFFECTS_GUIDE.md).

### Edit menu

The maximized Piano Roll fills the workspace without wallpaper gaps, follows
app resizing, and keeps the generator dock behind its bottom controls. Its header
Restore/Maximize button keeps the Piano Roll open at a remembered floating size.
Draw/Select controls are above the grid; Shift-selection also works while drawing.
View > Hotkeys opens a searchable guide. Note entry uses the whole clicked cell,
and velocity editing covers 1–127 with matching lane geometry. The lane shows
individual note handles for chords, with a gold highlight and pitch/value readout;
other chord notes no longer dilute the displayed range through averaging.

The Patterns pass adds a visible **VEL** mode for individual chord notes, group
move/Ctrl-drag copy, Alt brush, Rectangle selection, scissor/flips/rotation,
focused shortcuts, ghost notes and per-generator display/follow/audition settings.
Pattern automation includes a cancellable LFO preview. See the
[section checklist](Source/Editing/PATTERNS_IMPLEMENTATION.md) for scope and tests.
The Sampler slice-name ruler is deferred until the Sampler section by agreement.

Generator GUIs, Drum Racks and Piano Rolls share one attached content body and
common header. The extra Instrument GUI frame/padding is removed; switch views
without leaving the old GUI over the Piano Roll. Playlist patterns also accept
held-click up/down travel, using the same slot policy as the wheel.

Piano Roll Settings now offers optional **Snap to scale**, with a root key and
17 scales including major, minor, harmonic minor and double harmonic minor.
It constrains new drawing, vertical moves and recording from that editor's
clickable keyboard; hardware and separate virtual keyboards retain their pitch.
The preference survives reopening. Existing notes remain intact.

Playlist header drags stay captured through upward movement and reversals.
Drag the three-line grip beside the Generator column heading to narrow or widen
that column; its width is remembered, including when the Playlist is empty.
Click the ruler to place the cursor; Song Play starts there without turning on
Loop. Drag the ruler deliberately to select and enable a loop instead.

The Piano Roll edits all MIDI pitches, 0–127 (C-1 through G9 in Orbit's octave
labels). Use the mouse wheel or the right-side pitch scrollbar to reach other
octaves; Shift-wheel scrolls along time. Pitch zoom controls row size. Velocity
and automation lanes stay at the bottom while the keyboard scrolls. Switching
patterns focuses the note range, or C4 for an empty pattern.

The complete Edit menu has working musical tools, selection-aware cut/copy/paste,
shortcuts, groove and chord-pluck templates, and instrument/pad control variation.
Tools preserve exact native note timing and support undo/redo. See
[`Source/Editing/README.md`](Source/Editing/README.md) for command behavior,
shortcuts, validation and the recording-workflow limitation.

### MIDI recording and tester downloads

Record captures hardware MIDI and the Piano Roll, host instrument and F12 virtual
keyboards into native patterns. Click/hold or drag the left-side Piano Roll keys
while Record runs; pending Pattern notes appear immediately and Stop commits
the take. This provides a recording test without a MIDI keyboard. Recording has
one take undo. Playlist R arms generators; otherwise Record uses the selected
instrument. Pattern takes add notes to the remembered pattern. Song takes use
fresh slots and empty Playlist regions (initially 4/4, Song looping off).
See [recording scope](Source/Recording/README.md) for sustain, timing, quantization
and remaining workflows, and [tester downloads](DISTRIBUTION.md) for native
Mac/Windows/Linux packages. Audio recording remains unfinished.

Event Editor is an automation editor, separate from Piano Roll. Orbit's current
combined View entry is incorrect and is logged for the manual's View chapter.

### Options menu

Options starts with Skins and Wallpapers. Open Wallpaper Folder opens a local
image library; added PNG/JPEG/GIF files appear when Options is reopened.
Choose Image copies the selected image into this folder without replacing
same-name images. Navy Blue and Light Blue are coordinated
presets for Playlist, Piano Roll and Mixer, using the existing dark and light artwork.
The runnable app in `Builds/Current/Orbit.app` is updated to this checkpoint; restart
an older running instance to load it. The Options menu also includes separate audio
input/output settings, MIDI input filters/channel routing, CC assignments and
learning, MIDI output, a polyphonic virtual keyboard (F12), and project
export/display timebase and triplet-grid controls. Search paths feed the relevant
sample/VST3 workflows. See [`Source/Options/README.md`](Source/Options/README.md)
for exact behavior, persistence, reproducible checks and hardware/recording limits.

### Generator options

The downward triangle at the top-left of the generator header opens Rename Generator, Generator Settings, Rename
Current Pattern, Arpeggiator/Shuffle, Link To Next MIDI Controller, and Receive
MIDI From Generator. The dropdown stays available with hidden pattern controls;
the separate Drum Rack window has its own matching header dropdown.
Arpeggiator/Shuffle runs before the instrument on live and
sequenced MIDI; Render converts its output into editable notes with one undo for
notes and settings. General MIDI settings, stable-target CC learning and native
generator MIDI receiving are implemented. See
[`Source/Generator/README.md`](Source/Generator/README.md) for controls and limits,
including the engine's offline inter-generator-routing limitation. The installed
Orion Help is now an additional behavior reference; reviewed chapters and remaining
differences are tracked in [`ORION_MANUAL_AUDIT.md`](ORION_MANUAL_AUDIT.md).

### Drum Rack and Playlist previews

Playlist wheel navigation includes blank A1–H8 slots by default; Playlist Settings
offers **Scroll through existing patterns only**. Draw strokes stamp separate
copies at the source pattern's duration; use the right edge to intentionally
extend one placement. Pattern bodies hide Playlist bar lines, and maximizing
fills the workspace without wallpaper padding. See
[Playlist gestures](Source/Playlist/README.md) for exact behavior and validation.

The native Drum Rack provides 16 sample pads, an 8/16/32/64-step sequencer, per-pad playback and sound controls, and velocity editing for individual pattern hits. A pad's disclosure control opens a full-width waveform editor below the pad rows, with draggable trim handles and a velocity-expression lane; the waveform is hidden while the editor is collapsed. The Playlist can optionally show a miniature piano-grid preview inside pattern clips, either for all tracks from Playlist Settings or selectively per track from its context menu. Pattern previews refresh as Piano Roll notes are edited, and M/S/R buttons keep a fixed size while track height is zoomed.

### Load a VST3 instrument

Orbit opens with a fresh, ready-to-use project. Choose **Insert > VST Instruments > Scan / Manage VST3 Instruments…** to find a plugin; the first browser opening scans the standard VST3 folders for the current platform automatically. Select an instrument and choose **Insert** to add it as a generator and open its plugin editor. Discovered VST3 instruments appear under **VST Instruments** in the Insert menu; Orbit's built-in instrument remains under **Synths**. The menu follows Orion's sections and uses the same text styling for both. Each instrument track has its own embedded plugin GUI, and the generator header selector switches the active instrument while its pattern controls remain attached above the GUI. Starting a new project with unsaved changes offers Discard or Cancel. Add nonstandard folders and set **Scan VST3 plugins at startup** in **Options > VST Paths…**; startup scanning is off by default. Scan progress shows the plugin currently being checked. Each completed scan writes `Orbit-VST3-Scan-*.log` to the Desktop; fatal signals write `Orbit-Crash-Report-*.log` there with the signal and plugin being scanned.

Orbit is a working name and has not had final product-name clearance. Images used as Orion and mixer design references are intentionally kept out of this source repository.

## Build on macOS

Requirements: Xcode, CMake 3.22 or newer, and a macOS SDK supported by the checked-out JUCE/Tracktion Engine revisions. Orbit's application source is C++; CMake is the build configuration that generates Xcode projects, and JUCE/Tracktion Engine are C++ libraries.

Clone Orbit and fetch the pinned dependencies:

```sh
git clone https://github.com/tjgoa/orbit-daw.git
cd orbit-daw
git -c url.https://github.com/.insteadOf=git@github.com: submodule update --init --recursive --depth 1
```

Tracktion Engine's JUCE submodule uses SSH; the HTTPS rewrite above allows fetching it without setting up GitHub SSH keys. Keep upstream dependency license files and notices intact. Exact revisions are listed in `DEPENDENCIES.md`.

Generate an Xcode project and run the app:

```sh
cmake -S . -B build/xcode -G Xcode
open build/xcode/OrbitDAW.xcodeproj
```

In Xcode, select the **Orbit** scheme and press **Run**. The build can also be run from Terminal with `cmake --build build/xcode --config Debug --target Orbit`.

## Development builds

`Tools/build-orbit-dev.sh` rebuilds the app and copies changed output to `Builds/Current/Orbit.app`. Each changed successful local development build gets an incremental source-only ZIP under `Builds/Source Archives/`; these local build products and archives are ignored by Git. The helper scripts and iteration counter are included for contributors. A source archive does not contain compiled applications or vendored JUCE/Tracktion sources.

## Dependencies

The project records Tracktion Engine as a submodule at v3.2.0; that engine revision pins its JUCE submodule. Do not track a moving development branch for product builds.

## Licensing

Orbit application code is released under GNU AGPLv3 (see `LICENSE`). JUCE is used under AGPLv3 and Tracktion Engine under GPLv3, subject to their respective license files and notices. Third-party dependencies inside either repository may have additional licenses; preserve those notices when distributing builds. Source is published here; ready-to-run application packages, corresponding source and notices are attached to GitHub Releases.
