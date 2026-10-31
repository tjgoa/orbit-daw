// Standalone regression checks for Orbit's original musical editing algorithms.
// clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined \
//   Tools/test-pattern-tools.cpp -o /tmp/orbit-pattern-tools-check
#include "../Source/Editing/PatternTools.h"
#include <iostream>
#include <stdexcept>
using namespace orbit::editing;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static void valid(const std::vector<Note>& notes)
{
    for(const auto& n:notes)check(n.pitch>=0 && n.pitch<=127 && n.startTick>=0 && n.startTick<maximumTick
        && n.lengthTicks>0 && n.lengthTicks<=maximumTick-n.startTick && n.velocity>=1 && n.velocity<=127,"invalid transformed note");
}
int main()
{
    const std::vector<Note> source{{60,1,17,40},{60,1,29,120},{45,97,97,77},{0,2,1,1},{127,1023,1,127}};
    auto notes=source;quantize(notes,16,true,false);
    check(notes.size()==source.size() && notes[0].startTick==0 && notes[2].startTick==128 && notes[2].lengthTicks==97,"start quantize changed duration or merged events");
    notes=source;quantize(notes,16,false,true);check(notes[0].startTick==1 && notes[0].lengthTicks==1 && notes[2].lengthTicks==128,"duration-only quantize changed starts");
    check(snap(128,32)==128 && snap(127,12)==85,"rational/triplet grid incorrect");
    notes=source;quantize(notes,256,false,false);check(notes==source,"unchecked quantize changed notes");
    notes=source;check(!transpose(notes,1) && notes==source,"transpose overflow partially mutated notes");
    check(!transpose(notes,-1) && notes==source,"transpose underflow partially mutated notes");
    notes={{60,1,17,40},{64,1,29,120}};check(transpose(notes,12) && notes[0].pitch==72 && notes[1].pitch==76 && notes[0].startTick==1,"transpose lost timing");
    notes=source;scaleLengths(notes,100,true,true);check(notes==source,"100-percent scale changed exact notes");
    scaleLengths(notes,20,true,false);check(notes[2].startTick==19 && notes[2].lengthTicks==97 && notes.size()==5,"scale rounded untouched duration or merged notes");
    notes={{60,64,64,100},{64,0,64,100},{67,64,64,100},{72,128,64,100}};scaleVelocities(notes,20,100);
    check(notes[0].velocity==60 && notes[1].velocity==20 && notes[2].velocity==60 && notes[3].velocity==100,"velocity ramp is not chronological/chord-consistent");
    std::mt19937 random(12345);notes=source;humanize(notes,0,0,random);check(notes==source,"zero humanize changed notes");
    humanize(notes,2,10,random);valid(notes);for(size_t i=0;i<notes.size();++i)check(std::abs(notes[i].startTick-source[i].startTick)<=2 && std::abs(notes[i].velocity-source[i].velocity)<=10 && notes[i].lengthTicks==source[i].lengthTicks,"humanize exceeded bounds");
    for(auto type:{Groove::none,Groove::softerOneTwo,Groove::softerThreeFour,Groove::jazzSwing,Groove::bassline,Groove::salsa,Groove::shuffle})
    {
        notes=source;groove(notes,type,16,0,0,0);check(notes==source,"zero groove strength changed notes");
        notes=source;groove(notes,type,16,1,1,1);valid(notes);check(notes.size()==source.size(),"groove changed multiplicity");
        if(type==Groove::softerOneTwo || type==Groove::softerThreeFour)for(size_t i=0;i<notes.size();++i)check(notes[i].startTick==source[i].startTick && notes[i].lengthTicks==source[i].lengthTicks,"velocity-only groove changed timing");
    }
    const std::vector<Note> chord{{60,0,64,100},{64,0,64,100},{67,0,64,100},{72,256,97,77}};
    for(auto type:{Pluck::up,Pluck::down,Pluck::curvedUp,Pluck::curvedDown,Pluck::alternating,Pluck::random})
    {
        notes=chord;pluck(notes,type,0,0,random);check(notes==chord,"zero pluck strength changed notes");
        notes=chord;pluck(notes,type,1,1,random);valid(notes);check(notes.size()==4 && notes[3]==chord[3],"pluck altered a non-chord note");
    }
    notes=chord;pluck(notes,Pluck::up,1,0,random);check(notes[0].startTick==0 && notes[1].startTick==64 && notes[2].startTick==128,"up pluck direction/spread incorrect");
    notes=chord;pluck(notes,Pluck::down,1,0,random);check(notes[2].startTick==0 && notes[0].startTick==128,"down pluck direction incorrect");
    for(auto type:{RandomAlgorithm::tranceOne,RandomAlgorithm::tranceTwo,RandomAlgorithm::pureRandom,RandomAlgorithm::chord})
    {
        auto a=std::mt19937(7),b=std::mt19937(7);
        auto first=randomPattern(64,75,48,84,0,type,0,a),second=randomPattern(64,75,48,84,0,type,0,b);
        check(first==second && !first.empty(),"random generator is not reproducible with its seed");valid(first);
        for(const auto& n:first)check(n.startTick+n.lengthTicks<=4096,"random note escaped pattern");
        auto drums=randomPattern(16,100,48,84,0,type,0,a,{36,38,42});valid(drums);
        for(const auto& n:drums)check(n.pitch==36 || n.pitch==38 || n.pitch==42,"drum randomizer generated an unassigned pitch");
    }
    for(int i=0;i<int(pitchSets().size());++i){auto values=randomPattern(16,100,24,96,0,RandomAlgorithm::chord,i,random);valid(values);check(!values.empty(),"empty chord/scale catalog entry");}
    notes={{127,maximumTick-1,1,127}};scaleLengths(notes,1600,true,true);quantize(notes,4,true,true);humanize(notes,1024,126,random);groove(notes,Groove::shuffle,2,1,1,1);valid(notes);
    std::cout<<"PASS: rational quantize, independent timing fields, transpose rollback, exact scaling, chronological velocities, bounded humanize, all groove/pluck algorithms and zero-strength identity, seeded random algorithms/chord catalog/drum pitches, coincident-event retention and maximum timing bounds.\n";
}
