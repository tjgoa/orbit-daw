# Geonkick DSP used by Orbit's audition instrument

Copyright (C) 2017–2026 Iurie Nistor and upstream contributors.
Source: https://github.com/quamplex/geonkick
Pinned commit: `a046d614198227cbc737bb1d717b6e155394f15a` (3.9.0).
Fetched 2026-10-30. DSP headers specify GPL-3.0-or-later. See LICENSE;
Quamplex DSP Tools carries its own GPL-3.0-or-later notices and LICENSE.
UPSTREAM_SHA256.json records the original downloaded file hashes.

Orbit compiles oscillator.c, envelope.c, filter.c, distortion.c and
gkick_buffer.c. The remaining core files are retained as source/reference only.
The original Redkite UI, worker, kit/mixer, sample loading, presets and plugin
shell are not included in the running instrument.

Orbit modifications, 2026-10-30:

- filter.c and distortion.c: their lock/unlock wrappers accept the explicit
  ORBIT_GEONKICK_SINGLE_OWNER build definition. With this definition they do
  not lock. This build must only be used through Orbit's private, exclusively
  owned voice contexts. No shared upstream engine/worker may use this target.
  The DSP equations are unchanged. The default upstream locking remains when
  that definition is absent. The oscillator/filter/distortion constructors,
  destructors and struct headers also omit mutex fields/setup in this mode.
  geonkick_internal.h isolates the five-file DSP target from unused kit/worker
  and atomic headers; oscillator.h includes its distortion type directly.
  geonkick.h permits this atomic-free subset on C11 compilers without atomics.
  Missing reciprocal-pi constants are defined for non-POSIX compilers. This
  avoids introducing a pthread/atomic dependency into Windows Orbit builds.
- envelope.c propagates allocation failure instead of dereferencing NULL.
- GeonkickVoice.c outside this folder supplies a deterministic, per-oscillator
  LCG for geonkick_rand in place of the upstream platform-specific helper.
  It mixes three upstream oscillators and their envelopes/filter/distortion
  per voice, with eight preallocated voices and Orbit-authored sound recipes.
  Prepare, MIDI triggering and sample rendering allocate no memory. All
  mutable DSP is owned by the render thread; the UI uses host parameters.
- No claim of full Geonkick preset compatibility, an original Geonkick sound
  bank, Roland 909 accuracy, or Orion DSP equivalence is made. This is an
  audition instrument for comparing synthesis approaches.

The Orbit adapter/plugin/UI files are GPL-3.0-or-later and are distributed
within Orbit's AGPLv3 application. Keep these notices and the corresponding
source available with distributions; the application includes these notices
under Resources/Third-party notices/Geonkick.
