// Modified Orbit port, GPL-3.0 + additional terms: ThirdParty/9W9/NOTICE.
// 9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9
#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "NineKitVoice.h"
#include "NineKitMetadata.h"
namespace orbit::synth
{
class NineKitPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitNineKitAudition";
    inline static constexpr size_t count=nine::specs.size(),accentIndex=count-2,mapIndex=count-1;
    explicit NineKitPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info),voice(orbit_nine_create())
    {
        namespace te=tracktion::engine;
        for(size_t i=0;i<count;++i)
        {
            const auto& spec=nine::specs[i];defaults[i]=i==accentIndex?50:i==mapIndex?0:orbit_nine_get(voice,spec.id);
            values[i].referTo(state,juce::Identifier(spec.id),getUndoManager(),defaults[i]);
            if(spec.choices[0])parameters[i]=new te::DiscreteLabelledParameter(spec.id,spec.name,*this,{0,float(spec.maximum)},spec.maximum+1,juce::StringArray::fromTokens(spec.choices,"|",""));
            else parameters[i]=new te::SuffixedParameter(spec.id,spec.name,*this,{0,float(spec.maximum),1},"");
            parameters[i]->attachToCurrentValue(values[i]);addAutomatableParameter(parameters[i]);
        }
        last.fill(std::numeric_limits<float>::quiet_NaN());
    }
    ~NineKitPlugin() override
    {for(auto& p:parameters)if(p)p->detachFromCurrentValue();notifyListenersOfDeletion();orbit_nine_destroy(voice);}
    juce::String getName() const override{return "9W9 909 Audition";}
    juce::String getPluginType() override{return xmlTypeName;}
    juce::String getSelectableDescription() override{return "9W9 909 drum kit (modified Orbit port)";}
    bool isSynth() override{return true;}
    bool takesMidiInput() override{return true;}
    bool takesAudioInput() override{return false;}
    bool producesAudioWhenNoAudioInput() override{return true;}
    bool canBeAddedToClip() override{return false;}
    int getNumOutputChannelsGivenInputs(int n) override{return juce::jmax(2,n);}
    bool engineReady()const{return voice!=nullptr;}
    int noteForVoice(int v)const{return parameters[mapIndex]->getCurrentValue()<.5f?nine::gmNotes[size_t(v)]:nine::padNotes[size_t(v)];}
    int voiceForNote(int note)const
    {
        for(int i=0;i<11;++i)if(noteForVoice(i)==note)return i;
        if(parameters[mapIndex]->getCurrentValue()<.5f){switch(note){case 35:return 0;case 40:return 1;case 45:return 2;case 48:return 3;case 43:return 4;default:break;}}
        return -1;
    }
    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override
    {rate=info.sampleRate;orbit_nine_prepare(voice,rate);last.fill(std::numeric_limits<float>::quiet_NaN());lastTempo=-1;playing=false;previousEnd=-1;accentStep=false;}
    void deinitialise() override{orbit_nine_panic(voice);}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {for(size_t i=0;i<count;++i)parameters[i]->setParameter(static_cast<float>(tree.getProperty(nine::specs[i].id,defaults[i])),juce::dontSendNotification);}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override
    {
        if(!voice || !fc.destBuffer || fc.destBuffer->getNumChannels()<1)return;
        juce::ScopedNoDenormals noDenormals;
        for(size_t i=0;i<accentIndex;++i){float x=parameters[i]->getCurrentValue();if(!std::isfinite(x))x=defaults[i];if(x!=last[i]){orbit_nine_set(voice,nine::specs[i].id,x);last[i]=x;}}
        const float bpm=float(edit.tempoSequence.getTempoAt(fc.editTime.getStart()).getBpm());if(bpm!=lastTempo){orbit_nine_tempo(voice,bpm);lastTempo=bpm;}
        const auto loop=edit.getTransport().getLoopRange();const double start=fc.editTime.getStart().inSeconds();
        const bool wrap=fc.isPlaying && edit.getTransport().looping.get() && std::abs(previousEnd-loop.getEnd().inSeconds())<2/rate && std::abs(start-loop.getStart().inSeconds())<2/rate;
        if((playing && !fc.isPlaying) || (fc.isPlaying && previousEnd>=0 && std::abs(start-previousEnd)>2/rate && !wrap)){orbit_nine_panic(voice);accentStep=false;}
        playing=fc.isPlaying;fc.destBuffer->clear(fc.bufferStartSample,fc.bufferNumSamples);
        int cursor=0;auto previews=auditionMask.exchange(0,std::memory_order_relaxed);
        auto render=[&](int end)
        {
            if(previews && end>cursor){for(int v=0;v<11;++v)if(previews&(1u<<v))orbit_nine_trigger(voice,v,110,0,0);previews=0;}
            if(end>cursor)orbit_nine_render(voice,fc.destBuffer->getWritePointer(0,fc.bufferStartSample+cursor),end-cursor);cursor=end;
        };
        if(auto* midi=fc.bufferForMidiMessages)
        {
            midi->sortByTimestamp();if(midi->isAllNotesOff){orbit_nine_panic(voice);accentStep=false;}
            auto sampleOf=[&](const auto& m){return juce::jlimit(0,fc.bufferNumSamples,juce::roundToInt((m.getTimeStamp()+fc.midiBufferOffset)*rate));};
            auto it=midi->begin();
            while(it!=midi->end())
            {
                const int at=sampleOf(*it);render(at);auto end=it;
                while(end!=midi->end() && sampleOf(*end)==at)++end;
                // Accent belongs to the whole step, even if the host puts its
                // simultaneous controller after a note in the MIDI array.
                for(auto control=it;control!=end;++control)if(control->isController())
                {
                    if(control->getControllerNumber()==119)accentStep=(control->getControllerValue()&1)!=0;
                    else if(control->getControllerNumber()==120 || control->getControllerNumber()==123){orbit_nine_panic(voice);accentStep=false;}
                }
                for(auto note=it;note!=end;++note)if(note->isNoteOn())orbit_nine_trigger(voice,voiceForNote(note->getNoteNumber()),note->getVelocity(),accentStep,parameters[accentIndex]->getCurrentValue());
                it=end;
            }
            midi->clear();
        }
        render(fc.bufferNumSamples);
        for(int ch=1;ch<fc.destBuffer->getNumChannels();++ch)fc.destBuffer->copyFrom(ch,fc.bufferStartSample,*fc.destBuffer,0,fc.bufferStartSample,fc.bufferNumSamples);
        previousEnd=fc.editTime.getEnd().inSeconds();
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,count> parameters;
    std::array<float,count> defaults{};
    std::atomic<unsigned> auditionMask{0};static_assert(std::atomic<unsigned>::is_always_lock_free);
private:
    std::array<juce::CachedValue<float>,count> values;std::array<float,count> last{};
    OrbitNineKit* voice=nullptr;double rate=44100,previousEnd=-1;float lastTempo=-1;bool playing=false,accentStep=false;
};
}
