#include "../Source/Mixing/DistortionCore.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using Core=orbit::mixing::DistortionCore;
void check(bool ok,const char* s){if(!ok)throw std::runtime_error(s);}
double tone(Core::Mode mode,Core::Controls c,double hz,int harmonic,double rate)
{Core p(mode);p.prepare(rate);p.setControls(c);double real=0,imag=0;for(int i=0;i<int(rate);++i){const double a=i*hz*6.283185307179586/rate;const auto out=p.process(float(.15*std::sin(a)),0);if(i>=int(rate/2)){real+=out[0]*std::cos(a*harmonic);imag+=out[0]*std::sin(a*harmonic);}}return 4*std::hypot(real,imag)/rate;}
int main(){try{
 for(double rate:{44100.,48000.,96000.,192000.})
 {
  Core::Controls c;c.driveLow=0;c.driveHigh=0;c.highDamp=24000;const auto neutral=tone(Core::Mode::twoBand,c,200,1,rate*4);
  check(std::abs(neutral-.15)<.001,"complementary band split failed neutral low tone");
  c.driveLow=36;const auto low=tone(Core::Mode::twoBand,c,120,3,rate*4);c.driveLow=0;c.driveHigh=36;const auto highOnLow=tone(Core::Mode::twoBand,c,120,3,rate*4);
  check(low>highOnLow*1.2,"independent band drives failed low-frequency separation");
  c={};c.driveLow=24;c.shape=0;auto odd=tone(Core::Mode::tube,c,500,3,rate*4),even=tone(Core::Mode::tube,c,500,2,rate*4);check(odd>even*20,"symmetric shape introduced strong even harmonics");
  c.shape=1;check(tone(Core::Mode::tube,c,500,2,rate*4)>even*20,"asymmetric mode did not add even harmonics");
  const auto bass=tone(Core::Mode::tube,c,60,1,rate*4);c.lowCut=1500;check(tone(Core::Mode::tube,c,60,1,rate*4)<bass*.5,"pre-low-cut did not affect bass drive");
  c.lowCut=30;c.highDamp=500;const auto dark=tone(Core::Mode::tube,c,4000,1,rate*4);c.highDamp=20000;check(tone(Core::Mode::tube,c,4000,1,rate*4)>dark*1.2,"pre-high-damping did not affect drive");
  Core p(Core::Mode::tube);p.prepare(rate*4);p.setControls(c);double avg=0;for(int i=0;i<int(rate*4);++i){auto out=p.process(.2f,0);if(i>int(rate*3))avg+=out[0];check(out[1]==0,"stereo leaked between channels");}check(std::abs(avg/rate)<.001,"tube DC did not decay to silence");
  std::cout<<"rate="<<rate<<" low-band harmonic="<<low<<" high-band low-tone harmonic="<<highOnLow<<"\n";
 }
 for(auto mode:{Core::Mode::tube,Core::Mode::twoBand}){Core p(mode);p.prepare(192000);Core::Controls c;c.driveLow=48;c.driveHigh=48;c.shape=2;p.setControls(c);for(int i=0;i<10000;++i){auto out=p.process(i%2?std::numeric_limits<float>::infinity():1000, i%3?std::numeric_limits<float>::quiet_NaN():-1000);check(std::isfinite(out[0]) && std::isfinite(out[1]),"extremes produced nonfinite audio");}}
 std::cout<<"PASS: original distortion split/drive separation, harmonic symmetry, tone filtering, stereo isolation, DC recovery and finite extremes at four rates.\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
