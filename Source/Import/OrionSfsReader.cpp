#include "OrionSfsReader.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <string_view>
#include <utility>

namespace orbit::import::orion
{
namespace
{
constexpr std::array<char, 4> sfsHeaderId { 'V', 'F', 'h', 'd' };
constexpr std::array<char, 4> objectDirectoryId { 'I', 'M', 'O', 'D' };
constexpr std::array<char, 4> globalConfigId { 'G', 'L', 'O', 'B' };
constexpr std::array<char, 4> globalEventsId { 'E', 'V', 'T', 'S' };
constexpr std::array<char, 4> playlistPatternsId { 'P', 'A', 'T', 'C' };
constexpr std::array<char, 4> windowPositionsId { 'W', 'I', 'N', 'P' };
constexpr std::array<char, 4> objectCountId { 'O', 'B', 'J', 'S' };
constexpr std::array<char, 4> songInfoId { 0x04, 0x11, 0x00, 0x00 };
constexpr std::array<char, 4> moduleEndId { 'O', 'M', 'O', 'D' };
constexpr std::uint64_t chunkHeaderSize = 8;
constexpr std::uint32_t generatorInfoChunkId = 2006;
constexpr std::uint32_t effectInfoChunkId = 2007;
constexpr std::uint32_t trackDataChunkId = 5;
constexpr std::uint32_t patternChunkId = 2000;
constexpr std::uint32_t patternCountChunkId = 2001;
constexpr std::uint32_t patternLengthChunkId = 2005;
constexpr std::uint32_t patternNoteEventCode = 0x3002;
constexpr std::uint64_t patternEventRecordSize = 16;
// Pattern boundaries and 16-byte event records have been observed in these
// two local files. Other versions remain opaque until inspected.
constexpr std::array<std::uint32_t, 2> mappedPatternVersions { 0x8000, 0x8612 };

bool hasMappedPatternVersion(std::uint32_t version)
{
    return std::find(mappedPatternVersions.begin(), mappedPatternVersions.end(), version)
        != mappedPatternVersions.end();
}

bool isKnownGlobalPreludeChunk(const std::array<char, 4>& id)
{
    return id == objectDirectoryId || id == globalConfigId || id == globalEventsId
        || id == playlistPatternsId || id == windowPositionsId || id == objectCountId
        || id == songInfoId;
}

std::uint32_t readLittleEndian32(const char* bytes)
{
    const auto* u = reinterpret_cast<const unsigned char*>(bytes);
    return static_cast<std::uint32_t>(u[0])
        | (static_cast<std::uint32_t>(u[1]) << 8)
        | (static_cast<std::uint32_t>(u[2]) << 16)
        | (static_cast<std::uint32_t>(u[3]) << 24);
}

std::uint64_t readLittleEndian64(const char* bytes)
{
    return readLittleEndian32(bytes) | (static_cast<std::uint64_t>(readLittleEndian32(bytes + 4)) << 32);
}

double readLittleEndianDouble(const char* bytes)
{
    return std::bit_cast<double>(readLittleEndian64(bytes));
}

bool readExact(std::istream& stream, char* destination, std::streamsize count)
{
    stream.read(destination, count);
    return stream.gcount() == count;
}

bool seekAndRead(std::istream& stream, std::uint64_t offset, char* destination,
                 std::size_t count)
{
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())
        || count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        return false;

    stream.clear();
    stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    return stream && readExact(stream, destination, static_cast<std::streamsize>(count));
}

std::uint16_t readLittleEndian16(const char* bytes)
{
    const auto* u = reinterpret_cast<const unsigned char*>(bytes);
    return static_cast<std::uint16_t>(u[0])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(u[1]) << 8);
}

bool readChunkHeader(std::istream& stream, std::uint64_t offset, std::uint64_t limit,
                     std::uint32_t& id, std::uint32_t& payloadSize)
{
    std::array<char, chunkHeaderSize> header {};
    if (offset > limit || limit - offset < chunkHeaderSize
        || ! seekAndRead(stream, offset, header.data(), header.size()))
        return false;

    id = readLittleEndian32(header.data());
    payloadSize = readLittleEndian32(header.data() + 4);
    return payloadSize <= limit - offset - chunkHeaderSize;
}

std::string readCString(std::istream& stream, std::uint64_t offset, std::uint64_t length)
{
    constexpr std::uint64_t maximumTextBytes = 4096;
    if (length == 0 || length > maximumTextBytes)
        return {};

    std::string text(static_cast<std::size_t>(length), '\0');
    if (! seekAndRead(stream, offset, text.data(), text.size()))
        return {};

    const auto terminator = text.find('\0');
    if (terminator != std::string::npos)
        text.resize(terminator);
    return text;
}

bool findModuleEnd(std::istream& stream, std::uint64_t start, std::uint64_t fileSize,
                   std::uint64_t& markerOffset, std::uint32_t& markerPayloadSize)
{
    constexpr std::size_t scanBlockSize = 64 * 1024;
    constexpr std::string_view markerText = "EndOrionModule";
    std::array<char, scanBlockSize + 3> block {};
    std::uint64_t scanOffset = start;

    while (scanOffset + 4 <= fileSize)
    {
        const auto available = static_cast<std::size_t>(
            std::min<std::uint64_t>(scanBlockSize, fileSize - scanOffset));
        if (! seekAndRead(stream, scanOffset, block.data(), available))
            return false;

        for (std::size_t i = 0; i + moduleEndId.size() <= available; ++i)
        {
            if (! std::equal(moduleEndId.begin(), moduleEndId.end(), block.begin() + static_cast<std::ptrdiff_t>(i)))
                continue;

            const auto candidate = scanOffset + i;
            std::uint32_t id = 0;
            std::uint32_t payloadSize = 0;
            if (! readChunkHeader(stream, candidate, fileSize, id, payloadSize)
                || id != 0x444f4d4f || payloadSize < markerText.size()
                || payloadSize > 64)
                continue;

            std::array<char, markerText.size()> text {};
            if (! seekAndRead(stream, candidate + chunkHeaderSize, text.data(), text.size())
                || ! std::equal(markerText.begin(), markerText.end(), text.begin()))
                continue;

            markerOffset = candidate;
            markerPayloadSize = payloadSize;
            return true;
        }

        if (available < 4 || scanOffset + available >= fileSize)
            break;
        scanOffset += available - 3;
    }

    return false;
}

