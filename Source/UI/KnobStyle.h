#pragma once
#include "LabyrinthHomeKnob.h"
#include "LabyrinthHomeAtlas.h"
#include "KnobSprite.h"
namespace orbit::ui
{
// Appearance preferences only; these never change Slider values or audio state.
inline int currentKnobStyle=0;
inline const juce::StringArray knobStyleNames{"Skin default","Original","Labyrinth White"};
inline const juce::StringArray generatorKnobStyleNames{"Follow global","Skin default","Original","Labyrinth White"};
inline const juce::Image& homeKnobAtlas()
{
    static const juce::Image atlas=[] {juce::MemoryOutputStream out;juce::Base64::convertFromBase64(out,orbit::knobs::assets::knobSpriteBase64);return juce::ImageFileFormat::loadFrom(out.getData(),out.getDataSize());}();return atlas;
}
inline int effectiveKnobStyle(const juce::Component& component,int skinDefault=0)
{
    int choice=juce::jlimit(0,2,currentKnobStyle);
    for(auto* c=&component;c!=nullptr;c=c->getParentComponent())
        if(c->getProperties().contains("orbitKnobOverride"))
        {
            const int local=int(c->getProperties()["orbitKnobOverride"]);
            if(local>=0){choice=juce::jlimit(0,2,local);break;}
        }
    return choice==0?skinDefault:choice;
}
inline void configureCleanWhiteKnob(FirstSynthReusableHomeKnobLookAndFeel& look)
{
    auto& style=look.getStyle();style.localGlowScale=0;style.blueRingGlow=0;
    style.rimRingAlpha=0;style.spriteEdgeBlend=0;style.spriteBandAlpha=0;
    style.shadowMasterScale=.2f;style.castShadowAlpha=0;style.shadowOverlayAlpha=0;
}
inline bool lightMixerSurface(const juce::Component& component)
{for(auto* c=&component;c!=nullptr;c=c->getParentComponent())if(c->getProperties().contains("orbitMixerLightSurface"))return bool(c->getProperties()["orbitMixerLightSurface"]);return false;}
inline void configureLightWhiteKnob(FirstSynthReusableHomeKnobLookAndFeel& look)
{
    configureCleanWhiteKnob(look);auto& style=look.getStyle();
    style.shadowMasterScale=1;
    // Broad cast shadows are painted by the channel strip behind labels.
    // Keep the sprite renderer free of shadows clipped to the slider canvas.
    style.contactShadowAlpha=0;style.castShadowAlpha=0;
}
inline int mixerKnobFinish(const juce::Component& c)
{for(auto* p=&c;p!=nullptr;p=p->getParentComponent())if(p->getProperties().contains("orbitMixerKnobFinish")){int f=int(p->getProperties()["orbitMixerKnobFinish"]);if(f>0)return f;}for(auto* p=&c;p!=nullptr;p=p->getParentComponent())if(p->getProperties().contains("orbitMixerDefaultFinish"))return int(p->getProperties()["orbitMixerDefaultFinish"]);return 1;}
inline int mixerKnobDefault(const juce::Component& c)
{for(auto* p=&c;p!=nullptr;p=p->getParentComponent())if(bool(p->getProperties().getWithDefault("orbitMixerUseWhite",false)))return 2;return 0;}
inline const juce::Image& mixerKnobAtlas(int finish)
{
    // Cached appearance variants: original atlas and alpha remain untouched.
    static const auto variants=[] {
        std::array<juce::Image,2> result;
        for(int variant=0;variant<2;++variant){auto& image=result[size_t(variant)];image=homeKnobAtlas().createCopy();juce::Image::BitmapData pixels(image,juce::Image::BitmapData::readWrite);
            for(int y=0;y<image.getHeight();++y)for(int x=0;x<image.getWidth();++x){auto c=pixels.getPixelColour(x,y);if(c.getAlpha()==0)continue;const float r=c.getFloatRed(),g=c.getFloatGreen(),b=c.getFloatBlue();const float blue=juce::jlimit(0.f,1.f,(b-r-.025f)/.18f);float base=.299f*r+.587f*g+.114f*b;if(variant==1)base=juce::jlimit(0.f,1.f,base*.95f-.024f);auto neutral=juce::Colour::fromFloatRGBA(base,base,base,c.getFloatAlpha());auto body=variant==1?neutral:c;auto marker=juce::Colour(35,39,42).withAlpha(c.getAlpha());pixels.setPixelColour(x,y,body.interpolatedWith(marker,blue));}}
        return result;
    }();
    return finish==2?variants[0]:finish==3?variants[1]:homeKnobAtlas();
}
inline double mixerKnobScale(const juce::Component& component)
{for(auto* c=&component;c!=nullptr;c=c->getParentComponent())if(c->getProperties().contains("orbitMixerKnobScale"))return double(c->getProperties()["orbitMixerKnobScale"]);return 1.;}
inline juce::Rectangle<float> mixerKnobBounds(const juce::Slider& slider,juce::Rectangle<float> canvas,bool white=false)
{
    const float available=juce::jmin(canvas.getWidth(),canvas.getHeight());
    const float base=float(slider.getProperties().getWithDefault("orbitMixerBaseKnobSize",available));
    // The Home sprite reserves ~13 px around its face. Scale the face, not
    // that padding, and fit the complete face inside the fixed control cell.
    const float requested=white?13.f+juce::jmax(1.f,base-13.f)*float(mixerKnobScale(slider)):base*float(mixerKnobScale(slider));
    const float limit=white?13.f+juce::jmax(1.f,available-4.f)/.53f:juce::jmax(1.f,available-2.f);
    const float size=juce::jmin(requested,limit);
    return canvas.withSizeKeepingCentre(size,size);
}
inline bool isMixerKnob(const juce::Slider& slider){return slider.getProperties().contains("orbitMixerBaseKnobSize");}
inline bool mixerParentShadows(const juce::Component& c)
{for(auto* p=&c;p!=nullptr;p=p->getParentComponent())if(p->getProperties().contains("orbitMixerParentShadows"))return bool(p->getProperties()["orbitMixerParentShadows"]);return false;}
inline void drawMixerParentShadow(juce::Graphics& g,const juce::Slider& knob)
{
    if(sprites::forComponent(knob)!=nullptr)return; // Custom strips include their own lighting/shadows.
    if(effectiveKnobStyle(knob,mixerKnobDefault(knob))!=2 || !lightMixerSurface(knob) || !isMixerKnob(knob))return;
    const auto body=mixerKnobBounds(knob,knob.getBounds().toFloat(),true);
    const auto diameter=juce::jmax(3.f,(juce::jmin(body.getWidth(),body.getHeight())-13.f)*.53f);
    const auto centre=body.getCentre();
    const auto oval=[&](float width,float height,float dx,float dy,float alpha)
    {
        juce::Graphics::ScopedSaveState state(g);const auto cx=centre.x+dx*diameter,cy=centre.y+dy*diameter;
        const float w=diameter*width,h=diameter*height;
        g.addTransform(juce::AffineTransform::scale(1.f,h/w,cx,cy));
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(alpha),cx,cy,juce::Colours::transparentBlack,cx+w*.5f,cy,true));
        g.fillEllipse(cx-w*.5f,cy-w*.5f,w,w);
    };
    // Painted by the strip before labels/knob faces, with room to cast beyond
    // the Slider bounds. Text remains above the shadow without cutting it off.
    oval(2.15f,1.12f,-.05f,.39f,.32f);oval(1.42f,.62f,0,.37f,.22f);
}
inline bool drawSelectedKnob(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider& slider,int skinDefault=0)
{
    if(isMixerKnob(slider))if(const auto* sprite=sprites::forComponent(slider))
    {
        // Imported artwork uses the complete fixed cell at 500%. Unlike the
        // old clamp, 100..500 remains visibly adjustable even in short rows.
        const auto canvas=juce::Rectangle<float>(float(x),float(y),float(width),float(height));
        const float size=juce::jmax(1.f,juce::jmin(float(width),float(height))-2.f)*float(mixerKnobScale(slider))/5.f;
        sprite->draw(g,canvas.withSizeKeepingCentre(size,size),position);return true;
    }
    const int style=effectiveKnobStyle(slider,isMixerKnob(slider)?mixerKnobDefault(slider):skinDefault);if(style==0)return false;
    if(isMixerKnob(slider))
    {
        const auto canvas=juce::Rectangle<float>(float(x),float(y),float(width),float(height));
        const auto b=mixerKnobBounds(slider,{float(x),float(y),float(width),float(height)},style==2).toNearestInt();
        x=b.getX();y=b.getY();width=b.getWidth();height=b.getHeight();
        if(style==2)
        {
            static FirstSynthReusableHomeKnobLookAndFeel clean;
            static const bool loaded=[] {clean.setSpriteImage(homeKnobAtlas());configureCleanWhiteKnob(clean);return true;}();juce::ignoreUnused(loaded);
            const int finish=mixerKnobFinish(slider);
            if(finish>=2)
            {
                static std::array<FirstSynthReusableHomeKnobLookAndFeel,2> alternate;static bool ready=false;
                if(!ready){for(int i=0;i<2;++i){alternate[size_t(i)].setSpriteImage(mixerKnobAtlas(i+2));configureLightWhiteKnob(alternate[size_t(i)]);}ready=true;}
                alternate[size_t(finish-2)].drawRotarySlider(g,x,y,width,height,position,start,end,slider);return true;
            }
            if(lightMixerSurface(slider))
            {
                static FirstSynthReusableHomeKnobLookAndFeel light;
                static const bool ready=[] {light.setSpriteImage(homeKnobAtlas());configureLightWhiteKnob(light);return true;}();juce::ignoreUnused(ready);
                // Native feathered oval has zero opacity before the cell edge;
                // label components never cut a rectangular edge into the shadow.
                auto shadow=canvas.reduced(1.f).withTrimmedTop(canvas.getHeight()*.37f);
                if(!mixerParentShadows(slider)){
                    juce::Graphics::ScopedSaveState state(g);
                    const auto cx=shadow.getCentreX(),cy=shadow.getCentreY();
                    g.addTransform(juce::AffineTransform::scale(1.f,shadow.getHeight()/shadow.getWidth(),cx,cy));
                    auto circle=shadow.withSizeKeepingCentre(shadow.getWidth(),shadow.getWidth());
                    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.18f),cx,cy,
                        juce::Colours::transparentBlack,circle.getRight(),cy,true));
                    g.fillEllipse(circle);
                }
                light.drawRotarySlider(g,x,y,width,height,position,start,end,slider);
            }
            else clean.drawRotarySlider(g,x,y,width,height,position,start,end,slider);
            return true;
        }
    }
    if(style==1){static juce::LookAndFeel_V4 original;original.drawRotarySlider(g,x,y,width,height,position,start,end,slider);}
    else
    {
        static FirstSynthReusableHomeKnobLookAndFeel white;
        static const bool loaded=[] {white.setSpriteImage(homeKnobAtlas());return true;}();juce::ignoreUnused(loaded);
        white.drawRotarySlider(g,x,y,width,height,position,start,end,slider);
    }
    return true;
}
inline void repaintKnobTree(juce::Component& c)
{c.repaint();for(auto* child:c.getChildren())repaintKnobTree(*child);}
}
