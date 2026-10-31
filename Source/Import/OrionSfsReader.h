#pragma once

#include "OrionSfsImportModel.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace orbit::import::orion
{
// Describes the outer SFS chunk envelope and the known global prelude.
struct SfsChunk
{
    std::uint64_t headerOffset = 0;
    std::array<char, 4> id {};
    std::uint32_t payloadSize = 0;
    std::uint64_t consumedPayloadSize = 0;
};

struct SfsInspection
{
    std::uint32_t fileVersion = 0;
    std::uint64_t fileSize = 0;
    std::optional<std::uint32_t> moduleCount;
    std::uint64_t firstUnparsedOffset = 0;
    bool stoppedBeforeModules = false;
    std::vector<SfsChunk> globalPreludeChunks;
};

// Reads the SFS header and known global prelude. readDocument additionally
// extracts the supported module inventory and observed generator pattern data.
class OrionSfsReader final
{
public:
    // Experimental recovery holds a snapshot and retained regions in memory.
    // This is a resource policy, not a limit established by the SFS format.
    static constexpr std::uint64_t maximumSourceBytes = 64ull * 1024 * 1024;

    static bool inspect(const std::filesystem::path& file,
                        SfsInspection& result,
                        std::string& error);

    // Extracts module inventory and the shared generator pattern/event layout
    // into Orbit's neutral model. Unrecognized module state remains untouched.
    static bool readDocument(const std::filesystem::path& file,
                             ImportDocument& result,
                             std::string& error);

    // Returns inspection from the same immutable byte snapshot used to parse
    // and retain source data, avoiding repeated opens during preview.
    static bool readDocument(const std::filesystem::path& file,
                             ImportDocument& result, SfsInspection& inspection,
                             std::string& error);

    static std::string displayChunkId(const std::array<char, 4>& id);

private:
    static bool inspectStream(std::istream&, SfsInspection&, std::string&);
};
}