void addDiagnostic(ImportDocument& document, DiagnosticSeverity severity,
                   std::string code, std::string message,
                   std::optional<std::uint64_t> offset = {},
                   std::optional<SourceFourCC> chunk = {})
{
    document.diagnostics.push_back({ severity, std::move(code), std::move(message), offset, chunk });
}

void readObservedTempo(std::istream& stream, const SfsInspection& inspection, ImportDocument& document)
{
    // Byte observation, not a copied C/C++ layout: the GLOB+12 IEEE float is
    // consistent in the two known layouts. Song1's 120 value is user-confirmed
    // and independently consistent with audio frame counts and source PPQ.
    const auto count=std::count_if(inspection.globalPreludeChunks.begin(),inspection.globalPreludeChunks.end(),
        [](const SfsChunk& chunk){return chunk.id==globalConfigId;});
    if (count!=1) return;
    for (const auto& chunk:inspection.globalPreludeChunks)
    {
        if (chunk.id!=globalConfigId) continue;
        const bool observed=(inspection.fileVersion==0x8000 && chunk.payloadSize==1288)
            || (inspection.fileVersion==0x8612 && chunk.payloadSize==1304);
        if (!observed) return;
        std::array<char,4> bytes {};
        const auto offset=chunk.headerOffset+chunkHeaderSize+12;
        if (!seekAndRead(stream,offset,bytes.data(),bytes.size())) return;
        const auto raw=readLittleEndian32(bytes.data());
        document.metadata.sourceTempoRawBits=raw;
        document.metadata.sourceTempoSpan=SourceSpan{offset,4,globalConfigId};
        const auto bpm=std::bit_cast<float>(raw);
        if (std::isfinite(bpm) && bpm>0)
        {
            document.metadata.initialTempoBpm=bpm;
            document.metadata.initialTempoTentative=true;
            addDiagnostic(document,DiagnosticSeverity::information,"tentative-initial-tempo-field",
                "GLOB+12 is provisionally interpreted as an IEEE-float initial BPM. Confirm it before conversion; source bits/span are retained.",offset,globalConfigId);
        }
        else
            addDiagnostic(document,DiagnosticSeverity::warning,"invalid-initial-tempo-candidate",
                "The observed tempo field is not a finite positive value; its raw bits remain available and no tempo was assumed.",offset,globalConfigId);
    }
}

void mapObservedPlaylist(ImportDocument& document)
{
    if (!hasMappedPatternVersion(document.metadata.sfsVersion)) return;
    std::vector<std::vector<const RawTimelineRecord*>> grouped(document.tracks.size());
    bool incomplete = false;
    for (const auto& record : document.playlistChangeRecords)
    {
        auto track = std::find_if(document.tracks.begin(), document.tracks.end(), [&](const TrackRecord& candidate)
        {
            return candidate.sourceModuleIndex && *candidate.sourceModuleIndex < document.modules.size()
                && document.modules[*candidate.sourceModuleIndex].sourceObjectIndex == record.rawField0;
        });
        if (record.eventCode != 0x3007 || record.rawData2 != 0 || track == document.tracks.end())
        {
            incomplete = true;
            addDiagnostic(document, DiagnosticSeverity::warning, "playlist-record-not-mapped",
                          "A PATC record has an unsupported event code, field or module reference; no semantics were guessed.", record.source.offset);
            continue;
        }
        if (track->kind == TrackKind::audio) continue; // Audio placements use their own event stream.
        grouped[static_cast<std::size_t>(track - document.tracks.begin())].push_back(&record);
    }
    for (std::size_t index = 0; index < grouped.size(); ++index)
    {
        auto& records = grouped[index];
        auto& track = document.tracks[index];
        std::stable_sort(records.begin(), records.end(), [](const auto* a, const auto* b)
            { return a->rawPosition < b->rawPosition; });
        std::vector<PlaylistPlacement> placements;
        bool valid = true;
        for (std::size_t event = 0; event < records.size(); ++event)
        {
            const auto& record = *records[event];
            const auto pattern = std::find_if(track.patterns.begin(), track.patterns.end(), [&](const PatternRecord& candidate)
                { return candidate.sourcePatternIndex == record.rawData1; });
            if (pattern == track.patterns.end() || pattern->lengthTicks <= 0
                || (event + 1 < records.size() && records[event + 1]->rawPosition == record.rawPosition))
            {
                valid = false;
                addDiagnostic(document, DiagnosticSeverity::warning, "playlist-reference-not-mapped",
                              "A PATC pattern reference, length or simultaneous change is ambiguous; this track's placements remain raw.", record.source.offset);
                break;
            }
            auto length = pattern->lengthTicks;
            if (event + 1 < records.size())
                length = std::min<std::int64_t>(length, records[event + 1]->rawPosition - record.rawPosition);
            PlaylistPlacement placement;
            placement.sourcePlacementIndex = record.sourceIndex;
            placement.patternIndex = pattern->sourcePatternIndex;
            placement.startTicks = record.rawPosition;
            placement.lengthTicks = length;
            placement.interpretationTentative = true;
            placement.sourceRecord = record.source;
            placements.push_back(placement);
        }
        track.playlistMappingComplete = valid;
        if (valid) track.playlist = std::move(placements);
        else
            addDiagnostic(document, DiagnosticSeverity::warning, "playlist-track-mapping-incomplete",
                          "PATC changes for track " + std::to_string(track.sourceTrackIndex) + " (" + track.name
                              + ") remain unmapped. This track's arrangement cannot be recovered.");
    }
    if (incomplete)
        addDiagnostic(document, DiagnosticSeverity::warning, "playlist-mapping-incomplete",
                      "Some PATC changes cannot be assigned safely. Pattern arrangement conversion must be refused until they are understood.");
    if (!document.playlistChangeRecords.empty())
        addDiagnostic(document, DiagnosticSeverity::information, "tentative-playlist-mapping",
                      "0x3007 is provisionally treated as module/pattern changes: playback ends at the pattern end or the next change on that module. Song1's visible playlist supports this interpretation; source records remain raw and acceptance is required.");
}

