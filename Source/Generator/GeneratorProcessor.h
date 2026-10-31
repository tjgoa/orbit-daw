#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace orbit::generator
{
struct Settings
{
    bool arp = false, resetOnChord = true;
    int order = 0, octaves = 3, distance = 12, retrigger = 0, repeats = 0;
    int rate = 4, gate = 0, rhythm = 65535, shuffle = 0, shuffleRate = 4;
    int transpose = 0, velocity = 100, outputChannel = 0;
    std::array<int,32> custom {};
    int customSize = 1;
    bool operator==(const Settings&) const = default;
};
inline const juce::StringArray& rateNames()
{
    static const juce::StringArray names {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/8 triplet", "1/16 triplet", "1/32 triplet"};
    return names;
}
inline double rateBeats(int rate)
{
    static constexpr double beats[] {4,2,1,.5,.25,.125,.0625,1.0/3,1.0/6,1.0/12};
    return beats[juce::jlimit(0,9,rate)];
}
inline juce::String encode(const Settings& s)
{
    auto* o = new juce::DynamicObject();
    #define ORBIT_GENERATOR_WRITE(name) o->setProperty(#name,s.name)
    ORBIT_GENERATOR_WRITE(arp); ORBIT_GENERATOR_WRITE(resetOnChord); ORBIT_GENERATOR_WRITE(order);
    ORBIT_GENERATOR_WRITE(octaves); ORBIT_GENERATOR_WRITE(distance); ORBIT_GENERATOR_WRITE(retrigger);
    ORBIT_GENERATOR_WRITE(repeats); ORBIT_GENERATOR_WRITE(rate); ORBIT_GENERATOR_WRITE(gate);
    ORBIT_GENERATOR_WRITE(rhythm); ORBIT_GENERATOR_WRITE(shuffle); ORBIT_GENERATOR_WRITE(shuffleRate);
    ORBIT_GENERATOR_WRITE(transpose); ORBIT_GENERATOR_WRITE(velocity); ORBIT_GENERATOR_WRITE(outputChannel);
    #undef ORBIT_GENERATOR_WRITE
    juce::Array<juce::var> notes;
    for(int i=0;i<juce::jlimit(1,32,s.customSize);++i)notes.add(s.custom[size_t(i)]);
    o->setProperty("custom",notes);return juce::JSON::toString(juce::var(o),true);
}
inline Settings decode(const juce::String& text)
{
    Settings s;const auto v=juce::JSON::parse(text);auto* o=v.getDynamicObject();if(o==nullptr)return s;
    const auto number=[&](const char* key,int fallback,int low,int high)
    {return o->hasProperty(key)?juce::jlimit(low,high,int(o->getProperty(key))):fallback;};
    if(o->hasProperty("arp"))s.arp=bool(v["arp"]);
    if(o->hasProperty("resetOnChord"))s.resetOnChord=bool(v["resetOnChord"]);
    s.order=number("order",0,0,5);s.octaves=number("octaves",3,1,5);s.distance=number("distance",12,0,24);
    s.retrigger=number("retrigger",0,0,64);s.repeats=number("repeats",0,0,64);
    s.rate=number("rate",4,0,9);s.gate=number("gate",0,-90,100);s.rhythm=number("rhythm",65535,0,65535);
    s.shuffle=number("shuffle",0,0,75);s.shuffleRate=number("shuffleRate",4,0,9);
    s.transpose=number("transpose",0,-48,48);s.velocity=number("velocity",100,1,200);s.outputChannel=number("outputChannel",0,0,16);
    if(auto* a=v["custom"].getArray();a!=nullptr && !a->isEmpty())
    {s.customSize=juce::jmin(32,a->size());for(int i=0;i<s.customSize;++i)s.custom[size_t(i)]=juce::jlimit(-48,48,int((*a)[i]));}
    return s;
}

// One sample-clock-independent MIDI state machine serves both audio blocks and
// pattern rendering. All real-time state and queues have fixed capacity.
class Processor
{
public:
    Settings settings;
    template<class Emit> void panic(double time,Emit&& emit)
    {
        for(int channel=1;channel<=16;++channel)emit(juce::MidiMessage::controllerEvent(channel,64,0),time);
        for(int key=0;key<2048;++key)if(sounding[size_t(key)]>0)
            emit(juce::MidiMessage::noteOff(key/128+1,key%128),time);
        held={};sounding={};sustain={};delays={};pendingCount=0;heldCount=0;
        nextStep=infinity;if(settings.resetOnChord){stepIndex=0;played=0;}
    }
    template<class Emit> void input(const juce::MidiMessage& message,double beat,Emit&& emit)
    {
        const int ch=message.getChannel();
        if(message.isAllNotesOff() || message.isAllSoundOff())
        {panic(beat,emit);auto copy=message;if(settings.outputChannel>0)copy.setChannel(settings.outputChannel);emit(copy,beat);return;}
        if(!message.isNoteOnOrOff())
        {
            if(ch>=1 && ch<=16 && message.isController() && message.getControllerNumber()==64)
            {
                sustain[size_t(ch-1)]=message.getControllerValue()>=64;
                if(!sustain[size_t(ch-1)])for(int p=0;p<128;++p)
                {auto& h=held[size_t((ch-1)*128+p)];if(h.count==0 && h.velocity>0){h.velocity=0;--heldCount;}}
                if(heldCount==0)nextStep=infinity;
            }
            auto copy=message;if(ch>0 && settings.outputChannel>0)copy.setChannel(settings.outputChannel);emit(copy,beat);return;
        }
        if(ch<1 || ch>16)return;
        const int key=(ch-1)*128+message.getNoteNumber();
        if(!settings.arp)
        {
            auto copy=message;const int pitch=message.getNoteNumber()+settings.transpose;
            if(pitch<0 || pitch>127)return;
            copy.setNoteNumber(pitch);if(settings.outputChannel>0)copy.setChannel(settings.outputChannel);
            if(copy.isNoteOn())copy.setVelocity(float(juce::jlimit(1,127,int(copy.getVelocity())*settings.velocity/100))/127.0f);
            auto& delay=delays[size_t(key)];
            if(message.isNoteOn())delay=shuffleDelay(beat);
            queue(copy,beat+delay,emit);return;
        }
        auto& note=held[size_t(key)];
        if(message.isNoteOn())
        {
            if(heldCount==0)
            {if(settings.resetOnChord){stepIndex=0;played=0;}nextStep=beat;}
            if(note.velocity==0)++heldCount;
            note.count=juce::jmin(65535,note.count+1);note.velocity=int(message.getVelocity());
        }
        else
        {
            note.count=juce::jmax(0,note.count-1);
            if(note.count==0 && !sustain[size_t(ch-1)] && note.velocity>0){note.velocity=0;--heldCount;}
            if(heldCount==0)nextStep=infinity;
        }
    }
    template<class Emit> void advance(double end,bool inclusive,Emit&& emit)
    {
        const auto due=[&](double t){return inclusive?t<=end+1.0e-10:t<end-1.0e-10;};
        int work=0;
        while(++work<32768)
        {
            const double pendingTime=pendingCount>0?pending[0].time:infinity;
            if(pendingTime<=nextStep && due(pendingTime))
            {
                std::pop_heap(pending.begin(),pending.begin()+pendingCount,Later{});
                const auto event=pending[size_t(--pendingCount)];deliver(event.message,event.time,emit);continue;
            }
            if(!due(nextStep))break;
            const auto when=nextStep;nextStep+=rateBeats(settings.rate);trigger(when,emit);
        }
        if(work>=32768)panic(end,emit);
    }
private:
    static constexpr double infinity=std::numeric_limits<double>::infinity();
    struct Held {int count=0,velocity=0;};
    struct Pending {juce::MidiMessage message;double time=0;};
    struct Later {bool operator()(const Pending& a,const Pending& b)const
    {if(a.time!=b.time)return a.time>b.time;return a.message.isNoteOn() && !b.message.isNoteOn();}};
    std::array<Held,2048> held{};
    std::array<int,2048> sounding{};
    std::array<bool,16> sustain{};
    std::array<double,2048> delays{};
    std::array<Pending,8192> pending{};
    int pendingCount=0,heldCount=0;
    uint64_t stepIndex=0,played=0;
    double nextStep=infinity;
    double shuffleDelay(double beat)const
    {
        const auto interval=rateBeats(settings.shuffleRate);
        const auto cell=int64_t(std::floor(beat/interval+1.0e-8));
        return (cell&1)!=0?interval*settings.shuffle/100.0:0;
    }
    template<class Emit> void queue(const juce::MidiMessage& message,double time,Emit&& emit)
    {
        if(pendingCount==int(pending.size())){panic(time,emit);return;}
        pending[size_t(pendingCount++)]={message,time};std::push_heap(pending.begin(),pending.begin()+pendingCount,Later{});
    }
    template<class Emit> void deliver(const juce::MidiMessage& message,double time,Emit&& emit)
    {
        if(message.isNoteOnOrOff())
        {
            auto& count=sounding[size_t((message.getChannel()-1)*128+message.getNoteNumber())];
            if(message.isNoteOn())++count;
            else {if(count==0)return;if(--count>0)return;}
        }
        emit(message,time);
    }
    template<class Emit> void trigger(double beat,Emit&& emit)
    {
        if(heldCount==0){nextStep=infinity;return;}
        std::array<int,2048> keys{};int n=0;
        for(int p=0;p<128;++p)for(int ch=0;ch<16;++ch)
            if(held[size_t(ch*128+p)].velocity>0)keys[size_t(n++)]=ch*128+p;
        const auto count=uint64_t(n*settings.octaves);
        const auto cycle=settings.order==2 || settings.order==3 || settings.order==4
            ?juce::jmax(uint64_t(1),count*2-(settings.order==3?0:2)):count;
        const auto sequenceLength=settings.order==5?uint64_t(settings.customSize*settings.octaves):cycle;
        const auto limit=settings.retrigger>0?uint64_t(settings.retrigger):sequenceLength;
        if(settings.repeats>0 && played>=limit*uint64_t(settings.repeats)){nextStep=infinity;return;}
        const auto index=(played%juce::jmax(uint64_t(1),limit))%sequenceLength;
        uint64_t selected=index;
        if(settings.order==1)selected=count-1-index%count;
        else if(settings.order==2)selected=index<count?index:count*2-2-index;
        else if(settings.order==3)selected=index<count?index:count*2-1-index;
        else if(settings.order==4)selected=index<count?count-1-index:index-count+1;
        int key=keys[size_t(selected%uint64_t(n))];
        int pitch=key%128+int(selected/uint64_t(n))*settings.distance;
        if(settings.order==5)
        {key=keys[0];pitch=key%128+settings.custom[size_t(index%uint64_t(settings.customSize))]+int(index/uint64_t(settings.customSize))*settings.distance;}
        pitch+=settings.transpose;
        if((settings.rhythm&(1<<int(stepIndex%16)))!=0 && pitch>=0 && pitch<=127)
        {
            const int channel=settings.outputChannel>0?settings.outputChannel:key/128+1;
            const auto velocity=juce::uint8(juce::jlimit(1,127,held[size_t(key)].velocity*settings.velocity/100));
            const auto start=beat+shuffleDelay(beat);
            queue(juce::MidiMessage::noteOn(channel,pitch,velocity),start,emit);
            queue(juce::MidiMessage::noteOff(channel,pitch),start+rateBeats(settings.rate)*(1+settings.gate/100.0),emit);
        }
        ++stepIndex;++played;
        if(settings.retrigger>0 && played%uint64_t(settings.retrigger)==0)stepIndex=0;
    }
};

struct Note {int pitch=60,startTick=0,lengthTicks=64,velocity=100;};
inline std::vector<Note> render(const std::vector<Note>& notes,int lengthTicks,Settings settings)
{
    if(lengthTicks<=0 || lengthTicks>65536*64 || notes.size()>262144)
        throw std::runtime_error("Render supports patterns up to 65,536 steps and 262,144 source notes.");
    // Render only the arp/shuffle. Generator transpose/velocity/channel remain
    // playback settings and must not be applied twice after rendering.
    settings.transpose=0;settings.velocity=100;settings.outputChannel=0;
    Processor processor;processor.settings=settings;
    juce::MidiMessageSequence input;
    for(const auto& note:notes)
    {
        if(note.pitch<0 || note.pitch>127 || note.startTick<0 || note.startTick>=lengthTicks || note.lengthTicks<=0)continue;
        auto on=juce::MidiMessage::noteOn(1,note.pitch,juce::uint8(juce::jlimit(1,127,note.velocity)));on.setTimeStamp(note.startTick/256.0);input.addEvent(on);
        auto off=juce::MidiMessage::noteOff(1,note.pitch);off.setTimeStamp((double(note.startTick)+note.lengthTicks)/256.0);input.addEvent(off);
    }
    input.sort();juce::MidiMessageSequence output;
    const auto emit=[&](juce::MidiMessage message,double beat)
    {if(output.getNumEvents()>=524288)throw std::runtime_error("Rendered arpeggio exceeds 262,144 notes. Use a shorter pattern or slower rate.");message.setTimeStamp(beat);output.addEvent(message);};
    for(int i=0;i<input.getNumEvents();)
    {
        const auto time=input.getEventTime(i);if(time>=lengthTicks/256.0)break;
        processor.advance(time,false,emit);
        do{processor.input(input.getEventPointer(i)->message,time,emit);++i;}while(i<input.getNumEvents() && input.getEventTime(i)==time);
        processor.advance(time,true,emit);
    }
    processor.advance(lengthTicks/256.0,false,emit);processor.panic(lengthTicks/256.0,emit);
    output.sort();output.updateMatchedPairs();std::vector<Note> result;
    for(int i=0;i<output.getNumEvents();++i)
    {
        const auto* on=output.getEventPointer(i);if(!on->message.isNoteOn() || on->noteOffObject==nullptr)continue;
        const auto start=juce::jlimit(0,lengthTicks-1,juce::roundToInt(on->message.getTimeStamp()*256));
        const auto end=juce::jlimit(start+1,lengthTicks,juce::roundToInt(on->noteOffObject->message.getTimeStamp()*256));
        result.push_back({on->message.getNoteNumber(),start,end-start,int(on->message.getVelocity())});
    }
    return result;
}
}
