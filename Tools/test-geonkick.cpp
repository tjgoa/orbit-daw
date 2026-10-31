// Core adapter regression: real upstream DSP, no audio device or product build.
#include "../Source/Synth/GeonkickVoice.h"
#include <array>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <chrono>
#if defined(ORBIT_GEONKICK_RT_GUARD)
#include <cstdlib>
#include <pthread.h>
static bool renderThread=false;static int forbiddenCalls=0;
extern "C" void* orbit_checked_malloc(size_t n){if(renderThread)++forbiddenCalls;return malloc(n);}
extern "C" void* orbit_checked_calloc(size_t n,size_t s){if(renderThread)++forbiddenCalls;return calloc(n,s);}
extern "C" void orbit_checked_free(void* p){if(renderThread)++forbiddenCalls;free(p);}
extern "C" int orbit_checked_lock(pthread_mutex_t* p){if(renderThread)++forbiddenCalls;return pthread_mutex_lock(p);}
extern "C" int orbit_checked_unlock(pthread_mutex_t* p){if(renderThread)++forbiddenCalls;return pthread_mutex_unlock(p);}
#endif
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Drum=std::unique_ptr<OrbitGeonkick,decltype(&orbit_geonkick_destroy)>;
static Drum make(){Drum d(orbit_geonkick_create(),orbit_geonkick_destroy);check(bool(d),"allocation failed");return d;}
static std::vector<float> render(double rate,std::array<float,8> p,int velocity=127,int chunks=512)
{
    auto drum=make();orbit_geonkick_prepare(drum.get(),rate);orbit_geonkick_controls(drum.get(),p.data());orbit_geonkick_trigger(drum.get(),36,velocity);
    std::vector<float> out(size_t(rate*.6));
    for(size_t n=0;n<out.size();){orbit_geonkick_controls(drum.get(),p.data());for(int i=0;i<chunks && n<out.size();++i,++n){out[n]=orbit_geonkick_sample(drum.get());check(std::isfinite(out[n]) && std::abs(out[n])<=1,"non-finite or unbounded audio");}}
    return out;
}
static double energy(const std::vector<float>& a){double s=0;for(float x:a)s+=x*x;return s;}
int main()
{
 try {
    std::array<float,8> p{0,0,350,65,35,4500,0,-9};
    for(double rate:{22050.,44100.,48000.,96000.,192000.})for(int mode=0;mode<6;++mode){p[0]=float(mode);auto a=render(rate,p);check(energy(a)>1.e-4,"silent recipe");for(size_t i=size_t(rate*.351);i<a.size();++i)check(a[i]==0,"one-shot exceeded envelope duration");auto b=render(rate,p,127,79);check(a==b,"block size changed audio");}
    p[0]=0;auto loud=render(48000,p);auto soft=render(48000,p,50);check(std::abs(energy(soft)/energy(loud)-std::pow(50./127,2))<1.e-6,"velocity response wrong");
    auto changed=p;changed[5]=600;auto dark=render(48000,changed);check(dark!=loud,"Tone has no effect");changed=p;changed[6]=80;check(render(48000,changed)!=loud,"Drive has no effect");
    auto a=make(),b=make();orbit_geonkick_controls(a.get(),p.data());orbit_geonkick_controls(b.get(),p.data());orbit_geonkick_trigger(a.get(),36,100);orbit_geonkick_trigger(b.get(),36,100);
    for(int i=0;i<10000;i++)check(orbit_geonkick_sample(a.get())==orbit_geonkick_sample(b.get()),"instances share mutable state");
    orbit_geonkick_panic(a.get());for(int i=0;i<500;i++)check(orbit_geonkick_sample(a.get())==0,"panic leaked tail");
    const auto start=std::chrono::steady_clock::now();
    for(int mode=0;mode<6;++mode){p={float(mode),24,2000,100,100,12000,100,6};orbit_geonkick_controls(a.get(),p.data());for(int i=0;i<48000;i++){if(i%6000==0)orbit_geonkick_trigger(a.get(),127,127);const float x=orbit_geonkick_sample(a.get());check(std::isfinite(x) && std::abs(x)<=1,"extreme polyphony unstable");}}
    p[2]=NAN;p[5]=INFINITY;orbit_geonkick_controls(a.get(),p.data());orbit_geonkick_trigger(a.get(),-99,200);for(int i=0;i<10000;i++)check(std::isfinite(orbit_geonkick_sample(a.get())),"invalid controls unstable");
#if defined(ORBIT_GEONKICK_RT_GUARD)
    renderThread=true;
    for(int i=0;i<120000;++i){if(i%4000==0){orbit_geonkick_controls(a.get(),p.data());orbit_geonkick_trigger(a.get(),36,100);}if(i%13000==0)orbit_geonkick_panic(a.get());orbit_geonkick_sample(a.get());}
    renderThread=false;check(forbiddenCalls==0,"audio callback allocated/freed or called a mutex");std::cout<<"PASS: instrumented callback controls/trigger/sample/panic never allocate, free or lock.\n";
#endif
    std::cout<<"PASS: six recipes, five sample rates, block invariance, velocity, tone/drive response, isolated instances, panic, extremes/polyphony. Stress render "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" s (sanitizers included).\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