bool readPatternEvents(std::istream& stream, std::uint64_t payloadOffset,
                       std::uint32_t payloadSize, std::uint32_t fileVersion,
                       PatternRecord& pattern, ImportDocument& document,
                       std::uint64_t chunkHeaderOffset)
{
    const SourceSpan sourceData { payloadOffset, payloadSize, {} };
    if (! hasMappedPatternVersion(fileVersion))
    {
        pattern.opaqueEventData = sourceData;
        addDiagnostic(document, DiagnosticSeverity::warning, "pattern-version-not-mapped",
                      "Pattern event records from this SFS version were preserved as an opaque source span.",
                      chunkHeaderOffset);
        return true;
    }

    if (payloadSize % patternEventRecordSize != 0)
    {
        pattern.opaqueEventData = sourceData;
        addDiagnostic(document, DiagnosticSeverity::warning, "misaligned-pattern-events",
                      "The pattern event payload is not an exact multiple of the observed 16-byte record size; it was preserved without decoding.",
                      chunkHeaderOffset);
        return true;
    }

    const auto eventCount = payloadSize / patternEventRecordSize;
    if (eventCount > pattern.events.max_size())
    {
        pattern.opaqueEventData = sourceData;
        addDiagnostic(document, DiagnosticSeverity::warning, "pattern-event-count-too-large",
                      "The pattern event count cannot be represented in memory; its source span was preserved.",
                      chunkHeaderOffset);
        return true;
    }

    pattern.events.reserve(static_cast<std::size_t>(eventCount));
    for (std::uint32_t index = 0; index < eventCount; ++index)
    {
        std::array<char, patternEventRecordSize> bytes {};
        const auto recordOffset = payloadOffset + static_cast<std::uint64_t>(index) * patternEventRecordSize;
        if (! seekAndRead(stream, recordOffset, bytes.data(), bytes.size()))
            return false;

        const auto rawField0 = readLittleEndian16(bytes.data());
        const auto eventCode = readLittleEndian16(bytes.data() + 2);
        const auto position = readLittleEndian32(bytes.data() + 4);
        const auto rawData1 = readLittleEndian32(bytes.data() + 8);
        const auto rawData2 = readLittleEndian32(bytes.data() + 12);

        PatternEvent event;
        event.positionTicks = position;
        event.sourceRawField0 = rawField0;
        event.sourceEventTypeCode = eventCode;
        event.sourceRawData1 = rawData1;
        event.sourceRawData2 = rawData2;
        event.sourceEventIndex = index;
        event.opaqueSource = SourceSpan { recordOffset, patternEventRecordSize, {} };

        // Provisional interpretation from this fixture only: records with
        // code 0x3002 appear to contain note events. Raw source words remain
        // attached so the field mapping can be revised without data loss.
        if (eventCode == patternNoteEventCode)
        {
            event.kind = PatternEvent::Kind::note;
            event.durationTicks = rawData1;
            event.data1 = static_cast<std::int32_t>(rawData2 & 0xffu);
            event.data2 = static_cast<std::int32_t>((rawData2 >> 8) & 0xffu);
        }

        pattern.events.push_back(std::move(event));
    }

    return true;
}

