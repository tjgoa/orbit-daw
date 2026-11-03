# Mixer Layout and appearance variants

Open the Mixer or Master header menu and choose **Mixer Layout…**. The nonmodal
panel previews changes on the actual native mixer. Values save as application
preferences, shared by Mixer/Master banks and independent of the DAW skin.

- **Horizontal knob spacing:** a pixel offset from the original centers, applied
  within two- and three-knob rows, including the send rows.
- **Vertical row spacing:** positive values separate rows; negative values tighten
  their control cells. Taller layouts scroll inside the existing mixer window.
- **Label offset:** moves the compact EQ labels closer to or farther from controls.
- **Channel strip width:** explicit strip width; this does not resize the window.
- **Knob size:** the existing 100–500% face scaling. This alone never widens a
  strip or changes the mixer window size. Faces fit available cells; tight rows
  limit the maximum usable diameter so adjacent controls cannot overlap.
- **Fader cap width:** changes the cap artwork only. The track, value range,
  parameter bindings and drag behavior retain their existing geometry.
- **Fader finish:** Original, Studio Silver or Studio Dark. The Studio caps have
  a bevel, grip grooves and contact shadow. Original is the previous renderer.
- **Knob finish:** Palette default, original White/blue, White/charcoal or
  Silver/charcoal. This is scoped to Mixer/Master Labyrinth White controls;
  native generator knobs and plugin-owned artwork are unaffected.

Changes save after a drag or a short idle period. **Reset layout** restores
original spacing, widths, knob size and finishes; it leaves the chosen mixer
palette intact. **Revert** restores the values from when the panel was opened.
Closing the panel keeps the previewed settings. No mixer gain, pan, EQ/send value,
automation, routing or project Undo entry is changed by a layout/finish control.

**Mixer colors** adds two independent choices:

- **Warm White:** warm near-white surfaces, muted dividers/accents, white knobs
  with charcoal indicators by default.
- **Silver / Silver Knobs:** the existing lighter silver console surface with a
  darker silver knob face and charcoal indicators by default.

The original five mixer palettes and their default artwork remain available.
A selected global **Original** knob style still takes precedence; choose
**Labyrinth White** or **Skin default** to use the new palettes' white/silver faces.
An explicit Knob finish choice overrides the palette's finish until reset.

The two knob variants are cached native color transformations of the existing
Home atlas. Alpha, frame mapping, geometry and lighting contrast are retained;
the original atlas is never overwritten. These are appearance options, not a
change to the canonical Labyrinth renderer or its source assets.

Preferences use versioned `options.mixerLayoutV1` JSON and preserve the legacy
`options.mixerKnobSize` setting. Unknown/malformed versions use original defaults;
numeric values are bounded before use. **Knob Sprites…** in the header menu now
loads exported KnobMan PNGs live; see [KNOB_SPRITES.md](KNOB_SPRITES.md). Sharing
layout presets remains future work. KnobMan editing/export continues to use the
separate tool.

## Orion Console / Blue

An additional design in Mixer colors / designs uses one shelf gain at each end (12 kHz/80 Hz), stacked mid controls and a pan dome with stereo meters on opposite sides of the fader. Choosing it is an appearance change; adjusting a shelf gain anchors that shelf frequency. Existing designs remain unchanged. The compact console EQ cells keep their staggered arrangement; horizontal spacing continues to affect the paired sends, while row spacing separates the compact EQ sections.
