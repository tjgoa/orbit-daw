#include "../Source/Recording/PatternClipTake.h"
#include <iostream>
#include <stdexcept>
using orbit::recording::PatternClipTake;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main()
{
 try
 {
    PatternClipTake take;take.start({{"a",0},{"b",8}},2,8,2,4);
    check(take.active && take.boundary(4)==2 && take.boundary(4.001)==3,"bar quantization wrong");
    check(take.select("a",1,4.2) && take.select("a",7,4.3),"queued selection refused");
    check(take.launches["a"].at(3)==7 && take.launches["a"].size()==2,"same-bar last choice did not win");
    take.select("a",0,4.4);check(take.launches["a"].size()==1,"reselecting current pattern did not cancel queued switch");
    take.select("a",1,5);take.select("a",2,6.1);take.select("b",9,8.4);
    check(!take.select("other",0,5) && !take.select("a",64,5) && !take.select("a",1,16),"invalid target/slot/end accepted");
    auto p=take.segments(5);check(p.size()==4,"multi-track segment count wrong");
    check(p[0].startBar==2 && p[0].lengthBars==1 && p[1].slot==1 && p[1].lengthBars==1 && p[2].slot==2 && p[2].lengthBars==1 && p[3].id=="b" && p[3].lengthBars==3,"pattern ranges overlap or extend beyond visible cursor");
    take.finish(10);check(!take.active && take.finishedBar==5 && take.segments(take.finishedBar).size()==4,"Stop included unplayed pending launch");take.finish(100);check(take.finishedBar==5,"retry extended stopped take");
    take.start({{"a",0}},3,4,2,4.1);take.finish(5.9);check(take.segments(take.finishedBar).empty(),"canceled wait created a clip");
    for(int first:{0,1,510,511})
    {
        take.start({{"a",0}},first,512,2,first*2.);
        for(int b=first;b<512;++b){take.select("a",b%64,b*2.);const auto segments=take.segments(b+1);int last=first;for(auto s:segments){check(s.startBar==last && s.lengthBars>0 && s.startBar+s.lengthBars<=512,"coverage has gap, overlap or out-of-bounds range");last+=s.lengthBars;}check(last==b+1,"performed region not covered");}
        take.finish(2000);check(take.finishedBar==512,"playlist end not bounded");
    }
    std::cout<<"PASS: next-bar launches, last-choice/cancel, independent targets, contiguous bounded segments, stop/cancel/retry and full 512-bar coverage.\n";
 }
 catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
