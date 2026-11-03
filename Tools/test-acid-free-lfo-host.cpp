#define ORBIT_RACK_MAIN acidRackRegressionMain
#include "test-acid-rack-host.cpp"
#undef ORBIT_RACK_MAIN
// Measure the LFO's actual Output gain against identical unmodulated MIDI/audio.
// A slow saw sweeps -24..+12 dB in 20s (LFO1), or 10s (LFO2). This reveals phase
// resets without exposing or duplicating the plugin's phase implementation.
static juce::AudioBuffer<float> freeLfoAudio(orbit::synth::AcidBassPlugin& p,bool modulated,int row,int scenario,bool loop,bool retrigger=false,int noteOffset=0)
{
    constexpr int rate=48000,total=rate*8,blockSize=256;
    for(size_t i=0;i<p.parameters.size();++i)p.parameters[i]->setParameter(orbit::synth::AcidBassPlugin::defaults[i],juce::dontSendNotification);
    const size_t at=35+size_t(row)*5;
    p.parameters[orbit::synth::AcidBassPlugin::lfoRetriggerIndex+size_t(row)]->setParameter(retrigger?1.f:0.f,juce::dontSendNotification);
    p.parameters[15]->setParameter(1,juce::dontSendNotification);
    p.parameters[13]->setParameter(modulated?-6.f:0.f,juce::dontSendNotification);
    p.parameters[at+1]->setParameter(row==0?.05f:.1f,juce::dontSendNotification);
    p.parameters[at+3]->setParameter(2,juce::dontSendNotification); // Saw
    te::PluginInitialisationInfo info{};info.sampleRate=rate;info.blockSizeSamples=blockSize;p.initialise(info);
    auto& transport=p.edit.getTransport();transport.looping=loop;transport.setLoopRange({tracktion::TimePosition::fromSeconds(0),tracktion::TimePosition::fromSeconds(1)});
    juce::AudioBuffer<float> result(2,total),block(2,blockSize);te::MidiMessageArray midi;
    for(int first=0;first<total;)
    {
        const int count=juce::jmin(blockSize,rate-first%rate,total-first);
        const bool before=first<4*rate;
        p.parameters[at]->setParameter(modulated && !(scenario==1 && before)?1.f:0.f,juce::dontSendNotification);
        p.parameters[at+2]->setParameter(scenario==2 && before?0.f:100.f,juce::dontSendNotification);
        p.parameters[at+4]->setParameter(scenario==3 && before?0.f:10.f,juce::dontSendNotification); // Output
        double start=(loop?first%rate:first)/double(rate);
        if(scenario==4 && !before)start+=20; // Explicit non-loop seek
        if(scenario==5 && !before)
        {
            if(first==4*rate)
            {
                te::MidiMessageArray stopMidi;
                te::PluginRenderContext stop(&block,juce::AudioChannelSet::stereo(),0,blockSize,&stopMidi,0,{tracktion::TimePosition::fromSeconds(4),tracktion::TimePosition::fromSeconds(4+blockSize/double(rate))},false,false,false,false);
                p.applyToBuffer(stop);
            }
            start+=blockSize/double(rate); // Start resumes at the exact next time
        }
        // Pattern-boundary cleanup, then one note plus a long rest in each bar.
        // Start with two whole seconds of silence to catch skipped idle clocks.
        if(first%rate==0)midi.isAllNotesOff=true;
        for(int second=2;second<8;++second)
        {
            const int on=second*rate+noteOffset,off=on+rate*3/10;
            if(on>=first && on<first+count){midi.addMidiMessage(juce::MidiMessage::controllerEvent(1,119,1),(on-first)/double(rate),{});midi.addMidiMessage(juce::MidiMessage::noteOn(1,36,juce::uint8(100)),(on-first)/double(rate),{});}
            if(off>=first && off<first+count)midi.addMidiMessage(juce::MidiMessage::noteOff(1,36),(off-first)/double(rate),{});
        }
        te::PluginRenderContext context(&block,juce::AudioChannelSet::stereo(),0,count,&midi,0,{tracktion::TimePosition::fromSeconds(start),tracktion::TimePosition::fromSeconds(start+count/double(rate))},true,false,false,false);
        p.applyToBuffer(context);for(int ch=0;ch<2;++ch)result.copyFrom(ch,first,block,ch,0,count);first+=count;
    }
    p.deinitialise();return result;
}
static double measuredLfoDb(const juce::AudioBuffer<float>& wet,const juce::AudioBuffer<float>& dry,double time)
{
    double signal=0,reference=0;const int centre=juce::roundToInt(time*48000);
    for(int i=centre-240;i<centre+240;++i){const double a=wet.getSample(0,i),b=dry.getSample(0,i);signal+=a*a;reference+=b*b;}
    acidFxCheck(reference>1.e-8 && signal>1.e-12,"phase probe is silent");return 10*std::log10(signal/reference);
}
#ifndef ORBIT_FREE_MAIN
#define ORBIT_FREE_MAIN main
#endif
int ORBIT_FREE_MAIN(int argc,char** argv)
{
 try
 {
    if(argc>1 && juce::String(argv[1])=="--rack")return acidRackRegressionMain(1,nullptr);
    if(argc>1 && juce::String(argv[1])=="--legacy")return acidLegacyRegressionMain(1,nullptr);
    juce::ScopedJuceInitialiser_GUI gui;ProjectView h;h.stopTimer();h.createProjectDiscardingCurrent();h.addInstrumentTrack(true);
    auto* p=dynamic_cast<orbit::synth::AcidBassPlugin*>(h.generatorControlPlugin(h.selectedProjectTrackId()));acidFxCheck(p,"native synth missing");
    for(int row=0;row<2;++row)for(int scenario=0;scenario<6;++scenario)
    {
        const bool loop=scenario<4;auto dry=freeLfoAudio(*p,false,row,scenario,loop),wet=freeLfoAudio(*p,true,row,scenario,loop);
        for(double time:{4.2,7.2})
        {
            const double elapsed=scenario>=4?time-4:time;
            const double expected=-24+36*(row==0?.05:.1)*elapsed;
            const double actual=measuredLfoDb(wet,dry,time);
            std::cout<<"LFO "<<row+1<<" scenario="<<scenario<<" t="<<time<<" measured="<<actual<<" expected="<<expected<<" dB\n";
            acidFxCheck(std::abs(actual-expected)<.08,"LFO phase did not follow elapsed playback across notes/rests/loops or reset on transport seek/start");
        }
    }
    std::cout<<"PASS: both native LFOs continue through notes, long rests, all-notes-off/loop cleanup, disabled routes, zero depth and Off targets; deliberate seek and Stop/Start reset; real audio gain verified.\n";return 0;
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
