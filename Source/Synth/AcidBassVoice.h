#pragma once
#include <memory>
#include <array>

namespace orbit::synth
{
struct AcidParameters
{
    float tune=0,cutoff=650,resonance=65,envelope=55,decay=180,accent=70,accentEnvelope=25,slide=60,waveform=0;
    bool phaseRetrigger=false;
    bool filterSpreadEnabled=false;
    int filterVoices=2;
    float filterSpread=0,stereoWidth=0;
};
// The upstream MIT DSP is hidden behind this boundary. Rendering and MIDI
// dispatch use fixed-capacity note state, with no per-note allocation.
class AcidBassVoice
{
public:
    AcidBassVoice();~AcidBassVoice();
    void prepare(double sampleRate);
    void setParameters(AcidParameters);
    void noteOn(int channel,int pitch,int velocity);
    void noteOff(int channel,int pitch);
    void controller(int number,int value);
    void pitchBend(float semitones);
    void finishEvents();
    void panic();
    bool hasOutgoingSlide() const;
    bool isFilterSpreadActive() const;
    float sample();
    std::array<float,2> sampleStereo();
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
}
