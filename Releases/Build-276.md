# Orbit Build 276 / v0.3.255 — October 30, 2026

This tester refresh contains the major additions developed since the last public
Build 238. It carries the features tested locally through Build 274, with a fresh
build number for the native Mac, Windows and Linux release invocation.

## Major additions

- **Acid Bass:** native monophonic bass synth with saw/square, Accent and Slide,
  a ready-to-play demo pattern, and a saved oscillator-phase retrigger switch.
  Slides and live legato keep their phase continuous.
- **Acid Bass internal effects:** collapsible Drive/Distort/Low Cut/Tone/Output
  section, with Soft, Sine Fold and the new Triangle Foldback shape. Saved
  Soft/Sine Fold automation keeps its original meaning.
- **9W9 909 Audition:** eleven drum sounds, native sound/drive/FX controls,
  paged step sequencing, velocities, whole-kit step accents and a four-bar demo.
- **Geonkick Audition:** six percussion recipes using credited Geonkick DSP,
  native controls, independent instances and a velocity demo.
- **Song and pattern automation:** a dedicated embedded Song Event Editor,
  separate pattern curves, recording from native controls/MIDI, visible playback
  and loop ranges, gradient fills and editable loop boundaries.
- **LFO automation tool:** live-preview curve generation with tempo sync,
  waveform, amount, offset, phase, fades and curvature in Pattern/Song editors.
- **Native effects:** clean digital Peak Compressor, look-ahead limiter, original
  Tube Distortion and Two-Band Distortion, with native automation and saved state.
- **Independent workspace panels:** keep multiple generators and effects open,
  arrange them together, minimize to the window bar and restore their position.
  Transport and track changes preserve open/minimized panels.
- **Expanded Piano Roll:** group move/copy and selection tools, individual
  velocities, scale snapping, ghost notes, phrase editing, Mac clipboard/Undo
  shortcuts and a Hotkeys reference.
- **Expanded Playlist:** paint repeated patterns, resize from edges, navigate
  empty or existing slots, select/copy/move groups and transfer complete patterns
  between generators. Pattern lengths remain tied to the source pattern.
- **Recording:** configurable metronome count-in and audible Pattern MIDI
  overdubbing while the take loops; capture from hardware and clickable keyboards.
- **Mixer and routing:** VST3 effect browsing/inserts, channel/EQ/chain copy-paste,
  buses, sidechains, manual FX returns and independent effect editors.
- **Experimental Orion SFS recovery:** select whole generators, recover tentative
  notes/arrangement and retain missing-audio placeholders. Find Missing Media can
  search a chosen folder recursively and relink after save/reopen.

## Download and run — no compiler needed

Under **Assets**, choose the named package for your system. These are portable
app downloads, not setup-wizard installers. GitHub's automatic "Source code"
links contain source for developers rather than the app.

| System | Package | Run |
|---|---|---|
| Mac, Apple Silicon or Intel | `Orbit-Build-276-macos-universal.zip` | Extract; drag Orbit.app to Applications. |
| Windows 10/11, x64 | `Orbit-Build-276-windows-x64.zip` | Extract the whole folder; open Orbit.exe. |
| Linux, Ubuntu 24.04 x64 | `Orbit-Build-276-linux-x64.tar.gz` | Extract; run ./Orbit in a graphical/audio session. |

Each package includes corresponding source, licenses, third-party notices and
installation instructions. The separate `Orbit-Build-276-Source.zip` is a complete
Orbit-owned source baseline, with pinned JUCE/Tracktion dependency instructions.
Checksums accompany every package. Source archives contain required source sample
resources, not compiled plugins, apps, object files, caches or build directories.

## Mac first launch

Save/quit the old app before opening the new copy. This tester is ad-hoc signed,
not Developer ID signed or notarized. Try **System Settings > Privacy & Security
> Open Anyway** if macOS blocks it. If it remains blocked and you trust this
project's download, paste this app-specific command into Terminal:

```sh
xattr -dr com.apple.quarantine "/Applications/Orbit.app"
```

Then open Orbit again. Adjust the quoted path if installed elsewhere. This command
removes download quarantine from that app; it does not sign or notarize it. See
[Apple's guidance](https://support.apple.com/en-us/102445). Windows builds are
unsigned. Linux targets Ubuntu 24.04 libraries; see RUNTIME_LIBRARIES.txt.

## Quick tests

- Insert > Synths > Acid Bass: Demo pattern, Pattern, Play. Expand Effects / Drive,
  select Triangle Foldback and raise Distort above zero. For phase retrigger use
  the generator header triangle > Generator Settings, enable the switch, Apply.
- Insert > Drum Auditions > 9W9 909 Audition: Demo beat, Pattern, Play. Select a
  drum to edit its steps; right-click a step for velocity. Toggle Accent below.
- Open two instruments and some effects; arrange them side by side, then use
  Play/Stop and minimize/restore. Open the Event Editor and test automation.

## Scope and credits

These are early feedback builds. Test with copies of projects. Current saves use
schema 20; older builds reject them. Audio recording and Song-loop MIDI recording
remain unfinished; Song MIDI takes need 4/4, looping off and empty regions.
SFS recovery does not recreate Orion DSP, routing or original instrument/effect
sounds. The drum auditions are listening candidates, not the completed Pro-9
specification: 9W9 currently has one stereo output, without per-drum pan or
6/11-output variants; Geonkick is a limited native audition, not its complete kit
or preset system. ChowKick remains a separate comparison plugin and is not bundled.

9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9

The 9W9 integration is a modified Orbit port, not the original and not made or
endorsed by A Thousand Details. Its unaltered GPLv3 LICENSE and additional
section 7 NOTICE are included. Geonkick and Open303 authorship/provenance and all
required notices accompany the source and downloads. No proprietary Orion code,
manual, artwork or original instrument/sample library is bundled.

Local sanitizer and native host tests cover DSP, MIDI timing, automation,
project persistence and independent windows. Native CI builds are compilation
checks; physical devices, latency, third-party plugin compatibility and listening
still need tester feedback. Report build/version, OS/CPU, audio device, plugins,
exact steps, expected/actual behavior and logs in
[GitHub Issues](https://github.com/tjgoa/orbit-daw/issues).
