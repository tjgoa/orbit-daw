#pragma once
#include "Appearance.h"

namespace orbit::options::compact
{
inline constexpr int mixerSkin = 3;
inline const juce::Identifier property { "orbitCompactSilver" };
inline juce::Colour ink() { return juce::Colour(32, 39, 49); }
inline juce::Colour display() { return juce::Colour(29, 35, 44); }
inline juce::Colour accent() { return juce::Colour(96, 200, 226); }
inline int variant(const juce::Component& component)
{
    for (auto* c = &component; c != nullptr; c = c->getParentComponent())
        if (c->getProperties().contains(property)) return juce::jlimit(0,3,int(c->getProperties()[property]));
    return 0;
}
inline bool enabled(const juce::Component& component) { return variant(component)>0; }
inline juce::Colour accent(const juce::Component& c) { return variant(c)==3?juce::Colour(132,140,140):accent(); }
// A single neutral surface throughout each console, with no alternating EQ fills.
inline juce::Colour surface(int mode)
{ return mode==3 ? juce::Colour(250,249,244) : mode==2 ? juce::Colour(212,216,224) : juce::Colour(196,202,212); }
// Scoped to the console/Playlist, never the global skin or hosted plug-in UI.
inline juce::Colour colour(const juce::Component& component, juce::uint8 r, juce::uint8 g, juce::uint8 b)
{
    const int mode=variant(component);
    if (mode==0) return orbit::options::colour(r,g,b);
    if(mode==3 && g > r * 1.5f && b > 185)return accent(component);
    if(mode==2 && bool(component.getProperties().getWithDefault("orbitSilverPlaylistGrid",false)))
    {
        const auto key=(int(r)<<16)|(int(g)<<8)|b;
        switch(key)
        {
            case 0x141d26:case 0x192631:return juce::Colour(40,45,56);
            case 0x16212b:return juce::Colour(36,41,51);
            case 0x1d2b37:return juce::Colour(60,66,78);
            case 0x445865:return juce::Colour(27,32,42);
            case 0x6f97a9:return juce::Colour(77,85,101);
            case 0x277da5:return juce::Colour(144,159,187);
            case 0xdcf4fa:case 0xc2ebf8:return juce::Colours::white;
            default:break;
        }
    }
    if(mode==2 && bool(component.getProperties().getWithDefault("orbitSilverEqKnob",false)))
    {
        if(r==155 && g==190 && b==204)return juce::Colour(248,249,252);
        if(r==48 && g==76 && b==92)return juce::Colour(65,73,86);
        if(r==71 && g==119 && b==143)return juce::Colour(241,244,248);
        if(r==29 && g==54 && b==69)return juce::Colour(115,126,142);
        if(r>220 && g>230 && b>230)return ink();
    }
    const auto original = juce::Colour(r,g,b);
    // Meter warnings, clipping and record indicators retain their meaning.
    if ((r > g * 1.22f && r > b * 1.1f) || (g > r * 1.5f && g > b * 1.2f)) return original;
    if (g > r * 1.5f && b > 185) return accent(component).withBrightness(original.getBrightness());
    // Clip fills and selected controls keep a deeper steel-blue recess so
    // white note previews and active icons remain legible on the silver grid.
    if(g > r * 1.5f && b > g)
        return juce::Colour::fromHSV(.59f, original.getSaturation() * .65f, original.getBrightness() * .8f, 1.f);
    return juce::Colour::fromHSV(.61f, original.getSaturation() * .12f, original.getBrightness(), 1.f);
}
inline void panel(juce::Graphics& g, juce::Rectangle<float> bounds,int mode=1)
{
    g.setGradientFill(juce::ColourGradient(mode==3?juce::Colour(255,254,250):mode==2?juce::Colour(237,239,243):juce::Colour(222,226,233),0,bounds.getY(),
        mode==3?juce::Colour(229,229,221):mode==2?juce::Colour(179,187,201):juce::Colour(153,162,177),0,bounds.getBottom(),false));
    g.fillRect(bounds);
    g.setColour(juce::Colours::white.withAlpha(.55f));
    g.drawHorizontalLine(juce::roundToInt(bounds.getY()+1),bounds.getX(),bounds.getRight());
    g.setColour(juce::Colour(89,99,115));
    g.drawHorizontalLine(juce::roundToInt(bounds.getBottom()-1),bounds.getX(),bounds.getRight());
}
inline void controlColour(juce::Component& c, int id, juce::Colour silver, bool active)
{
    const juce::Identifier originalKey("silverOriginal" + juce::String(id));
    const juce::Identifier specifiedKey("silverSpecified" + juce::String(id));
    auto& values=c.getProperties();
    if (active)
    {
        if (!values.contains(originalKey))
        {
            values.set(originalKey, juce::int64(c.findColour(id).getARGB()));
            values.set(specifiedKey,c.isColourSpecified(id));
        }
        c.setColour(id,silver);
    }
    else if (values.contains(originalKey))
    {
        if (bool(values[specifiedKey])) c.setColour(id,juce::Colour(juce::uint32(juce::int64(values[originalKey]))));
        else c.removeColour(id);
        values.remove(originalKey);
        values.remove(specifiedKey);
    }
}
inline void applyControls(juce::Component& root, bool active)
{
    if (auto* button=dynamic_cast<juce::TextButton*>(&root))
    {
        // Invisible Playlist hit targets must remain transparent.
        if (!button->findColour(juce::TextButton::buttonColourId).isTransparent())
        {
            controlColour(root,juce::TextButton::buttonColourId,display(),active);
            controlColour(root,juce::TextButton::buttonOnColourId,
                button->getButtonText()=="R"?juce::Colour(181,64,56):juce::Colour(59,105,128),active);
            controlColour(root,juce::TextButton::textColourOffId,juce::Colour(227,234,241),active);
            controlColour(root,juce::TextButton::textColourOnId,juce::Colours::white,active);
        }
    }
    if (auto* label=dynamic_cast<juce::Label*>(&root))
    {
        const bool recessed=label->isColourSpecified(juce::Label::backgroundColourId)
            && !label->findColour(juce::Label::backgroundColourId).isTransparent();
        controlColour(root,juce::Label::textColourId,recessed?juce::Colour(231,237,244):ink(),active);
        if (recessed)
        {
            controlColour(root,juce::Label::backgroundColourId,display(),active);
            controlColour(root,juce::Label::outlineColourId,juce::Colour(107,119,137),active);
        }
    }
    if (dynamic_cast<juce::Slider*>(&root)!=nullptr)
    {
        controlColour(root,juce::Slider::thumbColourId,juce::Colour(224,231,240),active);
        controlColour(root,juce::Slider::rotarySliderFillColourId,accent(root),active);
        controlColour(root,juce::Slider::rotarySliderOutlineColourId,display(),active);
    }
    for (auto* child:root.getChildren()) applyControls(*child,active);
    root.repaint();
}
inline void applyVariant(juce::Component& root, int mode)
{
    root.getProperties().set(property,juce::jlimit(0,3,mode));
    applyControls(root,mode>0);
}
inline void apply(juce::Component& root, bool active) { applyVariant(root,active?1:0); }
}
