# MIDI take recording

Record captures live MIDI, the left-side Piano Roll keyboard, the instrument's
host keyboard and Virtual MIDI Keyboard notes into Orbit's native
patterns. Playlist R arms instrument generators. With no explicit arms, Record
uses the selected instrument. During a take, input stays with the armed/pinned
generators and respects their input channels. Audio-track recording is separate.

Build 240's optional Piano Roll scale snapping constrains its clickable keyboard
before audition/capture, so those recordings contain the heard scale pitches.
The user chose Piano Roll only; hardware and separate virtual/host keyboards
retain their received pitch. See ../Editing/README.md. A Playlist ruler click
places the Song start cursor without changing loop state; deliberate ruler
dragging still creates/enables a loop. Song MIDI recording's existing loop-off
requirement is unchanged.

## Build 251: count-in and audible Pattern overdub

Open **Options > Recording Settings...** and choose a count-in. In 4/4 the
choices are **0, 1, 2, 4 or 8 quarter notes**. Zero starts immediately. The two
longer choices follow the initial meter numerator (for example 3/6 in 3/4),
with the displayed values always meaning quarter notes. This preference persists.
Count-in applies when Record starts a stopped transport; Record during playback
starts capturing immediately. The engine runs the preroll and click schedule;
there is no GUI-timer metronome. The engine includes a short setup lead before
the first click. The status bar counts down the remaining quarters.

Count-in sounds with METRO off; it then goes quiet. Enable METRO to keep clicking
afterwards. Notes played during the count-in audition normally but are excluded
from the take, including a held note whose release occurs after recording starts.
Stopping/canceling a silent count-in restores the punch cursor without new Undo.
After starting transport, capture uses its live playback position to align the
monotonic input clock rather than waiting for the transport display to refresh.
Physical input/output latency still needs device-specific testing/calibration.

In Pattern mode, completed notes now replay on subsequent passes **while the
take remains active**. Keep Record on and layer another phrase. A held/sustained
note continues sounding live; its loop replay begins after its release, on the
next occurrence that has not already passed. Earlier recorded passes remain in
the runtime replay when another note is completed. Replay enters the existing
generator arp/transpose/velocity/channel path. It is not captured again.

The runtime replay stores no note data in plugin state or the native document.
It has bounded note-off storage, publishes prepared sorted notes outside the
audio callback and uses a nonblocking audio-side lock. Stop, commit and panic
flush its held notes. Offline rendering excludes this temporary replay. A failed
commit retains the take in memory as before, but stops its runtime replay.

Authored Pattern MIDI clips now repeat through a shared cycle for different
pattern lengths, matching the capture/replay modulo. The cycle is their least
common multiple, bounded to 512 bars for recording; an incompatible longer cycle
is refused before recording. This is Pattern playback, not Song-loop recording.
Pattern automation curves remain within the separately documented Automation scope.

To test without hardware: select an instrument, choose Pattern mode, open its
Piano Roll, press Record, then click/hold or drag across the keyboard at the left.
Pending notes appear while recording; Stop commits the take. Undo removes that
whole take. Without Record, the keys audition without adding notes. The separate
virtual keyboard (F12) also supports mouse/computer-key input. In Song mode,
first turn Song looping off and move to an empty region on the armed tracks.

- Pattern mode adds the finished performance to each destination's remembered
  pattern. Existing notes remain intact. Crossing its loop boundary splits a
  note into tail/head pieces; a single held note is capped at one pattern cycle.
- Song mode chooses fresh unused pattern slots and places takes at the current
  cursor's bar. Note offsets retain the cursor's position within that bar.
  The clip keeps the recorded silent tail through Stop. Capture is bounded by the
  next occupied region/end of the 512-bar Playlist.
  This initial Song recorder requires 4/4 and Song looping off.
- Stop or Record again commits all notes/placements as one undoable transaction.
  A silent take creates no undo. Conflicting edits retain the take in memory and
  leave the document untouched; remove the conflict and press Record/Save to retry.
  Pending takes are not crash-recovery files. Finish/commit before closing Orbit.
- Input arrival uses a monotonic timestamp recorded in the device callback, so a
  busy UI queue does not move note times. Virtual input uses the same clock.
  The take captures the tempo at its start. Changing tempo, mode, seeking,
  panic/input policy, explicit Save and project replacement finish the take first.
  Automatic save waits until recording finishes. Latency calibration and physical
  device accuracy still need real hardware checks.
- Sustain (CC64) becomes note duration. Different devices/channels retain separate
  held-note identities. Stop flushes held notes. Input quantization uses the
  existing Quantize policy; start and duration quantization are independent.
- MIDI is recorded before generator arp/transpose processing. Use the existing
  arpeggiator Render action afterwards to bake its result.

Pending Pattern notes, including held durations, are drawn without changing the
document. Completed notes are heard on later passes through runtime replay; the
native note data is committed only at the end of the take. This does not implement
general punch-in/out, replace mode, audio recording, controller/automation performance
recording, sostenuto/soft-pedal capture, or Song pattern-launch performance capture.
Those are remaining workflows, not completed Orion parity.

Run `python3 Tools/test-options-host.py --sanitize --recording` after a normal
CMake build. The check uses actual host input/capture/commit/persistence methods
with isolated settings and audio hardware disabled. It checks source timing,
full-range notes, sustain/device overlap, loop split/region bounds, quantization,
armed routing, atomic conflict retry, one undo/redo and native save/reopen.

The recording check also renders actual Generator MIDI plugin blocks through
three passes, publishes another note mid-take, checks exact on/off positions and
stop flushing, verifies native count-in click messages at sample-derived block
boundaries, excludes precount notes, cancels silent precounts, checks mixed-length
clips and confirms saved count-in choices in a fresh process. The Generator
profile verifies replay through MIDI transformation and offline-render exclusion.
It does not open physical audio devices or certify their latency.
