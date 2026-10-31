#include "OrionSfsReport.h"

#include <iomanip>
#include <sstream>

namespace orbit::import::orion
{
std::string createInspectionReport(const ImportDocument& document, const SfsInspection& inspection)
{
    std::ostringstream out;
    out << "Orbit SFS inspection report v1\nSource: " << std::quoted(document.metadata.sourcePath)
        << "\nVersion: 0x" << std::hex << document.metadata.sfsVersion << std::dec
        << "\nBytes: " << document.metadata.sourceFileSize
        << "\nStatus: extraction with tentative note/playlist mappings; conversion requires explicit policies and timing\n"
        << "Meter: unmapped\nTicks per quarter: unmapped\n";
    if (document.metadata.initialTempoBpm)
        out << "Tempo candidate (tentative): " << *document.metadata.initialTempoBpm << " BPM"
            << " raw32=0x" << std::hex << document.metadata.sourceTempoRawBits.value_or(0) << std::dec
            << " @" << (document.metadata.sourceTempoSpan ? document.metadata.sourceTempoSpan->offset : 0) << '\n';
    else out << "Tempo: unmapped\n";
    out << "\nGlobal records (offset, declared payload bytes, consumed payload bytes):\n";
    for (const auto& chunk : inspection.globalPreludeChunks)
        out << "  " << OrionSfsReader::displayChunkId(chunk.id) << " @" << chunk.headerOffset
            << " declared=" << chunk.payloadSize << " consumed=" << chunk.consumedPayloadSize << '\n';
    std::uint64_t retained = 0;
    for (const auto& region : document.opaqueRecords)
        retained += region.bytes.size();
    out << "\nRetained raw source bytes: " << retained << '/' << document.metadata.sourceFileSize << '\n';
    for (const auto& region : document.opaqueRecords)
        out << "  " << region.purpose << " @" << region.source.offset << " bytes=" << region.source.byteLength << '\n';
    out << "\nValidated trailing envelopes: " << document.tailRecords.size() << '\n';
    for (const auto& record : document.tailRecords)
        out << "  @" << record.source.offset << " bytes=" << record.source.byteLength
            << " declared-size-or-count=" << record.declaredSizeOrCount << " " << record.description << '\n';
    out << "\nTentative audio pool sources: " << document.audioSources.size() << '\n';
    for (const auto& source : document.audioSources)
        out << "  pool=" << source.sourcePoolIndex.value_or(0) << " name=" << std::quoted(source.displayName)
            << " path=" << std::quoted(source.sourceReference) << " rate=" << source.sampleRate.value_or(0)
            << " channels=" << source.channelCount.value_or(0) << " bits=" << source.bitsPerSample.value_or(0)
            << " frames=" << source.sourceFrameCount.value_or(0)
            << " @" << (source.sourceRecord ? source.sourceRecord->offset : 0) << " (external; mapping tentative)\n";
    out << "\nModules: " << document.modules.size() << '\n';
    for (const auto& module : document.modules)
    {
        out << "  object=" << module.sourceObjectIndex << " name=" << std::quoted(module.displayName);
        if (module.sourceTypeCode)
            out << " type=0x" << std::hex << *module.sourceTypeCode << std::dec;
        if (module.opaqueState)
            out << " @" << module.opaqueState->offset << " bytes=" << module.opaqueState->byteLength;
        out << " (instrument/effect state not converted)\n";
    }
    std::size_t totalEvents = 0;
    out << "\nTracks: " << document.tracks.size() << '\n';
    for (const auto& track : document.tracks)
    {
        out << "  track=" << track.sourceTrackIndex << " name=" << std::quoted(track.name)
            << " patterns=" << track.patterns.size() << " pattern-layout=" << (track.patternLayoutRecognized ? "recognized" : "incomplete")
            << " tentative-placements=" << track.playlist.size()
            << " track-playlist=" << (track.playlistMappingComplete ? "mapped" : "incomplete") << "\n";
        if (track.opaqueGeneratorPrefix)
            out << "    opaque-pattern-prefix @" << track.opaqueGeneratorPrefix->offset
                << " bytes=" << track.opaqueGeneratorPrefix->byteLength << '\n';
        for (const auto& clip : track.audioClips)
        {
            out << "    TENTATIVE-audio-clip index=" << clip.sourceClipIndex << " pool=" << clip.audioSourceIndex
                << " start-source-ticks=" << std::setprecision(17) << clip.startTicks << " length-source-ticks=" << clip.lengthTicks.value_or(0)
                << " source-frame-range=" << clip.sourceOffsetFrames.value_or(0) << ":" << clip.sourceEndFrames.value_or(0)
                << " @" << (clip.sourceRecord ? clip.sourceRecord->offset : 0) << " raw-words=" << std::hex;
            for (const auto word : clip.rawSourceWords) out << word << ',';
            out << std::dec << '\n';
        }
        for (const auto& placement : track.playlist)
            out << "    TENTATIVE-placement pattern=" << placement.patternIndex.value_or(0)
                << " start-source-ticks=" << placement.startTicks << " length-source-ticks=" << placement.lengthTicks
                << " @" << (placement.sourceRecord ? placement.sourceRecord->offset : 0) << '\n';
        for (const auto& pattern : track.patterns)
        {
            out << "    pattern=" << pattern.sourcePatternIndex << " name=" << std::quoted(pattern.name)
                << " length-source-ticks=" << pattern.lengthTicks << " events=" << pattern.events.size();
            if (pattern.sourceRecord)
                out << " @" << pattern.sourceRecord->offset << " bytes=" << pattern.sourceRecord->byteLength;
            if (pattern.opaqueEventData)
                out << " undecoded-event-bytes=" << pattern.opaqueEventData->byteLength
                    << " @" << pattern.opaqueEventData->offset;
            out << '\n';
            totalEvents += pattern.events.size();
            for (const auto& event : pattern.events)
            {
                out << "      event=" << event.sourceEventIndex.value_or(0)
                    << " @" << (event.opaqueSource ? event.opaqueSource->offset : 0)
                    << " code=0x" << std::hex << event.sourceEventTypeCode.value_or(0)
                    << " raw0=0x" << event.sourceRawField0.value_or(0)
                    << " raw1=0x" << event.sourceRawData1.value_or(0)
                    << " raw2=0x" << event.sourceRawData2.value_or(0) << std::dec
                    << " position-source-ticks=" << event.positionTicks;
                if (event.kind == PatternEvent::Kind::note)
                    out << " TENTATIVE-note pitch=" << event.data1 << " velocity=" << event.data2
                        << " duration-source-ticks=" << event.durationTicks;
                else
                    out << " unsupported-event";
                out << '\n';
            }
        }
    }
    out << "\nRaw playlist change records: " << document.playlistChangeRecords.size() << '\n';
    for (const auto& record : document.playlistChangeRecords)
        out << "  index=" << record.sourceIndex << " @" << record.source.offset
            << " field0=" << record.rawField0 << " code=0x" << std::hex << record.eventCode << std::dec
            << " raw-position=" << record.rawPosition << " raw-data1=" << record.rawData1
            << " raw-data2=" << record.rawData2 << " (semantics unverified)\n";
    out << "\nUnverified media path candidates: " << document.mediaReferenceCandidates.size() << '\n';
    for (const auto& candidate : document.mediaReferenceCandidates)
        out << "  @" << candidate.source.offset << " bytes=" << candidate.source.byteLength
            << " reference=" << std::quoted(candidate.reference)
            << " (clip/pad assignment, existence and bundled status unresolved)\n";
    out << "\nDecoded event records: " << totalEvents
        << "\nAudio pool and event timing have tentative mappings for observed 0x8612 records; drum sample/pad mapping remains unsupported"
        << "\nPlaylist interpretation is tentative; automation, routing and proprietary DSP are not converted\n\nDiagnostics:\n";
    for (const auto& diagnostic : document.diagnostics)
    {
        const auto severity = diagnostic.severity == DiagnosticSeverity::error ? "error"
            : diagnostic.severity == DiagnosticSeverity::warning ? "warning" : "info";
        out << "  [" << severity << "] " << diagnostic.code;
        if (diagnostic.sourceOffset)
            out << " @" << *diagnostic.sourceOffset;
        out << ": " << diagnostic.message << '\n';
    }
    return out.str();
}
}
