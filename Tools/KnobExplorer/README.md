# Orbit native JUCE knob comparison

Open `Orbit-Knob-Explorer.html` offline in a browser. All 33 styles already have
compiled JUCE C++ drawing implementations. HTML displays exported native image
frames; JavaScript selects the value frame. CSS positions the images and controls
but does not recreate any knob. There is no HTML-to-JUCE translation step.

The comparison contains the 20 earlier native styles, five original Labyrinth
reference variants (Monozotic/HF2, Formant FM Cyan/Gold, Vocal Motion, Spectral
Garden), and eight new procedural JUCE studies: 303 Ribbed Black, Aluminium
Radial, Ivory Pointer Cap, Vintage Skirt, LED Encoder, Glass Lens, Square Cap and
Concentric Face. Source provenance/hashes are in PROVENANCE.md. Labyrinth is
read only. The new studies are selectable comparisons, not new production skin
defaults.

Sizes are 32, 48 and 64 logical pixels. Atlases were rendered directly at
32/48/64/96/128 physical pixels, covering matching 1x and 2x screens. Other device
scales or browser zoom can resample. Each style has 128 native value positions;
the live JUCE renderer accepts a continuous normalized value. Transparent PNG
encoding can round colour channels; exported atlas frames are compared with
independently encoded native frames, and the raw renders are compared directly.

Drag a knob or use arrow/Home/End keys, compare backgrounds and sizes, keep
favorites, and save a JSON choice containing the stable style ID. No network
requests are made. The included C++ is authoritative:

```cpp
#include "KnobStyleRenderer.h"
// In a JUCE Slider LookAndFeel::drawRotarySlider:
orbit::knobs::draw(g, {float(x), float(y), float(width), float(height)},
                   chosenStyleId, sliderPos);
```

Keep NativeKnobStudies.h and References/ with the renderer. The two Home headers
forward to canonical Source/UI copies, so preserve those paths or adjust the
includes when reusing the renderer in another JUCE project. All drawing is JUCE;
a browser or web view is not required in the eventual native editor.

Reproduce the atlas exports with the normal isolated host harness:

```sh
python3 Tools/test-options-host.py --test-file Tools/test-native-knob-gallery-host.cpp --sanitize --compile-only
# Run the printed executable in a WindowServer session.
python3 Tools/KnobExplorer/build-explorer.py PATH_TO_ITS_TEMP_FOLDER
```

The host test also checks silver-only mixer shadow restoration and fixed mixer
geometry. Source-only archives include the HTML preview and C++ sources, with no
verification executable or compiled plugin.

The generated all-sprites HTML gallery is omitted from the public source checkout. Recreate it locally with `python3 build-explorer.py /path/to/native-atlas-output`, using the `knobs.json` and PNG atlases produced by `native-preview.cpp`.
