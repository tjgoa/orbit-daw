// Core integration tests: actual pinned 9W9 engine and samples.
#include "../Source/Synth/NineKitVoice.h"
#include "../Source/Synth/NineKitMetadata.h"
#include <vector>
#include <memory>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
static bool realtime=false;static int forbidden=0;
extern "C" void* nine_checked_malloc(size_t n){if(realtime)++forbidden;return malloc(n);}
extern "C" void* nine_checked_calloc(size_t n,size_t s){if(realtime)++forbidden;return calloc(n,s);}
extern "C" void nine_checked_free(void* p){if(realtime)++forbidden;free(p);}
using Kit=std::unique_ptr<OrbitNineKit,decltype(&orbit_nine_destroy)>;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static Kit make(double rate){Kit p(orbit_nine_create(),orbit_nine_destroy);check(bool(p),"allocation failed");orbit_nine_prepare(p.get(),rate);return p;}
static std::vector<float> sound(double rate,int voice,int velocity=127,int block=512,int accent=0)
{auto p=make(rate);std::vector<float> out(size_t(rate*.35));orbit_nine_trigger(p.get(),voice,velocity,accent,70);for(size_t i=0;i<out.size();i+=size_t(block))orbit_nine_render(p.get(),out.data()+i,int(std::min(size_t(block),out.size()-i)));for(float x:out)check(std::isfinite(x) && std::abs(x)<32,"non-finite or excessive output");return out;}
static double energy(const std::vector<float>& a){double x=0;for(float y:a)x+=y*y;return x;}
int main()
{
 try {
    for(double rate:{22050.,44100.,48000.,96000.,192000.})for(int voice=0;voice<11;++voice){auto a=sound(rate,voice);check(energy(a)>1.e-7,"silent drum");check(a==sound(rate,voice,127,79),"block segmentation changed sound");check(energy(sound(rate,voice,35))<energy(a),"velocity did not change loudness");}
    auto low=sound(44100,7),high=sound(88200,7);double error=0,reference=0;for(size_t n=0;n<low.size();++n){error+=std::pow(low[n]-high[n*2],2);reference+=low[n]*low[n];}check(error/reference<.001,"sampled hats changed pitch/duration with host rate");
    check(energy(sound(48000,0,100,512,1))>energy(sound(48000,0,100,512,0))*1.2,"step accent ineffective");
    auto p=make(48000);std::vector<float> out(24000);orbit_nine_trigger(p.get(),7,100,0,0);orbit_nine_render(p.get(),out.data(),4800);orbit_nine_trigger(p.get(),8,100,0,0);orbit_nine_render(p.get(),out.data(),24000);for(int n=12000;n<24000;++n)check(out[size_t(n)]==0,"closed hat did not choke open hat");
    auto a=make(48000),b=make(48000);orbit_nine_trigger(a.get(),1,100,0,0);orbit_nine_trigger(b.get(),1,100,0,0);std::vector<float> one(4000),two(4000);orbit_nine_render(a.get(),one.data(),4000);orbit_nine_render(b.get(),two.data(),4000);check(one==two,"instances share noise/voice state");
    for(const auto& s:orbit::synth::nine::specs){orbit_nine_set(p.get(),s.id,float(s.maximum));}
    for(int v=0;v<11;++v)orbit_nine_trigger(p.get(),v,127,1,100);orbit_nine_render(p.get(),out.data(),24000);for(float x:out)check(std::isfinite(x),"extreme controls unstable");orbit_nine_panic(p.get());orbit_nine_render(p.get(),out.data(),24000);for(float x:out)check(x==0,"panic leaked voice/FX tails");
    realtime=true;orbit_nine_prepare(p.get(),96000);for(int i=0;i<500;++i){orbit_nine_set(p.get(),"bd_c_attack",float(i%128));orbit_nine_tempo(p.get(),float(80+i%80));orbit_nine_trigger(p.get(),i%11,110,i%2,60);orbit_nine_render(p.get(),out.data(),64);if(i%17==0)orbit_nine_panic(p.get());}realtime=false;check(forbidden==0,"callback allocated/freed");
    std::cout<<"PASS: eleven audible drums, five rates, block invariance, velocity, sample-rate correction, step accent, hat choke, instance isolation, parameter extremes, FX panic, allocation-free callbacks.\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
