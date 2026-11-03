# Build 326 / v0.3.305 — Console mixer and sprite groups

This source update follows [Build 324](Build-324.md). The downloadable public
tester remains [Build 276](https://github.com/tjgoa/orbit-daw/releases/tag/tester-build-276).

- Added **Orion Console / Blue** under the Mixer/Master header menu's
  **Mixer colors / designs**. All seven previous designs remain available.
- Channel EQ has one gain control each for the 12 kHz high shelf and 80 Hz low
  shelf, stacked high-mid gain/frequency/Q, and low-mid gain/frequency controls.
  High-mid frequency spans 500–10,043 Hz; low-mid spans 45–2,457 Hz.
- Selecting the design preserves existing EQ sound. Adjusting one of its shelf
  gains also anchors that shelf to 12 kHz or 80 Hz. The full EQ editor remains
  available from the graph; the previous designs retain frequency controls.
- Added side-by-side mute/solo, a domed pan section and separate left/right
  segmented peak meters flanking the fader, with peak hold and clip reset.
- **Knob Sprites...** now shows Unknown/Unipolar/Bipolar artwork labels. Classify
  unknown artwork before applying it in grouped mode. Selecting classified
  artwork routes it to the matching controls: EQ gain/pan versus sends/frequency/Q.
  Optional per-role overrides allow separate send, frequency, gain and pan styles.
- Added stars, Favorites only, polarity filters and **Remove from list**.
  **Show removed** allows restoration; sprite and KnobMan source files are retained.
- Live **Knob size** scales imported sprites from 100–500% inside fixed control
  cells without resizing strips or the mixer. Transparent strip padding is
  cropped consistently across every frame, preserving soft shadows and rotation.
- **Test one sprite on all controls (temporary)** auditions any artwork everywhere.
  Turning it off restores saved polarity groups; test mode resets when Orbit opens.
- Group selections, classification, favorites and list pruning save locally and
  apply across Mixer/Master banks and DAW skins. Unknown labels are not guessed
  from filenames. KnobMan editing integration remains a future addition.

Native macOS ASan/UBSan checks cover actual parameter callbacks, shelf anchors,
old designs, group isolation/restore, stars/pruning, temporary audition and
visible fixed-cell sizing. Universal macOS Release (arm64 + x86_64) was built, signed/verified and installed locally;
this update does not publish a new binary tester package.
