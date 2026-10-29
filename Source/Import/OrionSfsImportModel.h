#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Orbit-owned, format-neutral destination for facts recovered from an Orion
// SFS file. Keep this independent of JUCE, Tracktion, and Orion implementation
// headers so parsing, review, conversion, and any future license split remain
// separate concerns.
namespace orbit::import::orion
{
using SourceFourCC = std::array<char, 4>;

enum class DiagnosticSeverity
{
    information,
    warning,
    error
};

struct ImportDiagnostic
{
    DiagnosticSeverity severity = DiagnosticSeverity::information;
    std::string code;
    std::string message;
    std::optional<std::uint64_t> sourceOffset;
    std::optional<SourceFourCC> sourceChunk;
};

// An opaque source span supports diagnostics and later extraction of media
// payloads without treating unknown plug-in state as a native Orbit effect.
struct SourceSpan
{
    std::uint64_t offset = 0;
    std::uint64_t byteLength = 0;
    std::optional<SourceFourCC> chunk;
};

struct ProjectMetadata
{
    std::uint32_t sfsVersion = 0;
    std::uint64_t sourceFileSize = 0;
    std::string sourcePath;
    std::string title;
    std::string author;
    std::optional<double> initialTempoBpm;
    bool initialTempoTentative = false;
    std::optional<std::uint32_t> sourceTempoRawBits;
    std::optional<SourceSpan> sourceTempoSpan;
    std::optional<std::int64_t> ticksPerQuarterNote;
    std::optional<std::uint16_t> initialMeterNumerator;
    std::optional<std::uint16_t> initialMeterDenominator;
};

struct TempoEvent
{
    std::int64_t positionTicks = 0;
    double bpm = 120.0;
    std::optional<std::uint32_t> sourceEventIndex;
};

struct MeterEvent
{
    std::int64_t positionTicks = 0;
    std::uint16_t numerator = 4;
    std::uint16_t denominator = 4;
    std::optional<std::uint32_t> sourceEventIndex;
};

enum class ModuleRole
{
    generator,
    audioTrack,
    drumRack,
    mixer,
    effect,
    sampler,
    unknown
};

struct ModuleRecord
{
    // Source object indices are not assumed to be stable IDs across files.
    std::uint32_t sourceObjectIndex = 0;
    std::optional<std::uint32_t> sourceObjectId;
    std::optional<SourceFourCC> sourceType;
    std::optional<std::uint32_t> sourceTypeCode;
    ModuleRole role = ModuleRole::unknown;
    std::string sourceTypeName;
    std::string displayName;
    std::optional<std::uint32_t> parentObjectIndex;
    std::optional<std::uint32_t> trackOrder;
    std::optional<SourceSpan> opaqueState;
};

enum class TrackKind
{
    instrument,
    midi,
    audio,
    drumRack,
    bus,
    unknown
};

struct PatternEvent
{
    enum class Kind
    {
        note,
        controlChange,
        pitchBend,
        programChange,
        channelPressure,
        polyPressure,
        unknown
    };

