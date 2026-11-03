// Copied from Labyrinth for the standalone knob comparison; original renderer unchanged.
#pragma once

#include <JuceHeader.h>

//==============================================================================
// Exact reusable transplant of the Home SpriteKnob visual renderer.
//
// This class copies the actual layer order and formulas from the smaller
// GUI-only Home branch, adapting only the value source from a custom Component
// to JUCE Slider::drawRotarySlider().
//
// The existing Home runtime is intentionally not modified.
//==============================================================================
class FirstSynthReusableHomeKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    struct Style
    {
        float knobShadowAlpha = 0.5000f;
        float knobFeatherAlpha = 0.0000f;
        float knobMatteAlpha = 0.0000f;
        float knobClipInset = 6.4900f;

        float knobHaloAlpha = 0.0000f;
        float knobHaloBlue = 0.0000f;
        float knobHaloSize = 0.0000f;
        float knobHaloFade = 0.0000f;
        float knobShadowY = -0.1200f;
        float knobShadowSpread = 0.2000f;
        float knobOpacity = 1.0000f;
        float knobBgBlend = 1.0000f;

        float spriteEdgeBlend = 1.0000f;
        float spriteEdgeCut = 0.6300f;
        float spriteEdgeCut2 = 0.2500f;
        float spriteBandAlpha = 0.0000f;
        float spriteBandFeather = 0.4500f;
        float spriteEdgeFade = 0.0000f;

        float whiteCircleAlpha = 0.0000f;
        float whiteCircleRadius = 0.0000f;
        float rimRingAlpha = 0.2200f;
        float rimRingRadius = 0.2700f;
        float rimRingGlow = 1.5000f;
        float rimRingThickness = 0.2000f;
        int knobBlendMethod = 3;
        int knobLayerOrder = 2;

        float plateOpacity = 0.0000f;
        float plateFeather = 0.8000f;
        float plateSize = 0.6100f;
        float plateCut = 0.0000f;
        float plateBlur = 0.7100f;
        float plateEdgeSoftness = 0.2400f;
        float plateWarmth = 0.9500f;
        float plateBrightness = 0.9400f;
        float plateBgMix = 0.6500f;

        float contactShadowAlpha = 0.0700f;
        float contactShadowBlur = 0.3100f;
        float contactShadowSpread = 0.3000f;
        float contactShadowX = -0.0500f;
        float contactShadowY = 0.1800f;

        float castShadowAlpha = 1.0000f;
        float castShadowBlur = 0.8300f;
        float castShadowDistance = 0.2600f;
        float castShadowAngle = 102.7400f;
        float castShadowSpread = 0.2000f;

        float shadowOverlayAlpha = 0.0500f;
        float shadowFusionMix = 0.0900f;
        float shadowFusionBlur = 0.0000f;
        int shadowLayerMode = 0;

        float blueRingGlow = 0.2200f;
        float foregroundOpacity = 1.0f;
        float localGlowScale = 1.0f;

        // Geometry controls. The original Home SpriteKnob reserves a label
        // strip before making the remaining knob body square.
        float labelReservePixels = 0.0f;
        float bodyScale = 1.0f;
        float bodyOffsetX = 0.0f;
        float bodyOffsetY = 0.0f;
        float shadowMasterScale = 1.0f;
    };

    void setSpriteImage (juce::Image image)
    {
        spriteImage = image;
    }

    Style& getStyle() noexcept { return style; }
    const Style& getStyle() const noexcept { return style; }

    void setBlendSettings (float innerCutValue,
                           float outerCutValue,
                           float bandAlphaValue,
                           float featherValue)
    {
        style.spriteEdgeCut = juce::jlimit (0.0f, 1.8f, innerCutValue);
        style.spriteEdgeCut2 = juce::jlimit (0.0f, 1.8f, outerCutValue);
        style.spriteBandAlpha = juce::jlimit (0.0f, 1.0f, bandAlphaValue);
        style.spriteBandFeather = juce::jlimit (0.0f, 1.0f, featherValue);
    }

    float getInnerCut() const noexcept { return style.spriteEdgeCut; }
    float getOuterCut() const noexcept { return style.spriteEdgeCut2; }
    float getBandAlpha() const noexcept { return style.spriteBandAlpha; }
    float getBandFeather() const noexcept { return style.spriteBandFeather; }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float, float,
                           juce::Slider&) override
    {
        auto knobArea = juce::Rectangle<float> (
            (float) x, (float) y, (float) width, (float) height);

        // Match the actual Home SpriteKnob geometry: reserve a label strip,
        // then form a square from the remaining body. Granular labels are
        // separate components, but using the same body calculation is vital
        // because every shadow formula is proportional to this rectangle.
        knobArea.removeFromBottom (
            juce::jlimit (0.0f, knobArea.getHeight() * 0.45f,
                          style.labelReservePixels));

        const auto side = juce::jmin (
            knobArea.getWidth(), knobArea.getHeight())
            * juce::jlimit (0.55f, 1.35f, style.bodyScale);

        knobArea = knobArea.withSizeKeepingCentre (side, side)
            .translated (style.bodyOffsetX, style.bodyOffsetY);

        const auto C = [] (juce::uint32 argb)
        {
            return juce::Colour (argb);
        };

        // Soft foreground plate/matte.
        auto plate = knobArea.expanded (
            knobArea.getWidth() * (style.plateSize - style.plateCut));

        const auto warm = C (0xfffff4df);
        const auto cool = C (0xffeef1f4);
        const auto plateBase =
            cool.interpolatedWith (warm, style.plateWarmth)
                .interpolatedWith (
                    C (0xffffffff), style.plateBrightness - 0.75f);
        const auto plateMixed =
            plateBase.interpolatedWith (
                C (0xffedf0f2), style.plateBgMix);

        juce::ColourGradient plateGrad (
            plateMixed.withAlpha (style.plateOpacity),
            plate.getCentreX(), plate.getCentreY(),
            plateMixed.withAlpha (0.0f),
            plate.getRight(), plate.getBottom(), true);

        g.setGradientFill (plateGrad);
        g.fillEllipse (
            plate.expanded (
                knobArea.getWidth()
                * (style.plateFeather
                   + style.plateBlur
                   + style.plateEdgeSoftness)
                * 0.07f));

        // Tight contact shadow.
        auto contact = knobArea.withSizeKeepingCentre (
            knobArea.getWidth()
                * (0.72f
                   + style.contactShadowBlur * 0.20f
                   + style.contactShadowSpread * 0.34f),
            knobArea.getHeight()
                * (0.20f
                   + style.contactShadowBlur * 0.24f
                   + style.contactShadowSpread * 0.10f));

        contact.setCentre (
            knobArea.getCentreX()
                + knobArea.getWidth() * style.contactShadowX,
            knobArea.getCentreY()
                + knobArea.getHeight() * style.contactShadowY);

        juce::ColourGradient contactGrad (
            juce::Colours::black.withAlpha (
                style.contactShadowAlpha * style.shadowMasterScale),
            contact.getCentreX(), contact.getCentreY(),
            juce::Colours::black.withAlpha (0.0f),
            contact.getRight(), contact.getBottom(), true);

        g.setGradientFill (contactGrad);
        g.fillEllipse (contact);

        // Directional cast shadow.
        const auto angleRad =
            juce::degreesToRadians (style.castShadowAngle);
        const float castX =
            std::cos (angleRad)
            * knobArea.getWidth()
            * style.castShadowDistance * 0.30f;
        const float castY =
            std::sin (angleRad)
            * knobArea.getHeight()
            * style.castShadowDistance * 0.30f;

        auto shadow = knobArea.withSizeKeepingCentre (
            knobArea.getWidth()
                * (style.castShadowSpread
                   + style.castShadowBlur * 0.35f),
            knobArea.getHeight()
                * (0.44f + style.castShadowBlur * 0.22f));

        shadow.setCentre (
            knobArea.getCentreX() + castX,
            knobArea.getCentreY() + castY);

        juce::ColourGradient castGradient (
            juce::Colours::black.withAlpha (
                style.castShadowAlpha * style.shadowMasterScale * 0.38f),
            shadow.getCentreX(), shadow.getCentreY(),
            juce::Colours::black.withAlpha (0.0f),
            shadow.getRight(), shadow.getBottom(), true);

        g.setGradientFill (castGradient);
        g.fillEllipse (shadow);

        // Off-white/blue halo.
        auto halo = knobArea.expanded (
            knobArea.getWidth()
            * style.knobHaloSize
            * (0.35f + style.knobHaloFade));

        auto haloColour =
            C (0xfff9f3d7).interpolatedWith (
                C (0xff6fd2ff), style.knobHaloBlue);

        juce::ColourGradient haloGradient (
            haloColour.withAlpha (
                juce::jlimit (
                    0.0f, 1.0f,
                    (style.knobHaloAlpha
                     + style.blueRingGlow * 0.45f)
                    * style.localGlowScale)),
            halo.getCentreX(), halo.getCentreY(),
            haloColour.withAlpha (0.0f),
            halo.getRight(), halo.getBottom(), true);

        g.setGradientFill (haloGradient);
        g.fillEllipse (halo);

        // Background feather.
        auto feather = knobArea.expanded (
            knobArea.getWidth()
            * (0.10f
               + style.whiteCircleRadius * 0.20f
               + style.knobBgBlend * 0.08f));

        juce::ColourGradient featherGradient (
            C (0xfff2f4f6).withAlpha (style.whiteCircleAlpha),
            feather.getCentreX(), feather.getCentreY(),
            C (0xfff2f4f6).withAlpha (0.0f),
            feather.getRight(), feather.getBottom(), true);

        if (style.knobBlendMethod != 2)
        {
            g.setGradientFill (featherGradient);
            g.fillEllipse (feather);
        }

        auto drawRimRing =
            [&] (juce::Graphics& gg, juce::Rectangle<float> base)
        {
            if (style.rimRingAlpha * style.localGlowScale <= 0.001f)
                return;

            auto ring = base.expanded (
                base.getWidth() * style.rimRingRadius);

            const auto glowWidth = juce::jmax (
                style.rimRingThickness,
                style.rimRingThickness + style.rimRingGlow * 7.0f);

            gg.setColour (
                C (0xff168cff).withAlpha (
                    juce::jlimit (
                        0.0f, 1.0f,
                        style.rimRingAlpha
                        * style.localGlowScale
                        * 0.36f
                        * (0.35f + style.rimRingGlow))));

            gg.drawEllipse (
                ring.expanded (style.rimRingGlow * 2.5f),
                glowWidth);

            gg.setColour (
                C (0xff168cff).withAlpha (
                    juce::jlimit (
                        0.0f, 1.0f,
                        style.rimRingAlpha
                        * style.localGlowScale)));

            gg.drawEllipse (ring, style.rimRingThickness);
        };

        auto drawVisibleShadowFusion =
            [&] (juce::Graphics& gg, juce::Rectangle<float> base)
        {
            if (style.shadowLayerMode == 0)
                return;

            const float alpha = juce::jlimit (
                0.0f, 1.0f,
                (style.shadowOverlayAlpha
                 + style.shadowFusionMix * 0.45f)
                * style.shadowMasterScale);

            if (alpha <= 0.001f)
                return;

            auto lower = base.withTrimmedTop (
                base.getHeight()
                * (0.42f - style.shadowFusionBlur * 0.08f));

            lower = lower.expanded (
                base.getWidth()
                    * (0.04f
                       + style.shadowFusionBlur * 0.08f),
                base.getHeight()
                    * (0.04f
                       + style.shadowFusionBlur * 0.06f));

            juce::ColourGradient fusion (
                juce::Colours::black.withAlpha (0.0f),
                lower.getCentreX(), lower.getY(),
                juce::Colours::black.withAlpha (alpha),
                lower.getCentreX(), lower.getBottom(), false);

            gg.setGradientFill (fusion);
            gg.fillEllipse (lower);

            if (style.shadowFusionMix > 0.001f)
            {
                juce::ColourGradient melt (
                    C (0xffeef1f4).withAlpha (0.0f),
                    base.getCentreX(), base.getCentreY(),
                    C (0xffeef1f4).withAlpha (
                        style.shadowFusionMix * 0.20f),
                    base.getRight(), base.getBottom(), true);

                gg.setGradientFill (melt);
                gg.fillEllipse (
                    base.expanded (
                        base.getWidth()
                        * style.shadowFusionBlur * 0.045f));
            }
        };

        auto destination =
            knobArea.reduced (style.knobClipInset);

        if (style.knobLayerOrder == 1)
            drawRimRing (g, destination);

        if (spriteImage.isValid())
        {
            constexpr int frameSize = 192;
            constexpr int columns = 16;
            constexpr int frameCount = 128;

            const bool looksLikeAtlas =
                spriteImage.getWidth() >= frameSize * columns
                && spriteImage.getHeight() >= frameSize;

            juce::Graphics::ScopedSaveState save (g);

            juce::Path clip;
            const auto clipInset =
                destination.getWidth()
                * style.spriteEdgeCut * 0.38f;
            clip.addEllipse (
                destination.reduced (clipInset).expanded (0.5f));
            g.reduceClipRegion (clip);

            g.setOpacity (
                style.knobOpacity * style.foregroundOpacity);

            if (looksLikeAtlas)
            {
                const int frame = juce::jlimit (
                    0, frameCount - 1,
                    juce::roundToInt (
                        sliderPos * (float) (frameCount - 1)));
                const int column = frame % columns;
                const int row = frame / columns;

                g.drawImage (
                    spriteImage,
                    destination.getX(), destination.getY(),
                    destination.getWidth(), destination.getHeight(),
                    column * frameSize, row * frameSize,
                    frameSize, frameSize);
            }
            else
            {
                g.drawImage (
                    spriteImage,
                    destination,
                    juce::RectanglePlacement::centred
                        | juce::RectanglePlacement::onlyReduceInSize,
                    false);
            }

            if (style.knobLayerOrder == 0
                || style.knobLayerOrder == 2)
                drawRimRing (g, destination);

            if (style.spriteEdgeBlend > 0.001f
                || style.spriteBandAlpha > 0.001f)
            {
                const float inner = juce::jlimit (
                    0.0f, 0.92f,
                    style.spriteEdgeCut * 0.38f);

                const float outer = juce::jlimit (
                    inner + 0.001f, 0.96f,
                    style.spriteEdgeCut2 * 0.38f);

                const auto innerArea =
                    destination.reduced (
                        destination.getWidth() * inner);
                const auto outerArea =
                    destination.reduced (
                        destination.getWidth() * outer);

                const auto edgeArea =
                    destination.expanded (
                        destination.getWidth()
                        * style.spriteEdgeFade * 0.035f);

                juce::ColourGradient edgeFade (
                    C (0xffeef1f4).withAlpha (0.0f),
                    edgeArea.getCentreX(), edgeArea.getCentreY(),
                    C (0xffeef1f4).withAlpha (
                        style.spriteEdgeBlend),
                    edgeArea.getRight(), edgeArea.getBottom(), true);

                g.setGradientFill (edgeFade);
                g.fillEllipse (edgeArea);

                if (style.spriteBandAlpha > 0.001f)
                {
                    const auto bandArea =
                        innerArea.expanded (
                            destination.getWidth()
                            * style.spriteBandFeather * 0.025f);

                    juce::ColourGradient bandFade (
                        C (0xffeef1f4).withAlpha (0.0f),
                        outerArea.getCentreX(),
                        outerArea.getCentreY(),
                        C (0xffeef1f4).withAlpha (
                            style.spriteBandAlpha),
                        bandArea.getRight(),
                        bandArea.getBottom(),
                        true);

                    g.setGradientFill (bandFade);
                    g.fillEllipse (bandArea);
                }
            }

            g.setOpacity (1.0f);

            if (style.knobMatteAlpha > 0.001f)
            {
                juce::ColourGradient matte (
                    C (0xfff4f6f8).withAlpha (0.0f),
                    destination.getCentreX(),
                    destination.getCentreY(),
                    C (0xfff4f6f8).withAlpha (
                        style.knobMatteAlpha),
                    destination.getRight(),
                    destination.getBottom(),
                    true);

                g.setGradientFill (matte);
                g.fillEllipse (destination);
            }
        }

        if (style.shadowLayerMode == 1
            || style.shadowLayerMode == 2)
            drawVisibleShadowFusion (g, destination);
    }

private:
    juce::Image spriteImage;
    Style style;
};
