#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "DistortionCore.h"

namespace orbit::mixing
{
class DistortionPluginBase:public tracktion::engine::Plugin
{
public:
    DistortionPluginBase(tracktion::engine::PluginCreationInfo info,bool tubeMode)
        :Plugin(info),tube(tubeMode),core(tube?DistortionCore::Mode::tube:DistortionCore::Mode::twoBand),
         oversampling(2,2,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true)
    {
        for(size_t i=0;i<6;++i)
        {
            values[i].referTo(state,juce::Identifier(ids()[i]),getUndoManager(),defaults()[i]);
            if(tube && i==3)parameters[i]=new tracktion::engine::DiscreteLabelledParameter(ids()[i],names()[i],*this,{0,2},3,{"Symmetric","Tube A","Tube B"});
            else
            {
                juce::NormalisableRange<float> range(minima()[i],maxima()[i]);
                if((tube && (i==1 || i==2)) || (!tube && (i==0 || i==3)))range.setSkewForCentre(defaults()[i]);
                parameters[i]=new tracktion::engine::SuffixedParameter(ids()[i],names()[i],*this,range,i==5?" %":((tube && (i==1 || i==2)) || (!tube && (i==0 || i==3)))?" Hz":" dB");
            }
            parameters[i]->attachToCurrentValue(values[i]);addAutomatableParameter(parameters[i]);
        }
        prepare(44100,512);
    }
    ~DistortionPluginBase()override{for(auto& p:parameters)p->detachFromCurrentValue();notifyListenersOfDeletion();}
    juce::String getName()const override{return tube?"Tube Distortion":"Two-Band Distortion";}
    juce::String getPluginType()override{return tube?"orbitTubeDistortion":"orbitTwoBandDistortion";}
    juce::String getSelectableDescription()override{return "Orbit original "+getName();}
    bool takesAudioInput()override{return true;}
    bool canBeAddedToClip()override{return false;}
    bool canBeAddedToRack()override{return false;}
    int getNumOutputChannelsGivenInputs(int)override{return 2;}
    void getChannelNames(juce::StringArray* in,juce::StringArray* out)override
    {if(in!=nullptr)in->addArray({"Left","Right"});if(out!=nullptr)out->addArray({"Left","Right"});}
    double getLatencySeconds()override{return latency/rate;}
    double getTailLength()const override{return 1.0;}
    void initialise(const tracktion::engine::PluginInitialisationInfo& info)override{prepare(info.sampleRate,info.blockSizeSamples);}
    void deinitialise()override{reset();}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree)override
    {for(size_t i=0;i<6;++i)values[i]=float(tree.getProperty(ids()[i],defaults()[i]));}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& context)override
    {
        if(context.destBuffer==nullptr || context.destBuffer->getNumChannels()==0)return;
        juce::ScopedNoDenormals noDenormals;
        const double start=context.editTime.getStart().inSeconds(),tolerance=2/rate;const auto loop=edit.getTransport().getLoopRange();
        const bool wrap=context.isPlaying && edit.getTransport().looping.get() && std::abs(lastEnd-loop.getEnd().inSeconds())<tolerance && std::abs(start-loop.getStart().inSeconds())<tolerance;
        if(!context.isRendering && haveLastEnd && lastEnd>=0 && std::abs(start-lastEnd)>tolerance && !wrap)reset();
        lastEnd=context.editTime.getEnd().inSeconds();haveLastEnd=true;
        DistortionCore::Controls controls;
        if(tube){controls.driveLow=parameters[0]->getCurrentValue();controls.lowCut=parameters[1]->getCurrentValue();controls.highDamp=parameters[2]->getCurrentValue();controls.shape=parameters[3]->getCurrentValue();}
        else{controls.breakpoint=parameters[0]->getCurrentValue();controls.driveLow=parameters[1]->getCurrentValue();controls.driveHigh=parameters[2]->getCurrentValue();controls.highDamp=parameters[3]->getCurrentValue();}
        core.setControls(controls);
        const double targetGain=std::pow(10.,juce::jlimit(-36.,12.,safeParameter(4))/20);
        const double targetMix=juce::jlimit(0.,1.,safeParameter(5)*.01);
        if(first){gain=targetGain;mix=targetMix;first=false;}
        auto& dest=*context.destBuffer;
        for(int offset=0;offset<context.bufferNumSamples;offset+=maxBlock)
        {
            const int count=juce::jmin(maxBlock,context.bufferNumSamples-offset),begin=context.bufferStartSample+offset;
            for(int i=0;i<count;++i)
            {
                const float left=clean(dest.getSample(0,begin+i)),right=dest.getNumChannels()>1?clean(dest.getSample(1,begin+i)):left;
                work.setSample(0,i,left);work.setSample(1,i,right);
                delay[position]={left,right};const auto dry=delay[(position+1)%delay.size()];position=(position+1)%delay.size();
                dryWork.setSample(0,i,dry[0]);dryWork.setSample(1,i,dry[1]);
            }
            auto block=juce::dsp::AudioBlock<float>(work).getSubBlock(0,size_t(count));auto up=oversampling.processSamplesUp(block);
            for(size_t i=0;i<up.getNumSamples();++i){const auto wet=core.process(up.getSample(0,int(i)),up.getSample(1,int(i)));up.setSample(0,int(i),wet[0]);up.setSample(1,int(i),wet[1]);}
            oversampling.processSamplesDown(block);
            for(int i=0;i<count;++i)
            {
                gain+=(targetGain-gain)*smooth;mix+=(targetMix-mix)*smooth;
                for(int ch=0;ch<juce::jmin(2,dest.getNumChannels());++ch)dest.setSample(ch,begin+i,float((dryWork.getSample(ch,i)*(1-mix)+work.getSample(ch,i)*mix)*gain));
            }
        }
        for(int ch=2;ch<dest.getNumChannels();++ch)dest.clear(ch,context.bufferStartSample,context.bufferNumSamples);
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,6> parameters;
    bool isTube()const{return tube;}
private:
    bool tube;DistortionCore core;juce::dsp::Oversampling<float> oversampling;
    std::array<juce::CachedValue<float>,6> values;juce::AudioBuffer<float> work,dryWork;
    std::vector<std::array<float,2>> delay;size_t position=0;int latency=0,maxBlock=512;double rate=44100,smooth=.004,gain=1,mix=1,lastEnd=-1;bool first=true,haveLastEnd=false;
    void prepare(double sampleRate,int blockSize)
    {rate=juce::jlimit(8000.,384000.,std::isfinite(sampleRate)?sampleRate:44100.);maxBlock=juce::jlimit(1,65536,blockSize);oversampling.initProcessing(size_t(maxBlock));latency=juce::roundToInt(oversampling.getLatencyInSamples());work.setSize(2,maxBlock);dryWork.setSize(2,maxBlock);delay.resize(size_t(latency+1));core.prepare(rate*4);smooth=1-std::exp(-1/(.005*rate));reset();}
    void reset(){oversampling.reset();core.reset();std::fill(delay.begin(),delay.end(),std::array<float,2>{});position=0;first=true;haveLastEnd=false;}
    double safeParameter(size_t index)const{const double value=parameters[index]->getCurrentValue();return std::isfinite(value)?value:defaults()[index];}
    static float clean(float x){return std::isfinite(x)?juce::jlimit(-1.e6f,1.e6f,x):0;}
    const std::array<const char*,6>& ids()const{static const std::array<const char*,6> a{"drive","lowCut","highDamp","shape","level","mix"},b{"breakpoint","driveLow","driveHigh","highTone","level","mix"};return tube?a:b;}
    const std::array<const char*,6>& names()const{static const std::array<const char*,6> a{"Drive","Lo Cut","Hi Damp","Shape","Level","Dry/Wet"},b{"Breakpoint","Low Drive","High Drive","Hi Tone","Level","Dry/Wet"};return tube?a:b;}
    const std::array<float,6>& defaults()const{static const std::array<float,6> a{12,30,12000,0,-12,100},b{800,0,0,16000,-6,100};return tube?a:b;}
    const std::array<float,6>& minima()const{static const std::array<float,6> a{0,20,500,0,-36,0},b{40,0,0,500,-36,0};return tube?a:b;}
    const std::array<float,6>& maxima()const{static const std::array<float,6> a{48,2000,20000,2,12,100},b{12000,48,48,24000,12,100};return tube?a:b;}
};
class TubeDistortionPlugin final:public DistortionPluginBase
{public:inline static const char* xmlTypeName="orbitTubeDistortion";explicit TubeDistortionPlugin(tracktion::engine::PluginCreationInfo info):DistortionPluginBase(info,true){}};
class TwoBandDistortionPlugin final:public DistortionPluginBase
{public:inline static const char* xmlTypeName="orbitTwoBandDistortion";explicit TwoBandDistortionPlugin(tracktion::engine::PluginCreationInfo info):DistortionPluginBase(info,false){}};
}
