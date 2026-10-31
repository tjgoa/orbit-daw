// Orbit Geonkick audition adapter. GPL-3.0-or-later; see ThirdParty/Geonkick/UPSTREAM.md.
#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "GeonkickVoice.h"

namespace orbit::synth
{
class GeonkickPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitGeonkickAudition";
    inline static constexpr std::array<const char*,8> ids{"sound","tune","decay","sweep","noise","tone","drive","level"};
    inline static constexpr std::array<const char*,8> names{"Sound","Tune","Decay","Pitch Sweep","Noise","Tone","Drive","Output"};
    inline static constexpr std::array<const char*,6> sounds{"Kick","Snare","Hat","Tom","Clap","Metal"};
    inline static constexpr std::array<float,8> defaults{0,0,350,65,35,4500,0,-9};
    inline static constexpr std::array<float,8> minima{0,-24,30,0,0,200,0,-36},maxima{5,24,2000,100,100,12000,100,6};
    explicit GeonkickPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info),voice(orbit_geonkick_create())
    {
        namespace te=tracktion::engine;
        for(size_t i=0;i<parameters.size();++i)
        {
            values[i].referTo(state,juce::Identifier(ids[i]),getUndoManager(),defaults[i]);
            if(i==0)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,5},6,{"Kick","Snare","Hat","Tom","Clap","Metal"});
            else
            {
                juce::NormalisableRange<float> range(minima[i],maxima[i]);
                if(i==1)range.interval=1;
                if(i==2 || i==5)range.setSkewForCentre(defaults[i]);
                parameters[i]=new te::SuffixedParameter(ids[i],names[i],*this,range,i==1?" st":i==2?" ms":i==5?" Hz":i==7?" dB":" %");
            }
            parameters[i]->attachToCurrentValue(values[i]);addAutomatableParameter(parameters[i]);
        }
    }
    ~GeonkickPlugin() override
    {for(auto& p:parameters)if(p)p->detachFromCurrentValue();notifyListenersOfDeletion();orbit_geonkick_destroy(voice);}
    juce::String getName() const override{return "Geonkick Audition";}
    juce::String getPluginType() override{return xmlTypeName;}
    juce::String getSelectableDescription() override{return "Geonkick percussion engine audition";}
    bool isSynth() override{return true;}
    bool takesMidiInput() override{return true;}
    bool takesAudioInput() override{return false;}
    bool producesAudioWhenNoAudioInput() override{return true;}
    bool canBeAddedToClip() override{return false;}
    int getNumOutputChannelsGivenInputs(int n) override{return juce::jmax(2,n);}
    bool engineReady()const{return voice!=nullptr;}
    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override
    {rate=info.sampleRate;orbit_geonkick_prepare(voice,rate);playing=false;previousEnd=-1;}
    void deinitialise() override{orbit_geonkick_panic(voice);}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {for(size_t i=0;i<parameters.size();++i)parameters[i]->setParameter(static_cast<float>(tree.getProperty(ids[i],defaults[i])),juce::dontSendNotification);}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override
    {
        if(fc.destBuffer==nullptr || rate<=0)return;
        std::array<float,8> p{};for(size_t i=0;i<p.size();++i)p[i]=parameters[i]->getCurrentValue();
        orbit_geonkick_controls(voice,p.data());
        if(playing && !fc.isPlaying)orbit_geonkick_panic(voice);
        const double start=fc.editTime.getStart().inSeconds();
        if(fc.isPlaying && previousEnd>=0 && std::abs(start-previousEnd)>2/rate)orbit_geonkick_panic(voice);
        playing=fc.isPlaying;
        int preview=auditionVelocity.exchange(0,std::memory_order_relaxed);
        fc.destBuffer->clear(fc.bufferStartSample,fc.bufferNumSamples);
        int cursor=0;
        auto render=[&](int end)
        {
            // A newly started/stopped host graph may carry All Notes Off.
            // Process that reset before honoring this explicit preview request.
            if(preview>0 && end>cursor){orbit_geonkick_trigger(voice,36,preview);preview=0;}
            for(;cursor<end;++cursor){const float x=orbit_geonkick_sample(voice);for(int ch=0;ch<fc.destBuffer->getNumChannels();++ch)fc.destBuffer->setSample(ch,fc.bufferStartSample+cursor,x);}
        };
        if(auto* midi=fc.bufferForMidiMessages)
        {
            midi->sortByTimestamp();if(midi->isAllNotesOff)orbit_geonkick_panic(voice);
            for(const auto& event:*midi)
            {
                render(juce::jlimit(cursor,fc.bufferNumSamples,juce::roundToInt((event.getTimeStamp()+fc.midiBufferOffset)*rate)));
                if(event.isNoteOn())orbit_geonkick_trigger(voice,event.getNoteNumber(),event.getVelocity());
                else if(event.isController() && (event.getControllerNumber()==120 || event.getControllerNumber()==123))orbit_geonkick_panic(voice);
                // Percussion one-shots finish their envelopes after note-off.
            }
            midi->clear();
        }
        render(fc.bufferNumSamples);previousEnd=fc.editTime.getEnd().inSeconds();
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,8> parameters;
    std::atomic<int> auditionVelocity{0}; // UI requests; DSP is touched only by rendering.
    static_assert(std::atomic<int>::is_always_lock_free);
private:
    std::array<juce::CachedValue<float>,8> values;
    OrbitGeonkick* voice=nullptr;double rate=48000,previousEnd=-1;bool playing=false;
};
}
