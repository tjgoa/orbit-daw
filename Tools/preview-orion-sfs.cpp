// Experimental standalone SFS inspection, conversion preview and adapter check.
// Native project saving is performed by the Orbit host with its engine session.
#include "../Source/Import/OrionSfsConversion.h"
#include "../Source/Import/OrionSfsReport.h"
#include "../Source/Import/ProjectImportAdapter.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
std::int64_t integer(const std::string& text)
{
    std::int64_t number = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::runtime_error("Invalid integer: " + text);
    return number;
}

void usage()
{
    std::cerr << "Usage: OrbitSfsPreview source.sfs --source-ppq N --tempo BPM --meter N/D\n"
        "  --accept-tentative-notes --accept-partial-recovery [--allow-tick-rounding] [--recover-tentative-playlist] [--recover-dry-audio] [--omit-unsupported-events]\n"
        "  [--pattern sourceTrack:patternSlot ...] [--audio-track sourceTrack ...]\n"
        "  [--media poolIndex=local.wav ...] [--report new.txt]\n"
        "Preview only; no audio session or native project file is written.\n";
}
}

int main(int argc, char** argv)
{
    namespace sfs = orbit::import::orion;
    if (argc < 2) { usage(); return 2; }
    try
    {
        sfs::ConversionOptions options;
        std::filesystem::path reportPath;
        const std::filesystem::path input(argv[1]);
        for (int i = 2; i < argc; ++i)
        {
            const std::string option(argv[i]);
            if (option == "--accept-tentative-notes") options.acceptTentativeNotes = true;
            else if (option == "--accept-pattern-only" || option == "--accept-partial-recovery") options.acceptPatternOnlyRecovery = true;
            else if (option == "--allow-tick-rounding") options.allowTickRounding = true;
            else if (option == "--recover-tentative-playlist") options.recoverTentativePlaylist = true;
            else if (option == "--recover-dry-audio") options.recoverTentativeDryAudio = true;
            else if (option == "--omit-unsupported-events") options.omitUnsupportedEvents = true;
            else
            {
                if (++i >= argc) throw std::runtime_error("Missing value for " + option);
                const std::string value(argv[i]);
                if (option == "--source-ppq") options.sourceTicksPerQuarterNote = integer(value);
                else if (option == "--tempo")
                {
                    std::size_t consumed = 0;
                    options.tempoBpm = std::stod(value, &consumed);
                    if (consumed != value.size()) throw std::runtime_error("Invalid tempo.");
                }
                else if (option == "--meter")
                {
                    const auto slash = value.find('/');
                    if (slash == std::string::npos) throw std::runtime_error("Meter must be N/D.");
                    const auto numerator = integer(value.substr(0, slash));
                    const auto denominator = integer(value.substr(slash + 1));
                    if (numerator < 1 || numerator > 32 || denominator < 1 || denominator > 32)
                        throw std::runtime_error("Meter is outside the supported range.");
                    options.meterNumerator = static_cast<std::uint16_t>(numerator);
                    options.meterDenominator = static_cast<std::uint16_t>(denominator);
                }
                else if (option == "--pattern")
                {
                    const auto colon = value.find(':');
                    if (colon == std::string::npos) throw std::runtime_error("Pattern selection must be sourceTrack:patternSlot.");
                    const auto track = integer(value.substr(0, colon));
                    const auto pattern = integer(value.substr(colon + 1));
                    if (track < 0 || track > std::numeric_limits<std::uint32_t>::max()
                        || pattern < 0 || pattern > std::numeric_limits<std::uint32_t>::max())
                        throw std::runtime_error("Pattern selection index is out of range.");
                    options.selectedPatterns.emplace_back(static_cast<std::uint32_t>(track), static_cast<std::uint32_t>(pattern));
                }
                else if (option=="--audio-track")
                {
                    const auto track=integer(value);
                    if (track<0 || track>std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("Audio track index is out of range.");
                    options.selectedAudioTracks.push_back(static_cast<std::uint32_t>(track));
                }
                else if (option=="--media")
                {
                    const auto equals=value.find('=');
                    if (equals==std::string::npos) throw std::runtime_error("Media relink must be poolIndex=path.wav.");
                    const auto index=integer(value.substr(0,equals));
                    if (index<0 || index>std::numeric_limits<std::uint32_t>::max() || equals+1==value.size()) throw std::runtime_error("Invalid media relink.");
                    if (!options.mediaRelinks.emplace(static_cast<std::uint32_t>(index),value.substr(equals+1)).second) throw std::runtime_error("A media source was relinked twice.");
                }
                else if (option == "--report") reportPath = value;
                else throw std::runtime_error("Unknown option: " + option);
            }
        }
        sfs::SfsInspection inspection;
        sfs::ImportDocument source;
        std::string error;
        if (! sfs::OrionSfsReader::readDocument(input, source, inspection, error))
            throw std::runtime_error(error);
        const auto inspectionReport = sfs::createInspectionReport(source, inspection);
        std::cout << inspectionReport << '\n';
        if (source.metadata.title.empty()) source.metadata.title = input.stem().string() + " (recovered patterns)";
        orbit::import::ProjectImportPlan plan;
        if (! sfs::prepareConversion(source, options, plan, error))
            throw std::runtime_error(error);
        const auto conversionReport = sfs::createConversionReport(plan);
        std::cout << conversionReport;
        auto project = ProjectDocument::createNew();
        // Exercise the same one-action commit boundary a host can use. This
        // document is detached from Orbit's live engine/UI.
        juce::UndoManager undo;
        if (! orbit::import::applyProjectImport(project, plan, undo, error))
            throw std::runtime_error(error);
        if (! reportPath.empty())
        {
            reportPath = std::filesystem::absolute(reportPath);
            if (std::filesystem::exists(reportPath))
                throw std::runtime_error("A report already exists; choose a new report path.");
            std::ofstream report(reportPath, std::ios::binary);
            report << inspectionReport << '\n' << conversionReport;
            report.close();
            if (! report) throw std::runtime_error("Could not write the preview report.");
        }
        std::cout << "Preview and detached adapter validation complete; no native project file written.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Recovery refused: " << error.what() << '\n';
        return 1;
    }
}
