#pragma once
#include <tracktion_engine/tracktion_engine.h>

namespace orbit::mixing
{
// A private final dry-branch gate. Sends earlier in the chain keep their signal.
// A separate type avoids becoming Tracktion's channel VolumeAndPan plugin.
class OutputGatePlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitOutputGate";
    explicit OutputGatePlugin(tracktion::engine::PluginCreationInfo info):Plugin(info){}
    ~OutputGatePlugin() override {notifyListenersOfDeletion();}
    juce::String getName() const override {return "Output Routing Gate";}
    juce::String getPluginType() override {return xmlTypeName;}
    juce::String getSelectableDescription() override {return "Private channel output routing";}
    bool takesAudioInput() override {return true;}
    bool canBeAddedToClip() override {return false;}
    bool canBeAddedToRack() override {return false;}
    int getNumOutputChannelsGivenInputs(int channels) override {return channels;}
    void initialise(const tracktion::engine::PluginInitialisationInfo&) override {}
    void deinitialise() override {}
    void restorePluginStateFromValueTree(const juce::ValueTree&) override {}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& context) override
    {
        if(context.destBuffer!=nullptr)context.destBuffer->clear(context.bufferStartSample,context.bufferNumSamples);
    }
};
}
