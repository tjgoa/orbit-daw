#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "GeneratorProcessor.h"

namespace orbit::generator
{
class MidiPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitGeneratorMidi";
    explicit MidiPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info),tempo(tracktion::engine::createPosition(edit.tempoSequence))
    {reload();input.reserve(4096);}
    ~MidiPlugin() override {notifyListenersOfDeletion();}
    juce::String getName() const override {return "Generator MIDI";}
    juce::String getPluginType() override {return xmlTypeName;}
    juce::String getSelectableDescription() override {return "Generator arpeggiator and shuffle";}
    void initialise(const tracktion::engine::PluginInitialisationInfo& info) override
    {rate=info.sampleRate;clock=0;playing=false;processor=std::make_unique<Processor>();changed=true;}
    void deinitialise() override {}
    double getLatencySeconds() override {return 0;}
    bool takesMidiInput() override {return true;}
    bool takesAudioInput() override {return true;}
    bool canBeAddedToClip() override {return false;}
    int getNumOutputChannelsGivenInputs(int channels) override {return channels;}
    void getChannelNames(juce::StringArray*,juce::StringArray*) override {}
    Settings getSettings() const {return decode(state.getProperty("orbitGeneratorSettings").toString());}
    void setSettings(const Settings& s){state.setProperty("orbitGeneratorSettings",encode(s),nullptr);}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {state.setProperty("orbitGeneratorSettings",tree.getProperty("orbitGeneratorSettings"),nullptr);reload();}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override
    {
        if(fc.bufferForMidiMessages==nullptr || processor==nullptr || rate<=0)return;
        auto& midi=*fc.bufferForMidiMessages;
        tempo.set(fc.editTime.getStart());const auto bpm=juce::jlimit(20.0,300.0,tempo.getTempo());
        const auto timelineBeat=tempo.getPPQTime();
        const bool jumped=fc.isPlaying && (!playing || std::abs(timelineBeat-clock)>1.0e-5);
        if(jumped)clock=timelineBeat;
        const auto duration=fc.bufferNumSamples/rate*bpm/60.0;
        const auto emit=[&](juce::MidiMessage message,double beat)
        {midi.addMidiMessage(std::move(message),juce::jlimit(0.0,juce::jmax(0.0,fc.bufferNumSamples/rate-1/rate),(beat-clock)*60/bpm),{});};
        input.clear();for(const auto& message:midi){if(input.size()<4096)input.emplace_back(message);}
        const bool overflow=midi.size()>4096;const bool panic=midi.isAllNotesOff;
        midi.clear();midi.isAllNotesOff=panic || overflow;
        bool reset=false;
        {const juce::SpinLock::ScopedTryLockType lock(configLock);if(lock.isLocked() && changed){processor->settings=config;changed=false;reset=true;}}
        if(reset || panic || overflow || jumped || (playing && !fc.isPlaying)){processor->panic(clock,emit);midi.isAllNotesOff=true;}
        playing=fc.isPlaying;
        std::sort(input.begin(),input.end(),[](const auto& a,const auto& b)
        {if(a.getTimeStamp()!=b.getTimeStamp())return a.getTimeStamp()<b.getTimeStamp();return a.isNoteOff() && !b.isNoteOff();});
        for(size_t i=0;i<input.size();)
        {
            const auto offset=juce::jlimit(0.0,fc.bufferNumSamples/rate,input[i].getTimeStamp()+fc.midiBufferOffset);
            const auto time=clock+offset*bpm/60.0;processor->advance(time,false,emit);
            const auto stamp=input[i].getTimeStamp();
            do{processor->input(input[i],time,emit);++i;}while(i<input.size() && input[i].getTimeStamp()==stamp);
            processor->advance(time,true,emit);
        }
        processor->advance(clock+duration,false,emit);clock+=duration;midi.sortByTimestamp();
    }
protected:
    void valueTreePropertyChanged(juce::ValueTree& tree,const juce::Identifier& property) override
    {Plugin::valueTreePropertyChanged(tree,property);if(tree==state && property==juce::Identifier("orbitGeneratorSettings"))reload();}
private:
    void reload(){const auto s=getSettings();const juce::SpinLock::ScopedLockType lock(configLock);config=s;changed=true;}
    juce::SpinLock configLock;
    Settings config;
    bool changed=true,playing=false;
    double clock=0,rate=44100;
    tracktion::tempo::Sequence::Position tempo;
    std::unique_ptr<Processor> processor=std::make_unique<Processor>();
    std::vector<juce::MidiMessage> input;
};
}
