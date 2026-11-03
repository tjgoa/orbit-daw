#pragma once
#include <JuceHeader.h>

namespace orbit::options::header
{
// Header styling is component-scoped: wallpaper, generators and plugin artwork
// never inherit this decoration. The workspace still begins at y=82.
inline const juce::StringArray names { "Classic", "Panelled", "Metallic", "Silver Flow", "Ambient Glow", "Champagne Wave", "Reference Ribbon / Views", "Reference Ribbon / Menus", "Compact Silver" };
inline int style(const juce::Component& c)
{return juce::jlimit(0, names.size()-1, int(c.getProperties().getWithDefault("orbitHeaderStyle", 0)));}
// Assets are bundled with the app, never referenced from a user's source path.
// Decode once, then cache an edge-preserving 2x raster for each style/width.
class Artwork
{
public:
    Artwork()
    {
        const auto executable=juce::File::getSpecialLocation(juce::File::currentExecutableFile);
       #if JUCE_MAC
        folder=executable.getParentDirectory().getSiblingFile("Resources").getChildFile("Header Artwork");
       #else
        folder=executable.getParentDirectory().getChildFile("Header Artwork");
       #endif
    }
    void setFolder(const juce::File& source) {folder=source;entries={};}
    juce::Image image(int variant,int width)
    {
        if(variant<3 || variant>=names.size() || width<1)return {};
        const size_t asset=variant==7?3:variant==8?4:size_t(variant-3);
        auto& entry=entries[asset];
        if(!entry.attempted)
        {
            entry.attempted=true;
            entry.original=juce::ImageFileFormat::loadFrom(folder.getChildFile(files[asset]));
        }
        if(!entry.original.isValid())return {};
        if(entry.width!=width)
        {
            entry.width=width;entry.scaled=juce::Image(juce::Image::RGB,width*2,164,false);
            juce::Graphics g(entry.scaled);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            const int edge=juce::jmin(256,width/4)*2,srcEdge=entry.original.getWidth()/5;
            // Frame the sculpted upper ribbons prominently in the shallow header;
            // Ambient Glow keeps its complete top/bottom light-trail composition.
            const int srcW=entry.original.getWidth(),srcH=(variant==4 || variant>=6)?entry.original.getHeight():juce::roundToInt(entry.original.getHeight()*.56),targetW=width*2;
            g.drawImage(entry.original,0,0,edge,164,0,0,srcEdge,srcH);
            g.drawImage(entry.original,edge,0,targetW-edge*2,164,srcEdge,0,srcW-srcEdge*2,srcH);
            g.drawImage(entry.original,targetW-edge,0,edge,164,srcW-srcEdge,0,srcEdge,srcH);
        }
        return entry.scaled;
    }
    inline static constexpr std::array<const char*,5> files {"silver-flow.png","ambient-glow.png","champagne-wave.png","reference-ribbon.png","compact-silver.png"};
private:
    struct Entry {bool attempted=false;int width=0;juce::Image original,scaled;};
    std::array<Entry,5> entries;
    juce::File folder;
};
inline juce::Colour ink(bool active)
{return active ? juce::Colour(25, 37, 44) : juce::Colour(207, 229, 238);}
inline juce::Colour textColour(const juce::Button& c)
{
    if(style(c)==8)
    {
        if(bool(c.getProperties().getWithDefault("orbitHeaderMenu",false)) || bool(c.getProperties().getWithDefault("orbitHeaderSilverTab",false)))return juce::Colour(34,41,49);
        return c.getToggleState()?juce::Colour(100,209,234):juce::Colour(220,229,234);
    }
    return ink(c.getToggleState());
}
inline void button(juce::Graphics& g, const juce::Button& c, bool hover, bool down)
{
    const auto b=c.getLocalBounds().toFloat().reduced(1.f);
    const bool active=c.getToggleState();
    const int variant=style(c);
    // The new menu strip is one continuous recessed panel, with live text
    // buttons rather than seven separate capsules baked into artwork.
    if(variant>=3 && bool(c.getProperties().getWithDefault("orbitHeaderMenu",false)) && !hover && !down)return;
    auto top=active?juce::Colour(239,230,199):juce::Colour(30,64,82);
    auto bottom=active?juce::Colour(175,167,143):juce::Colour(5,25,39);
    if(variant==4 && active){top=juce::Colour(179,234,245);bottom=juce::Colour(80,165,192);}
    if(variant==5 && active){top=juce::Colour(249,230,180);bottom=juce::Colour(183,151,95);}
    if(variant==8)
    {
        const bool silver=bool(c.getProperties().getWithDefault("orbitHeaderMenu",false)) || bool(c.getProperties().getWithDefault("orbitHeaderSilverTab",false));
        top=silver?juce::Colour(216,220,227):juce::Colour(65,70,79);
        bottom=silver?juce::Colour(163,169,180):juce::Colour(22,27,34);
    }
    if(hover){top=top.brighter(.12f);bottom=bottom.brighter(.12f);}
    if(down){top=top.darker(.18f);bottom=bottom.darker(.18f);}
    g.setOpacity(c.isEnabled()?1.f:.5f);
    g.setGradientFill(juce::ColourGradient(top,0,b.getY(),bottom,0,b.getBottom(),false));
    const float radius=variant==8?1.5f:style(c)>=2?b.getHeight()*.48f:6.f;
    g.fillRoundedRectangle(b,radius);
    g.setColour(active?juce::Colour(247,239,214):juce::Colour(79,130,151));
    g.drawRoundedRectangle(b,radius,1.f);
    g.setColour(juce::Colours::white.withAlpha(active?.35f:.1f));
    g.drawHorizontalLine(int(b.getY()+2),b.getX()+radius,b.getRight()-radius);
    g.setOpacity(1.f);
}
inline void panel(juce::Graphics& g,juce::Rectangle<float> b,int variant)
{
    const bool glass=variant>=3;
    g.setGradientFill(juce::ColourGradient(juce::Colour(4,20,33).withAlpha(glass?.68f:1.f),0,b.getY(),juce::Colour(12,40,57).withAlpha(glass?.58f:1.f),0,b.getBottom(),false));
    g.fillRoundedRectangle(b,variant>=2?b.getHeight()*.48f:7.f);
    g.setColour(variant==5?juce::Colour(166,145,101):juce::Colour(68,111,132));g.drawRoundedRectangle(b,variant>=2?b.getHeight()*.48f:7.f,1.f);
    g.setColour(juce::Colours::black.withAlpha(.5f));g.drawHorizontalLine(int(b.getY()+1),b.getX()+9,b.getRight()-9);
}
inline void background(juce::Graphics& g,int width,int variant,const juce::Image& artwork={})
{
    const auto b=juce::Rectangle<float>(0,0,float(width),82.f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(27,55,73),0,0,juce::Colour(4,22,36),0,82,false));g.fillRect(b);
    if(variant>=3 && artwork.isValid())
        g.drawImage(artwork,(variant==6 || variant==7)?b.withHeight(42.f):b);
    if(variant==2)
    {
        // Original vector trim, fitted around the existing two control rows.
        juce::Path trim;trim.startNewSubPath(0,0);trim.lineTo(float(width),0);
        trim.lineTo(float(width),3);trim.cubicTo(width*.87f,3,width*.91f,39,width*.76f,40);
        trim.lineTo(width*.24f,40);trim.cubicTo(width*.09f,39,width*.13f,3,0,3);trim.closeSubPath();
        juce::ColourGradient metal(juce::Colour(190,205,211),0,0,juce::Colour(32,57,74),0,40,false);
        metal.addColour(.45,juce::Colour(86,116,135));metal.addColour(.9,juce::Colour(210,220,222));
        g.setGradientFill(metal);g.fillPath(trim);
    }
    g.setColour(variant==8?juce::Colour(91,98,110):juce::Colour(98,142,160));g.drawHorizontalLine(81,0,float(width));
}
inline void brand(juce::Graphics& g,juce::Rectangle<int> bounds,int variant)
{
    g.setColour(variant==8?juce::Colour(36,43,53):juce::Colour(238,231,207));
    g.setFont(juce::FontOptions(23.f));g.drawFittedText("O R B I T",bounds,juce::Justification::centredLeft,1);
}
inline void compactTransport(juce::Graphics& g,juce::Rectangle<int> bounds)
{
    const auto b=bounds.toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(53,57,66),0,b.getY(),juce::Colour(12,16,22),0,b.getBottom(),false));g.fillRect(b);
    g.setColour(juce::Colour(86,92,102));g.drawRect(b,1.f);
}

}
