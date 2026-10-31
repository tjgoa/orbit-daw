#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "LookaheadLimiter.h"
#include <atomic>

namespace orbit::mixing
{
class LookaheadLimiterPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitLookaheadLimiter";
    explicit LookaheadLimiterPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info)
    {
        input.referTo(state,"inputDb",getUndoManager(),0.0f);
        release.referTo(state,"releaseMs",getUndoManager(),100.0f);
        parameters[0]=new tracktion::engine::SuffixedParameter("inputDb","Input",*this,{-24,36}," dB");
        juce::NormalisableRange<float> time(5,2000);time.setSkewForCentre(100);
        parameters[1]=new tracktion::engine::SuffixedParameter("releaseMs","Release",*this,time," ms");
        parameters[0]->attachToCurrentValue(input);parameters[1]->attachToCurrentValue(release);
        for(auto& p:parameters)addAutomatableParameter(p);
    }
    ~LookaheadLimiterPlugin() override
    {for(auto& p:parameters)p->detachFromCurrentValue();notifyListenersOfDeletion();}
    juce::String getName() const override {return "Look-ahead Brickwall Limiter";}
    juce::String getPluginType() override {return xmlTypeName;}
    juce::String getSelectableDescription() override {return "Orbit sample-peak look-ahead limiter";}
    bool takesAudioInput() override {return true;}
    bool canBeAddedToClip() override {return false;}
    bool canBeAddedToRack() override {return false;}
    int getNumOutputChannelsGivenInputs(int channels) override {return juce::jmin(2,channels);}
    double getLatencySeconds() override {return processor.latencySamples()/processor.sampleRate();}
    double getTailLength() const override {return processor.latencySamples()/processor.sampleRate();}
    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override
    {processor.prepare(info.sampleRate);lastEnd=-1;}
    void deinitialise() override {processor.reset();}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {input=float(tree.getProperty("inputDb",0.0f));release=float(tree.getProperty("releaseMs",100.0f));}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& context) override
    {
        if(context.destBuffer==nullptr || context.destBuffer->getNumChannels()==0)return;
        const double start=context.editTime.getStart().inSeconds();
        const auto loop=edit.getTransport().getLoopRange();
        const double tolerance=2/processor.sampleRate();
        const bool loopWrap=context.isPlaying && edit.getTransport().looping.get()
            && std::abs(lastEnd-loop.getEnd().inSeconds())<tolerance
            && std::abs(start-loop.getStart().inSeconds())<tolerance;
        if(lastEnd>=0 && std::abs(start-lastEnd)>tolerance && !loopWrap)processor.reset();
        lastEnd=context.editTime.getEnd().inSeconds();
        processor.setControls(parameters[0]->getCurrentValue(),parameters[1]->getCurrentValue());
        auto& buffer=*context.destBuffer;double peak=0,reduction=0;
        for(int sample=0;sample<context.bufferNumSamples;++sample)
        {
            const auto i=context.bufferStartSample+sample;
            const float left=buffer.getSample(0,i),right=buffer.getNumChannels()>1?buffer.getSample(1,i):left;
            const auto out=processor.process(left,right);
            buffer.setSample(0,i,out[0]);if(buffer.getNumChannels()>1)buffer.setSample(1,i,out[1]);
            peak=juce::jmax(peak,processor.inputPeak);reduction=juce::jmax(reduction,processor.reductionDb);
        }
        inputMeter.store(float(peak),std::memory_order_relaxed);reductionMeter.store(float(reduction),std::memory_order_relaxed);
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,2> parameters;
    std::atomic<float> inputMeter{0},reductionMeter{0};
private:
    juce::CachedValue<float> input,release;LookaheadLimiter processor;double lastEnd=-1;
};
}
