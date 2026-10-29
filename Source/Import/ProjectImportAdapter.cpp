#include "ProjectImportAdapter.h"

#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace orbit::import
{
namespace
{
class ImportAction final : public juce::UndoableAction
{
public:
    ImportAction(ProjectDocument& document, ProjectDocument candidate)
        : destination(document), before(document), after(std::move(candidate)) {}
    bool perform() override { destination = after; return true; }
    bool undo() override { destination = before; return true; }
    int getSizeInUnits() override { return 1; }
private:
    ProjectDocument& destination;
    ProjectDocument before, after;
};
}

bool prepareProjectImport(const ProjectDocument& base, const ProjectImportPlan& plan,
                          ProjectDocument& result, std::string& error)
{
    error.clear();
    const auto fail = [&](const std::string& message) { error = message; return false; };
    if (plan.tracks.empty())
        return fail("The import plan has no tracks.");
    if (! std::isfinite(plan.tempoBpm) || plan.tempoBpm < 20.0 || plan.tempoBpm > 300.0
        || plan.meterNumerator < 1 || plan.meterNumerator > 32
        || (plan.meterDenominator != 1 && plan.meterDenominator != 2 && plan.meterDenominator != 4
            && plan.meterDenominator != 8 && plan.meterDenominator != 16 && plan.meterDenominator != 32))
        return fail("The import plan has unsupported tempo or meter values.");
    const auto meter = base.getInitialTimeSignature();
    if (base.getTrackCount() != 0
        && (std::abs(base.getInitialTempoBpm() - plan.tempoBpm) > 1.0e-9
            || meter.numerator != plan.meterNumerator || meter.denominator != plan.meterDenominator))
        return fail("Merge refused: tempo or meter differs from the existing project; tempo-map merging is not supported.");
    constexpr auto maximumTicks = static_cast<std::int64_t>(ProjectDocument::maximumPatternLengthSteps) * 64;
    for (const auto& track : plan.tracks)
    {
        if (track.kind==PlannedTrack::Kind::audio)
        {
            if (track.audioClips.empty() || !track.patterns.empty() || !track.playlist.empty())
                return fail("An audio-track plan must contain audio clips without instrument patterns.");
            for (const auto& clip:track.audioClips)
            {
                const bool pcm=clip.waveFormatTag==1 && (clip.bitsPerSample==8 || clip.bitsPerSample==16
                    || clip.bitsPerSample==24 || clip.bitsPerSample==32);
                const bool ieee=clip.waveFormatTag==3 && (clip.bitsPerSample==32 || clip.bitsPerSample==64);
                if (clip.mediaPath.empty() || !clip.sampleRate || !clip.channels || !clip.sourceFrames
                    || (!pcm && !ieee)
                    || !std::isfinite(clip.startSeconds) || clip.startSeconds<0 || !std::isfinite(clip.lengthSeconds) || clip.lengthSeconds<=0
                    || !std::isfinite(clip.sourceOffsetSeconds) || clip.sourceOffsetSeconds<0
                    || !std::isfinite(clip.startSeconds+clip.lengthSeconds) || clip.startSeconds+clip.lengthSeconds<=clip.startSeconds
                    || clip.sourceOffsetSeconds+clip.lengthSeconds > static_cast<double>(clip.sourceFrames)/clip.sampleRate + 0.5/clip.sampleRate)
                    return fail("An audio clip has invalid media metadata or timing; import was refused.");
            }
        }
        else if (track.patterns.empty() || !track.audioClips.empty())
            return fail("An instrument plan must contain patterns without audio clips.");
        std::set<std::uint32_t> slots;
        for (const auto& pattern : track.patterns)
        {
            if (pattern.slot >= static_cast<std::uint32_t>(ProjectDocument::patternSlotCount)
                || ! slots.insert(pattern.slot).second)
                return fail("A pattern slot is duplicated or outside Orbit's pattern bank.");
            // The native schema stores whole sixteenth-note pattern lengths.
            // Refuse lossy length rounding; never clamp long imported patterns.
            if (pattern.lengthTicks <= 0 || pattern.lengthTicks > maximumTicks
                || pattern.lengthTicks % 64 != 0)
                return fail("A pattern length is outside Orbit's storage range or cannot be stored exactly in sixteenth-note steps.");
            for (const auto& note : pattern.notes)
            {
                if (note.pitch < 0 || note.pitch > 127 || note.velocity < 1 || note.velocity > 127
                    || note.startTick < 0 || note.lengthTicks <= 0
                    || note.startTick >= pattern.lengthTicks
                    || note.lengthTicks > pattern.lengthTicks - note.startTick)
                    return fail("A note is invalid or outside its pattern; import refused without clamping.");
            }
        }
        if (!track.playlist.empty() && (plan.meterNumerator != 4 || plan.meterDenominator != 4))
            return fail("Exact bar-range import requires 4/4 meter in the current native Playlist.");
        std::int64_t previousEnd = -1;
        for (const auto& placement : track.playlist)
        {
            constexpr std::int64_t ticksPerBar = ProjectImportPlan::patternTicksPerQuarterNote * 4;
            if (slots.count(placement.patternSlot) == 0 || placement.startTick < 0 || placement.lengthTicks <= 0
                || placement.startTick % ticksPerBar != 0 || placement.lengthTicks % ticksPerBar != 0
                || placement.startTick / ticksPerBar > std::numeric_limits<int>::max()
                || placement.lengthTicks / ticksPerBar > std::numeric_limits<int>::max()
                || placement.startTick / ticksPerBar + placement.lengthTicks / ticksPerBar > std::numeric_limits<int>::max()
                || placement.startTick < previousEnd)
                return fail("Playlist range is overlapping, unsorted, unreferenced, outside native storage or not aligned to whole bars; import refused without rounding.");
            previousEnd = placement.startTick + placement.lengthTicks;
        }
    }

    auto candidate = base;
    if (base.getTrackCount() == 0)
    {
        candidate.setInitialTempoBpm(plan.tempoBpm);
        candidate.setInitialTimeSignature(plan.meterNumerator, plan.meterDenominator);
        if (! plan.title.empty())
            candidate.setName(juce::String::fromUTF8(plan.title.c_str()));
    }
    for (const auto& track : plan.tracks)
    {
        const auto trackId = candidate.addTrack(track.kind==PlannedTrack::Kind::audio ? ProjectDocument::TrackKind::audio : ProjectDocument::TrackKind::instrument,
                                                juce::String::fromUTF8(track.name.c_str()));
        for (const auto& pattern : track.patterns)
        {
            const auto slot = static_cast<int>(pattern.slot);
            if (!pattern.name.empty()) candidate.setPatternName(trackId, slot, juce::String::fromUTF8(pattern.name.c_str()));
            for (const auto& note : pattern.notes)
                if (!candidate.appendPatternNoteTicks(trackId, slot, note.pitch, static_cast<int>(note.startTick),
                                               static_cast<int>(note.lengthTicks), note.velocity))
                    return fail("Could not stage every native note without clamping or merging.");
            candidate.setPatternLengthSteps(trackId, slot, static_cast<int>(pattern.lengthTicks / 64));
            candidate.setPatternCreated(trackId, slot, true);
        }
        for (const auto& placement : track.playlist)
        {
            constexpr std::int64_t ticksPerBar = ProjectImportPlan::patternTicksPerQuarterNote * 4;
            if (!candidate.setPlaylistClipExactBars(trackId, candidate.getPatternId(trackId, static_cast<int>(placement.patternSlot)),
                static_cast<int>(placement.startTick / ticksPerBar), static_cast<int>(placement.lengthTicks / ticksPerBar)))
                return fail("Could not stage an exact native playlist range; the live project was not changed.");
        }
        if (!track.patterns.empty()) candidate.setTrackSelectedPatternIndex(trackId, static_cast<int>(track.patterns.front().slot));
    }
    // Store recovery caveats in the native project's visible description.
    auto description = candidate.getDescription();
    if (description.isNotEmpty()) description += "\n\n";
    description += "Import recovery notes:\n";
    for (const auto& caveat : plan.caveats)
        description += juce::String::fromUTF8(caveat.c_str()) + "\n";
    if (!plan.unconvertedRecords.empty())
    {
        description += "Unconverted source records (omitted from playback):\n";
        for (const auto& record : plan.unconvertedRecords)
            description += juce::String::fromUTF8(describeUnconvertedRecord(record).c_str()) + "\n";
    }
    candidate.setDescription(description);
    result = std::move(candidate);
    return true;
}

bool applyProjectImport(ProjectDocument& destination, const ProjectImportPlan& plan,
                        juce::UndoManager& undoManager, std::string& error)
{
    ProjectDocument candidate;
    if (! prepareProjectImport(destination, plan, candidate, error))
        return false;
    undoManager.beginNewTransaction("Import recovered patterns");
    return undoManager.perform(new ImportAction(destination, std::move(candidate)));
}
}
