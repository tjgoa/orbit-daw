#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include "AcidBassVoice.h"
#include "AcidBassFx.h"
#include "AcidBassEq.h"

namespace orbit::synth
{
class AcidBassPlugin final : public tracktion::engine::Plugin
{
public:
    inline static const char* xmlTypeName="orbitAcidBass";
    inline static constexpr size_t phaseRetriggerIndex=15;
    inline static constexpr size_t triangleFoldIndex=16;
    inline static constexpr size_t spreadEnableIndex=17,spreadVoicesIndex=18,spreadAmountIndex=19,spreadWidthIndex=20;
    inline static constexpr size_t tubeIndex=21,bandIndex=28,lfoIndex=35;
    inline static constexpr size_t lfoRetriggerIndex=45;
    inline static constexpr size_t eqIndex=47;
    inline static constexpr size_t parameterCount=56;
    inline static constexpr std::array<int,22> modulationTargets{-1,1,2,3,4,5,7,9,10,12,13,19,20,22,23,24,27,29,30,31,32,34};
    static juce::StringArray modulationNames(){return {"Off","Cutoff","Resonance","Envelope","Decay","Accent","Slide","Drive","Distort mix","Tone","Output","Filter spread","Stereo width","Tube Drive","Tube Lo Cut","Tube Hi Damp","Tube Mix","Band Breakpoint","Band Low Drive","Band High Drive","Band Hi Tone","Band Mix"};}
    inline static constexpr std::array<const char*,parameterCount> ids{"tune","cutoff","resonance","envelope","decay","accent","accentEnvelope","slide","waveform","fxDrive","fxDistort","fxLowCut","fxTone","fxOutput","fxMode","phaseRetrigger","fxTriangleFold","filterSpreadEnabled","filterVoices","filterSpread","stereoWidth","tubeEnabled","tubeDrive","tubeLowCut","tubeTone","tubeShape","tubeLevel","tubeMix","bandEnabled","bandBreakpoint","bandLowDrive","bandHighDrive","bandTone","bandLevel","bandMix","lfo1Enabled","lfo1Rate","lfo1Depth","lfo1Wave","lfo1Target","lfo2Enabled","lfo2Rate","lfo2Depth","lfo2Wave","lfo2Target","lfo1Retrigger","lfo2Retrigger","eqEnabled","eqLowFrequency","eqLowGain","eqMidFrequency","eqMidGain","eqMidQ","eqHighFrequency","eqHighGain","eqOutput"};
    inline static constexpr std::array<const char*,parameterCount> names{"Tune","Cutoff","Reso","Env Amt","Decay","Accent","Acc Env","Slide","Waveform","Drive","Distort","Low Cut","Tone","Output","FX Mode","Retrigger oscillator phase","Triangle Foldback","Filter bank enabled","Filter voices","Filter spread","Stereo width","Tube enabled","Tube Drive","Tube Lo Cut","Tube Hi Damp","Tube Shape","Tube Level","Tube Mix","Two-Band enabled","Band Breakpoint","Band Low Drive","Band High Drive","Band Hi Tone","Band Level","Band Mix","LFO 1 enabled","LFO 1 Rate","LFO 1 Depth","LFO 1 Wave","LFO 1 Target","LFO 2 enabled","LFO 2 Rate","LFO 2 Depth","LFO 2 Wave","LFO 2 Target","LFO 1 trigger","LFO 2 trigger","EQ enabled","EQ Low Frequency","EQ Low Gain","EQ Mid Frequency","EQ Mid Gain","EQ Mid Q","EQ High Frequency","EQ High Gain","EQ Output"};
    inline static constexpr std::array<float,parameterCount> defaults{0,650,65,55,180,70,25,60,0,35,0,30,12000,0,0,0,0,0,0,0,0,0,12,30,12000,0,-12,100,0,800,0,0,16000,-6,100,0,1,0,0,0,0,.5,0,0,0,0,0,0,120,0,1000,0,.7071,6000,0,0};
    inline static constexpr std::array<float,parameterCount> minima{-12,30,0,0,30,0,0,1,0,0,0,20,500,-24,0,0,0,0,0,0,0,0,0,20,500,0,-36,0,0,40,0,0,500,-36,0,0,.05,-100,0,0,0,.05,-100,0,0,0,0,0,20,-18,100,-18,.1,1000,-18,-12};
    inline static constexpr std::array<float,parameterCount> maxima{12,10000,100,100,3000,100,100,400,1,100,100,1500,20000,12,1,1,1,1,2,100,100,1,48,2000,20000,2,12,100,1,12000,48,48,24000,12,100,1,20,100,3,21,1,20,100,3,21,1,1,1,2000,18,12000,18,8,20000,18,12};
    explicit AcidBassPlugin(tracktion::engine::PluginCreationInfo info):Plugin(info)
    {
        namespace te=tracktion::engine;
        for(std::size_t i=0;i<parameters.size();++i)
        {
            values[i].referTo(state,juce::Identifier(ids[i]),getUndoManager(),defaults[i]);
            if(i==8)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Saw","Square"});
            else if(i==14)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Soft","Sine Fold"});
            else if(i==tubeIndex+4)parameters[i]=new te::SuffixedParameter(ids[i],names[i],*this,{0.f,2.f});
            else if(i==38 || i==43)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,3},4,{"Sine","Triangle","Saw","Square"});
            else if(i==39 || i==44)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,21},22,modulationNames());
            else if(i==lfoRetriggerIndex || i==lfoRetriggerIndex+1)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Free-running","Retrigger on note"});
            else if(i==phaseRetriggerIndex || i==triangleFoldIndex || i==spreadEnableIndex || i==tubeIndex || i==bandIndex || i==35 || i==40 || i==eqIndex)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,1},2,{"Off","On"});
            else if(i==spreadVoicesIndex)parameters[i]=new te::DiscreteLabelledParameter(ids[i],names[i],*this,{0,2},3,{"2","4","8"});
            else
            {
                juce::NormalisableRange<float> range(minima[i],maxima[i]);
                if(i==0)range.interval=1;
                if(i>=eqIndex){if(i==48 || i==50 || i==52 || i==53)range.setSkewForCentre(defaults[i]);parameters[i]=new te::SuffixedParameter(ids[i],names[i],*this,range,i==48 || i==50 || i==53?" Hz":i==52?"":" dB");}
                else
                {
                if(i==1 || i==4 || i==7 || i==11 || i==12 || i==23 || i==24 || i==29 || i==32 || i==36 || i==41)range.setSkewForCentre(defaults[i]);
                parameters[i]=new te::SuffixedParameter(ids[i],names[i],*this,range,i==0?" st":(i==1 || i==11 || i==12)?" Hz":(i==13 || i==22 || i==26 || i==30 || i==31 || i==33)?" dB":(i==36 || i==41)?" Hz":(i==23 || i==24 || i==29 || i==32)?" Hz":(i==4 || i==7)?" ms":" %");
            }
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
    {rate=info.sampleRate;voice.prepare(rate);eq.prepare(rate);fx.prepare(rate,info.blockSizeSamples);rightFx.prepare(rate,info.blockSizeSamples);lfoPhase={};stereoMix=0;stereoFxRunning=false;stereoSmoothing=float(1-std::exp(-1/(.01*rate)));playing=false;previousEnd=-1;}
    void deinitialise() override{voice.panic();resetFx();lfoPhase={};}
    void restorePluginStateFromValueTree(const juce::ValueTree& tree) override
    {for(std::size_t i=0;i<parameters.size();++i)parameters[i]->setParameter(static_cast<float>(tree.getProperty(ids[i],defaults[i])),juce::dontSendNotification);state.setProperty("orbitAcidSkin",juce::jlimit(0,2,int(tree.getProperty("orbitAcidSkin",0))),nullptr);}
    void applyToBuffer(const tracktion::engine::PluginRenderContext& fc) override
    {
        if(fc.destBuffer==nullptr || rate<=0)return;
        std::array<float,parameterCount> p{};for(std::size_t i=0;i<parameters.size();++i)p[i]=parameters[i]->getCurrentValue();

        voice.setParameters({p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[phaseRetriggerIndex]>=.5f,p[spreadEnableIndex]>=.5f,2<<juce::jlimit(0,2,juce::roundToInt(p[spreadVoicesIndex])),p[spreadAmountIndex],p[spreadWidthIndex]});
        // Global LFOs restart for transport changes, never MIDI note cleanup.
        if(playing!=fc.isPlaying){if(!fc.isPlaying){voice.panic();resetFx();}lfoPhase={};}playing=fc.isPlaying;
        const double start=fc.editTime.getStart().inSeconds();const auto loopRange=edit.getTransport().getLoopRange();
        const bool wrap=fc.isPlaying && edit.getTransport().looping.get() && std::abs(previousEnd-loopRange.getEnd().inSeconds())<2/rate && std::abs(start-loopRange.getStart().inSeconds())<2/rate;
        if(!fc.isRendering && previousEnd>=0 && std::abs(start-previousEnd)>2/rate && !wrap){resetFx();lfoPhase={};}
        fc.destBuffer->clear(fc.bufferStartSample,fc.bufferNumSamples);
        const bool modulated=(p[35]>=.5f && p[39]>=.5f && p[37]!=0) || (p[40]>=.5f && p[44]>=.5f && p[42]!=0);
        int cursor=0;
        auto render=[&](int end)
        {
            voice.finishEvents();
            while(cursor<end)
            {
                const int count=modulated?juce::jmin(16,end-cursor):end-cursor;
                auto effective=p;
                std::array<float,parameterCount> offsets{};
                if(modulated)for(size_t l=0;l<2;++l)
                {
                    const size_t at=lfoIndex+l*5;
                    const int target=modulationTargets[size_t(juce::jlimit(0,21,juce::roundToInt(p[at+4])))];
                    if(p[at]>=.5f && target>=0)
                    {
                        const double phase=lfoPhase[l];double wave=std::sin(phase*juce::MathConstants<double>::twoPi);
                        switch(juce::roundToInt(p[at+3])){case 1:wave=1-4*std::abs(phase-.5);break;case 2:wave=phase*2-1;break;case 3:wave=phase<.5?1:-1;break;default:break;}
                        offsets[size_t(target)]+=float(wave)*p[at+2]*.005f;
                    }
                }
                if(modulated)for(size_t i=0;i<parameterCount;++i)if(offsets[i]!=0)
                    effective[i]=parameters[i]->valueRange.convertFrom0to1(juce::jlimit(0.f,1.f,parameters[i]->valueRange.convertTo0to1(p[i])+offsets[i]));
                voice.setParameters({effective[0],effective[1],effective[2],effective[3],effective[4],effective[5],effective[6],effective[7],effective[8],p[phaseRetriggerIndex]>=.5f,p[spreadEnableIndex]>=.5f,2<<juce::jlimit(0,2,juce::roundToInt(p[spreadVoicesIndex])),effective[spreadAmountIndex],effective[spreadWidthIndex]});
                const int first=cursor;
                for(int n=0;n<count;++n,++cursor)
                {
                    const auto value=voice.sampleStereo();
                    for(int channel=0;channel<fc.destBuffer->getNumChannels();++channel)
                        fc.destBuffer->setSample(channel,fc.bufferStartSample+cursor,fc.destBuffer->getNumChannels()==1?(value[0]+value[1])*.5f:value[size_t(channel%2)]);
                }
                processEffects(*fc.destBuffer,fc.bufferStartSample+first,count,effective,voice.isFilterSpreadActive());
                // Keep both clocks running through rests, disabled routes, zero
                // depth and Off destinations; enabling modulation joins the sweep.
                for(size_t l=0;l<2;++l)lfoPhase[l]=std::fmod(lfoPhase[l]+double(p[lfoIndex+l*5+1])*count/rate,1.0);
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
            if(midi->isAllNotesOff && !tiedLoop){voice.panic();resetFx();}
            for(const auto& event:*midi)
            {
                const int sample=juce::jlimit(cursor,fc.bufferNumSamples,juce::roundToInt((event.getTimeStamp()+fc.midiBufferOffset)*rate));
                if(sample>cursor)render(sample);
                if(event.isNoteOn())
                {
                    for(size_t l=0;l<2;++l)if(p[lfoRetriggerIndex+l]>=.5f)lfoPhase[l]=0;
                    voice.noteOn(event.getChannel(),event.getNoteNumber(),event.getVelocity());
                }
                else if(event.isNoteOff())voice.noteOff(event.getChannel(),event.getNoteNumber());
                else if(event.isController())voice.controller(event.getControllerNumber(),event.getControllerValue());
                else if(event.isPitchWheel())voice.pitchBend((event.getPitchWheelValue()-8192)/8192.0f*2);
            }
            midi->clear();
        }
        render(fc.bufferNumSamples);
        eq.setControls({p[47],p[48],p[49],p[50],p[51],p[52],p[53],p[54],p[55]});eq.process(*fc.destBuffer,fc.bufferStartSample,fc.bufferNumSamples);
        previousEnd=fc.editTime.getEnd().inSeconds();
    }
    std::array<tracktion::engine::AutomatableParameter::Ptr,parameterCount> parameters;
private:
    std::array<juce::CachedValue<float>,parameterCount> values;
    void processEffects(juce::AudioBuffer<float>& buffer,int start,int count,const std::array<float,parameterCount>& p,bool stereoWanted)
    {
        const AcidBassFx::Controls controls{p[9],p[10],p[11],p[12],p[13],p[14],p[triangleFoldIndex]};
        const AcidBassFx::StageControls tube{p[21],p[22],p[23],p[24],p[25],p[26],p[27]};
        const AcidBassFx::BandControls band{p[28],p[29],p[30],p[31],p[32],p[33],p[34]};
        fx.setControls(controls);fx.setExtraControls(tube,band);
        if(buffer.getNumChannels()>0)
        {
            fx.process(buffer.getWritePointer(0,start),count);
            if(buffer.getNumChannels()>1)
            {
                if(stereoWanted || stereoMix>1.e-6f)
                {
                    if(!stereoFxRunning){rightFx.reset();stereoFxRunning=true;}
                    rightFx.setControls(controls);rightFx.setExtraControls(tube,band);rightFx.process(buffer.getWritePointer(1,start),count);
                    for(int n=0;n<count;++n)
                    {
                        stereoMix+=stereoSmoothing*((stereoWanted?1.f:0.f)-stereoMix);
                        const int sample=start+n;const float left=buffer.getSample(0,sample);
                        buffer.setSample(1,sample,left+(buffer.getSample(1,sample)-left)*stereoMix);
                    }
                }
                else {stereoMix=0;stereoFxRunning=false;buffer.copyFrom(1,start,buffer,0,start,count);}
                for(int ch=2;ch<buffer.getNumChannels();++ch)buffer.copyFrom(ch,start,buffer,ch%2,start,count);
            }
        }
    }
    std::array<double,2> lfoPhase{};
    void resetFx(){eq.reset();fx.reset();rightFx.reset();stereoMix=0;stereoFxRunning=false;}
    AcidBassEq eq;AcidBassVoice voice;AcidBassFx fx,rightFx;float stereoMix=0,stereoSmoothing=0;bool stereoFxRunning=false;double rate=44100,previousEnd=-1;bool playing=false;
};
}