bool readGeneratorPatterns(std::istream& stream, std::uint64_t moduleInfoOffset,
                           std::uint64_t moduleEnd, std::uint32_t fileVersion,
                           std::uint32_t moduleType, TrackRecord& track, ImportDocument& document)
{
    if (! hasMappedPatternVersion(fileVersion))
    {
        addDiagnostic(document, DiagnosticSeverity::information, "pattern-version-not-mapped",
                      "Generator pattern boundaries are mapped only for SFS versions 0x8000 and 0x8612 so far.",
                      moduleInfoOffset);
        return false;
    }

    auto cursor = moduleInfoOffset;
    std::uint32_t id = 0;
    std::uint32_t size = 0;

    // The first module subrecord is its metadata chunk. Names are stored as
    // a null-terminated byte string after a four-byte module field.
    if (! readChunkHeader(stream, cursor, moduleEnd, id, size)
        || id != generatorInfoChunkId)
        return false;
    cursor += chunkHeaderSize + size;

    // Independently observed in both 0x8000/0x1003 reference modules: a
    // 208-byte metadata payload is followed by exactly the byte count stored
    // at payload+64 (14 and 11 respectively), then the ordinary 64-byte track
    // data chunk. The extension remains opaque; no plugin state is restored.
    if (fileVersion == 0x8000 && moduleType == 0x1003 && size == 208)
    {
        std::array<char, 4> length {};
        if (!seekAndRead(stream, moduleInfoOffset + chunkHeaderSize + 64, length.data(), length.size()))
            return false;
        const auto extensionBytes = readLittleEndian32(length.data());
        std::uint32_t nextId = 0, nextSize = 0;
        if (extensionBytes > 4096 || extensionBytes > moduleEnd - cursor)
            return false;
        if (extensionBytes != 0)
        {
            if (!readChunkHeader(stream, cursor + extensionBytes, moduleEnd, nextId, nextSize)
                || nextId != trackDataChunkId || nextSize != 64)
                return false;
            track.opaqueGeneratorPrefix = SourceSpan {cursor, extensionBytes, {}};
            addDiagnostic(document, DiagnosticSeverity::information, "observed-vst-metadata-extension",
                          "Observed 0x8000/0x1003 metadata extension: " + std::to_string(extensionBytes)
                              + " opaque bytes precede validated track/pattern chunks; plugin state is not restored.", cursor);
        }
        cursor += extensionBytes;
    }

    if (! readChunkHeader(stream, cursor, moduleEnd, id, size))
        return false;
    if (id == trackDataChunkId)
    {
        cursor += chunkHeaderSize + size;
        if (! readChunkHeader(stream, cursor, moduleEnd, id, size))
            return false;
    }

    if (id != patternCountChunkId || size != 4)
        return false;

    std::array<char, 4> countBytes {};
    if (! seekAndRead(stream, cursor + chunkHeaderSize, countBytes.data(), countBytes.size()))
        return false;
    const auto patternCount = readLittleEndian32(countBytes.data());
    cursor += chunkHeaderSize + size;

    // Length chunk (12 bytes), event chunk header (8 bytes), and the paired
    // pattern-name chunk header (8 bytes), with empty event/name payloads.
    constexpr std::uint32_t minimumBytesPerPattern = 28;
    if (patternCount > (moduleEnd - cursor) / minimumBytesPerPattern)
    {
        addDiagnostic(document, DiagnosticSeverity::warning, "pattern-count-out-of-range",
                      "The module's pattern count does not fit within its module record.", cursor);
        return false;
    }

    for (std::uint32_t patternIndex = 0; patternIndex < patternCount; ++patternIndex)
    {
        const auto patternStart = cursor;
        if (! readChunkHeader(stream, cursor, moduleEnd, id, size)
            || id != patternLengthChunkId || size != 4)
            return false;

        std::array<char, 4> lengthBytes {};
        if (! seekAndRead(stream, cursor + chunkHeaderSize, lengthBytes.data(), lengthBytes.size()))
            return false;

        PatternRecord pattern;
        pattern.sourcePatternIndex = patternIndex;
        pattern.lengthTicks = readLittleEndian32(lengthBytes.data());
        cursor += chunkHeaderSize + size;

        if (! readChunkHeader(stream, cursor, moduleEnd, id, size) || id != patternChunkId)
            return false;

        const auto eventChunkOffset = cursor;
        const auto eventPayloadOffset = cursor + chunkHeaderSize;
        if (! readPatternEvents(stream, eventPayloadOffset, size, fileVersion,
                                pattern, document, eventChunkOffset))
            return false;
        cursor += chunkHeaderSize + size;

        if (! readChunkHeader(stream, cursor, moduleEnd, id, size) || id != patternChunkId)
            return false;
        pattern.name = readCString(stream, cursor + chunkHeaderSize, size);
        cursor += chunkHeaderSize + size;
        pattern.sourceRecord = SourceSpan { patternStart, cursor - patternStart, {} };
        track.patterns.push_back(std::move(pattern));
    }
    track.opaqueGeneratorTail = SourceSpan { cursor, moduleEnd - cursor, {} };
    return true;
}
std::uint64_t readObservedTail(std::istream& stream, std::uint64_t cursor, ImportDocument& document)
{
    const auto limit = document.metadata.sourceFileSize;
    if (!hasMappedPatternVersion(document.metadata.sfsVersion)) return cursor;
    while (cursor < limit)
    {
        const auto start = cursor;
        std::array<char, 8> header {};
        if (limit - cursor < 8 || !seekAndRead(stream, cursor, header.data(), header.size())) return cursor;
        SourceFourCC id {}; std::copy_n(header.data(), 4, id.data());
        const auto count = readLittleEndian32(header.data() + 4);
        cursor += 8;
        std::string description;
        if (id == SourceFourCC{'C','H','A','N'} || id == SourceFourCC{'F','X','R','N'})
        {
            if (count > (limit - cursor) / 16) return start;
            for (std::uint32_t channel = 0; channel < count; ++channel)
                for (const auto expected : {2004u, 2003u})
                {
                    std::uint32_t code = 0, size = 0;
                    if (!readChunkHeader(stream, cursor, limit, code, size) || code != expected) return start;
                    cursor += 8 + size;
                }
            description = "Mixer channel envelopes (controls retained, not interpreted)";
        }
        else if (id == SourceFourCC{'A','U','I','O'})
        {
            // The observed 0x8612 pool uses a count, not a payload length.
            // Each metadata record has 104 bytes, then a length-prefixed path.
            // Its first 16 bytes match standard PCM/IEEE WAVE format fields.
            if (document.metadata.sfsVersion != 0x8612 || !document.audioSources.empty()
                || count > (limit - cursor) / 108) return start;
            std::vector<AudioSourceRecord> sources;
            for (std::uint32_t index = 0; index < count; ++index)
            {
                const auto recordStart = cursor;
                std::array<char, 108> metadata {};
                if (limit - cursor < metadata.size() || !seekAndRead(stream, cursor, metadata.data(), metadata.size())) return start;
                const auto format = readLittleEndian16(metadata.data());
                const auto channels = readLittleEndian16(metadata.data() + 2);
                const auto rate = readLittleEndian32(metadata.data() + 4);
                const auto bytesPerSecond = readLittleEndian32(metadata.data() + 8);
                const auto align = readLittleEndian16(metadata.data() + 12);
                const auto bits = readLittleEndian16(metadata.data() + 14);
                const auto scalarSamples = readLittleEndian64(metadata.data() + 24);
                const auto frames = readLittleEndian64(metadata.data() + 32);
                const auto length = readLittleEndian32(metadata.data() + 104);
                if (!channels || !rate || !bits || bits % 8 != 0
                    || (format != 1 && format != 3)
                    || static_cast<std::uint64_t>(channels) * (bits / 8) != align
                    || static_cast<std::uint64_t>(rate) * align != bytesPerSecond
                    || frames > std::numeric_limits<std::uint64_t>::max() / channels
                    || frames * channels != scalarSamples || length == 0 || length > limit - cursor - 108)
                    return start;
                cursor += 108;
                std::string path(length, '\0');
                if (!seekAndRead(stream, cursor, path.data(), path.size()) || path.find('\0') != std::string::npos) return start;
                cursor += length;
                AudioSourceRecord source;
                source.sourcePoolIndex = index; source.sourceReference = std::move(path);
                const auto nameEnd = std::find(metadata.begin() + 40, metadata.begin() + 64, '\0');
                source.displayName.assign(metadata.begin() + 40, nameEnd);
                source.waveFormatTag = format; source.sampleRate = rate; source.channelCount = channels;
                source.bitsPerSample = bits; source.sourceFrameCount = frames;
                source.sourceDurationSeconds = static_cast<double>(frames) / rate;
                source.sourceRecord = SourceSpan {recordStart, cursor - recordStart, id};
                source.interpretationTentative = true;
                sources.push_back(std::move(source));
            }
            document.audioSources = std::move(sources);
            description = "Tentative external audio pool (104-byte metadata and length-prefixed paths)";
        }
        else if (id == SourceFourCC{'V','U','U','U'} || id == SourceFourCC{'U','B','M','P'}
                 || id == SourceFourCC{static_cast<char>(0xff),static_cast<char>(0xff),static_cast<char>(0xff),static_cast<char>(0xff)})
        {
            if (count > limit - cursor) return start;
            cursor += count;
            description = id == SourceFourCC{'U','B','M','P'} ? "Project bitmap retained without interpretation" : "Opaque envelope";
        }
        else return start;
        document.tailRecords.push_back({{start, cursor - start, id}, count, std::move(description)});
    }
    return cursor;
}

