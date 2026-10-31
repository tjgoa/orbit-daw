# Tube / Two-Band Distortion research — 2026-10-29

The user prioritizes these effects next, then a Pro-9-style drum generator.
Functional references are the installed Orion Help topics FxTubeDistortion.htm,
Fx2BandDistortion.htm and GenPro9.htm, plus the user's recordings/screenshots.
No proprietary DSP code or binaries were inspected. These references establish
controls/workflow, not the original coefficients, circuit or exact sound.

## MIT candidates (research, not imported dependencies)

| Candidate | Role | Primary sources |
| --- | --- | --- |
| Airwindows Tube | Symmetric saturation candidate | https://github.com/airwindows/airwindows/tree/master/plugins/WinVST/Tube and https://raw.githubusercontent.com/airwindows/airwindows/master/plugins/WinVST/Tube/TubeProc.cpp |
| Airwindows Tube2 | More elaborate asymmetric/dynamic tube-style saturation | https://www.airwindows.com/tube2/ and https://github.com/airwindows/airwindows/tree/master/plugins/WinVST/Tube2 |
| Airwindows MultiBandDistortion | Complete split-band distortion candidate; deliberately extreme behavior | https://www.airwindows.com/multibanddistortion/ |
| DaisySP Overdrive + a filter split | Small drive building block for an independently constructed two-band processor, not a ready two-band plug-in | https://github.com/electro-smith/DaisySP/blob/master/Source/Effects/overdrive.cpp and https://github.com/electro-smith/DaisySP/blob/master/LICENSE |
| Signalsmith DSP filters + a waveshaper | Alternative filter building blocks for an original split-band architecture; not a complete distortion plug-in | https://github.com/Signalsmith-Audio/dsp and https://github.com/Signalsmith-Audio/dsp/blob/main/LICENSE.txt |

Airwindows publishes MIT terms at
https://raw.githubusercontent.com/airwindows/airwindows/master/LICENSE . File
selection, pinned revision, notices, build portability and DSP/audio validation
are required before any import. No old VST SDK wrapper should be copied into
Orbit merely to reuse a processor. Phlegetron was also checked, but its repository
is GPL-3.0, not an MIT candidate: https://github.com/igorski/phlegetron .

The intended Orbit effects can instead use original transfer functions and the
existing JUCE oversampling infrastructure, with the reference's control roles.
That is not evidence of matching an Orion or AudioRealism processor.

## AudioRealism ABL2 distortion

AudioRealism's official ABL2 page confirms an integrated distortion unit:
https://www.audiorealism.se/products/audiorealism-bass-line-2 . The developer's
ABL3.3.0 announcement explicitly confirms the return of ABL2 distortion and drive:
https://forum.audiorealism.se/post/audiorealism-bass-line-3-3-0-update-11810553 .
This supports the user's recollection of the old distortion section, but does
not identify a named pedal, transfer function, tube circuit or open-source DSP
implementation. The checked official pages do not document the exact old mode
formula. No claim that an MIT processor sounds identical is justified yet.
A later official ABL3.1.x update PDF supplies a separate useful reference:
https://www.audiorealism.se/uploads/4/5/5/4/45545237/abl3.1.0updatemanual.pdf .
Its complete signal-chain and Drive/Distort pages were visually checked. That
version places high-pass then distortion after the synth, describes transistor
modeling with output-power compensation, and defines Distort as the blend. This
is not evidence that the older ABL2 tube/other modes use the same algorithm.

An original optional Acid Bass post-drive stage can provide comparable controls;
a matched A/B recording is needed to evaluate resemblance.

## Functional effect scope

Tube: drive; low cut/high damping before the nonlinear stage; distinct shaping
modes; output level. The manual's shape description contains a repeated word,
so it does not reliably establish the exact symmetry of all three modes.

Two-Band: crossover/breakpoint, separate low/high drive, final high-frequency tone
control and dry/wet. Validate stereo independence, neutral split reconstruction,
finite extremes, alias suppression and latency-aligned blend/persistence.

## Next generator scope

Pro-9 reference: eleven synthesized drum instruments; per-instrument level, pan,
tune and decay; extra kick attack/tone/pitch-envelope controls and snare
noise/pitch-envelope controls; global accent; paged step editing and Piano Roll;
stereo plus six/eleven grouped-output variants. No native Orbit equivalent is
claimed by the current Drum Rack (which plays samples). Implement/review routing
explicitly rather than calling a stereo-only instrument full Pro-9 parity.

## 2026-10-30 — KVR lead for the remembered ABL distortion

A specific ear-based comparison is now located: gentleclockdivider, July 30,
2021, [Audiorealism ABL3, page 12](https://www.kvraudio.com/forum/viewtopic.php?p=8173205).
The participant calls the second type foldover / a zero-frequency sine shaper,
and reports a similar result from Zebrify's Fold and a Reaktor sine shaper.
They speculate the first type is tanh or atan shaping. These are listening
observations, not confirmed coefficients or a developer's circuit description.
The thread's comparison recording was linked but has not been downloaded or A/B
validated. No exact equivalence is claimed.

Context: [AudioRealism's 3.3.0 announcement](https://forum.audiorealism.se/post/audiorealism-bass-line-3-3-0-update-11810553)
confirms that ABL2's Drive and Distortion returned in that version. This makes the
comparison relevant to the older effect but does not establish identical settings
or an exact transfer function. [ReasonTalk's 2015 discussion](https://forum.reasontalk.com/viewtopic.php?start=25&t=7273070)
also records users missing ABL2's second mode and its character.

Next DSP experiment: an independently written sine-fold stage with antialiasing,
level compensation, filtering and dry/wet; compare equal-loudness audio before
claiming any sonic match. Current Tube and Two-Band effects remain their original
algorithms; neither is presented as the requested ABL2 foldover replica.


## Build 270 — original internal Acid Bass FX

Implemented the post-synth experiment in `Source/Synth/AcidBassFx.h`: smoothed
Soft (`tanh`) and Sine Fold (`sin`) shapes, Low Cut, Tone, Drive, Distort blend and
Output, with 4× oversampling and an aligned dry path. The control roles follow the
public manual lead above; the sine-fold option follows the explicitly unverified
KVR listening lead. No proprietary source, circuit coefficients or AudioRealism
assets were used. This does not identify an exact ABL mode or establish a match.
Distort defaults to zero for old patches. See `Source/Synth/ACID_BASS_FX.md`.
