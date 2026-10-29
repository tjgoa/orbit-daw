#include "WaveMediaInfo.h"
#include <array>
#include <fstream>
#include <limits>

namespace orbit::import
{
namespace
{
std::uint16_t u16(const unsigned char* p) { return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8)); }
std::uint32_t u32(const unsigned char* p) { return p[0] | (static_cast<std::uint32_t>(p[1]) << 8) | (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24); }
bool tag(const unsigned char* p, const char* text) { for (int i=0;i<4;++i) if(p[i]!=static_cast<unsigned char>(text[i])) return false; return true; }
}
bool inspectWaveMedia(const std::filesystem::path& path, WaveMediaInfo& result, std::string& error)
{
    error.clear();
    const auto fail=[&](const char* message){error=message;return false;};
    std::ifstream input(path,std::ios::binary | std::ios::ate);
    if(!input || input.tellg()==std::streampos(-1)) return fail("Could not open relinked media.");
    const auto size=static_cast<std::uint64_t>(input.tellg());
    const auto read=[&](std::uint64_t offset,unsigned char* data,std::size_t length)
    {
        if(offset>size || length>size-offset || offset>static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) return false;
        input.clear(); input.seekg(static_cast<std::streamoff>(offset));
        input.read(reinterpret_cast<char*>(data),static_cast<std::streamsize>(length));
        return input.gcount()==static_cast<std::streamsize>(length);
    };
    std::array<unsigned char,12> header {};
    if(!read(0,header.data(),header.size()) || !tag(header.data(),"RIFF") || !tag(header.data()+8,"WAVE"))
        return fail("Relink validation currently supports ordinary RIFF/WAVE files only.");
    const auto limit=static_cast<std::uint64_t>(u32(header.data()+4))+8;
    if(limit>size || limit<12) return fail("WAVE RIFF extent is invalid.");
    WaveMediaInfo info;
    bool hasFormat=false,hasData=false;
    std::uint64_t audioBytes=0;
    std::uint16_t align=0;
    for(std::uint64_t cursor=12;cursor<limit;)
    {
        std::array<unsigned char,8> chunk {};
        if(limit-cursor<8 || !read(cursor,chunk.data(),chunk.size())) return fail("WAVE chunk header is truncated.");
        const auto bytes=u32(chunk.data()+4);
        cursor+=8;
        if(bytes>limit-cursor) return fail("WAVE chunk extends beyond RIFF.");
        if(tag(chunk.data(),"fmt "))
        {
            std::array<unsigned char,16> format {};
            if(hasFormat || bytes<16 || !read(cursor,format.data(),format.size())) return fail("WAVE format chunk is missing or ambiguous.");
            info.formatTag=u16(format.data()); info.channels=u16(format.data()+2); info.sampleRate=u32(format.data()+4);
            align=u16(format.data()+12); info.bitsPerSample=u16(format.data()+14);
            const bool pcm=info.formatTag==1 && (info.bitsPerSample==8 || info.bitsPerSample==16 || info.bitsPerSample==24 || info.bitsPerSample==32);
            const bool ieee=info.formatTag==3 && (info.bitsPerSample==32 || info.bitsPerSample==64);
            if((!pcm && !ieee) || !info.channels || !info.sampleRate || !align
                || static_cast<std::uint64_t>(info.channels)*(info.bitsPerSample/8)!=align
                || static_cast<std::uint64_t>(info.sampleRate)*align!=u32(format.data()+8))
                return fail("WAVE format is unsupported or inconsistent; PCM and IEEE-float are supported.");
            hasFormat=true;
        }
        if(tag(chunk.data(),"data"))
        {
            if(hasData) return fail("Multiple WAVE data chunks are not supported by relink validation.");
            hasData=true; audioBytes=bytes;
        }
        const auto advance=static_cast<std::uint64_t>(bytes)+(bytes%2);
        if(advance>limit-cursor) return fail("WAVE chunk padding is truncated.");
        cursor+=advance;
    }
    if(!hasFormat || !hasData || audioBytes%align!=0) return fail("WAVE audio data or frame alignment is invalid.");
    info.frames=audioBytes/align;
    result=info;
    return true;
}
}