void readObservedAudioEvents(std::istream& stream, TrackRecord& track, ImportDocument& document)
{
    if (!track.opaqueGeneratorTail || track.kind != TrackKind::audio) return;
    auto cursor = track.opaqueGeneratorTail->offset;
    const auto limit = cursor + track.opaqueGeneratorTail->byteLength;
    std::uint32_t id = 0, size = 0;
    if (!readChunkHeader(stream, cursor, limit, id, size) || id != 2003)
    {
        addDiagnostic(document, DiagnosticSeverity::warning, "audio-events-not-mapped", "Audio-track control framing is unknown; the module tail remains raw.", cursor);
        return;
    }
    cursor += 8 + size;
    if (!readChunkHeader(stream, cursor, limit, id, size) || id != 0x53545645)
    {
        addDiagnostic(document, DiagnosticSeverity::warning, "audio-events-not-mapped", "Audio-track event framing is unknown; the module tail remains raw.", cursor);
        return;
    }
    if (size == 0) return;
    if (document.metadata.sfsVersion != 0x8612 || size % 96 != 0)
    {
        addDiagnostic(document, DiagnosticSeverity::warning, "audio-events-not-mapped", "Audio event framing is not established; the module tail remains opaque.", cursor);
        return;
    }
    std::vector<AudioClipRecord> clips;
    for (std::uint32_t index = 0; index < size / 96; ++index)
    {
        const auto offset = cursor + 8 + static_cast<std::uint64_t>(index) * 96;
        std::array<char, 96> bytes {};
        if (!seekAndRead(stream, offset, bytes.data(), bytes.size())) return;
        AudioClipRecord clip;
        clip.sourceClipIndex = index;
        for (std::size_t word = 0; word < clip.rawSourceWords.size(); ++word)
            clip.rawSourceWords[word] = readLittleEndian32(bytes.data() + word * 4);
        clip.audioSourceIndex = clip.rawSourceWords[2] & 0x00ffffffu;
        clip.startTicks = readLittleEndianDouble(bytes.data() + 24);
        clip.lengthTicks = readLittleEndianDouble(bytes.data() + 32);
        clip.sourceOffsetFrames = clip.rawSourceWords[12]; clip.sourceEndFrames = clip.rawSourceWords[13];
        clip.sourceRecord = SourceSpan {offset, bytes.size(), globalEventsId};
        clip.interpretationTentative = true;
        if ((clip.rawSourceWords[2] >> 24) != 1 || clip.audioSourceIndex >= document.audioSources.size()
            || !std::isfinite(clip.startTicks) || clip.startTicks < 0 || !std::isfinite(*clip.lengthTicks) || *clip.lengthTicks <= 0
            || *clip.sourceOffsetFrames != 0 || *clip.sourceEndFrames <= *clip.sourceOffsetFrames
            || document.audioSources[clip.audioSourceIndex].sourceFrameCount != clip.sourceEndFrames)
        {
            addDiagnostic(document, DiagnosticSeverity::warning, "audio-events-not-mapped",
                          "An audio event has an unsupported reference, nonzero unverified trim field, range or flag; this track's audio events remain raw.", offset);
            return;
        }
        clips.push_back(clip);
    }
    track.audioClips = std::move(clips);
    addDiagnostic(document, DiagnosticSeverity::warning, "tentative-audio-events",
                  "Audio pool/reference and 96-byte event timing mapping is tentative. Clip control words are raw; dry-media recovery requires explicit acceptance and relinking.", cursor);
}

}

bool OrionSfsReader::inspect(const std::filesystem::path& path,
                             SfsInspection& result,
                             std::string& error)
{
    std::ifstream stream(path, std::ios::binary);
    if (! stream)
    {
        result = {};
        error = "Could not open the selected file.";
        return false;
    }
    return inspectStream(stream, result, error);
}

