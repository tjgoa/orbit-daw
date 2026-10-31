# Orbit Build 270 / v0.3.249 — local development update

- Acid Bass gains a collapsible **Effects / Drive** section: Drive, Distort blend,
  Low Cut, Tone, Output and **Soft / Sine Fold** modes.
- Permanent Acid Bass numeric boxes are removed. Values appear while dragging.
- Old patches remain dry at Distort 0%; existing synth controls keep their IDs.
- Native automation and saved state include the new FX. Schema 16 protects them.
- Verified transport controls preserve open/minimized windows and their positions.

The FX are an original ABL-inspired design using public control descriptions and
an unverified listening lead, not AudioRealism code or an exact sonic clone.
Fourfold oversampling, aligned dry blend and declared latency are validated with
the engine's delay compensation. Native DSP/UI/transport checks passed under
ASan/UBSan with hosted buffers; listening and hardware performance remain manual.

Test steps: Source/Editing/TESTING_GUIDE.md, Build 270 section.
No public release was published.