    Kind kind = Kind::unknown;
    std::int64_t positionTicks = 0;
    std::int64_t durationTicks = 0;
    std::uint8_t channel = 0;
    std::int32_t data1 = 0;
    std::int32_t data2 = 0;
    // Raw first 16-bit word from the source record. Its meaning is not yet
    // established across SFS versions.
    std::optional<std::uint16_t> sourceRawField0;
    std::optional<std::uint16_t> sourceEventTypeCode;
    std::optional<std::uint32_t> sourceRawData1;
    std::optional<std::uint32_t> sourceRawData2;
    std::optional<std::uint32_t> sourceEventIndex;
    std::optional<SourceSpan> opaqueSource;
};

struct PatternRecord
{
    std::uint32_t sourcePatternIndex = 0;
    std::optional<std::uint32_t> sourcePatternId;
    std::string name;
    // Signed 64-bit timeline values intentionally avoid an arbitrary short
    // pattern cap and can represent long imported MIDI arrangements.
    std::int64_t lengthTicks = 0;
    std::optional<SourceSpan> sourceRecord;
    std::optional<SourceSpan> opaqueEventData;
    std::vector<PatternEvent> events;
};

struct AudioSourceRecord
{
    std::string sourceReference;
    std::optional<double> sourceDurationSeconds;
    std::optional<std::uint32_t> sampleRate;
    std::optional<std::uint16_t> channelCount;
    std::optional<SourceSpan> embeddedPayload;
    bool bundledInProject = false;
    std::optional<std::uint32_t> sourcePoolIndex;
    std::string displayName;
    std::optional<std::uint16_t> waveFormatTag;
    std::optional<std::uint16_t> bitsPerSample;
    std::optional<std::uint64_t> sourceFrameCount;
    std::optional<SourceSpan> sourceRecord;
    bool interpretationTentative = false;
};

struct PlaylistPlacement
{
    std::uint32_t sourcePlacementIndex = 0;
    std::optional<std::uint32_t> patternIndex;
    std::int64_t startTicks = 0;
    std::int64_t lengthTicks = 0;
    bool enabled = true;
    bool interpretationTentative = false;
    std::optional<SourceSpan> sourceRecord;
};

struct AudioClipRecord
{
    std::uint32_t sourceClipIndex = 0;
    std::uint32_t audioSourceIndex = 0;
    double startTicks = 0.0;
    std::optional<double> lengthTicks;
    std::optional<std::uint64_t> sourceOffsetFrames;
    std::optional<std::uint64_t> sourceEndFrames;
    std::array<std::uint32_t, 24> rawSourceWords {};
    bool interpretationTentative = false;
    std::optional<SourceSpan> sourceRecord;
};

struct TrackRecord
{
    std::uint32_t sourceTrackIndex = 0;
    std::optional<std::uint32_t> sourceModuleIndex;
    TrackKind kind = TrackKind::unknown;
    std::string name;
    std::uint32_t order = 0;
    bool muted = false;
    bool soloed = false;
    std::vector<std::uint32_t> effectModuleIndices;
    std::vector<PatternRecord> patterns;
    // Complete framing of this track's pattern list was recognized. Individual
    // payloads can still be opaque and must be checked when selected.
    bool patternLayoutRecognized = false;
    // Only describes changes assigned to this track. Unassignable global PATC
    // records are reported separately and still block arrangement recovery.
    bool playlistMappingComplete = false;
    std::vector<PlaylistPlacement> playlist;
    std::vector<AudioClipRecord> audioClips;
    std::optional<SourceSpan> opaqueGeneratorPrefix;
    std::optional<SourceSpan> opaqueGeneratorTail;
};

struct ParameterRecord
{
    std::string sourceParameterId;
    std::string displayName;
    std::optional<std::int64_t> rawIntegerValue;
    std::optional<double> rawFloatingValue;
    std::optional<double> normalizedValue;
    std::optional<std::string> documentedMapping;
    std::optional<SourceSpan> sourceRecord;
};

struct DrumPadRecord
{
    std::uint32_t padIndex = 0;
    std::string name;
    std::optional<std::uint32_t> audioSourceIndex;
    std::vector<ParameterRecord> parameters;
    std::vector<PatternEvent> hits;
};

// Observed 16-byte global/PATC record framing for the mapped versions. Field
// semantics deliberately remain raw; a pattern-change is not a duration clip.
struct RawTimelineRecord
{
    std::uint32_t sourceIndex = 0;
    std::uint16_t rawField0 = 0;
    std::uint16_t eventCode = 0;
    std::uint32_t rawPosition = 0;
    std::uint32_t rawData1 = 0;
    std::uint32_t rawData2 = 0;
    SourceSpan source;
};

// Raw source bytes for an unmapped region. These are data, never executable
// plugin state. Keeping bytes makes reports independent of later source edits.
struct OpaqueSourceRecord
{
    std::string purpose;
    SourceSpan source;
    std::vector<std::uint8_t> bytes;
};

// Byte-level discovery only: not proof of a clip, pad assignment, or missing
// file. Kept separate from structurally decoded AudioSourceRecord entries.
struct MediaReferenceCandidate
{
    std::string reference;
    SourceSpan source;
};

struct TailRecord
{
    SourceSpan source;
    std::uint32_t declaredSizeOrCount = 0;
    std::string description;
};

struct ImportDocument
{
    ProjectMetadata metadata;
    std::vector<TempoEvent> tempoEvents;
    std::vector<MeterEvent> meterEvents;
    std::vector<ModuleRecord> modules;
    std::vector<AudioSourceRecord> audioSources;
    std::vector<TrackRecord> tracks;
    std::vector<DrumPadRecord> drumPads;
    std::vector<ImportDiagnostic> diagnostics;
    std::vector<OpaqueSourceRecord> opaqueRecords;
    std::vector<MediaReferenceCandidate> mediaReferenceCandidates;
    std::vector<RawTimelineRecord> playlistChangeRecords;
    std::vector<TailRecord> tailRecords;
};
}