bool OrionSfsReader::inspectStream(std::istream& stream, SfsInspection& result, std::string& error)
{
    result = {};
    error.clear();

    stream.seekg(0, std::ios::end);
    const auto endPosition = stream.tellg();
    if (endPosition == std::streampos(-1))
    {
        error = "Could not determine the file size.";
        return false;
    }

    const auto fileSize = static_cast<std::uint64_t>(endPosition);
    result.fileSize = fileSize;
    if (fileSize > maximumSourceBytes)
    {
        error = "Experimental SFS recovery supports files up to 64 MiB; this file was not parsed.";
        return false;
    }
    if (fileSize < chunkHeaderSize + 4)
    {
        error = "The file is too short to contain an SFS header.";
        return false;
    }

    stream.seekg(0, std::ios::beg);
    std::array<char, chunkHeaderSize> header {};
    if (! readExact(stream, header.data(), static_cast<std::streamsize>(header.size())))
    {
        error = "Could not read the SFS header.";
        return false;
    }

    std::array<char, 4> headerId {};
    std::copy_n(header.data(), headerId.size(), headerId.data());
    const auto headerPayloadSize = readLittleEndian32(header.data() + 4);
    if (headerId != sfsHeaderId || headerPayloadSize != 4)
    {
        error = "This file does not have the expected Orion SFS header.";
        return false;
    }

    std::array<char, 4> versionBytes {};
    if (! readExact(stream, versionBytes.data(), static_cast<std::streamsize>(versionBytes.size())))
    {
        error = "The SFS version field is incomplete.";
        return false;
    }
    result.fileVersion = readLittleEndian32(versionBytes.data());

    std::uint64_t offset = chunkHeaderSize + 4;
    result.firstUnparsedOffset = offset;
    while (offset < fileSize)
    {
        const auto remaining = fileSize - offset;
        if (remaining < chunkHeaderSize)
        {
            error = "The file ends in the middle of a chunk header at byte " + std::to_string(offset) + ".";
            return false;
        }

        if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        {
            error = "A chunk offset exceeds this platform's stream range.";
            return false;
        }
        stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        if (! readExact(stream, header.data(), static_cast<std::streamsize>(header.size())))
        {
            error = "Could not read a chunk header at byte " + std::to_string(offset) + ".";
            return false;
        }

        SfsChunk chunk;
        chunk.headerOffset = offset;
        std::copy_n(header.data(), chunk.id.size(), chunk.id.data());
        chunk.payloadSize = readLittleEndian32(header.data() + 4);

        // SFS switches from the general song chunks into module-specific
        // records after the object directory. Leave that record stream opaque
        // until its layouts are mapped independently.
        if (! isKnownGlobalPreludeChunk(chunk.id))
        {
            result.firstUnparsedOffset = offset;
            result.stoppedBeforeModules = true;
            break;
        }

        const auto payloadOffset = offset + chunkHeaderSize;
        if (payloadOffset > fileSize || chunk.payloadSize > fileSize - payloadOffset)
        {
            error = "Chunk " + displayChunkId(chunk.id) + " at byte " + std::to_string(offset)
                + " extends beyond the end of the file.";
            return false;
        }

        auto consumedPayloadSize = static_cast<std::uint64_t>(chunk.payloadSize);
        if (chunk.id == songInfoId)
        {
            // The SONGINFO header's declared size can include one-byte empty
            // string lengths even when those terminators are absent. Follow
            // the embedded lengths for the actual bytes consumed instead.
            constexpr std::uint64_t fixedSongInfoSize = 16;
            if (chunk.payloadSize < fixedSongInfoSize)
            {
                error = "The song-info record is shorter than its fixed fields.";
                return false;
            }

            std::array<char, fixedSongInfoSize> infoBytes {};
            if (! seekAndRead(stream, payloadOffset, infoBytes.data(), infoBytes.size()))
            {
                error = "Could not read the fixed song-info fields.";
                return false;
            }

            const auto descriptionLength = readLittleEndian32(infoBytes.data() + 4);
            const auto urlLength = readLittleEndian32(infoBytes.data() + 8);
            const auto sizeFromEmbeddedLengths = fixedSongInfoSize
                + static_cast<std::uint64_t>(descriptionLength)
                + static_cast<std::uint64_t>(urlLength);
            consumedPayloadSize = fixedSongInfoSize
                + (descriptionLength > 1 ? descriptionLength : 0)
                + (urlLength > 1 ? urlLength : 0);
            if (chunk.payloadSize != sizeFromEmbeddedLengths
                && chunk.payloadSize != consumedPayloadSize)
            {
                error = "The song-info size disagrees with its embedded string lengths.";
                return false;
            }
            if (consumedPayloadSize > fileSize - payloadOffset)
            {
                error = "The song-info string lengths extend beyond the end of the file.";
                return false;
            }
        }

        chunk.consumedPayloadSize = consumedPayloadSize;
        result.globalPreludeChunks.push_back(chunk);
        const auto nextOffset = payloadOffset + consumedPayloadSize;

        if (chunk.id == objectCountId)
        {
            if (chunk.payloadSize == 4)
            {
                std::array<char, 4> countBytes {};
                if (! readExact(stream, countBytes.data(), static_cast<std::streamsize>(countBytes.size())))
                {
                    error = "The SFS object count field is incomplete.";
                    return false;
                }
                result.moduleCount = readLittleEndian32(countBytes.data());
            }
            result.firstUnparsedOffset = nextOffset;
            result.stoppedBeforeModules = nextOffset < fileSize;
            break;
        }

        offset = nextOffset;
        result.firstUnparsedOffset = offset;
    }

    return true;
}

std::string OrionSfsReader::displayChunkId(const std::array<char, 4>& id)
{
    const auto printable = std::all_of(id.begin(), id.end(), [](unsigned char character)
    {
        return character >= 0x20 && character <= 0x7e;
    });

    if (printable)
        return std::string(id.begin(), id.end());

    static constexpr char digits[] = "0123456789ABCDEF";
    std::string result = "0x";
    for (const auto character : id)
    {
        const auto value = static_cast<unsigned char>(character);
        result.push_back(digits[value >> 4]);
        result.push_back(digits[value & 0x0f]);
    }
    return result;
}

bool OrionSfsReader::readDocument(const std::filesystem::path& path,
                                  ImportDocument& result,
                                  std::string& error)
{
    SfsInspection inspection;
    return readDocument(path, result, inspection, error);
}

