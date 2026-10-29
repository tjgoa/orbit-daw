#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

// Format-neutral, engine-independent conversion boundary. Only the adapter
// knows how these records become a ProjectDocument. No SFS types escape here.
namespace orbit::import
{
struct PlannedNote
{
    int pitch = 0;
    int velocity = 0;
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0;
};

struct PlannedPattern
{
    std::uint32_t slot = 0;
    std::int64_t lengthTicks = 0;
    std::vector<PlannedNote> notes;
    std::string name;
};

struct PlannedPlacement
{
    std::uint32_t patternSlot = 0;
    std::int64_t startTick = 0;
    std::int64_t lengthTicks = 0;
};

struct PlannedAudioClip
{
    std::string name;
    std::string mediaPath;
    double startSeconds = 0.0;
    double lengthSeconds = 0.0;
    double sourceOffsetSeconds = 0.0;
    std::uint32_t sampleRate = 0;
    std::uint16_t channels = 0;
    // Validated WAVE encoding identity, retained through preview to commit.
    std::uint16_t waveFormatTag = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint64_t sourceFrames = 0;
};

struct PlannedTrack
{
    std::string name;
    std::vector<PlannedPattern> patterns;
    std::vector<PlannedPlacement> playlist;
    enum class Kind { instrument, audio };
    Kind kind = Kind::instrument;
    std::vector<PlannedAudioClip> audioClips;
};

struct UnconvertedSourceRecord
{
    std::string sourceFormat;
    std::string context;
    std::string reason;
    std::optional<std::uint64_t> sourceOffset;
    std::vector<std::uint8_t> bytes;
};

// Deterministic text form suitable for a report or native recovery notes.
std::string describeUnconvertedRecord(const UnconvertedSourceRecord&);

struct ProjectImportPlan
{
    // Orbit's pattern storage has 64 ticks/sixteenth, distinct from its
    // 960-PPQ project timeline. Never use the latter for pattern notes.
    static constexpr std::int64_t patternTicksPerQuarterNote = 256;
    std::string title;
    double tempoBpm = 0.0;
    int meterNumerator = 0;
    int meterDenominator = 0;
    std::vector<PlannedTrack> tracks;
    // A reviewable record of assumptions/omissions, retained with the plan.
    std::vector<std::string> caveats;
    std::vector<UnconvertedSourceRecord> unconvertedRecords;
};
}
