#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace orbit::mixing
{
// Original waveshaping/filtering core. Invoke at the oversampled rate.
// No Airwindows, DaisySP, Synapse or AudioRealism DSP implementation is copied.
class DistortionCore
{
public:
    enum class Mode{tube,twoBand};
    struct Controls{double driveLow=12,driveHigh=0,breakpoint=800,lowCut=30,highDamp=12000,shape=0;};
    explicit DistortionCore(Mode m):mode(m){}
    void prepare(double sampleRate)
    {rate=std::clamp(std::isfinite(sampleRate)?sampleRate:192000.,8000.,1536000.);step=1-std::exp(-1/(.005*rate));dcCoefficient=std::exp(-twoPi*5/rate);reset();}
    void reset(){states={};first=true;}
    void setControls(Controls c)
    {
        c.driveLow=bound(c.driveLow,0,48,12);c.driveHigh=bound(c.driveHigh,0,48,0);c.breakpoint=bound(c.breakpoint,40,std::min(12000.,rate*.4),800);
        c.lowCut=bound(c.lowCut,20,std::min(2000.,rate*.4),30);c.highDamp=bound(c.highDamp,500,std::min(24000.,rate*.45),12000);c.shape=bound(c.shape,0,2,0);
        target={std::pow(10.,c.driveLow/20)-1,std::pow(10.,c.driveHigh/20)-1,std::exp(-twoPi*c.breakpoint/rate),std::exp(-twoPi*c.lowCut/rate),std::exp(-twoPi*c.highDamp/rate),c.shape};
        if(first){current=target;first=false;}
    }
    std::array<float,2> process(float left,float right)
    {
        for(size_t i=0;i<current.size();++i)current[i]+=(target[i]-current[i])*step;
        std::array<float,2> result{};const float samples[]={left,right};
        for(size_t ch=0;ch<2;++ch)
        {
            const double x=std::isfinite(samples[ch])?std::clamp(double(samples[ch]),-1.e6,1.e6):0;auto& s=states[ch];double wet=0;
            if(mode==Mode::tube)
            {
                s.low=(1-current[3])*x+current[3]*s.low;const double highPassed=x-s.low;
                s.damp=(1-current[4])*highPassed+current[4]*s.damp;
                const int shape=int(current[5]);const double fraction=current[5]-shape;
                wet=curve(s.damp,current[0],shape)*(1-fraction)+curve(s.damp,current[0],std::min(shape+1,2))*fraction;
                const double filtered=wet-s.dcX+dcCoefficient*s.dcY;s.dcX=wet;s.dcY=filtered;wet=filtered;
            }
            else
            {
                // Complementary one-pole split: low + (input - low) reconstructs
                // exactly before nonlinear processing, avoiding crossover holes.
                s.low=(1-current[2])*x+current[2]*s.low;const double high=x-s.low;
                wet=curve(s.low,current[0],0)+curve(high,current[1],0);
                s.damp=(1-current[4])*wet+current[4]*s.damp;wet=s.damp;
            }
            result[ch]=float(wet);
        }
        return result;
    }
    static double curve(double x,double amount,int shape)
    {
        if(amount<1.e-6)return x;
        if(shape==0)return std::tanh(amount*x)/std::tanh(amount);
        const double bias=shape==1?.35:-.55;
        // Remove static bias; an output DC blocker removes signal-dependent DC.
        return (std::tanh(amount*x+bias)-std::tanh(bias))/(std::tanh(amount+bias)-std::tanh(bias));
    }
private:
    static double bound(double x,double a,double b,double fallback){return std::clamp(std::isfinite(x)?x:fallback,a,b);}
    struct State{double low=0,damp=0,dcX=0,dcY=0;};
    Mode mode;std::array<State,2> states{};std::array<double,6> current{},target{};
    double rate=192000,step=.001,dcCoefficient=.9998;bool first=true;static constexpr double twoPi=6.283185307179586;
};
}
