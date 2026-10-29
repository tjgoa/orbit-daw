# Orbit tester downloads

These are early feedback builds, not a production-ready DAW. Test with copies of
projects. Include BUILD.json's build/version, operating system/CPU, audio device,
plugin names/versions, exact steps, expected/actual results and any crash/scan log
when reporting a problem. Hardware playback, MIDI latency and third-party plugin
compatibility still need testing beyond automated checks.

## Download and run

- macOS: extract the ZIP and move Orbit.app to Applications. The universal binary
  contains Apple Silicon and Intel code. Its deployment target is macOS 11;
  older-OS compatibility has not been established by the local macOS 26 tests.
  This feedback package is ad-hoc signed, not Developer ID signed/notarized.
  macOS may require explicit approval in Privacy & Security to open it. A public
  notarized release needs an Apple Developer ID certificate and notarization.
- Windows: extract the ZIP to a folder and run Orbit.exe. The x64 build targets
  Windows 10/11 feedback testing. It is unsigned. Audio-driver/plugin behavior
  must be verified on a real Windows installation; compiling on a runner is not
  a hardware test. This build does not add a separately licensed ASIO SDK.
- Linux: extract the tar.gz and run ./Orbit. The build targets Ubuntu 24.04 x64
  and its system libraries; it is not an AppImage or a promise of compatibility
  with older distributions. RUNTIME_LIBRARIES.txt lists the linked libraries.
  Typical runtime packages include libasound2t64, libjack-jackd2-0, libfreetype6,
  libfontconfig1 and the X11 libraries listed there. A graphical/audio session is
  required; CI compilation does not establish desktop/audio operation.

No compiler is needed to run a tester download. The package includes licenses,
third-party notices and a complete Orbit-owned source ZIP. Fetch the pinned
external dependencies only if rebuilding; see DEPENDENCIES.md. Orion's manual,
artwork, sample libraries and other proprietary reference assets are not bundled.

## Creating the next tester build

On a native platform with CMake and the compiler installed:

    python3 Tools/prepare-dependencies.py
    python3 Tools/build-tester.py --platform macos-universal --install-current

Use windows-x64 or linux-x64 on their respective native machines. The default
invocation reserves the next unused product number before configuration or
compilation. A failed attempt consumes that number. Build label, CMake version
and tracking history advance together. Old tester packages are never overwritten.
The Mac install option preserves the previous Current app and leaves a running
user instance open; save/quit/reopen to load the new executable.

The GitHub workflow runs native Windows, Linux and universal Mac jobs from a
codex/tester-build-* branch. All three formats share that reserved invocation's
number. It verifies the reserved source metadata and fetches exact dependency
revisions, then applies Tools/tracktion-sampler.patch. It needs no signing secrets
and uploads downloadable artifacts, not a published GitHub Release. Workflow
permissions are read-only. Retrying a failed product build requires a fresh
number/source commit, not Re-run jobs under the same number.

Source-only backups remain the default for ordinary development. Binary tester
packages are produced only by the explicit tester-build command. It includes no
object files, debug symbols, caches, build directories or earlier archives.
