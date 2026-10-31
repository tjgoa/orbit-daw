Early tester checkpoint **Build 238 / v0.3.217 — October 29, 2026**.

Download the named platform file under **Assets**. These are portable feedback
packages, not production installers. No compiler is needed. GitHub's automatic
"Source code" links are for rebuilding, not running the application.

| System | Download | Install |
| --- | --- | --- |
| Mac, Intel or Apple Silicon | `Orbit-Build-238-macos-universal.zip` | Extract; drag `Orbit.app` to Applications. |
| Windows 10/11, x64 | `Orbit-Build-238-windows-x64.zip` | Extract; open `Orbit.exe`. |
| Linux, Ubuntu 24.04 x64 | `Orbit-Build-238-linux-x64.tar.gz` | Extract; run `./Orbit` in a graphical/audio session. See `RUNTIME_LIBRARIES.txt`. |

Each platform package includes licenses, notices and corresponding Orbit-owned
source with pinned external dependency instructions. The separate
`Orbit-Build-238-Source.zip` is a complete source-only baseline. `SHA256SUMS.txt`
and per-package checksums accompany the downloads. The three native CI builds
must succeed and their existing packages are verified before publication; the
publication workflow does not rebuild the product.

### Mac first launch

Save/quit an older Orbit, extract the ZIP, and move the extracted `Orbit.app` to
**Applications**. This tester is ad-hoc signed, **not Developer ID signed or
notarized**. If macOS blocks it, try opening it once, then use **System Settings >
Privacy & Security > Open Anyway** and confirm. See
[Apple's guidance](https://support.apple.com/en-us/102445).

If the downloaded app is still blocked and you trust this project's download,
paste this app-specific command into Terminal, then open Orbit again:

```sh
xattr -dr com.apple.quarantine "/Applications/Orbit.app"
```

It removes download quarantine only from this Orbit app; it does not sign or
notarize it. Adjust the quoted path if you installed elsewhere. Windows builds
are unsigned. The Mac deployment target is macOS 11, but older-OS compatibility
has not been established by the local macOS 26 tests.

### What is implemented

- Pattern/Song MIDI take recording, generator arming, sustain-to-duration,
  independent input/duration quantization, one-take Undo/Redo and native reopen.
- **Record without a MIDI keyboard:** select an instrument, choose Pattern mode,
  open Piano Roll, press Record and click/hold or drag on its left-side piano
  keys. Pending Pattern notes appear immediately; Stop commits the take. The
  host instrument keyboard and F12 virtual keyboard also record.
- Short black keys over continuous white-key tails, matching click hit-testing,
  key feedback and full MIDI 0–127 scrolling in both blue workspace themes.
- **Options > Wallpapers > Open Wallpaper Folder...** opens a persistent image
  library. Add PNG/JPEG/GIF files and reopen Options to see them. Choose Image
  copies and selects an image while preserving originals and same-name files.
  Mac folder: `~/Library/Application Support/Orbit/Wallpapers`.
- Working Edit and Options tools, coordinated Navy Blue/Light Blue themes,
  generator header dropdown, settings, arp/shuffle and arp Render.
- Existing VST3 instruments, built-in 4OSC, native Drum Rack, pattern banks,
  Playlist, Mixer/Master, MIDI import/export and `.orbit` project persistence.
- Experimental SFS recovery via File > Merge Project: previewed supported note,
  tentative placement and explicitly relinked dry-audio recovery. This is
  partial recovery, not faithful Orion instrument/effect reconstruction.

### Known limits and current work

- Song MIDI recording currently needs **4/4, Song looping off, and an empty
  region on every armed generator**. Pattern mode is the simplest recording
  test. Pending notes are committed on Stop; earlier overdub passes become
  audible after commit. Held Pattern preview is currently for Pattern takes.
- Audio recording, count-in/punch/replace and controller/automation performance
  capture remain unfinished. A conflicted take is retained in memory, not in
  crash recovery; resolve and commit it before closing.
- Event Editor means automation, separate from Piano Roll. The current combined
  View entry is wrong and logged for the manual's View/Event Editor section.
- SFS instruments/effects, routing, VST internal-state compatibility and Drum
  Rack restoration remain unsupported. No proprietary Orion DSP/assets ship.
- Other missing manual features remain open. This release does not claim
  complete Orion parity or production readiness. Test with copies of projects.

Piano Roll mouse-recording, keyboard render/hit-testing, wallpaper copying and
menu refresh, Options and Pattern/Song recording checks passed locally with
ASan/UBSan. CI compilation is not a physical audio/MIDI or plug-in hardware test.
Those need tester feedback.

Report the build/version, OS/CPU, audio device, plug-ins and versions, exact steps,
expected/actual results and crash/scan logs through
[GitHub Issues](https://github.com/tjgoa/orbit-daw/issues). Read `INSTALL.md` for
installation, recording and wallpaper instructions.
