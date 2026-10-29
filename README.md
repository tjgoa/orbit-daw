# Orbit DAW

Orbit is a standalone DAW inspired by Orion's generator → pattern → playlist → mixer workflow. The application is being developed as original software under AGPLv3.

## Current state

Orbit is an actively changing DAW foundation rather than a finished Orion-style workflow. The current source checkpoint is Build 235 / v0.3.214, with local development iteration **270**. It includes a Tracktion Engine-backed built-in 4OSC instrument, VST3 instruments, editable Piano Roll, generator-owned A1–H8 pattern banks, a 512-bar Playlist, MIDI export, and `.orbit` project save/reopen. Pattern mode plays each generator's remembered pattern together; Song mode follows Playlist clips and preserves the other generators' remembered positions. Automation has separate pattern-time lanes in the Piano Roll and song-time lanes in the Playlist. Playlist generator context actions include Show, Rename, Delete, Clone Track To, Edit Song Events, and Track Color; patterns/arrangement clone and persistent track colors are implemented. Loop start/length/enabled state persists and is drawn on the Playlist ruler. The Mixer contains generator strips with sends and output-bus assignment; the separate Master Section contains bus/return strips and a Master strip with four inserts and no sends. Routed buses appear there automatically. The Mixer stays in the Orbit workspace and can be moved, minimized, or closed. VST3 scan and crash reports are written to the Desktop. Current follow-up work includes immediate instrument-key audition, Playlist responsiveness and controls, Mixer Settings/Effects cloning, and runtime verification of loop playback, automation, bus routing, and context actions. See `ORION_LEARN_TODO.md` for remaining work, `ORION_NATIVE_EFFECTS_COMPATIBILITY.md` for the observed Orion effect inventory and migration plan, `BUILD_HISTORY.md` for product checkpoints, and `Source/Project/PROJECT_FORMAT.md` for the project format. Orbit is not yet verified as a finished production DAW.

See [`PROGRESS.md`](PROGRESS.md) for a dated development update and current focus.

Experimental Orion SFS recovery is available through **File > Merge Project...**.
It previews selected notes, tentative whole-bar placements and explicitly relinked
dry audio before committing one undoable import. Instruments/effects, routing and
Drum Rack pad/sample restoration remain unsupported. See
[`Source/Import/README.md`](Source/Import/README.md) for exact coverage and limits.

### Edit menu

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

Record captures MIDI/Virtual MIDI Keyboard performances into native patterns with
one take undo. Playlist R arms generators; otherwise Record uses the selected
instrument. Pattern takes add notes to the remembered pattern. Song takes use
fresh slots and empty Playlist regions (initially 4/4, Song looping off).
See [recording scope](Source/Recording/README.md) for sustain, timing, quantization
and remaining workflows, and [tester downloads](DISTRIBUTION.md) for native
Mac/Windows/Linux packages. Audio recording remains unfinished.

Event Editor is an automation editor, separate from Piano Roll. Orbit's current
combined View entry is incorrect and is logged for the manual's View chapter.

### Options menu

Options starts with Skins and Wallpapers. Navy Blue and Light Blue are coordinated
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

Orbit application code is released under GNU AGPLv3 (see `LICENSE`). JUCE is used under AGPLv3 and Tracktion Engine under GPLv3, subject to their respective license files and notices. Third-party dependencies inside either repository may have additional licenses; preserve those notices when distributing builds. This repository publishes source code; it does not include a compiled application bundle.
