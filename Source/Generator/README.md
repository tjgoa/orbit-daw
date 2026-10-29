# Generator options

The downward-triangle button at the left of the generator header opens the
hierarchy shown in the supplied
`2026-10-29 00-39-11.mov`: Rename Generator, Generator Settings, Rename Current
Pattern, Arpeggiator/Shuffle, Link To Next MIDI Controller, and Receive MIDI From
Generator. It remains available when pattern controls are hidden or the generator
is rolled up. The separate Drum Rack window has the same dropdown in its own
top-left header, targeting that rack even when another generator is selected.
Dialogs keep the original generator ID and pattern slot when selection
changes. Opening or cancelling a dialog does not change notes or add a processor.

The arpeggiator follows the behaviors described in Orion's installed Help chapter
Additional Features > Arpeggiator and Shuffle, read through the running Orion app.
The installed manual is compiled HTML (`Help/Orion Help.chm`), rather than a PDF.
No Orion source code, manual text/images, proprietary arpeggio files, or internal
parameter tables are included in Orbit source or its archives.

## Arpeggiator and Shuffle

- On enables generator-wide arpeggiation before the instrument, for sequenced
  patterns and live MIDI. Disabling it restores direct MIDI playback.
- Reset on Chord Trigger restarts the sequence when a new chord follows silence.
  With it disabled, the sequence position continues across gaps. Panic clears
  sounding notes, sustained keys and queued events.
- Note Order provides Up, Down, Up/Down, Alternate Up (endpoint repetition),
  Alternate Down, and a user sequence of semitone offsets from the lowest note.
  These are Orbit's original implementations; the less-documented alternate
  traversals are not claimed to be identical to Orion's implementation.
- Range spans 1-5 registers. Distance sets the semitone interval between them.
  Out-of-MIDI-range generated notes are omitted, never clamped to another pitch.
- Gate changes duration relative to the chosen rate: Off/0 means one interval.
  Retrigger limits the note cycle; Repeats gives a finite cycle count or unlimited.
  The 16 rhythm buttons toggle steps between notes and rests.
- Tempo supports whole through 64th notes plus 8th/16th/32nd triplets. Shuffle
  delays alternate cells at its independently selected rate and works with the
  arpeggiator disabled. Direct-note shuffle delays matching note-offs as well,
  retaining their durations. Live shuffle uses the running processor clock;
  transport playback aligns that clock with the edit's musical position.
- Load MIDI imports 1-32 note pitches as a user order. It is a standard MIDI
  workflow, not a parser for Orion's undocumented arpeggio preset format. Files
  over 1 MiB and offsets beyond 48 semitones are refused without applying changes.
- Apply keeps the authored notes unchanged. The original processor runs in the
  audio graph before 4OSC, Drum Rack or the hosted instrument. It is excluded from
  the Mixer's audio insert slots. Audio rendering includes this processor.
- Render writes arp/shuffle results into the pinned current pattern for its
  existing cycle length, then disables generator-wide arp/shuffle to avoid double
  processing. One Undo restores notes and settings together. General generator
  transpose/velocity/channel controls remain playback settings. An all-rest rhythm
  intentionally renders silence. Rendering refuses patterns over 65,536 steps
  or output beyond 262,144 notes before changing the project.

Generator Settings exposes input channel, output channel, playback transpose and
velocity scale. Input channel uses the existing Advanced MIDI routing mode.
Output, transpose and velocity apply to sequenced and live MIDI. Authored notes
and the Piano Roll remain unchanged. These are host controls; synth-specific
polyphony, glide or tuning controls remain in the instrument's own editor.

## MIDI linking and receiving

Link To Next MIDI Controller offers generator volume, pan and the actual
instrument parameters. The next CC on an enabled input stores a device/channel/CC
binding to that generator's stable ID. Later generator selection does not retarget
it. Assignments use the existing user controller preferences; subsequent changes
to generator parameters participate in project undo. Cancel Controller Link
disarms learning. Deleted/missing generator targets are ignored.

Receive MIDI From Generator configures the engine's native track MIDI input.
The route is saved in engine state, with a project-generator-ID property for menu
checks and cycle detection. Feedback loops are refused before changes; other
connection failures restore the prior project/session snapshot. None disconnects
the source. Connection/disconnection are undoable. The current engine provides
one destination per track MIDI source; a source already feeding another generator
is refused instead of silently displacing that route.

This route receives the source track's MIDI output. Plugins that consume incoming
MIDI without forwarding/outputting it may therefore provide no MIDI to receive.
Track MIDI routing is an engine live/transport path; its graph builder excludes
track-device sends during offline audio rendering. Offline rendering of these
inter-generator routes remains a separate engine-integration task. Ordinary
generator arpeggiation/shuffle itself is processed during offline rendering.
MIDI file export continues to export authored pattern notes; use Render first to
export baked arp/shuffle notes.

## Verification

With an existing Orbit CMake build:

```sh
python3 Tools/test-options-host.py --sanitize --generator
```

The harness uses production generator/menu/settings/processing code and actual
engine plugin instances with isolated preferences. It substitutes dialog display,
native window centering and hardware initialization. Checks cover musical output,
audio-block MIDI timing/panic, non-destructive Apply, Render/Undo, general settings,
names, identity-pinned controller learning, native MIDI routing and feedback
refusal, save/reopen and panel images. Regression checks also exercise the visible
header button with hidden pattern controls/rolled-up generators, and open the
Drum Rack menu while another generator is selected before rendering its remembered
pattern. Existing encoding and headless device debug
assertions are logged separately; this is not a hardware or native UI certification.
See `ORION_MANUAL_AUDIT.md` for the reference review and remaining work.

Build 235 desktop check: the drawn header triangle was inspected in an isolated
GUI harness with separate settings and hardware initialization disabled. Native
popup opening was not confirmed: automated clicks on both the dropdown and
unrelated test-window buttons left the view unchanged. The isolated window was
closed; the user's running app/project was not closed. Restart the Current app
after saving to load Build 235, then choose the top-left triangle >
Arpeggiator/Shuffle to test audible playback.
