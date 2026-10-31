#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include <cmath>

namespace orbit::mixing
{
// Orbit's optional symmetric soft saturation after its channel EQ.
// This is an original response, not an emulation of Orion's proprietary EQ.
class EqSaturationPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitEqSaturation";
    explicit EqSaturationPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info){}
    ~EqSaturationPlugin() override {notifyListenersOfDeletion();}
    juce::String getName() const override {return "EQ Soft Saturation";}
    juce::String getPluginType() override {return xmlTypeName;}
    juce::String getSelectableDescription() override {return "Channel EQ soft saturation";}
    bool takesAudioInput() override {return true;}
    bool canBeAddedToClip() override {return false;}
    bool canBeAddedToRack() override {return false;}
    int getNumOutputChannelsGivenInputs(int channels) override {return channels;}
    void initialise(const tracktion::engine::PluginInitialisationInfo&) override {}
    void deinitialise() override {}
    void restorePluginStateFromValueTree(const juce::ValueTree&) override {}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& context) override
    {
        if(context.destBuffer==nullptr)return;
        for(int channel=0;channel<context.destBuffer->getNumChannels();++channel)
        {
            auto* samples=context.destBuffer->getWritePointer(channel,context.bufferStartSample);
            for(int i=0;i<context.bufferNumSamples;++i)samples[i]=std::isfinite(samples[i])?std::tanh(samples[i]):0.0f;
        }
    }
};
}
