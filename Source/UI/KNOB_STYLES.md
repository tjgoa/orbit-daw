# Native knob styles — Build 308

**Options > Knob Style** offers Skin default, Original and Labyrinth White.
The Mixer/Master header menu exposes the same global choice under **Knob style
(global)**. It persists across restarts independently of the DAW skin or mixer
colors. Skin default restores each panel's own artwork; Original uses the
standard JUCE rotary design; Labyrinth White uses the existing unchanged Home
renderer and atlas. This is a paint change: values, ranges, gestures, event
menus, audio and window positions remain unchanged.

**Generator Settings > Knob style** offers Follow global, Skin default, Original
and Labyrinth White. An explicit per-generator choice overrides global; Skin
default explicitly uses that generator's own skin artwork. Follow global resumes
the Options choice. This instance setting saves in projects and generator
presets, supports Undo/Redo, and is pinned to the generator whose Settings opened.

**Generator Settings > Acid Bass skin** offers Silver / White, Dark / Original
and Dark / White. Both dark choices use the earlier fixed RGB(30,44,54) body with
light labels. Dark / Original uses the earlier standard rotary knobs, while
Dark / White uses the Home controls. Knob style can override a skin's knob choice
without changing its body colors. New/older presets without appearance metadata
retain the current Silver / White default. Acid appearance is instance-specific
and saves with presets/projects, independently of the workspace skin.

The shared selector covers Orbit's custom Mixer EQ/send controls, standard pan
and Master controls, Drum Rack dials, native generators and native dynamics
rotary panels. Plugin-owned VST3 editor artwork cannot be reskinned; Orbit's host
controls can still use the selected native style. Linear sliders and faders
remain linear controls.

Only two LFO trigger parameters are appended to the Acid Bass audio parameter
list (indices 45/46); appearance uses non-automated state properties. All original
45 IDs/ranges/defaults stay intact. The renderer and atlas are decoded once and
shared; the canonical Labyrinth files are not changed.

## Mixer sizing and shadows — Build 316

Mixer/Master knob scale offers 100–500%. It changes only the painted knob face
within fixed control cells; strip width, row spacing, faders and window bounds
stay fixed. The three-knob HM row has a smaller available face limit.

Labyrinth White on light/silver mixer palettes casts a deeper soft shadow from
the channel strip behind labels and controls. The shadow can extend beyond the
slider bounds; labels stay readable above it. Dark mixer palettes retain the
clean white knob treatment. Acid Bass has its own light/dark rendering and its
eight main white-knob labels now sit closer to the faces.

## Mixer finish choices and layout panel

The Mixer header's **Mixer Layout…** panel adds independent spacing and cap/knob
finish controls. **Warm White** and **Silver / Silver Knobs** are new mixer-only
palettes. They do not replace the existing workspace skins, header styles or
Acid Bass artwork. See [Mixer Layout](../Mixing/MIXER_LAYOUT.md) for settings,
reset behavior and appearance precedence.
