#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "PeakCompressor.h"
#include <atomic>

namespace orbit::mixing
{
class PeakCompressorPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitPeakCompressor";
    inline static constexpr std::array<const char*,8> ids{"inputDb","thresholdDb","ratio","outputDb","detectorEqDb","attackMs","releaseMs","mix"};
    inline static constexpr std::array<const char*,8> names{"Input","Threshold","Ratio","Output","Detector EQ","Attack","Release","Dry/Wet"};
    inline static constexpr std::array<float,8> defaults{0,-18,4,0,0,10,150,100};
    inline static constexpr std::array<float,8> minima{-24,-60,1,-24,-12,.1f,10,0};
    inline static constexpr std::array<float,8> maxima{36,0,100,24,12,200,2000,100};
    explicit PeakCompressorPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info)
    {
        for(size_t i=0;i<8;++i)
        {
            values[i].referTo(state,juce::Identifier(ids[i]),getUndoManager(),defaults[i]);
            juce::NormalisableRange<float> range(minima[i],maxima[i]);
            if(i==2 || i==5 || i==6)range.setSkewForCentre(defaults[i]);
            const auto suffix=i==2?"":i==5 || i==6?" ms":i==7?" %":" dB";
            parameters[i]=new tracktion::engine::SuffixedParameter(ids[i],names[i],*this,range,suffix);
            if(i==2)
            {
                parameters[i]->valueToStringFunction=[](float value){return value>=99.5f?juce::String("INF : 1"):juce::String(value,2)+" : 1";};
                parameters[i]->stringToValueFunction=[](const juce::String& value){return value.containsIgnoreCase("inf")?100.0f:value.getFloatValue();};
            }
            parameters[i]->attachToCurrentValue(values[i]);addAutomatableParameter(parameters[i]);
        }
    }
    ~PeakCompressorPlugin() override
    {for(auto& p:parameters)p->detachFromCurrentValue();notifyListenersOfDeletion();}
    juce::String getName() const override {return "Peak Compressor";}
    juce::String getPluginType() override {return xmlTypeName;}
    juce::String getSelectableDescription() override {return "Orbit clean digital peak compressor";}
    bool takesAudioInput() override {return true;}
    bool canBeAddedToClip() override {return false;}
    bool canBeAddedToRack() override {return false;}
    int getNumOutputChannelsGivenInputs(int channels) override {return juce::jmin(2,channels);}
    void getChannelNames(juce::StringArray* inputs,juce::StringArray* outputs) override
    {
        if(inputs!=nullptr)inputs->addArray({"Left","Right","Sidechain Left","Sidechain Right"});
        if(outputs!=nullptr)outputs->addArray({"Left","Right"});
    }
    double getLatencySeconds() override {return processor.latencySamples()/processor.sampleRate();}
    double getTailLength() const override {return processor.latencySamples()/processor.sampleRate();}
    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override
    {processor.prepare(info.sampleRate);haveLastEnd=false;}
    void deinitialise() override {processor.reset();haveLastEnd=false;inputMeter=outputMeter=reductionMeter=0;}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {for(size_t i=0;i<8;++i)values[i]=float(tree.getProperty(ids[i],defaults[i]));}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& context) override
    {
        if(context.destBuffer==nullptr || context.destBuffer->getNumChannels()==0)return;
        const double start=context.editTime.getStart().inSeconds(),tolerance=2/processor.sampleRate();
        const auto loop=edit.getTransport().getLoopRange();
        const bool wrap=context.isPlaying && edit.getTransport().looping.get()
            && std::abs(lastEnd-loop.getEnd().inSeconds())<tolerance && std::abs(start-loop.getStart().inSeconds())<tolerance;
        // Offline rendering primes the graph, then rewinds its time range to
        // export the latency-compensated head. It is not a user seek. Native
        // initialise resets each render; only live timeline jumps flush here.
        if(!context.isRendering && haveLastEnd && lastEnd>=0 && std::abs(start-lastEnd)>tolerance && !wrap)processor.reset();
        lastEnd=context.editTime.getEnd().inSeconds();haveLastEnd=true;
        processor.setControls({parameters[0]->getCurrentValue(),parameters[1]->getCurrentValue(),parameters[2]->getCurrentValue(),parameters[3]->getCurrentValue(),parameters[4]->getCurrentValue(),parameters[5]->getCurrentValue(),parameters[6]->getCurrentValue(),parameters[7]->getCurrentValue()*.01});
        auto& buffer=*context.destBuffer;
        // Native sidechain routing places its stereo trigger after the main pair.
        // An unresolved selected source supplies silence; never fall back to the
        // audible input while retaining an external-source selection.
        const bool external=getSidechainSourceID().isValid();
        double input=0,output=0,reduction=0;
        for(int sample=0;sample<context.bufferNumSamples;++sample)
        {
            const int i=context.bufferStartSample+sample;
            const float left=buffer.getSample(0,i),right=buffer.getNumChannels()>1?buffer.getSample(1,i):left;
            const float sideLeft=external && buffer.getNumChannels()>2?buffer.getSample(2,i):0;
            const float sideRight=external && buffer.getNumChannels()>3?buffer.getSample(3,i):sideLeft;
            const auto result=processor.process(left,right,sideLeft,sideRight,external);
            buffer.setSample(0,i,result[0]);if(buffer.getNumChannels()>1)buffer.setSample(1,i,result[1]);
            input=juce::jmax(input,processor.inputPeak);output=juce::jmax(output,processor.outputPeak);reduction=juce::jmax(reduction,processor.reductionDb);
        }
        for(int channel=2;channel<buffer.getNumChannels();++channel)buffer.clear(channel,context.bufferStartSample,context.bufferNumSamples);
        inputMeter.store(float(input),std::memory_order_relaxed);outputMeter.store(float(output),std::memory_order_relaxed);reductionMeter.store(float(reduction),std::memory_order_relaxed);
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,8> parameters;
    std::atomic<float> inputMeter{0},outputMeter{0},reductionMeter{0};
private:
    std::array<juce::CachedValue<float>,8> values;
    PeakCompressor processor;double lastEnd=0;bool haveLastEnd=false;
};
}
