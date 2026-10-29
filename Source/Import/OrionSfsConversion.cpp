#include "OrionSfsConversion.h"
#include "WaveMediaInfo.h"
#include <filesystem>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

namespace orbit::import::orion
{
bool prepareConversion(const ImportDocument& source, const ConversionOptions& options,
                       ProjectImportPlan& result, std::string& error)
{
    error.clear();
    const auto fail = [&](const std::string& message) { error = message; return false; };
    if (! options.acceptPatternOnlyRecovery)
        return fail("Conversion recovers an experimental subset. Explicitly accept partial recovery and omission of unsupported audio, automation, routing and instrument state.");
    if (source.metadata.sfsVersion != 0x8000 && source.metadata.sfsVersion != 0x8612)
        return fail("No conversion mapping is established for this SFS version.");
    if (! options.sourceTicksPerQuarterNote || *options.sourceTicksPerQuarterNote <= 0)
        return fail("Supply the source ticks per quarter note explicitly; it has not been decoded from SFS.");
    if (! options.tempoBpm || ! std::isfinite(*options.tempoBpm)
        || *options.tempoBpm < 20.0 || *options.tempoBpm > 300.0)
        return fail("Supply a finite tempo within Orbit's supported range (20 to 300 BPM).");
    if (! options.meterNumerator || *options.meterNumerator < 1 || *options.meterNumerator > 32
        || ! options.meterDenominator
        || (*options.meterDenominator != 1 && *options.meterDenominator != 2
            && *options.meterDenominator != 4 && *options.meterDenominator != 8
            && *options.meterDenominator != 16 && *options.meterDenominator != 32))
        return fail("Supply an explicit meter supported by Orbit (numerator 1..32; denominator 1, 2, 4, 8, 16 or 32).");
    std::set<std::pair<std::uint32_t, std::uint32_t>> selected(options.selectedPatterns.begin(), options.selectedPatterns.end());
    if (selected.size() != options.selectedPatterns.size())
        return fail("A pattern was selected more than once.");
    const bool hasExplicitSelection = !selected.empty() || !options.selectedAudioTracks.empty();
    std::set<std::uint32_t> selectedAudio(options.selectedAudioTracks.begin(),options.selectedAudioTracks.end());
    if (selectedAudio.size()!=options.selectedAudioTracks.size()) return fail("An audio track was selected more than once.");
    if (!selectedAudio.empty() && !options.recoverTentativeDryAudio) return fail("Explicitly accept tentative dry-audio recovery for selected audio tracks.");
    const bool wantsAnyPatterns = !hasExplicitSelection || !selected.empty();
    for (const auto& diagnostic : source.diagnostics)
        if (diagnostic.severity == DiagnosticSeverity::error
            || diagnostic.code == "module-count-mismatch"
            || diagnostic.code == "object-count-not-found")
            return fail("Source extraction is incomplete: " + diagnostic.code + ". Inspect before conversion.");

    if (options.recoverTentativePlaylist && wantsAnyPatterns)
    {
        if (*options.meterNumerator != 4 || *options.meterDenominator != 4)
            return fail("Playlist conversion currently requires explicit 4/4 meter; Orbit's Playlist uses sixteen steps per bar.");
        for (const auto& diagnostic : source.diagnostics)
            if (diagnostic.code == "playlist-mapping-incomplete" || diagnostic.code == "playlist-record-framing-unmapped")
                return fail("Playlist extraction contains unmapped records; arrangement conversion was refused.");
    }
    ProjectImportPlan plan;
    plan.title = source.metadata.title.empty() ? "Recovered SFS patterns" : source.metadata.title;
    plan.tempoBpm = *options.tempoBpm;
    plan.meterNumerator = *options.meterNumerator;
    plan.meterDenominator = *options.meterDenominator;
    plan.caveats.push_back("Source PPQ override: " + std::to_string(*options.sourceTicksPerQuarterNote)
        + "; tempo and meter are explicit import settings. Any detected source-tempo candidate remains tentative.");
    plan.caveats.push_back("Original source (" + std::to_string(source.metadata.sourceFileSize) + " bytes): " + source.metadata.sourcePath);
    plan.caveats.push_back(options.recoverTentativePlaylist
        ? "Experimental playlist recovery explicitly accepted: 0x3007 module/pattern changes end at the source pattern end or the next change, including changes to unselected patterns. Only selected pattern placements are included."
        : "Pattern recovery only: playlist is omitted. No arrangement is invented.");
    plan.caveats.push_back("Tempo/meter changes, automation, per-note channel mapping, mixer routing, instrument and effect state remain omitted.");
    plan.caveats.push_back(options.recoverTentativeDryAudio
        ? "Dry-audio recovery explicitly accepted: pool references and timing are tentative; only untrimmed full-file ranges at natural duration are supported; clip gain, mute, reverse, fades, pitch, stretching and other raw clip controls are NOT restored. Media remains at its relinked path."
        : "Audio clips/samples are omitted.");
    plan.caveats.push_back("Recovered tracks contain notes without an instrument; assign instruments manually. Drum events are recovered as ordinary pitched notes without pad/sample mapping.");
    plan.caveats.push_back("Keep the original SFS and inspection report. The complete raw source is retained in inspection; native recovery notes also retain any event records explicitly omitted from playback.");
    std::size_t roundedValues = 0;
    std::size_t coincidentNotes = 0;
    const auto scale = [&](std::int64_t value, std::int64_t& converted)
    {
        if (value < 0 || value > std::numeric_limits<std::int64_t>::max() / ProjectImportPlan::patternTicksPerQuarterNote)
            return false;
        const auto numerator = value * ProjectImportPlan::patternTicksPerQuarterNote;
        const auto divisor = *options.sourceTicksPerQuarterNote;
        converted = numerator / divisor;
        const auto remainder = numerator % divisor;
        if (remainder != 0)
        {
            if (! options.allowTickRounding)
                return false;
            // Nearest tick, halfway up; avoids numerator + divisor overflow.
            if (remainder >= divisor / 2 + divisor % 2)
                ++converted;
            ++roundedValues;
        }
        return true;
    };
    const auto retainUnsupported = [&](const TrackRecord& track, const PatternRecord& pattern,
                                        const PatternEvent& event, const std::string& reason)
    {
        if (!options.omitUnsupportedEvents)
            return fail("Unsupported event in " + track.name + ", pattern " + std::to_string(pattern.sourcePatternIndex)
                + ": " + reason + ". Explicit omission is required; the default does not drop it.");
        if (!event.opaqueSource || event.opaqueSource->byteLength != 16)
            return fail("Unsupported event bytes cannot be preserved exactly; recovery was refused.");
        const auto& span=*event.opaqueSource;
        const auto region=std::find_if(source.opaqueRecords.begin(),source.opaqueRecords.end(),[&](const OpaqueSourceRecord& record)
        {
            return span.offset>=record.source.offset && span.offset-record.source.offset<=record.bytes.size()
                && span.byteLength<=record.bytes.size()-(span.offset-record.source.offset);
        });
        if (region==source.opaqueRecords.end())
            return fail("Unsupported event bytes are outside retained source storage; recovery was refused.");
        UnconvertedSourceRecord record;
        std::ostringstream format; format << "Orion SFS 0x" << std::hex << source.metadata.sfsVersion;
        record.sourceFormat=format.str();
        record.context="track "+std::to_string(track.sourceTrackIndex)+" "+track.name+" / pattern "+std::to_string(pattern.sourcePatternIndex)+" / event "+std::to_string(event.sourceEventIndex.value_or(0));
        record.reason=reason;
        record.sourceOffset=span.offset;
        const auto start=region->bytes.begin()+static_cast<std::ptrdiff_t>(span.offset-region->source.offset);
        record.bytes.assign(start,start+16);
        plan.unconvertedRecords.push_back(std::move(record));
        return true;
    };
    auto unmatchedAudio=selectedAudio;
    auto unmatched = selected;
    std::size_t unselectedPatterns = 0;
    std::size_t noteCount = 0;
    std::size_t omittedAudioTracks = 0;
    for (const auto& track : source.tracks)
    {
        if (track.kind == TrackKind::audio)
        {
            if (!options.recoverTentativeDryAudio || (hasExplicitSelection && selectedAudio.count(track.sourceTrackIndex)==0))
            { ++omittedAudioTracks; continue; }
            unmatchedAudio.erase(track.sourceTrackIndex);
            if (track.audioClips.empty()) return fail("A selected audio track has no supported audio-event mapping.");
            PlannedTrack audioTrack; audioTrack.name=track.name; audioTrack.kind=PlannedTrack::Kind::audio;
            for (const auto& clip:track.audioClips)
            {
                if (clip.audioSourceIndex>=source.audioSources.size()) return fail("Audio source reference is not mapped.");
                const auto& media=source.audioSources[clip.audioSourceIndex];
                const auto relink=options.mediaRelinks.find(clip.audioSourceIndex);
                if (relink==options.mediaRelinks.end()) return fail("Relink source pool " + std::to_string(clip.audioSourceIndex) + " (" + media.displayName + ") before audio recovery.");
                WaveMediaInfo actual;
                std::string mediaError;
                const auto local=std::filesystem::absolute(std::filesystem::path(relink->second)).lexically_normal();
                if (!inspectWaveMedia(local,actual,mediaError)) return fail("Relinked media: " + mediaError);
                if (media.sampleRate!=actual.sampleRate || media.channelCount!=actual.channels
                    || media.sourceFrameCount!=actual.frames || media.bitsPerSample!=actual.bitsPerSample || media.waveFormatTag!=actual.formatTag)
                    return fail("Relinked WAV metadata differs from the source pool; audio conversion was refused.");
                if (!clip.lengthTicks || !clip.sourceOffsetFrames || !clip.sourceEndFrames || !media.sampleRate)
                    return fail("Audio range is incomplete.");
                PlannedAudioClip converted;
                converted.name=media.displayName; converted.mediaPath=local.string();
                converted.startSeconds=clip.startTicks / static_cast<double>(*options.sourceTicksPerQuarterNote) * 60.0 / *options.tempoBpm;
                converted.lengthSeconds=*clip.lengthTicks / static_cast<double>(*options.sourceTicksPerQuarterNote) * 60.0 / *options.tempoBpm;
                converted.sourceOffsetSeconds=static_cast<double>(*clip.sourceOffsetFrames) / *media.sampleRate;
                converted.sampleRate=actual.sampleRate; converted.channels=actual.channels; converted.sourceFrames=actual.frames;
                converted.waveFormatTag=actual.formatTag; converted.bitsPerSample=actual.bitsPerSample;
                const auto naturalDuration=static_cast<double>(*clip.sourceEndFrames-*clip.sourceOffsetFrames) / actual.sampleRate;
                if (!std::isfinite(converted.startSeconds) || !std::isfinite(converted.lengthSeconds)
                    || std::abs(converted.lengthSeconds-naturalDuration)>0.5/actual.sampleRate)
                    return fail("Audio duration does not match the source-frame range at the supplied timing; stretching/scaling is not guessed.");
                audioTrack.audioClips.push_back(std::move(converted));
                plan.caveats.push_back("Relinked pool " + std::to_string(clip.audioSourceIndex) + " to " + local.string()
                    + "; WAV format/rate/channels/frame count match, content identity is not established.");
            }
            plan.tracks.push_back(std::move(audioTrack));
            continue;
        }
        const bool wantsTrackPatterns = !hasExplicitSelection
            || std::any_of(selected.begin(), selected.end(), [&](const auto& key) { return key.first == track.sourceTrackIndex; });
        if (!track.patternLayoutRecognized)
        {
            if (wantsTrackPatterns)
                return fail("Selected track " + std::to_string(track.sourceTrackIndex) + " (" + track.name
                    + ") has an incomplete pattern layout; no partial pattern prefix was recovered.");
            plan.caveats.push_back("Unselected track " + std::to_string(track.sourceTrackIndex) + " (" + track.name
                + ") has an incomplete pattern layout. Its undecoded content and any partial pattern prefix are omitted; raw source remains in inspection.");
            continue;
        }
        if (options.recoverTentativePlaylist && wantsTrackPatterns && !track.playlistMappingComplete)
            return fail("Selected track " + std::to_string(track.sourceTrackIndex) + " (" + track.name
                + ") has unmapped playlist changes; its arrangement was refused.");
        if (!wantsTrackPatterns && !track.playlistMappingComplete)
            plan.caveats.push_back("Unselected track " + std::to_string(track.sourceTrackIndex) + " (" + track.name
                + ") has unmapped playlist changes; its arrangement is omitted.");
        PlannedTrack convertedTrack;
        convertedTrack.name = track.name;
        std::set<std::uint32_t> slots;
        for (const auto& pattern : track.patterns)
        {
            if (! slots.insert(pattern.sourcePatternIndex).second)
                return fail("Duplicate source pattern index.");
            const auto key = std::make_pair(track.sourceTrackIndex, pattern.sourcePatternIndex);
            if (hasExplicitSelection && selected.count(key) == 0)
            {
                if (! pattern.events.empty() || pattern.opaqueEventData) ++unselectedPatterns;
                continue;
            }
            unmatched.erase(key);
            if (pattern.opaqueEventData)
                return fail("A selected pattern contains undecoded event data.");
            const auto referenced = std::any_of(track.playlist.begin(), track.playlist.end(), [&](const PlaylistPlacement& placement)
                { return placement.patternIndex == pattern.sourcePatternIndex; });
            // Referenced empty patterns are real arrangement content (e.g. A3
            // in Song1), not an excuse to invent notes or discard a change.
            if (pattern.events.empty() && !(options.recoverTentativePlaylist && referenced))
            {
                if (! selected.empty()) return fail("A selected pattern has no decoded note events or recoverable placement.");
                continue;
            }
            if (!pattern.events.empty() && ! options.acceptTentativeNotes)
                return fail("Note field mapping is tentative. Explicitly accept it for experimental recovery.");
            PlannedPattern convertedPattern;
            convertedPattern.slot = pattern.sourcePatternIndex;
            convertedPattern.name = pattern.name;
            if (! scale(pattern.lengthTicks, convertedPattern.lengthTicks) || convertedPattern.lengthTicks <= 0)
                return fail("Pattern length cannot be represented exactly; review timing or explicitly allow tick rounding.");
            std::set<std::pair<int, std::int64_t>> noteKeys;
            for (const auto& event : pattern.events)
            {
                if (event.kind != PatternEvent::Kind::note || event.sourceEventTypeCode != 0x3002)
                {
                    if (!retainUnsupported(track,pattern,event,"event code/semantics are not mapped")) return false;
                    continue;
                }
                if (event.durationTicks <= 0 || event.positionTicks < 0
                    || event.positionTicks > pattern.lengthTicks
                    || event.durationTicks > pattern.lengthTicks - event.positionTicks)
                    return fail("A note lies outside its source pattern or has no positive duration.");
                if (event.data1 < 0 || event.data1 > 127 || event.data2 < 1 || event.data2 > 127)
                {
                    const auto reason=event.data2==0 ? "tentative note velocity zero cannot be represented without changing playback semantics"
                        : "tentative pitch/velocity fields are outside supported MIDI ranges";
                    if (!retainUnsupported(track,pattern,event,reason)) return false;
                    continue;
                }
                PlannedNote note;
                note.pitch = event.data1;
                note.velocity = event.data2;
                std::int64_t endTick = 0;
                if (! scale(event.positionTicks, note.startTick)
                    || ! scale(event.positionTicks + event.durationTicks, endTick)
                    || endTick <= note.startTick)
                    return fail("Note boundaries cannot be represented; review timing or explicitly allow tick rounding.");
                note.lengthTicks = endTick - note.startTick;
                if (! noteKeys.emplace(note.pitch, note.startTick).second) ++coincidentNotes;
                convertedPattern.notes.push_back(note);
                ++noteCount;
            }
            convertedTrack.patterns.push_back(std::move(convertedPattern));
        }
        if (options.recoverTentativePlaylist)
        {
            for (const auto& placement : track.playlist)
            {
                const auto convertedPattern = std::find_if(convertedTrack.patterns.begin(), convertedTrack.patterns.end(),
                    [&](const PlannedPattern& pattern) { return placement.patternIndex == pattern.slot; });
                if (convertedPattern == convertedTrack.patterns.end()) continue;
                PlannedPlacement converted;
                converted.patternSlot = convertedPattern->slot;
                if (!placement.enabled || !scale(placement.startTicks, converted.startTick)
                    || !scale(placement.lengthTicks, converted.lengthTicks) || converted.lengthTicks <= 0)
                    return fail("A playlist range cannot be represented; its source change remains in inspection.");
                convertedTrack.playlist.push_back(converted);
            }
        }
        if (! convertedTrack.patterns.empty())
            plan.tracks.push_back(std::move(convertedTrack));
    }
    if (!unmatchedAudio.empty()) return fail("A selected audio track does not exist or is not an audio track.");
    if (! unmatched.empty())
        return fail("A selected source track/pattern does not exist or is an unsupported audio track.");
    if (unselectedPatterns != 0)
        plan.caveats.push_back(std::to_string(unselectedPatterns) + " populated patterns omitted by explicit source track/pattern selection.");
    if (noteCount == 0 && std::none_of(plan.tracks.begin(), plan.tracks.end(), [](const PlannedTrack& track) { return !track.playlist.empty() || !track.audioClips.empty(); }))
        return fail("No supported note patterns or playlist ranges were found.");
    if (!plan.unconvertedRecords.empty())
        plan.caveats.push_back(std::to_string(plan.unconvertedRecords.size())
            + " unsupported event records were explicitly omitted from playback. Their exact bytes and locations are preserved below and in native recovery notes; their musical effect is not reconstructed.");
    if (noteCount != 0)
        plan.caveats.push_back("0x3002 duration/pitch/velocity interpretation is tentative and was explicitly accepted.");
    if (coincidentNotes != 0)
        plan.caveats.push_back(std::to_string(coincidentNotes)
            + " additional coincident notes are retained separately. Moving, resizing, changing velocity or erasing at their shared start edits the note stack together.");
    plan.caveats.push_back(std::to_string(omittedAudioTracks) + " audio tracks omitted; "
                           + std::to_string(source.mediaReferenceCandidates.size()) + " byte-scan media candidates retained for review (separate from structured relink status).");
    if (roundedValues != 0)
        plan.caveats.push_back(std::to_string(roundedValues) + " timing values rounded to the nearest Orbit pattern tick by explicit policy.");
    result = std::move(plan);
    return true;
}

std::string createConversionReport(const ProjectImportPlan& plan)
{
    std::ostringstream out;
    out << "Experimental SFS recovery preview\nTitle: " << std::quoted(plan.title)
        << "\nTempo override: " << plan.tempoBpm << " BPM\nMeter override: "
        << plan.meterNumerator << '/' << plan.meterDenominator
        << "\nDestination pattern ticks per quarter: " << ProjectImportPlan::patternTicksPerQuarterNote
        << "\nTracks to recover: " << plan.tracks.size() << '\n';
    for (const auto& track : plan.tracks)
    {
        out << "  " << std::quoted(track.name) << '\n';
        for (const auto& pattern : track.patterns)
            out << "    slot=" << pattern.slot << " length-ticks=" << pattern.lengthTicks
                << " notes=" << pattern.notes.size() << '\n';
        for (const auto& clip : track.audioClips)
            out << "    DRY-audio path=" << std::quoted(clip.mediaPath) << std::setprecision(17)
                << " start-seconds=" << clip.startSeconds << " length-seconds=" << clip.lengthSeconds
                << " offset-seconds=" << clip.sourceOffsetSeconds
                << " wave-format=" << clip.waveFormatTag << " bits=" << clip.bitsPerSample
                << " rate=" << clip.sampleRate << " channels=" << clip.channels << " frames=" << clip.sourceFrames << '\n';
        for (const auto& placement : track.playlist)
            out << "    placement slot=" << placement.patternSlot << " start-ticks=" << placement.startTick
                << " length-ticks=" << placement.lengthTicks << '\n';
    }
    out << "Unconverted event records: " << plan.unconvertedRecords.size() << '\n';
    for (const auto& record : plan.unconvertedRecords) out << "  " << describeUnconvertedRecord(record) << '\n';
    out << "Caveats:\n";
    for (const auto& caveat : plan.caveats)
        out << "  - " << caveat << '\n';
    return out.str();
}
}
