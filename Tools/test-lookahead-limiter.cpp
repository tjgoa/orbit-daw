#include "../Source/Mixing/LookaheadLimiter.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <random>
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  orbit::mixing::LookaheadLimiter p;p.prepare(rate);p.setControls(0,100);int L=p.latencySamples();
  for(int i=0;i<L+100;++i){auto out=p.process(i==0?.25f:0,i==0?-.5f:0);assert(std::abs(out[0]-(i==L?.25f:0))<1e-6);assert(std::abs(out[1]-(i==L?-.5f:0))<1e-6);}
  p.reset();p.setControls(36,5);std::mt19937 rng(42);std::uniform_real_distribution<float> random(-20,20);
  double maximum=0;for(int i=0;i<100000;++i){float a=random(rng);auto out=p.process(a,a*.25f);maximum=std::max(maximum,double(std::abs(out[0])));assert(std::isfinite(out[0])&&std::abs(out[0])<=1&&std::abs(out[1])<=1);assert(std::abs(out[1]-out[0]*.25f)<1e-6);}
  assert(maximum>.9);p.process(std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN());for(int i=0;i<L+2;++i){auto out=p.process(0,0);assert(std::isfinite(out[0])&&std::isfinite(out[1]));}
  p.reset();p.setControls(0,5);for(int i=0;i<L+1000;++i){auto out=p.process(i==100?10.f:.1f,0);assert(std::abs(out[0])<=1);if(i==100+L)assert(std::abs(out[0]-1)<1e-5);}for(int i=0;i<int(rate*2);++i)p.process(.1f,0);assert(p.reductionDb<.01);
  std::cout<<"rate "<<rate<<" delay "<<L<<" max "<<maximum<<"\n";
 }std::cout<<"PASS: limiter delay, transparency, ceiling, stereo link, finite extremes and recovery\n";
}
