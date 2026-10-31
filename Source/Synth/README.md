# Acid Bass

Insert > Synths > Acid Bass adds a native monophonic acid instrument. Click
**Demo pattern**, then Pattern Play, for a short original bassline. An existing
pattern is preserved: the demo chooses an unused slot, or refuses if all are used.
Return with **Instrument** above the Piano Roll to tweak its eight controls.

Tune covers -12..+12 semitones. Cutoff is 30..10000 Hz, Reso and Env Amt 0..100%,
Decay 30..3000 ms, Accent and Acc Env 0..100%, Slide 1..400 ms. The waveform
button switches saw/square. Acc Env increases accented filter recovery from
30 to 900 ms; left recovers faster. Knob changes are smoothed where appropriate,
saved as native automatable parameters, and participate in host Undo.

## Oscillator phase — Build 273

Open the generator header's left-hand triangle, choose **Generator Settings…**,
then enable **Retrigger oscillator phase (slides stay continuous)** and Apply.
This setting belongs to that Acid Bass instance. Every fresh note restarts the
oscillator at phase zero. Pattern slides, same-pitch ties and overlapping live
MIDI legato keep phase continuous. The switch changes the next fresh note; it
does not abruptly reset a currently held note or clear filter/envelope tails.

It defaults Off, preserving the original Open303 behavior, including its reset
when a note starts from idle. Off is therefore the original mode, not a promise
that the oscillator never resets. The native `phaseRetrigger` parameter survives
save/reopen and participates in host Undo and parameter automation. Existing
synth/FX parameter IDs and the original Soft/Sine Fold range remain unchanged.

The Piano Roll shows **Accent** and **Slide** rows only for Acid Bass. Each cell
uses the current grid subdivision and scrolls with the notes. Click to toggle;
drag to paint the same state with one Undo. Changing subdivisions does not
rewrite switches; clearing half a larger cell preserves its other half.
Accent changes volume/filter response independently of authored note velocity.
An outgoing Slide holds the gate to the following note without retriggering its
envelopes, including same-pitch ties. A rest releases the held gate. The Slide
knob controls portamento time. Live MIDI uses last-note priority and overlapping
notes for legato, with velocity 100+ accented; sustain and Panic are supported.

Switch intervals are optional Articulations metadata in native pattern slots.
Native save/reopen, project Undo and full-pattern clipboard copies retain them.
Selected-note edits leave these step switches anchored to the pattern timeline.
The engine emits internal CC119 markers to Acid Bass only, with separate Accent
and outgoing Slide bits. Authored notes/velocities remain unchanged. Existing
MIDI export exports authored notes; full articulation export to other synths is
not implemented. An explicit outgoing Slide can survive the loop-boundary reset when the loop
starts with another note; Stop/Panic still clear it. Arp/shuffle can change the
authored note/marker timing. This implementation does not claim exact Orion parity.

DSP: pinned MIT Open303 by Robin Schmidt, with Ooura FFT and documented
portability/safety patches in ThirdParty/Open303. No Orion source or UI assets
are used. Orbit's engine wrapper, fixed-capacity MIDI state and interface are
original additions. Native third-party notices ship with the app.

Validation uses the pure DSP sanitizer check and the production host profile:

```sh
clang++ -std=c++20 -O1 -fsanitize=address,undefined \
  Tools/test-acid-bass.cpp Source/Synth/AcidBassVoice.cpp \
  Source/Synth/ThirdParty/Open303/*.cpp -o /tmp/orbit-acid-voice-check
UBSAN_OPTIONS=halt_on_error=1 /tmp/orbit-acid-voice-check
python3 Tools/test-options-host.py --sanitize --acid --sfs-file /path/to/Song1.sfs
```

The local Song1 fixture is not distributed. The host check disables hardware and
native desktop windows. Listening, device latency and native plugin compatibility
remain hands-on checks; the established headless JUCE assertions remain logged.
