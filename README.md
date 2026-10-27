# Orbit DAW

Orbit is a standalone DAW inspired by Orion's generator → pattern → playlist → mixer workflow. The application is being developed as original software under AGPLv3.

## Current state

Orbit is an actively changing DAW foundation rather than a finished Orion-style workflow. The current source checkpoint is Build 100 / v0.3.79, with local development iteration **155**. It includes a Tracktion Engine-backed built-in 4OSC instrument, VST3 instruments, editable Piano Roll, generator-owned A1–H8 pattern banks, a 512-bar Playlist, MIDI export, and `.orbit` project save/reopen. Pattern mode plays each generator's remembered pattern together; Song mode follows Playlist clips and preserves the other generators' remembered positions. Automation has separate pattern-time lanes in the Piano Roll and song-time lanes in the Playlist. Playlist generator context actions include Show, Rename, Delete, Clone Track To, Edit Song Events, and Track Color; patterns/arrangement clone and persistent track colors are implemented. Loop start/length/enabled state persists and is drawn on the Playlist ruler. The Mixer contains generator strips with sends and output-bus assignment; the separate Master Section contains bus/return strips and a Master strip with four inserts and no sends. Routed buses appear there automatically. The Mixer stays in the Orbit workspace and can be moved, minimized, or closed. VST3 scan and crash reports are written to the Desktop. Current follow-up work includes immediate instrument-key audition, Playlist responsiveness and controls, Mixer Settings/Effects cloning, and runtime verification of loop playback, automation, bus routing, and context actions. See `ORION_LEARN_TODO.md` for remaining work, `ORION_NATIVE_EFFECTS_COMPATIBILITY.md` for the observed Orion effect inventory and migration plan, `BUILD_HISTORY.md` for product checkpoints, and `Source/Project/PROJECT_FORMAT.md` for the project format. Orbit is not yet verified as a finished production DAW.

See [`PROGRESS.md`](PROGRESS.md) for a dated development update and current focus.

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
