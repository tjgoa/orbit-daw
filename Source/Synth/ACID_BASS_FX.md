# Acid Bass internal FX — Builds 270 / 274

An original optional post-synth drive section. This is an ABL-inspired control
workflow and sine-fold experiment, not AudioRealism DSP or a verified sonic clone.
The synth's existing Open303 voice, Accent/Slide and first nine parameter IDs are
unchanged. No additional third-party DSP source is imported.

Click **Effects / Drive +** to reveal five knobs and the shape selector:

| Control | Function | Range / default |
| --- | --- | --- |
| Drive | Nonlinear input intensity | 0–100%, default 35% (up to 36 dB internal boost) |
| Distort | Dry/wet blend | 0–100%, default **0%** |
| Low Cut | High-pass applied to the wet signal before shaping | 20–1500 Hz / 30 Hz |
| Tone | Wet output low-pass | 500–20000 Hz / 12000 Hz |
| Output | Level after dry/wet blend | −24 to +12 dB / 0 dB |
| Shape | Soft, Sine Fold or Triangle Foldback | Soft |

The FX foldout changes only the owning generator window's height and restores
its previous height when collapsed. It does not enable/disable audio. Set Distort
above zero to hear the effect. Parameter labels remain visible; numeric boxes
are removed and knob values pop up during dragging. Existing right-click Event
menus, double-click defaults, grouped Undo and native automation remain available.

## DSP

`AcidBassFx.h` uses JUCE's existing 4× FIR oversampling. The wet chain is high-pass,
`tanh(x)` / `sin(x)` with smoothed mode crossfade, DC rejection and a low-pass.
Build 274 adds an original piecewise-linear triangle foldback. Its normalized
transfer is `4 * abs(t - floor(t + 0.5)) - 1`, with `t = (x + 1) / 4`.
It passes the central -1..1 region linearly, then folds alternately at subsequent
thresholds. The triangle selector crossfades over the same 5 ms smoothing as
the other controls, before DC rejection and Tone. The waveform is a triangle
folding characteristic; it does not change the synth oscillator waveform.

Drive maps to 0–36 dB of boost; a deliberately chosen `gain^-0.25` output scaling
moderates the level increase. This compensation is an original design choice,
not a reconstruction of AudioRealism's power-compensation circuit.

The dry signal is delayed to match the integer oversampler latency (61 samples
with the current JUCE configuration), then blended before Output. The native
synth declares this constant latency even at Distort 0%, allowing the engine's
plugin delay compensation to keep timeline placement stable during automation.
At Distort 0% and Output 0 dB, samples are unchanged apart from that declared delay.
Turning delay compensation off deliberately exposes the processor latency.

No allocations occur inside processing. Buffers are prepared ahead of time;
blocks exceeding the advertised size are split. Controls use 5 ms smoothing;
non-finite input/control values are sanitized. Preparation, panic, stop and
non-loop seeks clear FX history. Real loop wraps preserve it, including outgoing
Slide ties. Fourfold oversampling reduces aliasing; extreme fold settings can
still produce alias components and require listening at the intended rate.

## State and compatibility

Native IDs appended after `waveform`: `fxDrive`, `fxDistort`, `fxLowCut`, `fxTone`,
`fxOutput`, `fxMode`. Missing fields load defaults, especially Distort 0%. Preset
restoration updates native parameters rather than only their cached properties.
Schema 16 prevents earlier Orbit readers from silently discarding new FX settings.
The foldout is editor state, not an audio/automation parameter.

Build 274 appends `fxTriangleFold` (Off/On, default Off) after the Build 273
`phaseRetrigger` parameter. The UI shape selector combines those controls with
the original `fxMode`: Soft and Sine Fold keep values 0 and 1; Triangle Foldback
enables the new switch while retaining the old mode. The separate switch avoids
changing old normalized FX Mode curves. When Triangle is disabled those curves
still select/morph the original shapes. Schema 20 protects the new field.

## Verification

`Tools/test-acid-fx-host.cpp` exercises the real processor/editor and transport
handlers using isolated engine preferences and a hosted audio device, with no
hardware output. It covers multirate dry identity, fixed latency, aligned blend,
block segmentation, finite extremes, different shapes, legacy state, native
render/automation, saved state, gesture Undo and FX-panel expansion. It also
checks that Play/Stop, Pattern/Song, Return, previous/next, Loop and Record leave
open and intentionally minimized workspace panels in their existing states.

Listening comparisons with ABL remain manual; no matched A/B was performed.

## Candidate additions — 2026-10-30 (updated by user choice)

The user selected Triangle Foldback; Build 274 adds it. Other possible additions
for user audition are asymmetric drive (unequal positive/negative shaping) and hard
clipping. The first listening recommendation is asymmetric
drive; this is a sound-design preference, not a measured comparison. Kilohearts'
official distortion docs independently list clipping, sine and foldback families:
https://kilohearts.com/docs/snapins . Their source/code is not used in Orbit.

The new separate triangle switch preserves the meaning of existing normalized
`fxMode` automation (0=Soft, 1=Sine Fold); simply extending that parameter's range
would reinterpret saved curves. Build 274 uses a compatible selector instead. The phase-retrigger setting in
Build 273 changes no FX parameter IDs, ranges or audio processing.

## Quick Triangle Foldback test

Reopen Build 274. Insert Acid Bass, click Demo pattern, select Pattern and Play.
Expand **Effects / Drive +**, choose **Triangle Foldback** from the shape list,
then set **Distort** around 50–70% and **Drive** around 40–70% as a starting point.
Compare Soft, Sine Fold and Triangle at the same Drive/Output; adjust Tone to
taste. Distort 0% retains the dry sound. Try Cutoff/Reso sweeps and slide notes.
The phase switch remains in the generator dropdown > Generator Settings.
