# Orbit DAW

Orbit is a standalone DAW inspired by Orion's generator → pattern → playlist → mixer workflow. The application is being developed as original software under AGPLv3.

## Current state

Orbit is an early, actively changing DAW foundation rather than a finished Orion-style workflow. The current public source corresponds to product **Build 24 / version 0.3.3**, with local development iteration **49**. It includes a Tracktion Engine-backed built-in 4OSC instrument, an editable piano roll, generator-owned A1–H8 pattern banks, a 64-bar Playlist, MIDI export, and `.orbit` project save/reopen. The separate Mixer window has blue channel strips with gain, pan, mute, solo, four-band EQ with response graph, insert effects, and four FX sends/return routing controls. The app has a tempo and transport toolbar with Playlist, Mixer, and Master controls. Mixer meters, audio recording, a full third-party VST instrument workflow, richer playlist editing, undo, and portable media relinking are still in progress. Orbit has not been verified as a finished production DAW. See `BUILD_HISTORY.md` for product checkpoints and `Source/Project/PROJECT_FORMAT.md` for the project format.

Orbit is a working name and has not had final product-name clearance. Images used as Orion and mixer design references are intentionally kept out of this source repository.

## Build on macOS

Requirements: Xcode, CMake 3.22 or newer, and a macOS SDK supported by the checked-out JUCE/Tracktion Engine revisions. Orbit's application source is C++; CMake is the build configuration that generates Xcode projects, and JUCE/Tracktion Engine are C++ libraries.

Clone Orbit and fetch the pinned dependencies:

```sh
git clone https://github.com/<owner>/orbit-daw.git
cd orbit-daw
git -c url.https://github.com/.insteadOf=git@github.com: submodule update --init --recursive --depth 1
```

Replace `<owner>` with the GitHub account or organization hosting Orbit. Tracktion Engine's JUCE submodule uses SSH; the HTTPS rewrite above allows fetching it without setting up GitHub SSH keys. Keep upstream dependency license files and notices intact. Exact revisions are listed in `DEPENDENCIES.md`.

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

## Build on macOS

Requirements: Xcode, CMake 3.22 or newer, and a macOS SDK supported by the checked-out JUCE/Tracktion Engine revisions. Orbit's application source is C++; CMake is the build configuration that generates Xcode projects, and JUCE/Tracktion Engine are C++ libraries.

Generate an Xcode project:

```sh
cmake -S . -B build/xcode -G Xcode
open build/xcode/OrbitDAW.xcodeproj
```

In Xcode, select the **Orbit** scheme and press **Run**. The build can also be run from Terminal with:

```sh
cmake --build build/xcode --config Debug --target Orbit
```

## Licensing

Orbit application code is released under GNU AGPLv3 (see `LICENSE`). JUCE is used under AGPLv3 and Tracktion Engine under GPLv3, subject to their respective license files and notices. Third-party dependencies inside either repository may have additional licenses; preserve those notices when distributing builds. This repository publishes source code; it does not include a compiled application bundle.
