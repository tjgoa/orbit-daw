#pragma once
#include <JuceHeader.h>
#include <optional>
#include "HeaderChrome.h"
#include "../UI/KnobStyle.h"

namespace orbit::options
{
inline int currentSkin = 0;
inline constexpr int navyBlueSkin = 5, lightBlueSkin = 6, compactSilverSkin = 7, compactSilver2Skin = 8;
inline const juce::StringArray skinNames { "Default", "Azure", "Classic", "Grey", "Mixer", "Navy Blue", "Light Blue", "Compact Silver", "Compact Silver 2" };

// Keep persisted IDs stable when retiring choices from the menu.
inline bool selectableSkin(int skin) { return skin>=0 && skin<skinNames.size() && skin!=3 && skin!=4; }
inline int normaliseSkin(int skin)
{
    const int bounded=juce::jlimit(0,skinNames.size()-1,skin);
    return selectableSkin(bounded)?bounded:0;
}

struct WorkspaceAppearance
{
    bool lightCanvas;
    int mixerSkin;
    int headerStyle = 0;
    int playlistVariant = 0;
};

inline std::optional<WorkspaceAppearance> workspaceAppearance(int skin)
{
    // Stock skin definitions are complete and independent of the active header.
    if (skin >= 0 && skin < navyBlueSkin) return WorkspaceAppearance { false, 0, 0, 0 };
    if (skin == navyBlueSkin) return WorkspaceAppearance { false, 1 };
    if (skin == lightBlueSkin) return WorkspaceAppearance { true, 0 };
    if (skin == compactSilverSkin) return WorkspaceAppearance { true, 3, 8, 1 };
    if (skin == compactSilver2Skin) return WorkspaceAppearance { false, 4, 8, 2 };
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
        || currentSkin == compactSilverSkin || currentSkin == compactSilver2Skin
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
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider) override
    {if(!ui::drawSelectedKnob(g,x,y,w,h,position,start,end,slider))juce::LookAndFeel_V4::drawRotarySlider(g,x,y,w,h,position,start,end,slider);}
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool hover,bool down) override
    {
        if(header::style(b)>0){header::button(g,b,hover,down);return;}
        juce::LookAndFeel_V4::drawButtonBackground(g,b,colour,hover,down);
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool hover,bool down) override
    {
        if(header::style(b)>0)
        {
            g.setColour(header::textColour(b).withAlpha(b.isEnabled()?1.f:.5f));
            g.setFont(juce::FontOptions(12.f));g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(4,1),juce::Justification::centred,1);return;
        }
        juce::LookAndFeel_V4::drawButtonText(g,b,hover,down);
    }
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
