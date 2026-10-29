#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <string>
#include <vector>

// Original musical editing algorithms. No engine, UI or source-file-format types.
namespace orbit::editing
{
constexpr int ticksPerQuarter = 256;
constexpr int maximumTick = (std::numeric_limits<int>::max() / 64) * 64;
struct Note
{
    int pitch = 60, startTick = 0, lengthTicks = 64, velocity = 100;
    bool operator==(const Note&) const = default;
};
inline int rounded(double value, int low, int high)
{
    return static_cast<int>(std::llround(std::clamp(value, double(low), double(high))));
}
inline int snap(int tick, int resolution)
{
    // Rational grid calculation supports triplets as well as powers of two.
    const double grid = 1024.0 / std::clamp(resolution, 1, 1024);
    return rounded(std::round(tick / grid) * grid, 0, maximumTick - 1);
}
inline void quantize(std::vector<Note>& notes, int resolution, bool starts, bool durations)
{
    for (auto& n : notes)
    {
        if (starts) n.startTick = snap(n.startTick, resolution);
        if (durations) n.lengthTicks = std::max(1, snap(n.lengthTicks, resolution));
        n.lengthTicks = std::min(n.lengthTicks, maximumTick - n.startTick);
    }
}
inline bool transpose(std::vector<Note>& notes, int semitones)
{
    for (const auto& n : notes) if (n.pitch + semitones < 0 || n.pitch + semitones > 127) return false;
    for (auto& n : notes) n.pitch += semitones;
    return true;
}
inline void scaleLengths(std::vector<Note>& notes, double percent, bool starts, bool durations)
{
    const double ratio = std::clamp(percent, 1.0, 1600.0) / 100.0;
    for (auto& n : notes)
    {
        if (starts) n.startTick = rounded(n.startTick * ratio, 0, maximumTick - 1);
        if (durations) n.lengthTicks = rounded(n.lengthTicks * ratio, 1, maximumTick - n.startTick);
        else n.lengthTicks = std::min(n.lengthTicks, maximumTick - n.startTick);
    }
}
inline void scaleVelocities(std::vector<Note>& notes, int begin, int end)
{
    if (notes.empty()) return;
    const auto [first, last] = std::minmax_element(notes.begin(), notes.end(),
        [](const auto& a, const auto& b) { return a.startTick < b.startTick; });
    const auto start = first->startTick, length = last->startTick - start;
    for (auto& n : notes)
    {
        const double amount = length == 0 ? 0.0 : double(n.startTick - start) / length;
        n.velocity = rounded(begin + amount * (end - begin), 1, 127);
    }
}
inline void humanize(std::vector<Note>& notes, int maxTicks, int maxVelocity, std::mt19937& random)
{
    std::uniform_int_distribution<int> time(-std::clamp(maxTicks, 0, 1024), std::clamp(maxTicks, 0, 1024));
    std::uniform_int_distribution<int> velocity(-std::clamp(maxVelocity, 0, 126), std::clamp(maxVelocity, 0, 126));
    for (auto& n : notes)
    {
        n.startTick = rounded(double(n.startTick) + time(random), 0, maximumTick - n.lengthTicks);
        n.velocity = std::clamp(n.velocity + velocity(random), 1, 127);
    }
}
struct PitchSet { std::string name; std::vector<int> intervals; };
inline const std::vector<PitchSet>& pitchSets()
{
    static const std::vector<PitchSet> values {
        {"Major",{0,4,7}}, {"Minor",{0,3,7}}, {"Major 6",{0,4,7,9}}, {"Minor 6",{0,3,7,9}},
        {"Major 6/9",{0,2,4,7,9}}, {"Minor 6/9",{0,2,3,7,9}}, {"Dominant 7",{0,4,7,10}},
        {"Major 7",{0,4,7,11}}, {"Minor 7",{0,3,7,10}}, {"Minor major 7",{0,3,7,11}},
        {"Major 9",{0,2,4,7,11}}, {"Minor 9",{0,2,3,7,10}}, {"Dominant 9",{0,2,4,7,10}},
        {"Major 11",{0,2,4,5,7,11}}, {"Minor 11",{0,2,3,5,7,10}},
        {"Major 13",{0,2,4,7,9,11}}, {"Minor 13",{0,2,3,7,9,10}},
        {"Diminished",{0,3,6}}, {"Diminished 7",{0,3,6,9}}, {"Half diminished 7",{0,3,6,10}},
        {"Augmented",{0,4,8}}, {"Sus 2",{0,2,7}}, {"Sus 4",{0,5,7}}, {"Sus 7",{0,5,7,10}},
        {"Sus 9",{0,2,5,7,10}}, {"Power fifth",{0,7}},
        {"Ionian / major scale",{0,2,4,5,7,9,11}}, {"Dorian",{0,2,3,5,7,9,10}},
        {"Phrygian",{0,1,3,5,7,8,10}}, {"Lydian",{0,2,4,6,7,9,11}},
        {"Mixolydian",{0,2,4,5,7,9,10}}, {"Aeolian / natural minor",{0,2,3,5,7,8,10}},
        {"Locrian",{0,1,3,5,6,8,10}}, {"Harmonic minor",{0,2,3,5,7,8,11}},
        {"Melodic minor",{0,2,3,5,7,9,11}}, {"Major pentatonic",{0,2,4,7,9}},
        {"Minor pentatonic",{0,3,5,7,10}}, {"Blues",{0,3,5,6,7,10}},
        {"Whole tone",{0,2,4,6,8,10}}, {"Whole-half diminished",{0,2,3,5,6,8,9,11}},
        {"Half-whole diminished",{0,1,3,4,6,7,9,10}}, {"Chromatic",{0,1,2,3,4,5,6,7,8,9,10,11}}
    };
    return values;
}
enum class RandomAlgorithm { tranceOne, tranceTwo, pureRandom, chord };
inline std::vector<Note> randomPattern(int lengthSteps, int density, int low, int high, int root,
                                       RandomAlgorithm algorithm, int pitchSet, std::mt19937& random,
                                       const std::vector<int>& drumPitches = {})
{
    std::vector<Note> result;
    low = std::clamp(low, 0, 127); high = std::clamp(high, low, 127); root = std::clamp(root, 0, 11);
    const auto& intervals = pitchSets()[static_cast<size_t>(std::clamp(pitchSet, 0, int(pitchSets().size()) - 1))].intervals;
    std::vector<int> pitches;
    for (int p = low; p <= high; ++p)
        if (algorithm == RandomAlgorithm::pureRandom
            || std::find(intervals.begin(), intervals.end(), (p - root + 120) % 12) != intervals.end()) pitches.push_back(p);
    if (pitches.empty() && drumPitches.empty()) return result;
    std::uniform_int_distribution<int> chance(0, 99), velocity(75, 115);
    for (int step = 0; step < lengthSteps; ++step)
    {
        if (chance(random) >= std::clamp(density, 0, 100)) continue;
        const auto start = step * 64;
        const auto add = [&](int pitch, int length, int vel)
        { result.push_back({pitch, start, std::min(length, lengthSteps * 64 - start), vel}); };
        if (!drumPitches.empty())
        {
            const auto index = static_cast<size_t>(algorithm == RandomAlgorithm::pureRandom ? random() % drumPitches.size()
                : (step % 4 == 0 ? 0 : step % 4 == 2 && drumPitches.size() > 1 ? 1 : drumPitches.size() - 1));
            add(drumPitches[index], 64, velocity(random));
        }
        else if (algorithm == RandomAlgorithm::chord)
        {
            const auto base = low + (root - low % 12 + 12) % 12;
            for (int interval : intervals) if (base + interval <= high) add(base + interval, 64, velocity(random));
        }
        else
        {
            size_t index = 0;
            if (algorithm == RandomAlgorithm::pureRandom) index = random() % pitches.size();
            else if (algorithm == RandomAlgorithm::tranceOne) index = static_cast<size_t>(std::array<int,8>{0,0,2,0,1,0,3,1}[size_t(step % 8)]) % pitches.size();
            else index = static_cast<size_t>(std::array<int,8>{0,2,1,3,2,4,1,2}[size_t(step % 8)]) % pitches.size();
            add(pitches[index], algorithm == RandomAlgorithm::tranceTwo ? 48 : 64, velocity(random));
        }
    }
    return result;
}
enum class Groove { none, softerOneTwo, softerThreeFour, jazzSwing, bassline, salsa, shuffle };
inline void groove(std::vector<Note>& notes, Groove type, int resolution, double position, double length, double velocity)
{
    if (type == Groove::none) return;
    const double grid = 1024.0 / std::clamp(resolution, 2, 64);
    for (auto& n : notes)
    {
        const auto slot = static_cast<std::int64_t>(std::llround(n.startTick / grid));
        const auto phase = static_cast<size_t>(slot % 4);
        double offset = 0, lengthFactor = 1, velocityFactor = 1;
        switch (type)
        {
            case Groove::softerOneTwo: velocityFactor = phase < 2 ? 0.65 : 1.0; break;
            case Groove::softerThreeFour: velocityFactor = phase >= 2 ? 0.65 : 1.0; break;
            case Groove::jazzSwing: offset = (slot % 2) * grid / 3; lengthFactor = slot % 2 ? 2.0/3 : 4.0/3; velocityFactor = slot % 2 ? 0.8 : 1; break;
            case Groove::bassline: offset = phase == 3 ? -grid * 0.1 : 0; lengthFactor = phase % 2 ? 0.5 : 0.8; velocityFactor = phase == 0 ? 1 : 0.75; break;
            case Groove::salsa: offset = grid * std::array<double,4>{0,-0.08,0.12,-0.04}[phase]; lengthFactor = 0.7; velocityFactor = std::array<double,4>{1,0.7,0.9,0.75}[phase]; break;
            case Groove::shuffle: offset = (slot % 2) * grid / 3; lengthFactor = slot % 2 ? 0.6 : 1.2; velocityFactor = slot % 2 ? 0.85 : 1; break;
            case Groove::none: break;
        }
        const bool velocityOnly = type == Groove::softerOneTwo || type == Groove::softerThreeFour;
        if (!velocityOnly)
        {
            n.startTick = rounded(n.startTick + (slot * grid + offset - n.startTick) * std::clamp(position,0.0,1.0),0,maximumTick-1);
            n.lengthTicks = rounded(n.lengthTicks + (grid * lengthFactor - n.lengthTicks) * std::clamp(length,0.0,1.0),1,maximumTick-n.startTick);
        }
        n.velocity = rounded(n.velocity * (1 + (velocityFactor - 1) * std::clamp(velocity,0.0,1.0)),1,127);
    }
}
enum class Pluck { up, down, curvedUp, curvedDown, alternating, random };
inline void pluck(std::vector<Note>& notes, Pluck type, double position, double velocity, std::mt19937& random)
{
    std::map<int,std::vector<size_t>> chords;
    for (size_t i=0;i<notes.size();++i) chords[notes[i].startTick].push_back(i);
    for (auto& [start, indices] : chords)
    {
        if (indices.size()<2) continue;
        std::stable_sort(indices.begin(),indices.end(),[&](size_t a,size_t b) {return notes[a].pitch<notes[b].pitch;});
        if (type==Pluck::down || type==Pluck::curvedDown) std::reverse(indices.begin(),indices.end());
        if (type==Pluck::alternating)
        {
            auto source=indices;indices.clear();size_t lo=0,hi=source.size()-1;
            while(lo<=hi) {indices.push_back(source[lo++]);if(lo<=hi)indices.push_back(source[hi--]);}
        }
        if (type==Pluck::random) std::shuffle(indices.begin(),indices.end(),random);
        for(size_t rank=0;rank<indices.size();++rank)
        {
            auto& n=notes[indices[rank]];double amount=double(rank)/double(indices.size()-1);
            if(type==Pluck::curvedUp || type==Pluck::curvedDown) amount*=amount;
            n.startTick=rounded(start+amount*128*std::clamp(position,0.0,1.0),0,maximumTick-1);
            n.lengthTicks=std::min(n.lengthTicks,maximumTick-n.startTick);
            n.velocity=rounded(n.velocity+(110-45*amount-n.velocity)*std::clamp(velocity,0.0,1.0),1,127);
        }
    }
}
}
