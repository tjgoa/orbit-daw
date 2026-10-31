# 9W9 909 Audition — Build 272

9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9

This is a modified Orbit port, not the original and not made or endorsed by
A Thousand Details. The pinned upstream engine is GPL-3.0 with additional
section 7 terms; retain the unaltered LICENSE and NOTICE in ThirdParty/9W9.
The source, samples, hashes and Orbit modifications are recorded there.

## First test

1. Reopen the locally installed Build 272. Choose **Insert > Drum Auditions >
   9W9 909 Audition**. The generator opens its own panel.
2. Click a drum name or **Hear hit** to preview without starting playback.
   There are eleven sounds: kick, snare, three toms, rim, clap, two hats, ride
   and crash. Open and closed hats choke each other.
3. Click **Demo beat**, select **Pattern**, and press **Play**. It creates a
   four-bar pattern exercising all eleven sounds. An occupied pattern is kept;
   the demo uses an empty slot. With no free slots it reports that condition.
4. Select **Closed Hat**, **Snare** or another drum to see its lit steps. Click
   a step to add/remove a hit; right-click for velocity 32/64/96/127 or Remove.
   Use the length selector and arrows to edit the next sixteen steps. The
   orange **Accent** switches affect every drum sounding on that step.
5. Adjust the selected drum's knobs. Values appear while dragging; double-click
   resets a knob. Right-click opens its Event menu for Pattern/Song automation.
6. Try **Master**, **Reverb** and **Delay**. Each sound except kick has Rev/Dly
   sends. Turn up a sound's send to hear that return. Delay follows host tempo.
   Per-drum Drive/Distortion and the separate master distortion/compression
   controls are upstream 9W9 processing, not the Acid Bass distortion effects.
7. Insert another instance or Geonkick for comparison. Keep both panels open,
   and check that Play/Stop and selecting another track keep their placement.
   Save/reopen a `.orbit` project to retain sound controls, notes and accents.

Use the Piano Roll for note timing, lengths, velocities, and normal note edits.
This kit's Accent switches are in its step panel; Acid Bass's Slide row is not
a 909 articulation. Stop/Panic silences drum and effect tails. Note-off normally
lets the drum's one-shot finish.

## Controls and MIDI

Most controls use the original 0–127 panel range, rather than Hz/ms labels.
Distortion and time-division knobs show their named choices while dragging.
**BD Tune** is the upstream sweep control; **BD Pitch** and **BD P. Depth** are
separate pitch controls. Snare Tone shapes the noise decay; shell decay remains
fixed as upstream. This panel exposes the upstream controls rather than claiming
Orion's exact Pro-9 parameter ranges.

The upstream Master **Accent** is a global gain/velocity ceiling, affecting all
hits. Orbit's separate **Step Accent** adds up to 100% gain to accented steps
(default 50%). Master **Velocity** changes velocity sensitivity. The distinct
step controls do not reinterpret the original engine's master Accent parameter.

The default **Note Map** is General MIDI. Set Note Map on the Master page before
programming; changing maps changes note interpretation, not the stored notes.

| Sound | General MIDI | 9W9 Pads |
|---|---:|---:|
| Bass Drum | 36 | 36 |
| Snare | 38 | 37 |
| Low Tom | 41 | 38 |
| Med Tom | 47 | 39 |
| Hi Tom | 50 | 40 |
| Rim Shot | 37 | 41 |
| Hand Clap | 39 | 42 |
| Open Hat | 46 | 44 |
| Closed Hat | 42 | 43 |
| Ride | 51 | 46 |
| Crash | 49 | 45 |

GM aliases: 35 kick, 40 snare, 45 low tom, 48 middle tom, 43 high tom. The panel
shows the chosen drum's MIDI number so octave naming differences do not matter.

## Scope and checks

This is a playable engine audition with one stereo output (duplicated mono).
Seven voices use synthesis; hats/cymbals use the three unmodified upstream WAV
resources compiled into immutable tables. There is no missing-sample path to
resolve. The adapter corrects playback rate for the source's 44.1 kHz samples.
No independent hardware-ROM provenance or exact Roland/Orion equivalence is
claimed. Per-drum pan, Pro-9's extended controls and 6/11-output versions remain
future work. The existing Pro-9 menu placeholder is not this instrument.

Automated checks cover all eleven sounds at five sample rates, block-size
invariance, velocity/accent, hat choke, sample pitch, independent instances,
finite extremes, panic, and allocation-free callback paths. Native host checks
exercise MIDI sample positions, step/knob/Event bindings, audio automation,
simultaneous panels, preview, transport, and fresh-process project reopen.
Listening, hardware CPU testing, and actual Windows/Linux builds remain manual.
Run Tools/test-ninekit.py and the native profile in Tools/test-ninekit-host.cpp
with Tools/test-options-host.py to reproduce the checks.
