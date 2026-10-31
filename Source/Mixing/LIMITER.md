# Original look-ahead brickwall limiter — Build 261

Insert **FX Dynamics > Look-ahead Brickwall Limiter**, or choose it from a Mixer
strip's **Built-in Effects**. Put it last in the Master insert chain when the
whole mix should be limited. Processing after the limiter can raise peaks again.

Input ranges from -24 to +36 dB; Release ranges from 5 to 2000 ms, initially
100 ms. The processor delays audio by approximately 5 ms, applies a linked stereo
attack ramp using upcoming peaks, and recovers exponentially. Its ceiling is
0 dBFS for samples. It does not claim inter-sample/true-peak limiting, nor Orion
DSP equivalence. The original algorithm was written from the behavioral manual
reference without proprietary Orion code or third-party DSP source.

Below-threshold audio, with no nearby over-threshold peaks, passes unchanged
apart from declared latency. Future loud peaks deliberately lower gain shortly
before they arrive. Both channels receive the same reduction. Input and gain
reduction readouts update from audio-thread atomics; they display recent block
measurements rather than a latched maximum. Input/release are native automatable
parameters with event-menu entry, chain copy, Undo and project persistence.

Initialization allocates the delay and monotonic queue; sample processing does
not allocate or lock. Native latency compensation aligns parallel tracks, and
the delayed end of an export is retained. An actual time jump clears old audio;
a recognized transport loop wrap retains the delay history. The native graph
still owns bypass/PDC behavior. User listening/live looping and hardware latency
checks remain necessary; headless render checks cannot establish those.

Schema 13 requires Build 261 or newer so older hosts cannot unknowingly drop the
new native processor. Projects from earlier supported schemas still load.

Validation:

```sh
clang++ -std=c++17 -fsanitize=address,undefined -g \
  Tools/test-lookahead-limiter.cpp -o /tmp/orbit-limiter-check
/tmp/orbit-limiter-check
python3 Tools/test-options-host.py --sanitize --limiter
```

The core check covers 44.1/48/96/192 kHz, exact delay/quiet transparency, random
large peaks, asymmetric stereo, nonfinite input and release recovery. Native
checks cover parallel-track compensation, final export samples, bypass,
boosted-peak rendering, meters, loop/seek state, Undo/Redo, chain copying and
same/fresh-process project loading. Sidechain/parameter-window regressions also
cover the shared effect controls.
