# Modified 9W9 port for Orbit

9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9

Copyright (C) 2026 A Thousand Details. GPL-3.0 with additional terms under
section 7; retain LICENSE and NOTICE unaltered. This is a modified version for
Orbit, not the original and not made or endorsed by A Thousand Details.

Pinned revision: `a9e030d9bad46e3e53aafce92c7104b8e7bba47f` (v2.7.2).
Fetched 2026-10-30. UPSTREAM_SHA256.json records downloaded source file hashes.
The original header comments, authorship and all additional NOTICE terms remain.
The samples in samples/ are the unmodified resources distributed by this pinned
repository; PROVENANCE.json records their Git blob SHA, SHA256, rate and format.
No hardware-ROM identity or independent provenance beyond that repository is
claimed. Consult the original repository for its development/reference material.

Orbit modifications, 2026-10-30:

- 9w9_engine.h/c: sampler rate_ratio member and source/host-rate correction in
  sample advancement. Default ratio remains 1.0, preserving upstream behavior
  until the Orbit adapter supplies the 44.1 kHz source-rate ratio.
- NineKitVoice.c wraps the engine with private instance ownership, direct
  predecoded PCM arrays, no runtime file dependency, panic/reset, host tempo and
  a separate per-step accent multiplier. The seven synthesized voices retain
  the upstream algorithms/control policy. In particular, kick Tune is the
  original sweep knob and the snare has its original fixed shell decay.
- NineKitSamples.h and NineKitMetadata.h are generated from the pinned WAVs
  and upstream UI metadata by Tools/generate-ninekit-data.py. The native Orbit
  plugin/editor, MIDI maps, sequencer, per-step accent and demo are additions.
- All allocations occur during construction. Trigger/render/control/panic paths
  have no audio-file loading, memory allocation or locking. Samples are immutable
  shared data; voice, noise, filter and FX state are per-instance.

This audition has one stereo output (duplicated mono). Orion Pro-9's additional
controls, per-voice pan and 6/11 stereo output variants are not implemented by
this port. No claim of exact hardware or Orion sound equivalence is made.

Credits and the modified-version statement are displayed in Orbit's About
screen, this instrument's Credits button, source and documentation. App builds
carry LICENSE/NOTICE/UPSTREAM.md under Third-party notices/9W9.
