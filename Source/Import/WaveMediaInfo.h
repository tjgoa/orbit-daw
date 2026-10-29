#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace orbit::import
{
struct WaveMediaInfo
{
    std::uint16_t formatTag = 0, channels = 0, bitsPerSample = 0;
    std::uint32_t sampleRate = 0;
    std::uint64_t frames = 0;
};
// Original RIFF/WAVE header inspection for relink validation. Supports ordinary
// little-endian RIFF PCM/IEEE-float files, without decoding or copying media.
// Other containers/encodings are refused explicitly.
bool inspectWaveMedia(const std::filesystem::path&, WaveMediaInfo&, std::string& error);
}
