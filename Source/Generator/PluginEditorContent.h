#pragma once
#include "NativeViewClip.h"

class OrbitPluginEditorContent final : public te::Plugin::EditorComponent, private juce::ComponentListener, private juce::AsyncUpdater
{
public:
    explicit OrbitPluginEditorContent(te::ExternalPlugin& external):externalOwner(&external)
    {initialiseSurface();setSize(preferredWidth,preferredHeight);triggerAsyncUpdate();}
    explicit OrbitPluginEditorContent(std::unique_ptr<juce::AudioProcessorEditor> supplied)
        : editor(std::move(supplied))
    {
        initialiseSurface();attachEditor();setSize(preferredWidth,preferredHeight);
    }
    ~OrbitPluginEditorContent() override
    {cancelPendingUpdate();nativeClip.clear();if(editor!=nullptr)editor->removeComponentListener(this);}
    static std::unique_ptr<juce::AudioProcessorEditor> createNativeEditor(te::ExternalPlugin& external)
    {
        auto* instance=external.getAudioPluginInstance();if(instance==nullptr)return {};
        auto native=std::unique_ptr<juce::AudioProcessorEditor>(instance->createEditorIfNeeded());
        if(native==nullptr)native=std::make_unique<juce::GenericAudioProcessorEditor>(*instance);
        return native;
    }

    void setRequestedScale(float scale)
    {
        requestedScale = scale;
        resized();
    }

    int getPreferredWidth() const { return preferredWidth; }
    int getPreferredHeight() const { return preferredHeight; }
    std::function<void()> preferredSizeChanged;
    bool editorReady() const {return editor!=nullptr;}
    bool isEditorFor(te::Plugin* plugin) const
    {
        auto* external=externalOwner.get();
        return external!=nullptr && external==plugin
            && (editor==nullptr || editor->getAudioProcessor()==external->getAudioPluginInstance());
    }
    bool ensureEditorCreated()
    {
        if(editor!=nullptr)return true;
        if(getPeer()==nullptr || !isShowing())return false;
        if(auto* external=dynamic_cast<te::ExternalPlugin*>(externalOwner.get()))
        {
            editor=createNativeEditor(*external);attachEditor();
            if(preferredSizeChanged)preferredSizeChanged();
            resized();
        }
        return editor!=nullptr;
    }
    void syncNativeViews(const std::vector<juce::Rectangle<int>>& blocked={})
    {nativeOcclusion=blocked;refreshNativeViews();}
    void parentHierarchyChanged() override {triggerAsyncUpdate();}
    void visibilityChanged() override {if(!isShowing())nativeClip.clear();else triggerAsyncUpdate();}
    void paint(juce::Graphics& g) override
    {if(editor==nullptr){g.fillAll(orbit::options::colour(24,37,47));g.setColour(juce::Colours::white);g.drawText("Opening instrument editor...",getLocalBounds(),juce::Justification::centred);}}


    bool allowWindowResizing() override { return editor != nullptr && editor->isResizable(); }
    juce::ComponentBoundsConstrainer* getBoundsConstrainer() override
    {
        return editor != nullptr ? editor->getConstrainer() : nullptr;
    }
    void resized() override
    {
        if(editor==nullptr || layingOut || getWidth()<=0 || getHeight()<=0)return;
        const juce::ScopedValueSetter<bool> guard(layingOut,true);
        const float fit=juce::jmin(float(getWidth())/preferredWidth,float(getHeight())/preferredHeight);
        const float scale=juce::jmax(0.001f,requestedScale<=0?fit:juce::jmin(requestedScale,fit));
        if(std::abs(scale-appliedScale)>0.001f)
        {
            // Generic editors scale their Component transform; native VST3
            // editors can instead change their reported size, or ignore DPI.
            scalingBody.setTransform({});
            const float previous=appliedScale>0?appliedScale:1.0f;
            const int beforeW=editor->getWidth(),beforeH=editor->getHeight();
            editor->setScaleFactor(scale);
            if(beforeW>0 && beforeH>0 && (beforeW!=editor->getWidth() || beforeH!=editor->getHeight()))
                reportsScaledSize=std::abs(editor->getWidth()/float(beforeW)-scale/previous)<0.03f
                    && std::abs(editor->getHeight()/float(beforeH)-scale/previous)<0.03f;
            appliedScale=scale;
        }
        const auto visible=editor->getLocalBounds().toFloat().transformedBy(editor->getTransform());
        const float residual=juce::jmax(0.001f,juce::jmin(float(getWidth())/juce::jmax(1.0f,visible.getWidth()),
                                                        float(getHeight())/juce::jmax(1.0f,visible.getHeight())));
        // Keep fallback scaling on a plain parent Component. Mutating the
        // AudioProcessorEditor's own transform bypasses JUCE's host-scale state.
        scalingBody.setBounds(0,0,juce::jmax(1,int(std::ceil(visible.getWidth()))),juce::jmax(1,int(std::ceil(visible.getHeight()))));
        scalingBody.setTransform(juce::AffineTransform::scale(residual));
        // Never centre an oversized native view at a negative origin: on macOS
        // it can draw above its JUCE clipping parent and cover Orbit's toolbar.
        editor->setTopLeftPosition(0,0);
    }

private:
    void initialiseSurface()
    {scalingBody.setInterceptsMouseClicks(false,true);addAndMakeVisible(scalingBody);setOpaque(false);}
    void attachEditor()
    {
        if(editor==nullptr)return;
        preferredWidth=juce::jmax(1,editor->getWidth());preferredHeight=juce::jmax(1,editor->getHeight());
        editor->setTopLeftPosition(0,0);scalingBody.addAndMakeVisible(*editor);editor->addComponentListener(this);
    }
    void componentMovedOrResized(juce::Component&,bool,bool wasResized) override
    {if(!layingOut && wasResized){naturalSizeChanged=true;triggerAsyncUpdate();}}
    void handleAsyncUpdate() override
    {
        if(!ensureEditorCreated())return;
        if(naturalSizeChanged)
        {
            naturalSizeChanged=false;
            const float denominator=reportsScaledSize?juce::jmax(0.001f,appliedScale):1.0f;
            preferredWidth=juce::jmax(1,juce::roundToInt(editor->getWidth()/denominator));
            preferredHeight=juce::jmax(1,juce::roundToInt(editor->getHeight()/denominator));
            if(preferredSizeChanged)preferredSizeChanged();
        }
        resized();refreshNativeViews();
    }
    void refreshNativeViews()
    {if(editor!=nullptr)nativeClip.update(*editor,*this,nativeOcclusion);}
    bool layingOut=false,naturalSizeChanged=false,reportsScaledSize=false;
    te::SafeSelectable<te::ExternalPlugin> externalOwner;
    std::vector<juce::Rectangle<int>> nativeOcclusion;
    orbit::generator::NativeViewClip nativeClip;
    juce::Component scalingBody;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    int preferredWidth = 320;
    int preferredHeight = 120;
    float requestedScale = 0.0f; // Auto Fit
    float appliedScale = -1.0f;
};

