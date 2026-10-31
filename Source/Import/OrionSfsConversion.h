#pragma once

#include "OrionSfsImportModel.h"
#include "ProjectImportPlan.h"
#include <utility>
#include <map>

namespace orbit::import::orion
{
struct ConversionOptions
{
    // Required explicit overrides until corresponding source fields have an
    // established mapping. No default silently asserts a source tempo or PPQ.
    std::optional<std::int64_t> sourceTicksPerQuarterNote;
    std::optional<double> tempoBpm;
    std::optional<std::uint16_t> meterNumerator;
    std::optional<std::uint16_t> meterDenominator;
    bool acceptTentativeNotes = false;
    bool acceptPatternOnlyRecovery = false;
    bool recoverTentativePlaylist = false;
    bool recoverTentativeDryAudio = false;
    bool allowMissingMedia = false;
    std::vector<std::uint32_t> selectedAudioTracks;
    // Explicit source-pool index -> local WAV file chosen by the user.
    std::map<std::uint32_t, std::string> mediaRelinks;
    bool allowTickRounding = false;
    // Explicit partial-recovery policy: preserve unsupported event bytes in
    // reports/native notes while omitting their unverified playback semantics.
    // Framing and timing errors in selected content remain fatal. Coincident
    // notes are preserved as separate native records rather than merged.
    // Explicit selection can omit unrecognized layouts on other tracks; a
    // selected track's partially parsed pattern prefix is never recovered.
    bool omitUnsupportedEvents = false;
    // Empty selects all populated non-audio patterns. Explicit pairs select
    // source track index / pattern index, with omitted content reported.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> selectedPatterns;
};

// Produces a detached note-pattern recovery plan. Refuses ambiguous/invalid
// conversion unless its documented experimental policies are explicitly set.
// Failure leaves the caller's previous plan unchanged.
bool prepareConversion(const ImportDocument&, const ConversionOptions&,
                       ProjectImportPlan&, std::string& error);
std::string createConversionReport(const ProjectImportPlan&);
}
