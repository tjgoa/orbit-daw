# Original Tube / Two-Band Distortion — Build 267

**Insert > FX Distortion** or a Mixer/Master insert chooser provides both native
effects. Select Acid Bass first to add one to that track. These original effects
implement the control roles described by Orion Help; they do not claim its DSP,
presets or an AudioRealism ABL2 match. No researched third-party distortion code
was imported. See DISTORTION_RESEARCH.md for verified MIT alternatives and the
AudioRealism developer's confirmation that ABL2 drive/distortion returned in ABL3.3.

## Tube Distortion

| Control | Range / default | Role |
| --- | --- | --- |
| Drive | 0–48 dB / 12 | Controls nonlinear intensity; zero is the linear shaper limit. |
| Lo Cut | 20–2000 Hz / 30 | High-pass before shaping. |
| Hi Damp | 500–20000 Hz / 12000 | Low-pass before shaping; generated harmonics may extend higher. |
| Shape | Symmetric, Tube A, Tube B / Symmetric | Original odd-symmetric or two differently biased soft curves. Not calibrated hardware tube models. |
| Level | -36 to +12 dB / -12 | Output gain after blend. |
| Dry/Wet | 0–100% / 100% | Original input versus processed signal; output Level affects both. |

The biased curves subtract their static bias; a 5 Hz DC blocker removes
signal-dependent DC. Low-cut/damping are before distortion, as the reference
specifies. Modes interpolate during switching. A cabinet convolution, named amp
or physical vacuum-tube circuit is not modeled.

## Two-Band Distortion

| Control | Range / default | Role |
| --- | --- | --- |
| Breakpoint | 40–12000 Hz / 800 | Complementary one-pole low/residual-high split. |
| Low Drive | 0–48 dB / 0 | Distortion applied to the lower band. |
| High Drive | 0–48 dB / 0 | Independent distortion on the upper band. |
| Hi Tone | 500–24000 Hz / 16000 | Final high-frequency damping after recombination. |
| Level | -36 to +12 dB / -6 | Output gain after blend. |
| Dry/Wet | 0–100% / 100% | Blend with the latency-aligned input. |

The complementary bands reconstruct exactly before nonlinear processing;
separate drive and final tone intentionally change that signal. Breakpoint slope
and transfer curves are Orbit choices, not measurements of Orion's effect.

## Processing and integration

Both use original normalized tanh-family transfer functions and the existing
JUCE 4x linear-phase FIR oversampling infrastructure. At current filter settings,
61 samples of latency are reported to the native graph; a matching dry delay
keeps blending aligned. This is delay alignment, not cancellation of intentional
tone-filter phase changes. Initialization supports sample-rate/block changes;
large blocks are chunked without processing-time allocation. Live seeks reset
filter/delay history; recognized loop wraps retain it. Offline compensation is
covered by native start-of-file tests. Stereo channels have independent states.
Gain/mix/filter/drive changes are smoothed. These effects do not guarantee an
output ceiling: use the limiter afterward if needed.

Each has six native parameters, instance IDs, Pattern/Song event entry, recording,
settings copy, chain copy, Undo and native persistence. Schema 15 prevents older
hosts from silently dropping the new native types. Existing Acid Bass sound/state
is preserved; distortion is an optional insert, not a change to old synth presets.

## Validation

```sh
clang++ -std=c++17 -O2 -g -fsanitize=address,undefined \
  Tools/test-distortion-core.cpp -o /tmp/orbit-distortion-core-check
/tmp/orbit-distortion-core-check
python3 Tools/test-options-host.py --sanitize --distortion
```

Core checks cover four rates, low/high drive separation, odd/even harmonic
behavior, pre-filter roles, channel isolation, DC recovery and extreme inputs.
Native tests cover 44.1/48/96 kHz, exact dry PDC including first sample, 50% blend
against summed dry/wet renders, stereo polarity, folded-harmonic suppression,
real level automation, editor binding, Undo, unique copied IDs and same/fresh
process reopen. The high-frequency Tube fixture's 21 kHz folded third harmonic
was about 5.3e-6 with oversampling versus 0.173 in the native-rate reference core.
This single fixture is not a universal aliasing specification or listening test.
Human listening and matched Orion/ABL2 measurements remain open.
