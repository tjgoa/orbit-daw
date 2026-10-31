# Orbit tester downloads

These are early feedback builds, not a production-ready DAW. Test with copies of
projects. Include BUILD.json's build/version, operating system/CPU, audio device,
plugin names/versions, exact steps, expected/actual results and any crash/scan log
when reporting a problem. Hardware playback, MIDI latency and third-party plugin
compatibility still need testing beyond automated checks.

## Download and run

Download the platform package from [GitHub Releases](https://github.com/tjgoa/orbit-daw/releases).
These are portable app packages, not setup-wizard installers. No compiler is needed.
Choose the named Mac ZIP, Windows ZIP or Linux tar.gz under Assets; GitHub's
automatic "Source code" downloads contain source rather than the runnable app.

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

### First launch on Mac

1. Extract the Mac ZIP, open its extracted folder and drag **Orbit.app** into
   **Applications**. Save/quit an older running Orbit before opening the new copy.
2. Try opening `/Applications/Orbit.app`. If macOS blocks this early tester build,
   use **System Settings > Privacy & Security > Open Anyway**, then confirm.
   See [Apple's instructions](https://support.apple.com/en-us/102445).
3. If the downloaded app is still blocked, testers who trust this project's
   download can paste this app-specific command into Terminal, then open Orbit:

   ```sh
   xattr -dr com.apple.quarantine "/Applications/Orbit.app"
   ```

   This removes the download-quarantine attribute only from that Orbit app. It
   does not sign or notarize it. Keep the quoted path; adjust it if you installed
   somewhere else. There is no need to disable Gatekeeper for your whole Mac.

### Test MIDI recording without a MIDI keyboard

Insert/select an instrument, choose **Pattern** mode, open **Piano Roll**, and
press **Record**. Click/hold or drag on the piano keys at the left. Pending notes
appear while you record. Press **Stop** to commit, Play to hear the pattern, and
Undo to remove the whole take. The host instrument keyboard and F12 virtual
keyboard also record. Song mode currently needs **4/4**, **Song looping off** and
an empty region on the armed generators. Audio recording is unfinished.

### Add wallpapers

Choose **Options > Wallpapers > Open Wallpaper Folder...**, place PNG/JPEG/GIF
images there, then reopen Options to see them in the menu. **Choose Image...**
also copies an image into this folder and selects the copy. Originals and
same-name library images are preserved. Fit, Fill and Tile control placement.
The Mac folder is `~/Library/Application Support/Orbit/Wallpapers`; Windows uses
`%APPDATA%/Orbit/Wallpapers`, Linux `~/.Orbit/Wallpapers`. Images must be at most
32 MiB and 8192 pixels per side.

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
revisions, then applies the sampler and export overlays with Tools/prepare-dependencies.py. It needs no signing secrets
and uploads downloadable artifacts, not a published GitHub Release. Workflow
permissions are read-only. Retrying a failed product build requires a fresh
number/source commit, not Re-run jobs under the same number.

Release publication is separate: `Tools/publish-tester-release.py` promotes the
three artifacts from an explicitly pinned successful run, checks their digests,
build metadata and matching source contents, and publishes a prerelease only
after uploading every download, source, installation guide and checksums. It
does not compile a product or consume a new product build number. The publishing
workflow grants only its job contents-write/actions-read permissions.

Source-only backups remain the default for ordinary development. Binary tester
packages are produced only by the explicit tester-build command. It includes no
object files, debug symbols, caches, build directories or earlier archives.
