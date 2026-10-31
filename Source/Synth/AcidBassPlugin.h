#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "AcidBassVoice.h"
#include "AcidBassFx.h"

namespace orbit::synth
{
class AcidBassPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitAcidBass";
    inline static constexpr size_t phaseRetriggerIndex=15;
    inline static constexpr size_t triangleFoldIndex=16;
    inline static constexpr std::array<const char*,17> ids{"tune","cutoff","resonance","envelope","decay","accent","accentEnvelope","slide","waveform","fxDrive","fxDistort","fxLowCut","fxTone","fxOutput","fxMode","phaseRetrigger","fxTriangleFold"};
    inline static constexpr std::array<const char*,17> names{"Tune","Cutoff","Reso","Env Amt","Decay","Accent","Acc Env","Slide","Waveform","Drive","Distort","Low Cut","Tone","Output","FX Mode","Retrigger oscillator phase","Triangle Foldback"};
    inline static constexpr std::array<float,17> defaults{0,650,65,55,180,70,25,60,0,35,0,30,12000,0,0,0,0};
    inline static constexpr std::array<float,17> minima{-12,30,0,0,30,0,0,1,0,0,0,20,500,-24,0,0,0};
    inline static constexpr std::array<float,17> maxima{12,10000,100,100,3000,100,100,400,1,100,100,1500,20000,12,1,1,1};
    explicit AcidBassPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info)
    {
        namespace te=tracktion::engine;
        for(std::size_t i=0;i<parameters.size();++i)
        {
            values[i].referTo(state,juce::Identifier(ids[i]),getUndoManager(),defaults[i]);
            if(i==8)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Saw","Square"});
            else if(i==14)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Soft","Sine Fold"});
            else if(i==phaseRetriggerIndex || i==triangleFoldIndex)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Off","On"});
            else
            {
                juce::NormalisableRange<float> range(minima[i],maxima[i]);
                if(i==0)range.interval=1;
                if(i==1 || i==4 || i==7 || i==11 || i==12)range.setSkewForCentre(defaults[i]);
                parameters[i]=new te::SuffixedParameter(ids[i],names[i],*this,range,i==0?" st":(i==1 || i==11 || i==12)?" Hz":i==13?" dB":(i==4 || i==7)?" ms":" %");
            }
            parameters[i]->attachToCurrentValue(values[i]);
            addAutomatableParameter(parameters[i]);
        }
    }
    ~AcidBassPlugin() override{for(auto& p:parameters)if(p)p->detachFromCurrentValue();notifyListenersOfDeletion();}
    juce::String getName() const override{return "Acid Bass";}
    juce::String getPluginType() override{return xmlTypeName;}
    juce::String getSelectableDescription() override{return "Acid Bass monophonic synthesizer";}
    bool isSynth() override{return true;}
    bool takesMidiInput() override{return true;}
    bool takesAudioInput() override{return false;}
    bool producesAudioWhenNoAudioInput() override{return true;}
    bool canBeAddedToClip() override{return false;}
    double getLatencySeconds() override{return fx.latencySamples()/rate;}
    int getNumOutputChannelsGivenInputs(int channels) override{return juce::jmax(2,channels);}
    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override
    {rate=info.sampleRate;voice.prepare(rate);fx.prepare(rate,info.blockSizeSamples);playing=false;previousEnd=-1;}
    void deinitialise() override{voice.panic();fx.reset();}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {for(std::size_t i=0;i<parameters.size();++i)parameters[i]->setParameter(static_cast<float>(tree.getProperty(ids[i],defaults[i])),juce::dontSendNotification);}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override
    {
        if(fc.destBuffer==nullptr || rate<=0)return;
        std::array<float,17> p{};for(std::size_t i=0;i<parameters.size();++i)p[i]=parameters[i]->getCurrentValue();
        voice.setParameters({p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[phaseRetriggerIndex]>=.5f});
        if(playing && !fc.isPlaying){voice.panic();fx.reset();}playing=fc.isPlaying;
        const double start=fc.editTime.getStart().inSeconds();const auto loopRange=edit.getTransport().getLoopRange();
        const bool wrap=fc.isPlaying && edit.getTransport().looping.get() && std::abs(previousEnd-loopRange.getEnd().inSeconds())<2/rate && std::abs(start-loopRange.getStart().inSeconds())<2/rate;
        if(!fc.isRendering && previousEnd>=0 && std::abs(start-previousEnd)>2/rate && !wrap)fx.reset();
        fc.destBuffer->clear(fc.bufferStartSample,fc.bufferNumSamples);
        int cursor=0;
        auto render=[&](int end)
        {
            voice.finishEvents();
            for(;cursor<end;++cursor)
            {
                const auto value=voice.sample();
                for(int channel=0;channel<fc.destBuffer->getNumChannels();++channel)
                    fc.destBuffer->setSample(channel,fc.bufferStartSample+cursor,value);
            }
        };
        if(auto* midi=fc.bufferForMidiMessages)
        {
            midi->sortByTimestamp();
            bool boundaryNote=false,boundaryMarker=false;
            for(const auto& event:*midi)
                if(event.getTimeStamp()+fc.midiBufferOffset<2/rate)
                {
                    boundaryNote=boundaryNote || event.isNoteOn();
                    boundaryMarker=boundaryMarker || (event.isController() && event.getControllerNumber()==119);
                }
            const auto loop=edit.getTransport().getLoopRange();
            const bool tiedLoop=fc.isPlaying && edit.getTransport().looping.get() && voice.hasOutgoingSlide()
                && boundaryNote && boundaryMarker
                && std::abs(previousEnd-loop.getEnd().inSeconds())<2/rate
                && std::abs(fc.editTime.getStart().inSeconds()-loop.getStart().inSeconds())<2/rate;
            if(midi->isAllNotesOff && !tiedLoop){voice.panic();fx.reset();}
            for(const auto& event:*midi)
            {
                const int sample=juce::jlimit(cursor,fc.bufferNumSamples,juce::roundToInt((event.getTimeStamp()+fc.midiBufferOffset)*rate));
                if(sample>cursor)render(sample);
                if(event.isNoteOn())voice.noteOn(event.getChannel(),event.getNoteNumber(),event.getVelocity());
                else if(event.isNoteOff())voice.noteOff(event.getChannel(),event.getNoteNumber());
                else if(event.isController())voice.controller(event.getControllerNumber(),event.getControllerValue());
                else if(event.isPitchWheel())voice.pitchBend((event.getPitchWheelValue()-8192)/8192.0f*2);
            }
            midi->clear();
        }
        render(fc.bufferNumSamples);
        fx.setControls({p[9],p[10],p[11],p[12],p[13],p[14],p[triangleFoldIndex]});
        if(fc.destBuffer->getNumChannels()>0)
        {
            fx.process(fc.destBuffer->getWritePointer(0,fc.bufferStartSample),fc.bufferNumSamples);
            for(int ch=1;ch<fc.destBuffer->getNumChannels();++ch)fc.destBuffer->copyFrom(ch,fc.bufferStartSample,*fc.destBuffer,0,fc.bufferStartSample,fc.bufferNumSamples);
        }
        previousEnd=fc.editTime.getEnd().inSeconds();
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,17> parameters;
private:
    std::array<juce::CachedValue<float>,17> values;
    AcidBassVoice voice;AcidBassFx fx;double rate=44100,previousEnd=-1;bool playing=false;
};
}