bool OrionSfsReader::readDocument(const std::filesystem::path& path,
                                  ImportDocument& result, SfsInspection& inspection,
                                  std::string& error)
{
    result = {};
    inspection = {};
    error.clear();
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto sourceEnd = file.tellg();
    if (!file || sourceEnd == std::streampos(-1))
    {
        error = "Could not open or determine the size of the selected SFS file.";
        return false;
    }
    const auto size = static_cast<std::uint64_t>(sourceEnd);
    if (size > maximumSourceBytes)
    {
        error = "Experimental SFS recovery supports files up to 64 MiB; no source snapshot was allocated.";
        return false;
    }
    std::string snapshot;
    if (size > snapshot.max_size() || size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()))
    {
        error = "The source file cannot fit into an in-memory inspection snapshot.";
        return false;
    }
    snapshot.resize(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!readExact(file, snapshot.data(), static_cast<std::streamsize>(size)))
    {
        error = "The source file changed or could not be read completely.";
        return false;
    }
    file.close();
    std::istringstream stream(std::move(snapshot), std::ios::in | std::ios::binary);
    if (!inspectStream(stream, inspection, error)) return false;
    result.metadata.sfsVersion = inspection.fileVersion;
    result.metadata.sourceFileSize = inspection.fileSize;
    result.metadata.sourcePath = path.string();

    readObservedTempo(stream,inspection,result);

    // Preserve the complete source in non-overlapping regions, including
    // globals that have no established semantic mapping and trailing data.
    // Module spans below refer into these retained bytes.
    auto retain = [&](std::string purpose, SourceSpan span)
    {
        OpaqueSourceRecord record;
        record.purpose = std::move(purpose);
        record.source = span;
        if (span.byteLength > record.bytes.max_size())
            return false;
        record.bytes.resize(static_cast<std::size_t>(span.byteLength));
        if (! seekAndRead(stream, span.offset,
                          reinterpret_cast<char*>(record.bytes.data()), record.bytes.size()))
            return false;
        result.opaqueRecords.push_back(std::move(record));
        return true;
    };
    if (! retain("SFS header", { 0, 12, sfsHeaderId }))
    {
        error = "Could not retain the source header.";
        return false;
    }
    for (const auto& chunk : inspection.globalPreludeChunks)
    {
        if (! retain("Global " + displayChunkId(chunk.id),
                     { chunk.headerOffset, chunkHeaderSize + chunk.consumedPayloadSize, chunk.id }))
        {
            error = "Could not retain a global source record.";
            return false;
        }
        if (chunk.payloadSize != chunk.consumedPayloadSize)
            addDiagnostic(result, DiagnosticSeverity::warning, "song-info-size-quirk",
                          "Declared song-info size differs from consumed embedded string data; both sizes are retained in inspection.",
                          chunk.headerOffset, chunk.id);
    }
    if (inspection.firstUnparsedOffset < inspection.fileSize
        && ! retain("Module stream and trailing data",
                    { inspection.firstUnparsedOffset, inspection.fileSize - inspection.firstUnparsedOffset, {} }))
    {
        error = "Could not retain the remaining source bytes.";
        return false;
    }
    // Both observed versions use the same 16-byte framing in PATC. Extract
    // raw fields without claiming that change events are independent clips.
    // EVTS is empty in both current files, so nonempty global events remain
    // opaque until their record framing is independently evidenced.
    for (const auto& chunk : inspection.globalPreludeChunks)
    {
        if (chunk.id != playlistPatternsId)
            continue;
        if (! hasMappedPatternVersion(inspection.fileVersion) || chunk.payloadSize % 16 != 0)
        {
            addDiagnostic(result, DiagnosticSeverity::warning, "playlist-record-framing-unmapped",
                          "PATC event framing is unknown or misaligned; raw bytes were retained.", chunk.headerOffset, chunk.id);
            continue;
        }
        for (std::uint32_t index = 0; index < chunk.payloadSize / 16; ++index)
        {
            const auto offset = chunk.headerOffset + chunkHeaderSize + static_cast<std::uint64_t>(index) * 16;
            std::array<char, 16> bytes {};
            if (! seekAndRead(stream, offset, bytes.data(), bytes.size()))
            {
                error = "Could not read a PATC source record.";
                return false;
            }
            result.playlistChangeRecords.push_back({ index, readLittleEndian16(bytes.data()),
                readLittleEndian16(bytes.data() + 2), readLittleEndian32(bytes.data() + 4),
                readLittleEndian32(bytes.data() + 8), readLittleEndian32(bytes.data() + 12),
                { offset, 16, chunk.id } });
        }
        addDiagnostic(result, DiagnosticSeverity::information, "raw-playlist-change-records",
                      "PATC records retain raw 16-byte fields; the observed 0x3007 change mapping remains tentative.",
                      chunk.headerOffset, chunk.id);
    }

    // Discover printable drive-qualified paths independently of undocumented
    // module layouts. These remain candidates, never decoded audio sources.
    constexpr std::array<std::string_view, 7> mediaExtensions {
        ".wav", ".aiff", ".aif", ".flac", ".ogg", ".mp3", ".sf2"
    };
    for (const auto& region : result.opaqueRecords)
    {
        const auto& bytes = region.bytes;
        for (std::size_t start = 0; start + 3 < bytes.size(); ++start)
        {
            if (! ((bytes[start] >= 'A' && bytes[start] <= 'Z')
                    || (bytes[start] >= 'a' && bytes[start] <= 'z'))
                || bytes[start + 1] != ':'
                || (bytes[start + 2] != '\\' && bytes[start + 2] != '/'))
                continue;
            auto end = start;
            while (end < bytes.size() && bytes[end] >= 0x20 && bytes[end] <= 0x7e
                   && end - start < 4096)
                ++end;
            std::string text(bytes.begin() + static_cast<std::ptrdiff_t>(start),
                             bytes.begin() + static_cast<std::ptrdiff_t>(end));
            auto lower = text;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::size_t referenceEnd = std::string::npos;
            for (const auto extension : mediaExtensions)
            {
                // Use the last extension in the printable run so directories
                // containing extension-like text do not truncate the candidate.
                const auto position = lower.rfind(extension);
                if (position != std::string::npos)
                    referenceEnd = std::max(referenceEnd == std::string::npos ? 0 : referenceEnd,
                                            position + extension.size());
            }
            if (referenceEnd != std::string::npos)
            {
                text.resize(referenceEnd);
                result.mediaReferenceCandidates.push_back({ std::move(text),
                    { region.source.offset + start, referenceEnd, {} } });
                start += referenceEnd - 1;
            }
        }
    }
    if (! result.mediaReferenceCandidates.empty())
        addDiagnostic(result, DiagnosticSeverity::warning, "unverified-media-references",
                      "Printable media paths were discovered without structural clip/pad mapping. Relinking and bundled-media status remain unresolved.");

    addDiagnostic(result, DiagnosticSeverity::warning, "timing-basis-unmapped",
                  "Tempo, meter and source ticks per quarter note are not established. Source tick values must not be converted to Orbit timing yet.");

    auto moduleStart = inspection.firstUnparsedOffset;
    const auto expectedModuleCount = inspection.moduleCount.value_or(0);
    std::uint32_t trackOrder = 0;
    std::uint32_t parsedModuleCount = 0;

    if (! inspection.moduleCount.has_value())
        addDiagnostic(result, DiagnosticSeverity::warning, "object-count-not-found",
                      "The global object count is missing, so module extraction could not start.",
                      inspection.firstUnparsedOffset);

    for (; parsedModuleCount < expectedModuleCount; ++parsedModuleCount)
    {
        std::uint64_t moduleEnd = 0;
        std::uint32_t moduleEndPayloadSize = 0;
        if (! findModuleEnd(stream, moduleStart + chunkHeaderSize,
                            inspection.fileSize, moduleEnd, moduleEndPayloadSize))
        {
            addDiagnostic(result, DiagnosticSeverity::warning, "module-end-not-found",
                          "Could not find the next validated module-end record; parsing stopped at this module.",
                          moduleStart);
            break;
        }

        std::uint32_t typeCode = 0;
        std::uint32_t outerPayloadSize = 0;
        if (! readChunkHeader(stream, moduleStart, moduleEnd, typeCode, outerPayloadSize))
        {
            addDiagnostic(result, DiagnosticSeverity::warning, "invalid-module-header",
                          "A module begins with an invalid chunk envelope; parsing stopped at this module.", moduleStart);
            break;
        }

        ModuleRecord module;
        module.sourceObjectIndex = parsedModuleCount;
        module.sourceTypeCode = typeCode;
        module.opaqueState = SourceSpan {
            moduleStart,
            moduleEnd + chunkHeaderSize + moduleEndPayloadSize - moduleStart,
            {}
        };
        const auto moduleInfoOffset = moduleStart + chunkHeaderSize + outerPayloadSize;
        std::uint32_t infoId = 0;
        std::uint32_t infoSize = 0;
        const auto generatorModule = typeCode >= 0x1000 && typeCode < 0x1100;
        const auto effectModule = typeCode >= 0x1100 && typeCode < 0x1200;

        if (typeCode == 0x1006)
            module.role = ModuleRole::audioTrack;
        else if (typeCode == 0x1007)
            module.role = ModuleRole::drumRack;
        else if (typeCode == 0x1000)
            module.role = ModuleRole::sampler;
        else if (generatorModule)
            module.role = ModuleRole::generator;
        else if (effectModule)
            module.role = ModuleRole::effect;
        else
            module.role = ModuleRole::unknown;

        if (generatorModule)
            module.trackOrder = trackOrder;

        if ((generatorModule || effectModule)
            && readChunkHeader(stream, moduleInfoOffset, moduleEnd, infoId, infoSize)
            && ((generatorModule && infoId == generatorInfoChunkId)
                || (effectModule && infoId == effectInfoChunkId)))
        {
            module.displayName = readCString(stream, moduleInfoOffset + chunkHeaderSize + 4,
                                             infoSize > 4 ? infoSize - 4 : 0);
        }
        else
        {
            addDiagnostic(result, DiagnosticSeverity::warning, "module-info-not-recognized",
                          "The module type or its metadata record is not recognized; its raw type code was retained.",
                          moduleStart);
        }

        result.modules.push_back(module);
        const auto moduleIndex = static_cast<std::uint32_t>(result.modules.size() - 1);

        if (generatorModule)
        {
            TrackRecord track;
            track.sourceTrackIndex = trackOrder;
            track.sourceModuleIndex = moduleIndex;
            track.order = trackOrder;
            track.name = module.displayName;
            track.kind = typeCode == 0x1006 ? TrackKind::audio
                : typeCode == 0x1007 ? TrackKind::drumRack
                : typeCode == 0x1008 ? TrackKind::midi
                : TrackKind::instrument;
            result.tracks.push_back(std::move(track));
            ++trackOrder;

            result.tracks.back().patternLayoutRecognized = readGeneratorPatterns(stream, moduleInfoOffset, moduleEnd,
                                        inspection.fileVersion, typeCode, result.tracks.back(), result);
            if (! result.tracks.back().patternLayoutRecognized)
            {
                addDiagnostic(result, DiagnosticSeverity::warning, "generator-patterns-not-decoded",
                              "The shared generator pattern layout was not recognized for this module; module metadata was retained.",
                              moduleInfoOffset);
            }
        }

        moduleStart = moduleEnd + chunkHeaderSize + moduleEndPayloadSize;
        if (moduleStart > inspection.fileSize)
        {
            addDiagnostic(result, DiagnosticSeverity::warning, "module-end-out-of-range",
                          "The module-end record extends beyond the file.", moduleEnd);
            break;
        }
    }

    const auto firstUnparsedTail = parsedModuleCount == expectedModuleCount
        ? readObservedTail(stream, moduleStart, result) : moduleStart;
    for (auto& track : result.tracks) readObservedAudioEvents(stream, track, result);
    if (firstUnparsedTail < inspection.fileSize)
        addDiagnostic(result, DiagnosticSeverity::warning, "unparsed-trailing-data",
                      std::to_string(inspection.fileSize - firstUnparsedTail)
                          + " trailing bytes remain structurally undecoded and retained in raw source storage.",
                      firstUnparsedTail);

    if (parsedModuleCount != expectedModuleCount)
        addDiagnostic(result, DiagnosticSeverity::warning, "module-count-mismatch",
                      "The number of validated module records differs from the SFS object count.",
                      inspection.firstUnparsedOffset);

    mapObservedPlaylist(result);

    addDiagnostic(result, DiagnosticSeverity::information, "partial-sfs-import",
                  "Module inventory and supported generator pattern events were extracted. Playlist changes have a tentative mapping; audio pool/event timing has an opt-in tentative mapping; global tempo/meter events, mixer routing, embedded sample payloads, and effect parameters remain unmapped.");
    if (std::any_of(result.tracks.begin(), result.tracks.end(), [](const TrackRecord& track)
        { return std::any_of(track.patterns.begin(), track.patterns.end(), [](const PatternRecord& pattern)
            { return ! pattern.events.empty(); }); }))
    {
        addDiagnostic(result, DiagnosticSeverity::information, "tentative-note-field-mapping",
                      "The bundled fixture's 0x3002 records are provisionally treated as notes; their duration, pitch, and velocity field meanings need validation against additional SFS examples.");
    }
    return true;
}
}
