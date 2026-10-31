#pragma once
#include <JuceHeader.h>
#include <optional>

namespace orbit::options
{
inline int currentSkin = 0;
inline constexpr int navyBlueSkin = 5, lightBlueSkin = 6;
inline const juce::StringArray skinNames { "Default", "Azure", "Classic", "Grey", "Mixer", "Navy Blue", "Light Blue" };

struct WorkspaceAppearance
{
    bool lightCanvas;
    int mixerSkin;
};

inline std::optional<WorkspaceAppearance> workspaceAppearance(int skin)
{
    if (skin == navyBlueSkin) return WorkspaceAppearance { false, 1 };
    if (skin == lightBlueSkin) return WorkspaceAppearance { true, 0 };
    return std::nullopt;
}

// Recolours Orbit-owned drawing only. Meter warning colours and plug-in UIs keep
// their semantic colours. The default skin retains the existing artwork.
inline juce::Colour colour(juce::uint8 r, juce::uint8 g, juce::uint8 b)
{
    const auto original = juce::Colour::fromRGB(r, g, b);
    // Workspace presets use the original blue/dark and cool light canvases,
    // together with the original Midnight Navy or Classic Blue mixer artwork.
    if (currentSkin == 0 || currentSkin == navyBlueSkin || currentSkin == lightBlueSkin
        || (r > g * 1.22f && r > b * 1.1f)
        || (g > r * 1.5f && g > b * 1.2f)) return original;
    const auto light = original.getBrightness();
    switch (currentSkin)
    {
        case 1: return juce::Colour::fromHSV(0.57f, original.getSaturation() * 0.9f, light, 1);
        case 2: return juce::Colour::fromHSV(0.10f, original.getSaturation() * 0.35f, light, 1);
        case 3: return juce::Colour::fromHSV(0.0f, 0.0f, light, 1);
        case 4: return juce::Colour::fromHSV(0.48f, original.getSaturation() * 0.75f, light * 0.83f, 1);
        default: return original;
    }
}

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void refresh()
    {
        setColour(juce::ResizableWindow::backgroundColourId, colour(27, 43, 55));
        setColour(juce::Label::textColourId, colour(225, 239, 247));
        setColour(juce::TextButton::buttonColourId, colour(38, 56, 68));
        setColour(juce::TextButton::buttonOnColourId, colour(37, 112, 143));
        setColour(juce::TextButton::textColourOffId, colour(225, 239, 247));
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        setColour(juce::ComboBox::backgroundColourId, colour(34, 49, 61));
        setColour(juce::ComboBox::textColourId, colour(225, 239, 247));
        setColour(juce::PopupMenu::backgroundColourId, colour(27, 43, 55));
        setColour(juce::PopupMenu::textColourId, colour(225, 239, 247));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colour(37, 112, 143));
        setColour(juce::TextEditor::backgroundColourId, colour(24, 36, 45));
        setColour(juce::TextEditor::textColourId, colour(225, 239, 247));
        setColour(juce::Slider::thumbColourId, colour(78, 172, 201));
        setColour(juce::ToggleButton::textColourId, colour(225, 239, 247));
        setColour(juce::ToggleButton::tickColourId, colour(78, 172, 201));
    }
};
}
