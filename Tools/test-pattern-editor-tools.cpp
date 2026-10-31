#include "../Source/Editing/PatternEditorTools.h"
#include <cassert>
#include <iostream>
int main()
{
    using namespace orbit::editing;
    std::vector<Note> phrase{{60,0,65,71},{64,128,64,103},{67,128,64,96}};
    auto moved=phrase;assert(!translatePhrase(moved,0,-1,1024) && moved==phrase);
    assert(!translatePhrase(moved,70,0,1024) && moved==phrase);
    assert(translatePhrase(moved,2,64,1024));assert(moved[1].pitch==66 && moved[1].startTick==192 && moved[1].velocity==103);
    auto cut=scissorPhrase(phrase);assert(cut.size()==6 && cut[0].lengthTicks==32 && cut[1].lengthTicks==33 && cut[1].startTick==32);
    auto flip=phrase;flipPhrase(flip,true,1024,true);flipPhrase(flip,true,1024,true);assert(flip==phrase);
    flipPhrase(flip,false,1024,false);flipPhrase(flip,false,1024,false);assert(flip==phrase);
    std::vector<Note> far{{60,maximumTick-200,63,100},{64,maximumTick-100,64,90}};
    auto farFlip=far;flipPhrase(farFlip,true,maximumTick,false);flipPhrase(farFlip,true,maximumTick,false);assert(far==farFlip);
    const auto rotated=rotatePhrase({{60,960,64,100},{64,0,128,90}},64,1024);
    assert(rotated.size()==2 && rotated[0].startTick==0 && rotated[1].startTick==64);
    const auto split=rotatePhrase({{60,900,100,100}},64,1024);assert(split.size()==2 && split[0].lengthTicks+split[1].lengthTicks==100);
    for(auto shape:{LfoShape::sine,LfoShape::triangle,LfoShape::rising,LfoShape::falling,LfoShape::square})
        for(int steps:{1,16,1024,65536})
        {
            auto points=lfoPoints(steps,shape,4,0.125,0.5,0.5);assert(points.size()<=4096 && points.front().first==0);
            int last=-1;for(auto [position,value]:points){assert(position>last && position<steps && value>=0 && value<=1);last=position;}
            if(steps>1)assert(last==steps-1);
        }
    std::cout<<"PASS: atomic group bounds, exact odd-tick scissor, reversible flips, wrapped rotation and bounded five-shape LFO points.\n";
}
