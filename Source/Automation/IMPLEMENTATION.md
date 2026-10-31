# Automation completion checklist

Read EventEditor.htm and MenuView.htm in the installed Orion manual. Complete
implementation and meaningful playback/persistence tests before advancing chapters.

## Complete through Build 254

- Dedicated Song Event Editor, track/plugin/parameter selection and native units.
- Draw, Line, Erase, whole-graph Move, independent curve clipboard, Clear/Smooth/Scale.
- One gesture Undo, Escape cancel, stale-source protection, persisted instance IDs.

## Current user requests

- [x] Song playback cursor, matching loop section and follow behavior.
- [x] Optional gradient fill beneath the Song curve, like Pattern mode.
- [x] Repeat Pattern plugin automation at arranged pattern occurrences, with explicit Song precedence.
- [x] Parameter events from host volume/pan, Acid Bass and generic parameter controls.
- [x] Build 256: direct EQ/send event menus; send must first be created by adjusting it.
- [ ] Instance-specific Pattern lane identity for duplicate-name plugins.
- [x] Master and bus/return automation targets.
- [x] Capture live mouse/MIDI parameter movements with explicit time/write/loop policy and one-take Undo.
- [ ] Song tempo automation and correct time conversion/playback/render/export.

The user also requested simultaneous generator windows (Acid Bass + Drum Rack).
That separate window architecture task is tracked in ORION_LEARN_TODO.md.

## Build 255 validation and limits

ASan/UBSan host tests exercise native curve conversion, fractional take storage,
stop/punch-out handoff, one-take Undo, lazy send/EQ discovery, Pattern repetition
and Song precedence, module persistence, view preferences and parameter entry.
Hardware is disabled in these tests: physical MIDI, audio-device scheduling and
listening checks remain manual. Tempo remains unimplemented; the next manual
chapter may proceed under the user's autonomous-work instruction while that
larger timing task remains explicitly open.
