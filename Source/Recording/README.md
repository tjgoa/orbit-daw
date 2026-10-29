# MIDI take recording

Record captures live MIDI and Virtual MIDI Keyboard notes into Orbit's native
patterns. Playlist R arms instrument generators. With no explicit arms, Record
uses the selected instrument. During a take, input stays with the armed/pinned
generators and respects their input channels. Audio-track recording is separate.

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

New notes are committed at the end of the take, so an earlier overdub pass is not
heard on the next loop until committed. This does not implement count-in,
punch-in/out, replace mode, audio recording, controller/automation performance
recording, sostenuto/soft-pedal capture, or Song pattern-launch performance capture.
Those are remaining workflows, not completed Orion parity.

Run `python3 Tools/test-options-host.py --sanitize --recording` after a normal
CMake build. The check uses actual host input/capture/commit/persistence methods
with isolated settings and audio hardware disabled. It checks source timing,
full-range notes, sustain/device overlap, loop split/region bounds, quantization,
armed routing, atomic conflict retry, one undo/redo and native save/reopen.
