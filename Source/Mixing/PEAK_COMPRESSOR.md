# Peak Compressor — original utility dynamics

Build 263 adds **Insert > FX Dynamics > Peak Compressor**, also available in
Mixer/Master insert choosers. This is an original clean digital peak compressor
inspired by the CV-82 manual's functional controls. It contains no imported
Signalsmith, Airwindows, sndfilter or proprietary Synapse compressor code.
It does not claim an exact CV-82 sound, timing calibration or legacy preset match.

Synapse's Orion 8.5 announcement confirms that CV-82 upgraded Compressor 8:
https://www.synapse-audio.com/news-orion-studio-85-isoutnow.html . This confirms
the product history, not the implementation. The installed Orion manual supplies
functional behavior. The detector bell uses a self-written implementation of
the published RBJ peaking-EQ equations: https://www.w3.org/TR/audio-eq-cookbook/ .

| Control | Range/default | Behavior |
| --- | --- | --- |
| Input | -24 to +36 dB / 0 dB | Gain before compression and the internal detector. |
| Threshold | -60 to 0 dB / -18 dB | Level above which compression applies. |
| Ratio | 1:1 to INF:1 / 4:1 | Hard-knee straight-line transfer in dB; the top value selects infinity. |
| Output | -24 to +24 dB / 0 dB | Gain after the blended signal. |
| Detector EQ | -12 to +12 dB / 0 dB | A 1 kHz bell, Q 0.707, in the detector only. Frequency/Q are Orbit choices, not measured Synapse values. |
| Attack | 0.1 to 200 ms / 10 ms | Values at/below 0.5 ms use instant gain tracking with look-ahead anticipation. Longer values allow transients through. |
| Release | 10 to 2000 ms / 150 ms | Exponential recovery of gain reduction. |
| Dry/Wet | 0 to 100% / 100% | Both paths share the same approximately 5 ms delay; Input and Output gain apply to both. |

The detector uses the larger absolute channel level, so opposite-polarity stereo
cannot cancel its trigger. Look-ahead gain anticipation and attack/release smooth
the gain change. Input/output gain, threshold, ratio and mix changes are smoothed.
There is no saturation, waveshaping, modeled analog circuitry or output clipping.
A 1:1 ratio with neutral gain passes the delayed waveform, including samples above
0 dBFS. Use the separate limiter afterward for a final sample-peak ceiling.

Sidechain uses Orbit's existing source selector and feedback checks. A selected
external source controls gain independently of the audible input, passes through
the detector EQ, and is not mixed into the destination. Disconnect returns to
self detection. Input/output and wet-stage gain-reduction readouts follow recent
blocks. The generic host sliders now honor parameter range skew and give one
Orbit Undo per drag (automation recording retains its one-take Undo policy).

Native parameters have Pattern/Song event entry, instance identity, settings
copy and save/reopen. Schema 14 requires Build 263 or newer for new saves so
older hosts cannot drop this native processor. Existing Compressor plugin types
and their saved parameter meanings are preserved. No SFS effect-state mapping to
this new type is asserted.

## Validation

```sh
clang++ -std=c++17 -O2 -g -fsanitize=address,undefined \
  Tools/test-peak-compressor.cpp -o /tmp/orbit-peak-check
/tmp/orbit-peak-check
python3 Tools/test-options-host.py --sanitize --peak-compressor
```

Core checks cover 44.1/48/96/192 kHz, the dB transfer curve, unity/no clipping,
anti-phase stereo, instant/infinite ratio, dry/wet gain, external isolation,
detector-only EQ, timing/recovery, nonfinite data and sample-rate reset.
Native tests cover 44.1/48/96 kHz transparent PDC at three blends, external
right-channel triggering of anti-phase audio, no trigger leak, parameter display,
slider mapping, one-gesture Undo/Redo, real Song automation rendering, state
copy and same/fresh-process reopen. Human listening and matched Orion measurements
remain open; numerical checks do not establish a sonic clone.

## Export offset correction

The new tests begin audio at the export's first sample. They exposed a native
engine bug: `NodeRenderContext::writeAudioBlock` ignored the frame-view offset
when wrapping its buffer. That wrote the wrong first portion after dropping
plugin latency. Orbit's small `Tools/tracktion-export-offset.patch` fixes the
buffer constructor to respect the selected offset. The limiter regression now
includes a first-sample impulse; it previously began its useful signal later.
The pinned dependency revision is unchanged and the overlay is reproducible.

## Other implementation options evaluated

- Signalsmith Basics Dynamics (MIT): https://github.com/Signalsmith-Audio/basics .
  Includes timing, look-ahead and blend; its energy-based detector differs from
  the peak model chosen here. It remains a fallback, not a dependency.
- sndfilter (currently 0BSD): https://github.com/velipso/sndfilter . Another digital
  dynamics implementation with pre-delay and adaptive release to evaluate.
- Airwindows Pressure5 (MIT): https://github.com/airwindows/airwindows . A different
  compression character; not assumed to match a transparent CV-82-style utility.

Any future adoption needs review of the selected files, dependencies and notices.
The original implementation was selected for direct control of the desired peak
behavior and minimal new dependencies.

## Dedicated panel — Build 268

Peak Compressor uses an original dark rack panel with eight metal-style rotary
controls, value entry, gain-reduction needle and input/output/reduction readouts.
The sidechain chooser stays available below the controls. The panel follows
native automation and preserves gesture grouping/Undo, range skew, infinite
ratio display and parameter-event context menus. The DSP and stored parameters
are unchanged. This is an Orbit vector design using the Orion compressor
reference for layout direction, without copied brand artwork.

## Window controls — Build 269

Each effect instance owns an embedded panel. Insertion opens it automatically;
opening another effect preserves its panel. Minus hides to a persistent bottom
window-bar tab. The tab restores the same instance and position. X now removes
that effect through the normal Undo transaction, per the updated user request.
Other effects and generator/editor panels stay open. Unrelated Undo rebuilds
restore each surviving effect panel's bounds and minimized state.
